#include "redguards3d.h"

#include <QFile>
#include <QFileInfo>

namespace
{
bool setError(QString* errorMessage, const QString& text)
{
  if (errorMessage != nullptr) {
    *errorMessage = text;
  }
  return false;
}

bool readFileBytes(const QString& filePath, QByteArray& outData,
                   QString* errorMessage)
{
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return setError(errorMessage,
                    QString("Unable to open 3D file: %1").arg(filePath));
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

bool isLegacy3dcVersion(Redguards3d::VersionTag versionTag)
{
  switch (versionTag) {
    case Redguards3d::VersionTag::V2_5:
    case Redguards3d::VersionTag::V2_6:
    case Redguards3d::VersionTag::V2_7:
      return true;
    default:
      return false;
  }
}

bool computeLegacy3dcPointListOffset(const QByteArray& data,
                                     const Redguards3d::Header& header,
                                     qint32& outOffset)
{
  if (!isLegacy3dcVersion(header.versionTag) || header.numVertices < 0 ||
      header.numFaces < 0 || header.offsetFaceData < 0 || header.offsetFrameData < 0) {
    return false;
  }

  const qsizetype frameStart = static_cast<qsizetype>(header.offsetFrameData);
  quint32 u3 = 0;
  if (frameStart < 0 || frameStart + 12 > data.size()) {
    return false;
  }
  qint32 u3Signed = 0;
  if (!readLE32(data, frameStart + 8, u3Signed) || u3Signed < 0) {
    return false;
  }
  u3 = static_cast<quint32>(u3Signed);

  qsizetype cursor = static_cast<qsizetype>(header.offsetFaceData);
  for (qint32 i = 0; i < header.numFaces; ++i) {
    if (cursor < 0 || cursor + 8 > data.size()) {
      return false;
    }

    const quint8 pointCount = static_cast<quint8>(data.at(cursor));
    cursor += 8;

    const qsizetype planePointBytes = static_cast<qsizetype>(pointCount) * 8;
    if (cursor + planePointBytes > data.size()) {
      return false;
    }
    cursor += planePointBytes;
  }

  const qsizetype pointBytes = static_cast<qsizetype>(header.numVertices) * 12;
  const qsizetype fallbackOffset = cursor + static_cast<qsizetype>(u3);
  if (fallbackOffset < 0 || fallbackOffset + pointBytes > data.size()) {
    return false;
  }

  outOffset = static_cast<qint32>(fallbackOffset);
  return true;
}

}  // namespace

QString Redguards3d::versionTagToString(VersionTag versionTag)
{
  return Xngine3dFormat::versionTagToString(versionTag);
}

bool Redguards3d::isSupportedRedguardVersion(VersionTag versionTag)
{
  switch (versionTag) {
    case VersionTag::V2_6:
    case VersionTag::V2_7:
    case VersionTag::V4_0:
    case VersionTag::V5_0:
      return true;
    default:
      return false;
  }
}

bool Redguards3d::parseHeader(const QByteArray& data, Header& outHeader,
                              QString* errorMessage)
{
  if (!Xngine3dFormat::parseHeader(data, outHeader, errorMessage)) {
    return false;
  }

  if (!isSupportedRedguardVersion(outHeader.versionTag)) {
    return setError(
        errorMessage,
        QString("Unsupported Redguard 3D version tag: '%1'")
            .arg(outHeader.versionString));
  }

  return true;
}

bool Redguards3d::parseRecord(const QByteArray& data, MeshRecord& outMesh,
                              QString* errorMessage)
{
  if (!Xngine3dFormat::parseRecord(data, outMesh, errorMessage)) {
    return false;
  }

  if (!isSupportedRedguardVersion(outMesh.header.versionTag)) {
    return setError(
        errorMessage,
        QString("Unsupported Redguard 3D version tag: '%1'")
            .arg(outMesh.header.versionString));
  }

  return true;
}

bool Redguards3d::loadHeader(const QString& filePath, Header& outHeader,
                             QString* errorMessage)
{
  QByteArray data;
  if (!readFileBytes(filePath, data, errorMessage)) {
    return false;
  }

  return parseHeader(data, outHeader, errorMessage);
}

bool Redguards3d::load(const QString& filePath, MeshRecord& outMesh,
                       QString* errorMessage)
{
  QByteArray data;
  if (!readFileBytes(filePath, data, errorMessage)) {
    return false;
  }

  const bool is3dc =
      QFileInfo(filePath).suffix().compare("3dc", Qt::CaseInsensitive) == 0;

  if (is3dc) {
    Header header;
    if (!Xngine3dFormat::parseHeader(data, header, errorMessage)) {
      return false;
    }

    qint32 fallbackOffset = 0;
    if (computeLegacy3dcPointListOffset(data, header, fallbackOffset) &&
        fallbackOffset != header.offsetVertexCoors) {
      QByteArray patchedData = data;
      writeLE32(patchedData, 48, fallbackOffset);

      if (Xngine3dFormat::parseRecord(patchedData, outMesh, errorMessage)) {
        if (outMesh.warning.isEmpty()) {
          outMesh.warning =
              "Used Redguard legacy 3DC vertex-coordinate fallback offset";
        }
      } else if (!Xngine3dFormat::parseRecord(data, outMesh, errorMessage)) {
        return false;
      }
    } else if (!Xngine3dFormat::parseRecord(data, outMesh, errorMessage)) {
      return false;
    }
  } else if (!Xngine3dFormat::parseRecord(data, outMesh, errorMessage)) {
    return false;
  }

  if (!isSupportedRedguardVersion(outMesh.header.versionTag)) {
    return setError(
        errorMessage,
        QString("Unsupported Redguard 3D version tag: '%1'")
            .arg(outMesh.header.versionString));
  }

  return true;
}

bool Redguards3d::looksLikeRedguard3dName(const QString& fileName)
{
  const QString base = QFileInfo(fileName).fileName().toUpper();
  return base.endsWith(".3D") || base.endsWith(".3DC");
}
