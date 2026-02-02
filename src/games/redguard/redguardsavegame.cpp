#include "redguardsavegame.h"

RedguardsSaveGame::RedguardsSaveGame(const QString& saveFolder,
                                     const GameRedguard* game)
    : XngineSaveGame(saveFolder), m_Game(game)
{
}
