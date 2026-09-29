#ifndef BATTLESPIRE_SAVEFORMAT_H
#define BATTLESPIRE_SAVEFORMAT_H

#include <QtGlobal>

#include <limits>

namespace BattlespireSaveFormat {

inline constexpr quint8 kRecordTypeItem = 2;
inline constexpr quint8 kRecordTypePlayer = 3;
inline constexpr quint32 kPlayerRecordId = 50000U;
inline constexpr quint16 kItemIdGoldPieces = 33;
inline constexpr qsizetype kSaveTreeRecordHeaderSize = 65;
inline constexpr qsizetype kCurrentMapLevelOffset = 1051;
inline constexpr qsizetype kCurrentTimestampOffset = 1055;

namespace PlayerTreeOffset {
inline constexpr qsizetype Name = 65;
inline constexpr qsizetype RecordId = 33;
inline constexpr qsizetype ParentId = 61;
inline constexpr qsizetype SpellPoints = 161;
inline constexpr qsizetype SpellPointsMax = 163;
inline constexpr qsizetype Wounds = 167;
inline constexpr qsizetype WoundsMax = 171;
inline constexpr qsizetype ActiveSpells = 435;
inline constexpr qsizetype CharacterFlags = 615;
inline constexpr qsizetype Team = 623;
inline constexpr qsizetype Goal = 627;
inline constexpr qsizetype Race = 670;
inline constexpr qsizetype Level = 736;
inline constexpr qsizetype GoldItemId = 97;
inline constexpr qsizetype GoldQuantity = 127;
}  // namespace PlayerTreeOffset

namespace PlayerVarsOffset {
inline constexpr qsizetype Name = PlayerTreeOffset::Name - kSaveTreeRecordHeaderSize;
inline constexpr qsizetype SpellPoints = PlayerTreeOffset::SpellPoints - kSaveTreeRecordHeaderSize;
inline constexpr qsizetype SpellPointsMax =
    PlayerTreeOffset::SpellPointsMax - kSaveTreeRecordHeaderSize;
inline constexpr qsizetype Wounds = PlayerTreeOffset::Wounds - kSaveTreeRecordHeaderSize;
inline constexpr qsizetype WoundsMax = PlayerTreeOffset::WoundsMax - kSaveTreeRecordHeaderSize;
inline constexpr qsizetype ActiveSpells =
    PlayerTreeOffset::ActiveSpells - kSaveTreeRecordHeaderSize;
inline constexpr qsizetype CharacterFlags =
    PlayerTreeOffset::CharacterFlags - kSaveTreeRecordHeaderSize;
inline constexpr qsizetype Team = PlayerTreeOffset::Team - kSaveTreeRecordHeaderSize;
inline constexpr qsizetype Goal = PlayerTreeOffset::Goal - kSaveTreeRecordHeaderSize;
inline constexpr qsizetype Race = PlayerTreeOffset::Race - kSaveTreeRecordHeaderSize;
inline constexpr qsizetype Level = PlayerTreeOffset::Level - kSaveTreeRecordHeaderSize;
inline constexpr qsizetype ClassName = 378;
}  // namespace PlayerVarsOffset

struct PlayerRecordMatch
{
  bool byType = false;
  bool byId = false;

  constexpr bool canonical() const { return byType && byId; }
  constexpr bool recoveryCandidate() const { return byType != byId; }
};

constexpr PlayerRecordMatch classifyPlayerRecord(quint8 type, bool hasRecordId, quint32 recordId)
{
  return {type == kRecordTypePlayer, hasRecordId && recordId == kPlayerRecordId};
}

constexpr bool isDocumentedCampaignMap(quint32 mapId)
{
  return mapId >= 1 && mapId <= 7;
}

static_assert(PlayerVarsOffset::ActiveSpells == 370);
static_assert(PlayerVarsOffset::CharacterFlags == 550);
static_assert(PlayerVarsOffset::Team == 558);
static_assert(PlayerVarsOffset::Goal == 562);
static_assert(kCurrentMapLevelOffset == 1051);
static_assert(classifyPlayerRecord(kRecordTypePlayer, true, kPlayerRecordId).canonical());
static_assert(classifyPlayerRecord(kRecordTypePlayer, true, 1234).recoveryCandidate());
static_assert(classifyPlayerRecord(18, true, kPlayerRecordId).recoveryCandidate());
static_assert(std::numeric_limits<qsizetype>::max() > 0);

}  // namespace BattlespireSaveFormat

#endif  // BATTLESPIRE_SAVEFORMAT_H
