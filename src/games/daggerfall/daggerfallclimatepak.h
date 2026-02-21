#ifndef DAGGERFALL_CLIMATEPAK_H
#define DAGGERFALL_CLIMATEPAK_H

#include <QByteArray>
#include <QString>
#include <QVector>

class DaggerfallClimatePak
{
public:
  struct TextureMapping
  {
    int value = 0;
    int groundTextureBase = 0;
    int flatTexture = 0;
    int skyBase = 0;
  };

  struct Data
  {
    int width = 1001;
    int height = 500;
    QVector<quint32> offsets;  // 500 entries
    QVector<QByteArray> rows;  // decompressed rows (1001 bytes each)
    QByteArray bitmap;         // rows concatenated
    QString warning;
  };

  static bool load(const QString& filePath, Data& outData,
                   QString* errorMessage = nullptr);

  static int valueAt(const Data& data, int x, int y);
  static TextureMapping mappingForValue(int value);
};

#endif  // DAGGERFALL_CLIMATEPAK_H
