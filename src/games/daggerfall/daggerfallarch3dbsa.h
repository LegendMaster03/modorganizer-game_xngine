#ifndef DAGGERFALL_ARCH3DBSA_H
#define DAGGERFALL_ARCH3DBSA_H

#include <QByteArray>
#include <QString>
#include <QVector>
#include <QtGlobal>

class DaggerfallArch3dBsa
{
public:
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

  struct Point
  {
    qint32 x = 0;
    qint32 y = 0;
    qint32 z = 0;
  };

  struct PlanePoint
  {
    qint32 pointOffset = 0;
    qint16 u = 0;  // signed 12-bit decoded component from file
    qint16 v = 0;  // signed 12-bit decoded component from file
    int pointIndex = -1;
    qint32 uAbsolute = 0;  // Daggerfall absolute UV subpixel units
    qint32 vAbsolute = 0;  // Daggerfall absolute UV subpixel units
    double uPixels = 0.0;  // uAbsolute / 16.0
    double vPixels = 0.0;  // vAbsolute / 16.0
    bool uvReconstructed = false;
  };

  struct Plane
  {
    quint8 pointCount = 0;
    quint8 unknown1 = 0;
    quint16 textureRaw = 0;
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

  struct MeshRecord
  {
    quint16 recordId = 0;
    QString version;
    qint32 pointCount = 0;
    qint32 planeCount = 0;
    qint32 radius = 0;
    qint32 planeDataOffset = 0;
    qint32 objectDataOffset = 0;
    qint32 objectDataCount = 0;
    quint32 unknown2 = 0;
    qint32 pointListOffset = 0;
    qint32 normalListOffset = 0;
    quint32 unknown3 = 0;
    qint32 planeListOffset = 0;

    QVector<Point> points;
    QVector<Plane> planes;
    QVector<Point> normals;
    QVector<QByteArray> planeData;  // 24-byte entries
    QVector<ObjectData> objectData;
    int planesUvAffine4 = 0;
    int planesUvTriangle3 = 0;
    int planesUvNone = 0;

    QString warning;
    bool isValid() const { return !version.isEmpty(); }
  };

  static bool listRecordIds(const QString& arch3dBsaPath, QVector<quint16>& outRecordIds,
                            QString* errorMessage = nullptr);

  static bool loadMeshRecord(const QString& arch3dBsaPath, quint16 recordId,
                             MeshRecord& outMesh, QString* errorMessage = nullptr);
};

#endif  // DAGGERFALL_ARCH3DBSA_H
