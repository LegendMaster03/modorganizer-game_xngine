#ifndef DAGGERFALL_WOODSWLD_H
#define DAGGERFALL_WOODSWLD_H

#include <QByteArray>
#include <QString>
#include <QVector>
#include <QtGlobal>

class DaggerfallWoodsWld
{
public:
  struct Header
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
  };

  struct TerrainParameter
  {
    quint8 clampHeight = 0;
    quint8 clampSensitivity = 0;
    quint8 unknown2 = 0;
    quint8 unknown3 = 0;
  };

  struct PixelData
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

  struct Data
  {
    Header header;
    QVector<quint32> pixelOffsets;          // width*height entries
    QVector<TerrainParameter> terrainTypes; // 256 entries
    QVector<qint8> elevationMap;            // width*height entries
    QVector<PixelData> pixelData;           // width*height entries
    QString warning;
  };

  static bool load(const QString& filePath, Data& outData,
                   QString* errorMessage = nullptr);

  static int mapIndex(const Data& data, int x, int y);
};

#endif  // DAGGERFALL_WOODSWLD_H
