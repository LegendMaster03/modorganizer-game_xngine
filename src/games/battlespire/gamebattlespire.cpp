#include "gamebattlespire.h"

// Game-specific feature classes disabled - using XnGine base implementations instead
// #include "battlespiredatachecker.h"
// #include "battlespiremodatacontent.h"
// #include "battlespireafegame.h"

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

GameBattlespire::GameBattlespire() {}

bool GameBattlespire::init(IOrganizer* moInfo)
{
  if (!GameXngine::init(moInfo)) {
    return false;
  }

  // BattlespiresModDataChecker registration disabled - legacy implementation
  // BattlespireModDataContent registration disabled - legacy implementation
  // registerFeature(std::make_shared<XngineSaveGameInfo>(this));
  // registerFeature(std::make_shared<XngineLocalSavegames>(this));
  // registerFeature(std::make_shared<XngineUnmanagedMods>(this));

  return true;
}

QString GameBattlespire::gameName() const
{
  return "An Elder Scrolls Legend: Battlespire";
}

QList<ExecutableInfo> GameBattlespire::executables() const
{
  QList<ExecutableInfo> executables;
  QDir gameDir = gameDirectory();

  // Steam DOSBox 0.73 launcher
  QFileInfo steamDosbox(gameDir.filePath("DOSBox-0.73/dosbox.exe"));
  if (steamDosbox.exists()) {
    executables << ExecutableInfo("Battlespire (Steam DOSBox)", steamDosbox)
                   .withArgument("-conf bs_single.conf");
  }

  // GOG DOSBox launcher (if different version)
  QFileInfo gogDosbox(gameDir.filePath("DOSBox/dosbox.exe"));
  if (gogDosbox.exists() && !steamDosbox.exists()) {
    executables << ExecutableInfo("Battlespire (GOG DOSBox)", gogDosbox)
                   .withArgument("-conf bs_single.conf");
  }

  // Standalone SPIRE.BAT if available
  QFileInfo spireBat(gameDir.filePath("SPIRE.BAT"));
  if (spireBat.exists()) {
    executables << ExecutableInfo("Battlespire", spireBat);
  }

  // Fallback: look for any dosbox.exe in root
  if (executables.empty()) {
    QFileInfo fallbackDosbox(gameDir.filePath("dosbox.exe"));
    if (fallbackDosbox.exists()) {
      executables << ExecutableInfo("Battlespire (DOSBox)", fallbackDosbox)
                     .withArgument("-conf bs_single.conf");
    }
  }

  return executables;
}

QString GameBattlespire::steamAPPId() const
{
  return "1812420";
}

QString GameBattlespire::gogAPPId() const
{
  return "1435829464";
}

QString GameBattlespire::binaryName() const
{
  return "BATTLESP.EXE";
}

QString GameBattlespire::gameShortName() const
{
  return "Battlespire";
}

QString GameBattlespire::gameNexusName() const
{
  return "anelderscrollslegendbattlespire";
}

QStringList GameBattlespire::validShortNames() const
{
  return {"battlespire", "an elder scrolls legend", "tesbattlespire"};
}

int GameBattlespire::nexusModOrganizerID() const
{
  return 0;  // To be determined
}

int GameBattlespire::nexusGameID() const
{
  return 1788;  // Nexus Game ID for Battlespire/An Elder Scrolls Legend: Battlespire
}

QString GameBattlespire::name() const
{
  return "An Elder Scrolls Legend: Battlespire Support Plugin";
}

QString GameBattlespire::localizedName() const
{
  return tr("An Elder Scrolls Legend: Battlespire Support Plugin");
}

QString GameBattlespire::author() const
{
  return "Legend_Master";
}

QString GameBattlespire::description() const
{
  return tr("Adds support for the game An Elder Scrolls Legend: Battlespire");
}

MOBase::VersionInfo GameBattlespire::version() const
{
  return VersionInfo(1, 0, 0, VersionInfo::RELEASE_FINAL);
}

QList<PluginSetting> GameBattlespire::settings() const
{
  return QList<PluginSetting>();
}

QString GameBattlespire::identifyGamePath() const
{
  // Try Steam first (using Steam App ID 1812420)
  QString steamPath = findInRegistry(HKEY_LOCAL_MACHINE,
                                     L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 1812420",
                                     L"InstallLocation");
  if (!steamPath.isEmpty()) {
    if (QDir(steamPath).exists() && QDir(steamPath + "/GAMEDATA").exists()) {
      qDebug() << "[GameBattlespire] Found via Steam registry:" << steamPath;
      return steamPath;
    }
  }

  // Try GOG (common GOG install paths)
  QStringList gogPaths = {
      "C:/Program Files (x86)/GOG Galaxy/Games/An Elder Scrolls Legend Battlespire",
      "C:/Program Files/GOG Galaxy/Games/An Elder Scrolls Legend Battlespire",
      "D:/SteamLibrary/steamapps/common/An Elder Scrolls Legend Battlespire"};

  for (const QString& gogPath : gogPaths) {
    if (QDir(gogPath).exists() && QDir(gogPath + "/GAMEDATA").exists()) {
      qDebug() << "[GameBattlespire] Found via GOG path:" << gogPath;
      return gogPath;
    }
  }

  qDebug() << "[GameBattlespire] Could not identify game path";
  return {};
}

QDir GameBattlespire::savesDirectory() const
{
  QDir gameDir = gameDirectory();

  // Battlespire saves are at root level (SAVE0-SAVE9)
  if (QDir(gameDir.filePath("SAVE0")).exists()) {
    return gameDir;
  }

  return gameDir;
}

QString GameBattlespire::savegameExtension() const
{
  return "sav";
}

QString GameBattlespire::savegameSEExtension() const
{
  return "sav";
}

std::shared_ptr<const XngineSaveGame> GameBattlespire::makeSaveGame(QString filepath) const
{
  return std::make_shared<XngineSaveGame>(filepath, this);
}

QString GameBattlespire::findInRegistry(HKEY baseKey, LPCWSTR path, LPCWSTR value) const
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
