#ifndef BATTLESPIRE_SAVEGAME_H
#define BATTLESPIRE_SAVEGAME_H

#include <xnginesavegame.h>

#include <QHash>
#include <QStringList>

class GameBattlespire;

class BattlespireSaveGame : public XngineSaveGame
{
public:
  BattlespireSaveGame(QString const& folder, GameBattlespire const* game);
  virtual QString getName() const override;
  virtual QString getSaveGroupIdentifier() const override;
  virtual QString getGameDetails() const override;

protected:
  virtual std::unique_ptr<DataFields> fetchDataFields() const override;

private:
  bool parseSaveName();
  bool parseSaveTree();
  bool parseSaveVars();
  bool parsePlayerBlockFromSaveVars(const QByteArray& data);
  void evaluateDeveloperValidation();
  bool hasAnySavePayload() const;
  static QString raceName(quint8 raceId);
  static QString levelLocationName(quint32 currentLevel);
  QString saveFilePath(const QString& fileName) const;
  static QString readFixedString(const QByteArray& data, qsizetype offset, qsizetype size);

private:
  QString m_SaveFolder;
  QString m_PositionText;
  quint16 m_SpellPoints = 0;
  quint16 m_SpellPointsMax = 0;
  quint32 m_Gold = 0;
  qint32 m_Wounds = 0;
  qint32 m_WoundsMax = 0;
  quint32 m_CurrentLevelId = 0;
  quint8 m_Race = 0xFF;
  QString m_ClassName;

  bool m_IsEmptySlot = false;

  // Developer detail fields
  bool m_HasSaveName = false;
  bool m_HasSaveTree = false;
  bool m_HasSaveVars = false;
  bool m_HasImage = false;
  bool m_PlayerRecordFound = false;
  bool m_PlayerRecordByTypeFound = false;
  bool m_PlayerRecordByIdFound = false;
  bool m_LevelReadFromAlternateOffset = false;
  bool m_ValidationLikelyModified = false;
  quint32 m_SaveTreeVersion = 0;
  quint32 m_SaveTreeTailBytes = 0;
  quint32 m_CurrentTimestamp = 0;
  quint32 m_ActiveSpellsMask = 0;
  quint32 m_CharacterFlagsMask = 0;
  quint32 m_TeamValue = 0;
  quint32 m_GoalValue = 0;
  quint64 m_GoldAccumulator = 0;
  int m_RecordCountTotal = 0;
  int m_GoldItemRecordCount = 0;
  int m_CurrentLevelOffset = -1;
  int m_ConversationMapCount = 0;
  int m_StaticEnemyCount = 0;
  int m_GlobalVariableCount = 0;
  int m_LocalVariableCount = 0;
  int m_MonsterTypeCountNonZero = 0;
  int m_MonsterTypeCountTotal = 0;
  QHash<int, int> m_RecordTypeCounts;
  QStringList m_ActiveSpellNames;
  QStringList m_CharacterFlagNames;
  QStringList m_ValidationNotes;
};

#endif  // BATTLESPIRE_SAVEGAME_H
