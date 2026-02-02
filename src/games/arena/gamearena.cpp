#include "gamearena.h"

// Game-specific feature classes disabled - using XnGine base implementations instead
// #include "arenasmoddatachecker.h"
// #include "arenasmoddatacontent.h"
// #include "arenasavegame.h"

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

GameArena::GameArena() {}

bool GameArena::init(IOrganizer* moInfo)
{
  if (!GameXngine::init(moInfo)) {
    return false;
  }

  // ArenasModDataChecker registration disabled - legacy implementation
  // ArenasModDataContent registration disabled - legacy implementation
  // registerFeature(std::make_shared<XngineSaveGameInfo>(this));
  // registerFeature(std::make_shared<XngineLocalSavegames>(this));
  // registerFeature(std::make_shared<XngineUnmanagedMods>(this));

  return true;
}

QString GameArena::gameName() const
{
  return "The Elder Scrolls: Arena";
}

QList<ExecutableInfo> GameArena::executables() const
{
  QList<ExecutableInfo> executables;
  QDir gameDir = gameDirectory();

  // Steam DOSBox 0.74 launcher
  QFileInfo steamDosbox(gameDir.filePath("DOSBox-0.74/dosbox.exe"));
  if (steamDosbox.exists()) {
    executables << ExecutableInfo("Arena (Steam DOSBox)", steamDosbox)
                   .withArgument("-conf dosbox.conf");
  }

  // GOG DOSBox launcher (if different version)
  QFileInfo gogDosbox(gameDir.filePath("DOSBox/dosbox.exe"));
  if (gogDosbox.exists() && !steamDosbox.exists()) {
    executables << ExecutableInfo("Arena (GOG DOSBox)", gogDosbox)
                   .withArgument("-conf dosbox.conf");
  }

  // Standalone executable if it exists
  QFileInfo arenaExe(gameDir.filePath("ARENA.EXE"));
  if (arenaExe.exists()) {
    executables << ExecutableInfo("Arena", arenaExe);
  }

  // Fallback: look for any dosbox.exe
  if (executables.empty()) {
    QFileInfo fallbackDosbox(gameDir.filePath("dosbox.exe"));
    if (fallbackDosbox.exists()) {
      executables << ExecutableInfo("Arena (DOSBox)", fallbackDosbox)
                     .withArgument("-conf dosbox.conf");
    }
  }

  return executables;
}

QString GameArena::steamAPPId() const
{
  return "1812290";
}

QString GameArena::gogAPPId() const
{
  return "1435828982";
}

QString GameArena::binaryName() const
{
  return "ARENA.EXE";
}

QString GameArena::gameShortName() const
{
  return "Arena";
}

QString GameArena::gameNexusName() const
{
  return "tesarena";
}

QStringList GameArena::validShortNames() const
{
  return {"arena", "tes1"};
}

int GameArena::nexusModOrganizerID() const
{
  return 0;  // To be determined
}

int GameArena::nexusGameID() const
{
  return 3;  // Nexus Game ID for Arena
}

QString GameArena::name() const
{
  return "The Elder Scrolls: Arena Support Plugin";
}

QString GameArena::localizedName() const
{
  return tr("The Elder Scrolls: Arena Support Plugin");
}

QString GameArena::author() const
{
  return "Legend_Master";
}

QString GameArena::description() const
{
  return tr("Adds support for the game The Elder Scrolls: Arena");
}

VersionInfo GameArena::version() const
{
  return VersionInfo(1, 0, 0, VersionInfo::RELEASE_FINAL);
}

QList<PluginSetting> GameArena::settings() const
{
  return QList<PluginSetting>();
}

QString GameArena::identifyGamePath() const
{
  // Try GOG install paths (Arena not on Steam currently)
  QStringList gogPaths = {
      "C:/Program Files (x86)/GOG Galaxy/Games/Arena",
      "C:/Program Files/GOG Galaxy/Games/Arena",
      "C:/Program Files/GOG Galaxy/Games/Arena"};

  for (const QString& gogPath : gogPaths) {
    if (QDir(gogPath).exists() && QFile::exists(gogPath + "/ARENA.EXE")) {
      qDebug() << "[GameArena] Found via GOG path:" << gogPath;
      return gogPath;
    }
  }

  qDebug() << "[GameArena] Could not identify game path";
  return {};
}

QDir GameArena::savesDirectory() const
{
  QDir gameDir = gameDirectory();

  // Arena saves location varies - typically at game root or specific folder
  // This is a placeholder and may need adjustment based on testing
  QDir saves(gameDir.filePath("SAVE"));
  if (saves.exists()) {
    return saves;
  }

  return gameDir;
}

QString GameArena::savegameExtension() const
{
  return "sav";
}

QString GameArena::savegameSEExtension() const
{
  return "sav";
}

std::shared_ptr<const XngineSaveGame> GameArena::makeSaveGame(QString filepath) const
{
  return std::make_shared<XngineSaveGame>(filepath, this);
}

QString GameArena::findInRegistry(HKEY baseKey, LPCWSTR path, LPCWSTR value) const
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
