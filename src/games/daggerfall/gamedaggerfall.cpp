#include "gamedaggerfall.h"

// Game-specific feature classes disabled - using XnGine base implementations instead
// #include "daggerfallsmoddatachecker.h"
// #include "daggerfallsmoddatacontent.h"
// #include "daggerfallsavegame.h"

#include <executableinfo.h>
#include <pluginsetting.h>

#include <xnginelocalsavegames.h>
#include <xnginemoddatachecker.h>
#include <xnginemoddatacontent.h>
#include <xnginesavegameinfo.h>
#include <xngineunmanagedmods.h>

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QStandardPaths>
#include <QFile>

#include <Windows.h>

#include <exception>
#include <memory>
#include <stdexcept>

using namespace MOBase;

GameDaggerfall::GameDaggerfall() {}

bool GameDaggerfall::init(IOrganizer* moInfo)
{
  if (!GameXngine::init(moInfo)) {
    return false;
  }

  // DaggerfallsModDataChecker registration disabled - legacy implementation
  // DaggerfallsModDataContent registration disabled - legacy implementation
  // registerFeature(std::make_shared<XngineSaveGameInfo>(this));
  // registerFeature(std::make_shared<XngineLocalSavegames>(this));
  // registerFeature(std::make_shared<XngineUnmanagedMods>(this));

  return true;
}

QString GameDaggerfall::gameName() const
{
  return "The Elder Scrolls Adventures: Daggerfall";
}

QList<ExecutableInfo> GameDaggerfall::executables() const
{
  QList<ExecutableInfo> executables;

  // Steam DOSBox launcher
  QFileInfo steamDosbox(gameDirectory().filePath("DOSBox-0.74/dosbox.exe"));
  if (steamDosbox.exists()) {
    executables << ExecutableInfo("Daggerfall (Steam DOSBox)", steamDosbox)
                   .withArgument("-conf dosbox_daggerfall.conf");
  }

  // GOG DOSBox launcher
  QFileInfo gogDosbox(gameDirectory().filePath("DOSBOX/dosbox.exe"));
  if (gogDosbox.exists()) {
    executables << ExecutableInfo("Daggerfall (GOG DOSBox)", gogDosbox)
                   .withArgument("-conf dosbox_daggerfall.conf");
  }

  // Standalone executable if it exists
  QFileInfo daggerExe(gameDirectory().filePath("DF/DAGGER/DAGGER.EXE"));
  if (daggerExe.exists()) {
    executables << ExecutableInfo("Daggerfall", daggerExe);
  }

  return executables;
}

QString GameDaggerfall::steamAPPId() const
{
  return "275170";
}

QString GameDaggerfall::gogAPPId() const
{
  return "1435829353";
}

QString GameDaggerfall::binaryName() const
{
  return "DAGGER.EXE";
}

QString GameDaggerfall::gameShortName() const
{
  return "Daggerfall";
}

QString GameDaggerfall::gameNexusName() const
{
  return "daggerfall";
}

QStringList GameDaggerfall::validShortNames() const
{
  return {"daggerfall", "df"};
}

int GameDaggerfall::nexusModOrganizerID() const
{
  return 0;  // To be determined
}

int GameDaggerfall::nexusGameID() const
{
  return 232;  // Nexus Game ID for Daggerfall
}

QString GameDaggerfall::name() const
{
  return "The Elder Scrolls Adventures: Daggerfall Support Plugin";
}

QString GameDaggerfall::localizedName() const
{
  return tr("The Elder Scrolls Adventures: Daggerfall Support Plugin");
}

QString GameDaggerfall::author() const
{
  return "Legend_Master";
}

QString GameDaggerfall::description() const
{
  return tr("Adds support for the game The Elder Scrolls Adventures: Daggerfall");
}

VersionInfo GameDaggerfall::version() const
{
  return VersionInfo(1, 0, 0, VersionInfo::RELEASE_FINAL);
}

