#include "battlespire3dbsa.h"

#include <QFile>
#include <QtEndian>

#include <xnginebsaformat.h>

#include <cstring>
#include <limits>

namespace {

bool setError(QString* errorMessage, const QString& text)
{
  if (errorMessage != nullptr) {
    *errorMessage = text;
  }
  return false;
}

XngineBSAFormat::Traits battlespireArchiveTraits()
{
  XngineBSAFormat::Traits traits;
  traits.allowCompressed = true;
  traits.compressionMode = XngineBSAFormat::CompressionMode::BattlespireLzss;
  traits.descriptorLayout = XngineBSAFormat::DescriptorLayout::Battlespire;
  traits.allowMissingTypeHeader = true;
  return traits;
}

bool readFileBytes(const QString& filePath, QByteArray& outData, QString* errorMessage)
{
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return setError(errorMessage, QString("Unable to open 3D file: %1").arg(filePath));
  }
  outData = file.readAll();
  return true;
}

bool hasRange(const QByteArray& data, qsizetype offset, qsizetype size)
{
  if (offset < 0 || size < 0 || offset > data.size()) {
    return false;
  }
  return size <= data.size() - offset;
}

bool readLE16(const QByteArray& data, qsizetype offset, quint16& outValue)
{
  if (!hasRange(data, offset, 2)) {
    return false;
  }
  quint16 raw = 0;
  std::memcpy(&raw, data.constData() + offset, sizeof(raw));
  outValue = qFromLittleEndian(raw);
  return true;
}

void writeLE32(QByteArray& data, qsizetype offset, qint32 value)
{
  quint32 raw = qToLittleEndian(static_cast<quint32>(value));
  std::memcpy(data.data() + offset, &raw, sizeof(raw));
}

bool isSupportedV2(const Xngine3dFormat::Header& header)
{
  return header.versionTag == Xngine3dFormat::VersionTag::V2_5 ||
         header.versionTag == Xngine3dFormat::VersionTag::V2_6 ||
         header.versionTag == Xngine3dFormat::VersionTag::V2_7;
}

qsizetype computeFaceSectionEnd(const QByteArray& data, const Xngine3dFormat::Header& header,
                                qsizetype faceHeaderBytes)
{
  if (header.numFaces < 0 || header.offsetFaceData < 0 || faceHeaderBytes <= 0) {
    return -1;
  }

  qsizetype pos = static_cast<qsizetype>(header.offsetFaceData);
  for (qint32 i = 0; i < header.numFaces; ++i) {
    if (!hasRange(data, pos, faceHeaderBytes)) {
      return -1;
    }

    const quint8 pointCount = static_cast<quint8>(data.at(pos));
    const qsizetype pointBytes = static_cast<qsizetype>(pointCount) * 8;
    pos += faceHeaderBytes;
    if (!hasRange(data, pos, pointBytes)) {
      return -1;
    }
    pos += pointBytes;
  }

  return pos;
}

bool readNativeFaceHeaders(const QByteArray& data, const Xngine3dFormat::Header& header,
                           QVector<Battlespire3dBsa::NativeFaceHeader>& outHeaders,
                           QString* errorMessage)
{
  outHeaders.clear();
  outHeaders.reserve(header.numFaces);

  qsizetype pos = static_cast<qsizetype>(header.offsetFaceData);
  for (qint32 i = 0; i < header.numFaces; ++i) {
    if (!hasRange(data, pos, 10)) {
      return setError(errorMessage,
                      QString("Battlespire face %1 header exceeds record bounds").arg(i));
    }

    Battlespire3dBsa::NativeFaceHeader face;
    face.pointCount = static_cast<quint8>(data.at(pos));
    face.unknown1 = static_cast<quint8>(data.at(pos + 1));
    if (!readLE16(data, pos + 2, face.textureRaw)) {
      return setError(errorMessage,
                      QString("Failed reading Battlespire face %1 texture field").arg(i));
    }
    face.unknownBytes = data.mid(pos + 4, 6);
    outHeaders.push_back(face);

    const qsizetype pointBytes = static_cast<qsizetype>(face.pointCount) * 8;
    pos += 10;
    if (!hasRange(data, pos, pointBytes)) {
      return setError(errorMessage,
                      QString("Battlespire face %1 point list exceeds record bounds").arg(i));
    }
    pos += pointBytes;
  }
  return true;
}

