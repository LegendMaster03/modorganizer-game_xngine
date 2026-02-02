#ifndef ARENASAVEGAME_H
#define ARENASAVEGAME_H

#include <xnginesavegame.h>

class GameArena;

class ArenaSaveGame : public XngineSaveGame
{
public:
  ArenaSaveGame(QString const& folder, GameArena const* game) : XngineSaveGame(folder) {}
};

#endif  // ARENASAVEGAME_H
