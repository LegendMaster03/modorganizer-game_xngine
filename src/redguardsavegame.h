#pragma once

#include <QString>
#include <QDateTime>
#include <isavegame.h>
#include <memory>

class GameRedguard;

class RedguardSaveGame : public MOBase::ISaveGame
{
public:
  RedguardSaveGame(const QString& saveFolder, const GameRedguard* game);

  // ISaveGame interface
  virtual QString getFilepath() const override;
  virtual QString getName() const override;
  virtual QDateTime getCreationTime() const override;
  virtual QString getSaveGroupIdentifier() const override;
  virtual QStringList allFiles() const override;

private:
  QString mSaveFolder;  // Path to SAVEGAME.XXX folder
  QString mSaveName;    // e.g., "SAVEGAME.001"
  QDateTime mCreationTime;
  const GameRedguard* mGame;
};
