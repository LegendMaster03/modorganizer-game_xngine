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

protected:
  virtual std::unique_ptr<DataFields> fetchDataFields() const override;

private:
  bool parseSaveName();
  bool parseSaveTree();
  bool parseSaveVars();
  static QString levelLocationName(quint32 currentLevel);
  QString saveFilePath(const QString& fileName) const;
  static QString readFixedString(const QByteArray& data, qsizetype offset, qsizetype size);

private:
  QString m_SaveFolder;
  QString m_PositionText;
};

#endif  // BATTLESPIRE_SAVEGAME_H
