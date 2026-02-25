#ifndef BATTLESPIRE_SAVEGAME_H
#define BATTLESPIRE_SAVEGAME_H

#include <xnginesavegame.h>

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
};

#endif  // BATTLESPIRE_SAVEGAME_H