QByteArray normalizeNativeTenByteFaces(const QByteArray& data,
                                       const Xngine3dFormat::Header& header,
                                       QString* errorMessage)
{
  if (header.numFaces < 0 || header.offsetFaceData < 0 ||
      header.offsetFaceData > data.size()) {
    setError(errorMessage, "Invalid Battlespire face section header");
    return {};
  }

  const qsizetype removedBytes = static_cast<qsizetype>(header.numFaces) * 2;
  if (removedBytes > data.size()) {
    setError(errorMessage, "Battlespire face normalization size underflow");
    return {};
  }

  QByteArray out;
  out.reserve(data.size() - removedBytes);
  out.append(data.constData(), header.offsetFaceData);

  qsizetype pos = static_cast<qsizetype>(header.offsetFaceData);
  for (qint32 i = 0; i < header.numFaces; ++i) {
    if (!hasRange(data, pos, 10)) {
      setError(errorMessage, QString("Battlespire face %1 is truncated").arg(i));
      return {};
    }

    const quint8 pointCount = static_cast<quint8>(data.at(pos));

    // The native header is <pointCount:u8, unknown1:u8, texture:u16, unknown2:6 bytes>.
    // Xngine3dFormat consumes the common first eight bytes. Preserve all six unknown
    // bytes separately in NativeFaceHeader; only the final two are omitted here to
    // feed the established shared v2.x parser without inventing semantics for them.
    out.append(data.constData() + pos, 8);

    const qsizetype pointBytes = static_cast<qsizetype>(pointCount) * 8;
    const qsizetype pointsOffset = pos + 10;
    if (!hasRange(data, pointsOffset, pointBytes)) {
      setError(errorMessage,
               QString("Battlespire face %1 point list is truncated").arg(i));
      return {};
    }
    out.append(data.constData() + pointsOffset, pointBytes);
    pos = pointsOffset + pointBytes;
  }

  out.append(data.constData() + pos, data.size() - pos);

  auto adjustOffset = [&](qsizetype fieldOffset, qint32 original) {
    if (original > header.offsetFaceData) {
      const qsizetype adjusted = static_cast<qsizetype>(original) - removedBytes;
      if (adjusted < 0 || adjusted > std::numeric_limits<qint32>::max()) {
        return false;
      }
      writeLE32(out, fieldOffset, static_cast<qint32>(adjusted));
    }
    return true;
  };

  if (!adjustOffset(20, header.offsetFrameData) ||
      !adjustOffset(24, header.numUVOffsets) ||
      !adjustOffset(28, header.offsetSection4) ||
      !adjustOffset(40, header.offsetUVOffsets) ||
      !adjustOffset(44, header.offsetUVData) ||
      !adjustOffset(48, header.offsetVertexCoors) ||
      !adjustOffset(52, header.offsetFaceNormals) ||
      !adjustOffset(60, header.offsetFaceData)) {
    setError(errorMessage, "Battlespire face normalization produced an invalid section offset");
    return {};
  }

  return out;
}

