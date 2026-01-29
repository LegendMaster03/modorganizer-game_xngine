#include "ModLoader.h"

ModLoader::ModLoader(void* moInfo) : mMoInfo(moInfo)
{
}

bool ModLoader::buildModdedGameFiles(const QString& gamePath, const QString& dataPath)
{
  // TODO: Implement mod file building
  return true;
}

QStringList ModLoader::getEnabledMods() const
{
  // TODO: Get enabled mods from MO2
  return QStringList();
}

QString ModLoader::getModPath(const QString& modName) const
{
  // TODO: Get mod path from MO2
  return QString();
}

