#ifndef BATTLESPIRE_WATERTBL_H
#define BATTLESPIRE_WATERTBL_H

#include <QString>
#include <QVector>
#include <QtGlobal>

// Speculative Daggerfall-derived compatibility adapter. The BS6 research parser
// separately projects candidate WATR values, but neither that interpretation nor
// a Battlespire WATER.TBL resource has been validated with original-game assets.
// No production caller is established. Retain this adapter only for research.
// This compatibility adapter uses the shared TBL helper to read and write
// fixed 256-byte tables without decoding the individual byte meanings.
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
