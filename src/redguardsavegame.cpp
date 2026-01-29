#include "redguardsavegame.h"
#include "gameRedguard.h"
#include <QFileInfo>
#include <QDir>

RedguardSaveGame::RedguardSaveGame(const QString& saveFolder, const GameRedguard* game)
    : mSaveFolder(saveFolder), mGame(game)
{
  QFileInfo folderInfo(saveFolder);
  mSaveName = folderInfo.fileName();  // e.g., "SAVEGAME.001"
  
  // Get creation time from SAVEGAME.SAV file inside the folder
  QString saveFile = saveFolder + "/SAVEGAME.SAV";
  QFileInfo saveFileInfo(saveFile);
  if (saveFileInfo.exists()) {
    mCreationTime = saveFileInfo.lastModified();
  } else {
    mCreationTime = folderInfo.lastModified();
  }
}

QString RedguardSaveGame::getFilepath() const
{
  return mSaveFolder;
}

QString RedguardSaveGame::getName() const
{
  return mSaveName;  // e.g., "SAVEGAME.001"
}

QDateTime RedguardSaveGame::getCreationTime() const
{
  return mCreationTime;
}

QString RedguardSaveGame::getSaveGroupIdentifier() const
{
  // All Redguard saves are in the same group
  return "";
}

QStringList RedguardSaveGame::allFiles() const
{
  QStringList files;
  QDir saveDir(mSaveFolder);
  
  // List all files in the save folder
  QFileInfoList fileList = saveDir.entryInfoList(QDir::Files);
  for (const QFileInfo& fileInfo : fileList) {
    files.append(fileInfo.absoluteFilePath());
  }
  
  return files;
}
