#ifndef DAGGERFALL_WATERTBL_H
#define DAGGERFALL_WATERTBL_H

#include <QString>
#include <QVector>
#include <QtGlobal>

// Reads and writes Daggerfall's fixed 256-byte water lookup table using
// the shared TBL format helper without interpreting its individual values.

class DaggerfallWaterTbl
{
public:
  struct Data
  {
    QVector<quint8> values;  // 256-byte LUT
  };

  static bool load(const QString& filePath, Data& outData,
                   QString* errorMessage = nullptr);

  static bool save(const QString& filePath, const Data& data,
                   QString* errorMessage = nullptr);
};

#endif  // DAGGERFALL_WATERTBL_H
