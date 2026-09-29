#ifndef BATTLESPIRE_SAVEJOINS_H
#define BATTLESPIRE_SAVEJOINS_H

#include <QByteArray>
#include <QHash>
#include <QVector>
#include <QtGlobal>

namespace BattlespireSaveJoins {

inline constexpr qsizetype kConversationMapOffset = 4492;
inline constexpr qsizetype kConversationMapRecordSize = 8;
inline constexpr qsizetype kConversationMapMaxRecords = 128;
inline constexpr qsizetype kStaticEnemyOffset = 5520;
inline constexpr qsizetype kStaticEnemyRecordSize = 56;
inline constexpr qsizetype kStaticEnemyMaxRecords = 128;

struct Summary
{
  int populated = 0;
  int resolved = 0;
  int missing = 0;
  QVector<quint32> missingRecordIds;
};

Summary joinRecordIds(const QByteArray& data, qsizetype start, qsizetype recordSize,
                      qsizetype maxRecords, const QHash<quint32, int>& saveTreeRecordTypes);

}  // namespace BattlespireSaveJoins

#endif  // BATTLESPIRE_SAVEJOINS_H
