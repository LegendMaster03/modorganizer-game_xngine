#ifndef XNGINEWLDFORMAT_H
#define XNGINEWLDFORMAT_H

#include <QByteArray>
#include <QString>
#include <QVector>
#include <QtGlobal>

class XngineWldFormat
{
public:
  enum class Variant
  {
    Auto,
    DaggerfallWoods,
    Redguard
  };

  struct Traits
  {
    Variant variant = Variant::Auto;
    bool strictValidation = true;
  };

  struct DaggerfallTerrainParameter
  {
    quint8 clampHeight = 0;
    quint8 clampSensitivity = 0;
    quint8 unknown2 = 0;
    quint8 unknown3 = 0;
  };

  struct DaggerfallPixelData
  {
    quint16 unknown1 = 0;
    quint32 nullValue1 = 0;
    quint16 fileIndex = 0;
    quint8 terrainType = 0;
    quint8 terrainNoise = 0;
    quint32 nullValue2a = 0;
    quint32 nullValue2b = 0;
    quint32 nullValue2c = 0;
    qint8 elevationNoise[25] = {};
  };

  struct DaggerfallWoodsData
  {
    qint32 offsetSize = 0;
    qint32 width = 0;
    qint32 height = 0;
    qint32 nullValue1 = 0;
    qint32 terrainTypesOffset = 0;
    quint32 constant1 = 0;
    quint32 constant2 = 0;
    qint32 elevationMapOffset = 0;
    QVector<qint32> nullValue2;  // 28 ints

    QVector<quint32> pixelOffsets;                  // width*height
    QVector<DaggerfallTerrainParameter> terrainTypes; // 256
    QVector<qint8> elevationMap;                    // width*height
    QVector<DaggerfallPixelData> pixelData;         // width*height
  };

  struct RedguardSection
  {
    QVector<quint16> headerWords;   // 11 words (big-endian)
    QVector<quint8> map1;           // 16384 bytes
    QVector<quint8> map2;           // 16384 bytes
    QVector<quint8> map3;           // 16384 bytes
    QVector<quint8> map4;           // 16384 bytes
  };

  struct RedguardData
  {
    QVector<quint32> headerDwords;  // 296 dwords (big-endian)
    QVector<RedguardSection> sections;  // 4 sections
    QVector<quint32> footerDwords;  // 4 dwords (big-endian)
  };

  struct Document
  {
    Variant variant = Variant::Auto;
    DaggerfallWoodsData daggerfallWoods;
    RedguardData redguard;
    QString warning;
  };

public:
  static bool readFile(const QString& filePath, Document& outDocument,
                       QString* errorMessage = nullptr,
                       const Traits& traits = Traits{});

  static bool writeFile(const QString& filePath, const Document& document,
                        QString* errorMessage = nullptr,
                        const Traits& traits = Traits{});

  static int daggerfallMapIndex(const DaggerfallWoodsData& data, int x, int y);

  // Redguard utility: combines one map plane from all 4 sections into a 256x256 grid.
  // planeIndex is 0..3 for map1..map4.
  static QVector<quint8> redguardCombinedMap(const RedguardData& data, int planeIndex);
};

#endif  // XNGINEWLDFORMAT_H
