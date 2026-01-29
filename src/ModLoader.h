#pragma once

#include <QString>
#include <QStringList>

// Mod loading engine (minimal for testing)
class ModLoader {
public:
  ModLoader(void* moInfo);

  bool buildModdedGameFiles(const QString& gamePath, const QString& dataPath);

private:
  QStringList getEnabledMods() const;
  QString getModPath(const QString& modName) const;

  void* mMoInfo;
};

