#ifndef DAGGERFALLS_SAVEGAME_H
#define DAGGERFALLS_SAVEGAME_H

#include <xnginesavegame.h>

#include <QString>

class GameDaggerfall;

/**
 * Daggerfall-specific save game handler.
 * Daggerfall saves are numbered SAVE0 through SAVE5 (6 save slots)
 */
class DaggerfallsSaveGame : public XngineSaveGame
{
public:
  DaggerfallsSaveGame(const QString& saveFolder, const GameDaggerfall* game);

private:
  const GameDaggerfall* m_Game;
};

#endif  // DAGGERFALLS_SAVEGAME_H
