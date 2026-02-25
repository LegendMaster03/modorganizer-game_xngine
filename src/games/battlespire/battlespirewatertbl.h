#ifndef BATTLESPIRE_WATERTBL_H
#define BATTLESPIRE_WATERTBL_H

#include <QString>
#include <QVector>
#include <QtGlobal>

class BattlespireWaterTbl
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

#endif  // BATTLESPIRE_WATERTBL_H