QList<PluginSetting> GameDaggerfall::settings() const
{
  return QList<PluginSetting>();
}

QString GameDaggerfall::identifyGamePath() const
{
  // Try Steam first (using Steam App ID 275170)
  QString steamPath = findInRegistry(HKEY_LOCAL_MACHINE,
                                     L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 275170",
                                     L"InstallLocation");
  if (!steamPath.isEmpty()) {
    // Verify it has the Steam DOSBox structure
    if (QDir(steamPath + "/DOSBox-0.74").exists() &&
        QFile::exists(steamPath + "/DOSBox-0.74/dosbox.exe") &&
        QFile::exists(steamPath + "/DF/DAGGER/DAGGER.EXE")) {
      qDebug() << "[GameDaggerfall] Found via Steam registry:" << steamPath;
      return steamPath;
    }
  }

  // Try GOG (common GOG install paths)
  QStringList gogPaths = {
      "C:/Program Files (x86)/GOG Galaxy/Games/Daggerfall",
      "C:/Program Files/GOG Galaxy/Games/Daggerfall",
      "C:/Program Files/GOG Galaxy/Games/Daggerfall"};

  for (const QString& gogPath : gogPaths) {
    if (QDir(gogPath).exists() && QDir(gogPath + "/DOSBOX").exists() &&
        QFile::exists(gogPath + "/DOSBOX/dosbox.exe") &&
        (QFile::exists(gogPath + "/dosbox_daggerfall.conf") ||
         QFile::exists(gogPath + "/DF/DAGGER/DAGGER.EXE"))) {
      qDebug() << "[GameDaggerfall] Found via GOG path:" << gogPath;
      return gogPath;
    }
  }

  qDebug() << "[GameDaggerfall] Could not identify game path";
  return {};
}

QDir GameDaggerfall::savesDirectory() const
{
  QDir gameDir = gameDirectory();

  // Steam version: saves in DF/DAGGER/SAVE0-SAVE5
  QDir steamSaves(gameDir.filePath("DF/DAGGER"));
  if (steamSaves.exists() && QDir(steamSaves.filePath("SAVE0")).exists()) {
    return steamSaves;
  }

  // GOG version: saves at root level SAVE0-SAVE5
  if (QDir(gameDir.filePath("SAVE0")).exists()) {
    return gameDir;
  }

  // Fallback to DF/DAGGER directory
  return gameDir.filePath("DF/DAGGER");
}

QString GameDaggerfall::savegameExtension() const
{
  return "sav";
}

QString GameDaggerfall::savegameSEExtension() const
{
  return "sav";
}

std::shared_ptr<const XngineSaveGame> GameDaggerfall::makeSaveGame(QString filepath) const
{
  return std::make_shared<XngineSaveGame>(filepath, this);
}

QString GameDaggerfall::findInRegistry(HKEY baseKey, LPCWSTR path, LPCWSTR value) const
{
  DWORD size = 0;
  HKEY subKey;
  LONG res = ::RegOpenKeyExW(baseKey, path, 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY, &subKey);
  if (res != ERROR_SUCCESS) {
    res = ::RegOpenKeyExW(baseKey, path, 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &subKey);
    if (res != ERROR_SUCCESS)
      return QString();
  }

  std::unique_ptr<wchar_t[]> buffer;
  res = ::RegQueryValueExW(subKey, value, nullptr, nullptr, nullptr, &size);
  if (res != ERROR_SUCCESS)
    return QString();

  buffer = std::make_unique<wchar_t[]>(size / sizeof(wchar_t) + 1);
  res = ::RegQueryValueExW(subKey, value, nullptr, nullptr,
                           reinterpret_cast<LPBYTE>(buffer.get()), &size);
  ::RegCloseKey(subKey);

  if (res != ERROR_SUCCESS)
    return QString();

  return QString::fromWCharArray(buffer.get());
}
