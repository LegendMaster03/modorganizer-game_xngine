#include "daggerfallsavegame.h"

DaggerfallsSaveGame::DaggerfallsSaveGame(const QString& saveFolder,
                                         const GameDaggerfall* game)
    : XngineSaveGame(saveFolder), m_Game(game)
{
}
