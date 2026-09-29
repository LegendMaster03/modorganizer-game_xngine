#ifndef BATTLESPIRE_WATERTBL_H
#define BATTLESPIRE_WATERTBL_H

#include <QString>
#include <QVector>
#include <QtGlobal>

// Speculative Daggerfall-derived compatibility adapter. Verified Battlespire level
// data carries water information in BS6 WATR chunks, now handled by BattlespireBs6;
// no real Battlespire WATER.TBL resource or production caller is currently established.
// Retain this adapter for compatibility/research only until original-game evidence exists.
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
