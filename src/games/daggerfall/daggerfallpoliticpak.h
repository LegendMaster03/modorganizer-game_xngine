#ifndef DAGGERFALL_POLITICPAK_H
#define DAGGERFALL_POLITICPAK_H

#include <QByteArray>
#include <QString>
#include <QVector>

class DaggerfallPoliticPak
{
public:
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
  static int regionFromValue(int value);  // 128..233 -> value-128, 64 water -> -1
};

#endif  // DAGGERFALL_POLITICPAK_H