bool parseBattlespireMeshData(const QByteArray& data,
                              Battlespire3dBsa::NativeMeshRecord& outMesh,
                              QString* errorMessage)
{
  outMesh = {};

  Xngine3dFormat::Header header;
  if (!Xngine3dFormat::parseHeader(data, header, errorMessage)) {
    return false;
  }

  if (!isSupportedV2(header)) {
    return setError(errorMessage,
                    QString("Battlespire 3D record uses unsupported version '%1'")
                        .arg(header.versionString));
  }

  const qsizetype normalOffset = static_cast<qsizetype>(header.offsetFaceNormals);
  const qsizetype nativeEnd = computeFaceSectionEnd(data, header, 10);
  const qsizetype daggerfallEnd = computeFaceSectionEnd(data, header, 8);

  // Select the native layout before the Daggerfall compatibility layout. This prevents
  // a superficially valid but misaligned eight-byte parse from winning for Battlespire data.
  if (header.numFaces > 0 && nativeEnd >= 0 && nativeEnd == normalOffset) {
    if (!readNativeFaceHeaders(data, header, outMesh.nativeFaceHeaders, errorMessage)) {
      return false;
    }

    QByteArray normalized = normalizeNativeTenByteFaces(data, header, errorMessage);
    if (normalized.isEmpty() && !data.isEmpty()) {
      return false;
    }
    if (!Xngine3dFormat::parseRecord(normalized, outMesh.mesh, errorMessage)) {
      return false;
    }

    outMesh.usesNativeTenByteFaces = true;
    const QString note =
        "Parsed native Battlespire ten-byte face headers; six unknown header bytes are preserved";
    if (outMesh.mesh.warning.isEmpty()) {
      outMesh.mesh.warning = note;
    } else {
      outMesh.mesh.warning.append("; " + note);
    }
    return true;
  }

  if (daggerfallEnd >= 0 && daggerfallEnd == normalOffset) {
    return Xngine3dFormat::parseRecord(data, outMesh.mesh, errorMessage);
  }

  return setError(errorMessage,
                  QString("Battlespire face section does not align with the normal list "
                          "as either native 10-byte or inherited 8-byte headers "
                          "(faceData=%1, nativeEnd=%2, inheritedEnd=%3, normals=%4)")
                      .arg(header.offsetFaceData)
                      .arg(nativeEnd)
                      .arg(daggerfallEnd)
                      .arg(normalOffset));
}

bool readArchive(const QString& archivePath, XngineBSAFormat::Archive& outArchive,
                 QString* errorMessage)
{
  return XngineBSAFormat::readArchive(archivePath, outArchive, errorMessage,
                                      battlespireArchiveTraits());
}

}  // namespace

bool Battlespire3dBsa::listRecordNames(const QString& archivePath,
                                       QStringList& outRecordNames,
                                       QString* errorMessage)
{
  outRecordNames.clear();

  XngineBSAFormat::Archive archive;
  if (!readArchive(archivePath, archive, errorMessage)) {
    return false;
  }
  if (archive.type != XngineBSAFormat::IndexType::NameRecord) {
    return setError(errorMessage, "Battlespire 3D archive is not a NameRecord BSA");
  }

  outRecordNames.reserve(archive.entries.size());
  for (const auto& entry : archive.entries) {
    outRecordNames.push_back(entry.name);
  }
  return true;
}

bool Battlespire3dBsa::loadNativeMeshRecordByName(const QString& archivePath,
                                                  const QString& recordName,
                                                  NativeMeshRecord& outMesh,
                                                  QString* errorMessage)
{
  XngineBSAFormat::Archive archive;
  if (!readArchive(archivePath, archive, errorMessage)) {
    return false;
  }
  if (archive.type != XngineBSAFormat::IndexType::NameRecord) {
    return setError(errorMessage, "Battlespire 3D archive is not a NameRecord BSA");
  }

  const QString wanted = recordName.trimmed().toUpper();
  const XngineBSAFormat::Entry* target = nullptr;
  for (const auto& entry : archive.entries) {
    if (entry.name.trimmed().toUpper() == wanted) {
      target = &entry;
      break;
    }
  }
  if (target == nullptr) {
    return setError(errorMessage, QString("3D record '%1' not found").arg(recordName));
  }

  return parseBattlespireMeshData(target->data, outMesh, errorMessage);
}

bool Battlespire3dBsa::loadMeshRecordByName(const QString& archivePath,
                                            const QString& recordName,
                                            Xngine3dFormat::MeshRecord& outMesh,
                                            QString* errorMessage)
{
  NativeMeshRecord native;
  if (!loadNativeMeshRecordByName(archivePath, recordName, native, errorMessage)) {
    return false;
  }
  outMesh = std::move(native.mesh);
  return true;
}

bool Battlespire3dBsa::loadNativeMeshFile(const QString& filePath,
                                          NativeMeshRecord& outMesh,
                                          QString* errorMessage)
{
  QByteArray data;
  if (!readFileBytes(filePath, data, errorMessage)) {
    return false;
  }
  return parseBattlespireMeshData(data, outMesh, errorMessage);
}

bool Battlespire3dBsa::loadMeshFile(const QString& filePath,
                                    Xngine3dFormat::MeshRecord& outMesh,
                                    QString* errorMessage)
{
  NativeMeshRecord native;
  if (!loadNativeMeshFile(filePath, native, errorMessage)) {
    return false;
  }
  outMesh = std::move(native.mesh);
  return true;
}
