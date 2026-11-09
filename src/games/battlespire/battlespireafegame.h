#ifndef BATTLESPIREAFEGAME_H
#define BATTLESPIREAFEGAME_H

#include <xnginesavegame.h>

class GameBattlespire;

// Battlespire save-folder adapter using the shared XngineSaveGame metadata.
// The game pointer is accepted but not currently used by this constructor.

class BattleSpireSaveGame : public XngineSaveGame
{
public:
  BattleSpireSaveGame(QString const& folder, GameBattlespire const* game)
      : XngineSaveGame(folder)
  {
  }
};

#endif  // BATTLESPIREAFEGAME_H
