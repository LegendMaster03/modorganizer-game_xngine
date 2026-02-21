#include "arenasavegame.h"

ArenaSaveGame::ArenaSaveGame(const QString& saveFolder,
                             const GameArena* game)
    : XngineSaveGame(saveFolder), m_Game(game)
{
}

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringList>

// Arena save slot detection: looks for SAVEENGN.0x, LOG.0x, SPELLS.0x (x=0..9)
QStringList detectArenaSaveFiles(const QDir& gameDir)
{
    QStringList saveFiles;
    for (int slot = 0; slot <= 9; ++slot) {
        QStringList patterns = {
            QString("SAVEENGN.%1").arg(slot, 2, 10, QChar('0')),
            QString("LOG.%1").arg(slot, 2, 10, QChar('0')),
            QString("SPELLS.%1").arg(slot, 2, 10, QChar('0'))
        };
        for (const auto& pat : patterns) {
            QFileInfo fi(gameDir.filePath(pat));
            if (fi.exists() && fi.isFile()) {
                saveFiles << fi.absoluteFilePath();
            }
        }
    }
    return saveFiles;
}

// Example usage: call detectArenaSaveFiles(gameDir) to get all valid save files.
