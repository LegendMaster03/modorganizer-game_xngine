#ifndef BATTLESPIREAFEGAME_H
#define BATTLESPIREAFEGAME_H

#include <xnginesavegame.h>

class GameBattlespire;

class BattleSpireSaveGame : public XngineSaveGame
{
public:
  BattleSpireSaveGame(QString const& folder, GameBattlespire const* game)
      : XngineSaveGame(folder)
  {
  }
};

#endif  // BATTLESPIREAFEGAME_H
