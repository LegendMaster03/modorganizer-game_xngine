#include "battlespire3dbsa.h"

#include <QFile>

#include <xnginebsaformat.h>

namespace {

bool setError(QString* errorMessage, const QString& text)
{
  if (errorMessage != nullptr) {
    *errorMessage = text;
  }
  return false;
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

bool readLE32(const QByteArray& data, qsizetype offset, qint32& outValue)
{
  if (offset < 0 || offset + 4 > data.size()) {
    return false;
  }

  const auto* p = reinterpret_cast<const unsigned char*>(data.constData() + offset);
  outValue = static_cast<qint32>(static_cast<quint32>(p[0]) |
                                 (static_cast<quint32>(p[1]) << 8) |
                                 (static_cast<quint32>(p[2]) << 16) |
                                 (static_cast<quint32>(p[3]) << 24));
  return true;
}

void writeLE32(QByteArray& data, qsizetype offset, qint32 value)
{
  char* p = data.data() + offset;
  p[0] = static_cast<char>(value & 0xFF);
  p[1] = static_cast<char>((value >> 8) & 0xFF);
  p[2] = static_cast<char>((value >> 16) & 0xFF);
  p[3] = static_cast<char>((value >> 24) & 0xFF);
}

qsizetype computeFaceSectionEnd(const QByteArray& data, const Xngine3dFormat::Header& header,
                                int faceHeaderBytes)
{
  if (header.numFaces < 0 || header.offsetFaceData < 0) {
    return -1;
  }

  qsizetype pos = static_cast<qsizetype>(header.offsetFaceData);
  for (qint32 i = 0; i < header.numFaces; ++i) {
    if (pos < 0 || pos + faceHeaderBytes > data.size()) {
      return -1;
    }

    const quint8 pointCount = static_cast<quint8>(data.at(pos));
    pos += faceHeaderBytes + static_cast<qsizetype>(pointCount) * 8;
    if (pos > data.size()) {
      return -1;
    }
  }

  return pos;
}

bool looksLikeBattlespireExtendedFaceLayout(const QByteArray& data,
                                            const Xngine3dFormat::Header& header)
{
  if (header.versionTag != Xngine3dFormat::VersionTag::V2_7) {
    return false;
  }

  const qsizetype end8 = computeFaceSectionEnd(data, header, 8);
  const qsizetype end10 = computeFaceSectionEnd(data, header, 10);
  const qsizetype normalOffset = static_cast<qsizetype>(header.offsetFaceNormals);
  if (normalOffset < 0) {
    return false;
  }

  return end10 == normalOffset && end8 != normalOffset;
}

QByteArray normalizeBattlespireExtendedFaceLayoutDropMiddle(const QByteArray& data,
                                                            const Xngine3dFormat::Header& header)
{
  const int removedBytes = header.numFaces * 2;
  QByteArray out;
  out.reserve(data.size() - removedBytes);
  out.append(data.constData(), 64);
  if (header.offsetFaceData > 64) {
    out.append(data.constData() + 64, header.offsetFaceData - 64);
  }

  qsizetype pos = static_cast<qsizetype>(header.offsetFaceData);
  for (qint32 i = 0; i < header.numFaces; ++i) {
    const quint8 pointCount = static_cast<quint8>(data.at(pos));

    out.append(data.constData() + pos, 4);
    out.append(data.constData() + pos + 6, 4);

    const qsizetype pointsOffset = pos + 10;
    const qsizetype pointsBytes = static_cast<qsizetype>(pointCount) * 8;
    out.append(data.constData() + pointsOffset, pointsBytes);

    pos = pointsOffset + pointsBytes;
  }

  out.append(data.constData() + pos, data.size() - pos);

  auto adjustOffset = [&](qsizetype fieldOffset, qint32 original) {
    if (original > header.offsetFaceData) {
      writeLE32(out, fieldOffset, original - removedBytes);
    }
  };

  adjustOffset(20, header.offsetFrameData);
  adjustOffset(24, header.numUVOffsets);
  adjustOffset(28, header.offsetSection4);
  adjustOffset(40, header.offsetUVOffsets);
  adjustOffset(44, header.offsetUVData);
  adjustOffset(52, header.offsetFaceNormals);

  return out;
}

QByteArray normalizeBattlespireExtendedFaceLayoutDropTail(const QByteArray& data,
                                                          const Xngine3dFormat::Header& header)
{
  const int removedBytes = header.numFaces * 2;
  QByteArray out;
  out.reserve(data.size() - removedBytes);
  out.append(data.constData(), 64);
  if (header.offsetFaceData > 64) {
    out.append(data.constData() + 64, header.offsetFaceData - 64);
  }

  qsizetype pos = static_cast<qsizetype>(header.offsetFaceData);
  for (qint32 i = 0; i < header.numFaces; ++i) {
    const quint8 pointCount = static_cast<quint8>(data.at(pos));

    out.append(data.constData() + pos, 8);

    const qsizetype pointsOffset = pos + 10;
    const qsizetype pointsBytes = static_cast<qsizetype>(pointCount) * 8;
    out.append(data.constData() + pointsOffset, pointsBytes);

    pos = pointsOffset + pointsBytes;
  }

  out.append(data.constData() + pos, data.size() - pos);

  auto adjustOffset = [&](qsizetype fieldOffset, qint32 original) {
    if (original > header.offsetFaceData) {
      writeLE32(out, fieldOffset, original - removedBytes);
    }
  };

  adjustOffset(20, header.offsetFrameData);
  adjustOffset(24, header.numUVOffsets);
  adjustOffset(28, header.offsetSection4);
  adjustOffset(40, header.offsetUVOffsets);
  adjustOffset(44, header.offsetUVData);
  adjustOffset(52, header.offsetFaceNormals);

  return out;
}

bool parseBattlespireMeshData(const QByteArray& data, Xngine3dFormat::MeshRecord& outMesh,
                              QString* errorMessage)
{
  Xngine3dFormat::Header header;
  if (!Xngine3dFormat::parseHeader(data, header, errorMessage)) {
    return false;
  }

  if (header.versionTag != Xngine3dFormat::VersionTag::V2_5 &&
      header.versionTag != Xngine3dFormat::VersionTag::V2_6 &&
      header.versionTag != Xngine3dFormat::VersionTag::V2_7) {
    return setError(errorMessage,
                    QString("Battlespire 3D record uses unsupported version '%1'")
                        .arg(header.versionString));
  }

  if (Xngine3dFormat::parseRecord(data, outMesh, errorMessage)) {
    return true;
  }

  if (!looksLikeBattlespireExtendedFaceLayout(data, header)) {
    return false;
  }

  const struct {
    const char* label;
    QByteArray (*fn)(const QByteArray&, const Xngine3dFormat::Header&);
  } strategies[] = {
      {"drop-middle", normalizeBattlespireExtendedFaceLayoutDropMiddle},
      {"drop-tail", normalizeBattlespireExtendedFaceLayoutDropTail},
  };

  QString lastError;
  for (const auto& strategy : strategies) {
    QByteArray normalized = strategy.fn(data, header);
    QString parseError;
    if (!Xngine3dFormat::parseRecord(normalized, outMesh, &parseError)) {
      lastError = parseError;
      continue;
    }

    const QString note =
        QString("Used Battlespire v2.7 extended face-header compatibility path (%1)")
            .arg(QString::fromLatin1(strategy.label));
    if (outMesh.warning.isEmpty()) {
      outMesh.warning = note;
    } else {
      outMesh.warning.append("; " + note);
    }
    return true;
  }

  if (errorMessage != nullptr && !lastError.isEmpty()) {
    *errorMessage = lastError;
  }
  return false;
}

}  // namespace

bool Battlespire3dBsa::listRecordNames(const QString& archivePath, QStringList& outRecordNames,
                                       QString* errorMessage)
{
  outRecordNames.clear();

  XngineBSAFormat::Archive archive;
  if (!XngineBSAFormat::readArchive(archivePath, archive, errorMessage)) {
    return false;
  }
  if (archive.type != XngineBSAFormat::IndexType::NameRecord) {
    return setError(errorMessage, "Battlespire 3D archive is not a NameRecord BSA");
  }

  outRecordNames.reserve(archive.entries.size());
  for (const auto& e : archive.entries) {
    outRecordNames.push_back(e.name);
  }
  return true;
}

bool Battlespire3dBsa::loadMeshRecordByName(const QString& archivePath, const QString& recordName,
                                            Xngine3dFormat::MeshRecord& outMesh,
                                            QString* errorMessage)
{
  XngineBSAFormat::Archive archive;
  if (!XngineBSAFormat::readArchive(archivePath, archive, errorMessage)) {
    return false;
  }
  if (archive.type != XngineBSAFormat::IndexType::NameRecord) {
    return setError(errorMessage, "Battlespire 3D archive is not a NameRecord BSA");
  }

  const QString wanted = recordName.trimmed().toUpper();
  const XngineBSAFormat::Entry* target = nullptr;
  for (const auto& e : archive.entries) {
    if (e.name.trimmed().toUpper() == wanted) {
      target = &e;
      break;
    }
  }
  if (target == nullptr) {
    return setError(errorMessage, QString("3D record '%1' not found").arg(recordName));
  }

  return parseBattlespireMeshData(target->data, outMesh, errorMessage);
}

bool Battlespire3dBsa::loadMeshFile(const QString& filePath, Xngine3dFormat::MeshRecord& outMesh,
                                    QString* errorMessage)
{
  QByteArray data;
  if (!readFileBytes(filePath, data, errorMessage)) {
    return false;
  }
  return parseBattlespireMeshData(data, outMesh, errorMessage);
}
