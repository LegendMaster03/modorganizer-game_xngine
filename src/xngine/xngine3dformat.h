#ifndef XNGINE3DFORMAT_H
#define XNGINE3DFORMAT_H

#include <QByteArray>
#include <QString>
#include <QVector>
#include <QtGlobal>

class Xngine3dFormat
{
public:
  // Keep decimal version notation in all string forms:
  // "v2.5", "v2.6", "v2.7", "v4.0", "v5.0".
  enum class VersionTag
  {
    Unknown,
    V2_5,
    V2_6,
    V2_7,
    V4_0,
    V5_0
  };

  enum class UvReconstructionMode
  {
    None,
    Affine4,
    Triangle3
  };

  struct TextureRef
  {
    int fileIndex = 0;
    int imageIndex = 0;
  };

  struct Header
  {
    QString versionString;
    VersionTag versionTag = VersionTag::Unknown;

    qint32 numVertices = 0;
    qint32 numFaces = 0;
    qint32 radius = 0;
    qint32 numFrames = 0;

    qint32 offsetFrameData = 0;
    qint32 numUVOffsets = 0;
    qint32 offsetSection4 = 0;
    qint32 section4Count = 0;
    quint32 unknown4 = 0;
    qint32 offsetUVOffsets = 0;
    qint32 offsetUVData = 0;
    qint32 offsetVertexCoors = 0;
    qint32 offsetFaceNormals = 0;
    quint32 numUVOffsets2 = 0;
    qint32 offsetFaceData = 0;
  };

  struct Point
  {
    qint32 x = 0;
    qint32 y = 0;
    qint32 z = 0;
  };

  struct PlanePoint
  {
    qint32 pointOffset = 0;
    quint32 vertexIndexRaw = 0;
    qint16 u = 0;  // signed 12-bit decoded component from file
    qint16 v = 0;  // signed 12-bit decoded component from file
    int pointIndex = -1;
    qint32 uAbsolute = 0;  // Daggerfall-style absolute UV in subpixel units
    qint32 vAbsolute = 0;  // Daggerfall-style absolute UV in subpixel units
    double uPixels = 0.0;  // uAbsolute / 16.0
    double vPixels = 0.0;  // vAbsolute / 16.0
    bool uvReconstructed = false;
  };

  struct Plane
  {
    quint8 pointCount = 0;
    quint8 unknown1 = 0;
    quint16 textureRaw = 0;
    quint32 textureRaw32 = 0;
    bool textureIs32Bit = false;
    quint32 unknown2 = 0;
    TextureRef texture;
    QVector<PlanePoint> points;
    UvReconstructionMode uvMode = UvReconstructionMode::None;
  };

  struct ObjectData
  {
    qint32 number0 = 0;
    qint32 number1 = 0;
    qint32 number2 = 0;
    qint32 number3 = 0;
    quint16 subrecordCount = 0;
    QVector<QByteArray> values;  // 6-byte entries
  };

  struct FrameData
  {
    quint32 u1 = 0;
    quint32 u2 = 0;
    quint32 u3 = 0;
    quint32 u4 = 0;
    QByteArray payload;
  };

  struct UvCoordinate
  {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
  };

  struct MeshRecord
  {
    Header header;
    QVector<Point> points;
    QVector<Plane> planes;
    QVector<Point> normals;
    FrameData frameData;
    QVector<QByteArray> planeData;  // 24-byte entries in v2.x
    QVector<ObjectData> objectData;
    QVector<quint32> uvOffsets;
    QVector<UvCoordinate> uvCoordinates;
    QString warning;

    bool isValid() const { return header.versionTag != VersionTag::Unknown; }
  };

public:
  static QString versionTagToString(VersionTag versionTag);
  static VersionTag versionTagFromString(const QString& versionString);

  static bool parseHeader(const QByteArray& data, Header& outHeader,
                          QString* errorMessage = nullptr);

  // Current implementation decodes v2.5/v2.6/v2.7 mesh payloads.
  // Header parsing supports all known tags.
  static bool parseRecord(const QByteArray& data, MeshRecord& outMesh,
                          QString* errorMessage = nullptr);
};

#endif  // XNGINE3DFORMAT_H
