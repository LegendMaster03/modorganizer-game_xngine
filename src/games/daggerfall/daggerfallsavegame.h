#ifndef DAGGERFALLS_SAVEGAME_H
#define DAGGERFALLS_SAVEGAME_H

#include <xnginesavegame.h>

#include <QColor>
#include <QString>
#include <QByteArray>
#include <QMap>
#include <QList>
#include <QtGlobal>
#include <memory>
#include <vector>

class GameDaggerfall;

/**
 * Daggerfall-specific save game handler.
 * Daggerfall saves are numbered SAVE0 through SAVE5 (6 save slots)
 */
class DaggerfallsSaveGame : public XngineSaveGame
{
public:
  DaggerfallsSaveGame(const QString& saveFolder, const GameDaggerfall* game);
  virtual QString getName() const override;
  virtual QString getPCLocation() const override;
  virtual QString getGameDetails() const override;

protected:
  virtual std::unique_ptr<DataFields> fetchDataFields() const override;

private:
  struct ParsedRecord
  {
    quint8 type = 0;
    qsizetype payloadOffset = 0;
    qsizetype payloadLength = 0;
  };

  struct RecordParseStats
  {
    int zeroLengthSeparators = 0;
    int truncatedRecords = 0;
    int negativeLengths = 0;
    int dungeonLengthAdjusted = 0;
    int invalidDungeonLength = 0;
  };

  struct FactionReputationEntry
  {
    quint8 type = 0;
    quint8 region = 0;
    qint16 reputation = 0;
    qint16 factionId = 0;
    QString name;
  };

  struct GuildMembershipEntry
  {
    quint32 guildIdRaw = 0;
    quint32 rankRaw = 0;
    quint32 promotionDateRaw = 0;
  };

  struct SpellSummary
  {
    int count = 0;
    int effectSlotsUsed = 0;
    QMap<QString, int> names;
  };

  bool parseSaveTree();
  bool parseSaveName();
  bool parseSaveVars();
  static bool readLE32(const QByteArray& data, qsizetype offset, qint32& value);
  static bool readLE32U(const QByteArray& data, qsizetype offset, quint32& value);
  static bool readLE16U(const QByteArray& data, qsizetype offset, quint16& value);
  static bool readU8(const QByteArray& data, qsizetype offset, quint8& value);
  static std::vector<ParsedRecord> parseRecordStream(const QByteArray& data,
                                                     qsizetype startOffset,
                                                     qsizetype* endOffset,
                                                     RecordParseStats* stats);
  static std::vector<ParsedRecord> findBestRecordStream(const QByteArray& data,
                                                        qsizetype* startOffset,
                                                        qsizetype* endOffset,
                                                        RecordParseStats* stats);
  static QString readFixedString(const QByteArray& data, qsizetype offset, qsizetype size);
  static QString formatPositionText(qint32 x, quint16 yOffset, quint16 yBase, qint32 z);
  QString formatHeaderLocationText(quint16 locationCode, quint8 zoneType, qint32 x, qint32 y,
                                   qint32 z);
  static QString formatDaggerfallDate(quint32 minutes);
  static QString recordTypeName(quint8 type);
  static QStringList decodeElementMask(quint8 mask);
  static QStringList decodeRapidHealMask(quint8 mask);
  static QStringList decodeRegenHealthMask(quint8 mask);
  static QString decodeSpellAbsorbMask(quint8 mask);
  static QStringList decodeHitPhobiaMask(quint8 mask);
  static QStringList decodeWeaponExpertMask(quint8 mask);
  static QStringList decodeForbiddenMaterialMask(quint16 mask);
  static QStringList decodeForbiddenArmorMask(quint16 mask);
  static QString computeHealthStatus(int anomalies, int orphans, int parentTypeMismatches);
  static QString raceName(quint8 race);
  static bool isLikelyClassName(const QString& value);
  static QString reflexName(quint8 reflex);
  QString saveFilePath(const QString& fileName) const;

private:
  QString m_SaveFolder;
  const GameDaggerfall* m_Game;
  quint16 m_HP = 0;
  quint16 m_HPMax = 0;
  quint16 m_Mana = 0;
  quint16 m_ManaMax = 0;
  quint32 m_Gold = 0;
  quint8 m_Race = 0xFF;
  QString m_ClassName;
  quint8 m_Reflex = 0xFF;
  QString m_InGameDate;
  QString m_LocationNameDetail;
  QString m_LocationTypeNameDetail;
  QString m_LocationRegionNameDetail;
  int m_LocationTypeIndexDetail = -1;
  QColor m_LocationTypeColorDetail;

  quint32 m_SaveTreeHeaderVersion = 0;
  qint32 m_SaveTreeHeaderX = 0;
  qint32 m_SaveTreeHeaderY = 0;
  qint32 m_SaveTreeHeaderZ = 0;
  quint16 m_SaveTreeLocationCode = 0;
  quint8 m_SaveTreeZoneType = 0;
  quint8 m_SaveTreeLocationDetailCount = 0;
  qsizetype m_SaveTreeRecordStreamStart = 0;
  qsizetype m_SaveTreeRecordStreamEnd = 0;
  int m_SaveTreeRecordCount = 0;
  int m_SaveTreeDungeonRecordCount = 0;
  QMap<quint8, int> m_SaveTreeRecordTypeCounts;
  RecordParseStats m_RecordParseStats;
  int m_SaveTreeRootRecordCount = 0;
  int m_SaveTreeOrphanRecordCount = 0;
  int m_SaveTreeParentTypeMismatchCount = 0;

  quint32 m_CharacterTimestampRaw = 0;
  quint32 m_SaveVarsTimestampRaw = 0;
  qint32 m_SaveVarsTimestampOffset = -1;
  int m_SaveVarsFactionRecordCountEstimate = 0;
  int m_SaveVarsRegionalRepRecordCountEstimate = 0;
  QList<FactionReputationEntry> m_TopFactionReputations;
  qint8 m_MinRegionalReputation = 0;
  qint8 m_MaxRegionalReputation = 0;
  QList<GuildMembershipEntry> m_GuildMemberships;
  int m_BankRegionCount = 0;
  int m_BankPositiveBalanceRegions = 0;
  int m_BankDebtRegions = 0;
  int m_BankDefaultedRegions = 0;
  qint64 m_BankTotalBalance = 0;
  qint64 m_BankTotalDebt = 0;
  quint32 m_BankEarliestDueDateRaw = 0;
  SpellSummary m_SpellSummary;
  int m_ItemRecordCount = 0;
  int m_ItemInContainerCount = 0;
  int m_ItemQuestBoundCount = 0;
  qint64 m_ItemTotalValueRaw = 0;
  int m_ContainerRecordCount = 0;
  QMap<quint16, int> m_ItemCategoryCounts;
  QMap<quint16, QMap<QString, int>> m_ItemCategoryLikelyGroups;

  quint8 m_ResistMask = 0;
  quint8 m_ImmuneMask = 0;
  quint8 m_LowToleranceMask = 0;
  quint8 m_CriticalWeaknessMask = 0;
  quint16 m_Flags1Mask = 0;
  quint8 m_RapidHealMask = 0;
  quint8 m_RegenHealthMask = 0;
  quint8 m_SpellAbsorbMask = 0;
  quint8 m_HitPhobiaMask = 0;
  quint16 m_ForbiddenMaterialMask = 0;
  quint8 m_WeaponExpertMask = 0;
  quint16 m_ForbiddenArmorMask = 0;
};

#endif  // DAGGERFALLS_SAVEGAME_H
