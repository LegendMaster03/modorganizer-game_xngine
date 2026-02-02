#ifndef REDGUARDS_SAVEGAME_H
#define REDGUARDS_SAVEGAME_H

#include <xnginesavegame.h>

#include <QString>
#include <QDateTime>

class GameRedguard;

/**
 * Redguard-specific save game handler.
 * Redguard saves are folders named SAVEGAME.XXX containing SAVEGAME.SAV
 */
class RedguardsSaveGame : public XngineSaveGame
{
public:
  RedguardsSaveGame(const QString& saveFolder, const GameRedguard* game);

private:
  const GameRedguard* m_Game;
};

#endif  // REDGUARDS_SAVEGAME_H
