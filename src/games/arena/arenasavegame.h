#ifndef ARENA_SAVEGAME_H
#define ARENA_SAVEGAME_H

#include <xnginesavegame.h>
#include <QString>

class GameArena;

class ArenaSaveGame : public XngineSaveGame
{
public:
  ArenaSaveGame(const QString& saveFolder, const GameArena* game);

private:
  const GameArena* m_Game;
};

#endif  // ARENA_SAVEGAME_H
