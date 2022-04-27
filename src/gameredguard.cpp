#include "gameRedguard.h"

#include "redguardsavegame.h"

#include "pluginsetting.h"
#include "executableinfo.h"
#include "RGMODFrameworkWrapper.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryFile>
#include <iplugingame>

#include <memory>

InstallerRGMOD::InstallerRGMOD() : mMoInfo(nullptr), mRGmodFrameworkWrapper(nullptr)
{
}

using namespace MOBase;

GameRedguard::GameRedguard()
{
}

QString GameRedguard::gameName() const
{
  return "redguard";
}

QList<ExecutableInfo> GameRedguard::executables() const
{
  return QList<ExecutableInfo>()
      << ExecutableInfo("REDGUARD", findInGameFolder(binaryName()))
  ;
}

QList<ExecutableForcedLoadSetting> GameRedguard::executableForcedLoads() const
{
  return QList<ExecutableForcedLoadSetting>()
      << ExecutableForcedLoadSetting("redguard.exe").withForced().withEnabled()
  ;
}

QString GameRedguard::name() const
{
  return "The Elder Scrolls Adventures: Redguard";
}

QString GameRedguard::author() const
{
  return "Legend_Master";
}

QString GameRedguard::description() const
{
  return tr("Adds support for the game Redguard");
}

MOBase::VersionInfo GameRedguard::version() const
{
  return VersionInfo(1, 6, 0, VersionInfo::RELEASE_FINAL);
}

void GameRedguard::initializeProfile(const QDir &path, ProfileSettings settings) const
{
  if (settings.testFlag(IPluginGame::MODS)) {
    copyToProfile(localAppFolder() + "/redguard", path, "plugins.txt");
  }

  if (settings.testFlag(IPluginGame::CONFIGURATION)) {
    if (settings.testFlag(IPluginGame::PREFER_DEFAULTS)
        || !QFileInfo(myGamesPath() + "/redguard.ini").exists()) {
      copyToProfile(gameDirectory().absolutePath(), path, "redguard_default.ini", "redguard.ini");
    } else {
      copyToProfile(myGamesPath(), path, "redguard.ini");
    }

    copyToProfile(myGamesPath(), path, "redguardprefs.ini");
  }
}

QString GameRedguard::savegameExtension() const
{
  return "sav";
}

std::shared_ptr<const GamebryoSaveGame> GameRedguard::makeSaveGame(QString filePath) const
{
  return std::make_shared<const redguardSaveGame>(filePath, this);
}

QString GameRedguard::steamAPPId() const
{
  return "1812410";
}

QString GameRedguard::gogAPPId() const
{
  return "1440162836, 1435829617";
}

QString GameRedguard::gameShortName() const
{
  return "redguard";
}

QString GameRedguard::gameNexusName() const
{
  return "redguard";
}


QStringList GameRedguard::iniFiles() const
{
  return { "system.ini", "registry.ini" };
}

int GameRedguard::nexusModOrganizerID() const
{
  return 38277;
}

int GameRedguard::nexusGameID() const
{
  return 101;
}

bool InstallerRGMOD::init(MOBase::IOrganizer* moInfo)
{
  mMoInfo = moInfo;
  mRGmodFrameworkWrapper = std::make_unique<RGMODFrameworkWrapper>(mMoInfo);
  mMoInfo->onAboutToRun([this](const QString&) { buildSDPs(); return true; });
  mMoInfo->onFinishedRun([this](const QString&, unsigned int) { clearSDPs(); });
  return true;
}

InstallerRGMOD::EInstallResult InstallerRGMOD::install(MOBase::GuessedValue<QString>& modName, QString gameName, const QString& archiveName, const QString& version, int nexusID)
{
  return mRGmodFrameworkWrapper->installInAnotherThread(modName, gameName, archiveName, version, nexusID);
}

MappingType InstallerRGMOD::mappings() const
{
  MappingType mappings;

  for (const auto& [realSDPPath, virtualSDP] : mVirtualSDPs)
    mappings.push_back({ virtualSDP->fileName(), realSDPPath, false, false });

  return mappings;
}