#include "gameredguard.h"

#include "redguardsmoddatachecker.h"
#include "redguardsmoddatacontent.h"
#include "redguardsavegame.h"

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
#include <QSettings>
#include <QFile>

#include <Windows.h>
#include <winver.h>

#include <exception>
#include <memory>
#include <stdexcept>

using namespace MOBase;

GameRedguard::GameRedguard() {}

bool GameRedguard::init(IOrganizer* moInfo)
{
  if (!GameXngine::init(moInfo)) {
    return false;
  }

  registerFeature(std::make_shared<RedguardsModDataChecker>(this));
  registerFeature(std::make_shared<RedguardsModDataContent>(m_Organizer->gameFeatures()));
  // registerFeature(std::make_shared<XngineSaveGameInfo>(this));
  // registerFeature(std::make_shared<XngineLocalSavegames>(this));
  // registerFeature(std::make_shared<XngineUnmanagedMods>(this));

  return true;
}

QString GameRedguard::gameName() const
{
  return "The Elder Scrolls Adventures: Redguard";
}

QList<ExecutableInfo> GameRedguard::executables() const
{
  QList<ExecutableInfo> executables;
  
  // Steam DOSBox launcher
  QFileInfo steamDosbox(gameDirectory().filePath("DOSBox-0.73/dosbox.exe"));
  if (steamDosbox.exists()) {
    executables << ExecutableInfo("Redguard (Steam DOSBox)", steamDosbox)
                   .withArgument(R"(-conf rg.conf -c "cd Redguard" -c "REDGUARD.EXE")");
  }

  // GOG DOSBox launcher
  QFileInfo gogDosbox(gameDirectory().filePath("DOSBOX/dosbox.exe"));
  if (gogDosbox.exists()) {
    executables << ExecutableInfo("Redguard (GOG DOSBox)", gogDosbox)
                   .withArgument(R"(-conf dosbox_redguard.conf)");
  }

  // Standalone Redguard exe if it exists
  QFileInfo redguardExe(gameDirectory().filePath("Redguard/REDGUARD.EXE"));
  if (redguardExe.exists()) {
    executables << ExecutableInfo("Redguard", redguardExe);
  }

  return executables;
}

QString GameRedguard::steamAPPId() const
{
  return "1812410";
}

QString GameRedguard::gogAPPId() const
{
  return "1435829617";
}

QString GameRedguard::binaryName() const
{
  return "REDGUARD.EXE";
}

QString GameRedguard::gameShortName() const
{
  return "Redguard";
}

QString GameRedguard::gameNexusName() const
{
  return "theelderscrollsadventuresredguard";
}

QStringList GameRedguard::validShortNames() const
{
  return {"redguard", "rg"};
}

int GameRedguard::nexusModOrganizerID() const
{
  return 6220;  // Nexus MO Organizer ID for Redguard
}

int GameRedguard::nexusGameID() const
{
  return 4462;  // Nexus Game ID for Redguard
}

QString GameRedguard::name() const
{
  return "The Elder Scrolls Adventures: Redguard Support Plugin";
}

QString GameRedguard::localizedName() const
{
  return tr("The Elder Scrolls Adventures: Redguard Support Plugin");
}

QString GameRedguard::author() const
{
  return "Legend_Master";
}

QString GameRedguard::description() const
{
  return tr("Adds support for the game The Elder Scrolls Adventures: Redguard");
}

VersionInfo GameRedguard::version() const
{
  return VersionInfo(1, 0, 0, VersionInfo::RELEASE_FINAL);
}

QList<PluginSetting> GameRedguard::settings() const
{
  return QList<PluginSetting>();
}

QString GameRedguard::identifyGamePath() const
{
  // Try Steam first (using Steam App ID 1812410)
  QString steamPath = findInRegistry(HKEY_LOCAL_MACHINE,
                                     L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 1812410",
                                     L"InstallLocation");
  if (!steamPath.isEmpty()) {
    // Verify it has the Steam DOSBox structure
    if (QDir(steamPath + "/DOSBox-0.73").exists() &&
        QFile::exists(steamPath + "/DOSBox-0.73/dosbox.exe") &&
        QFile::exists(steamPath + "/Redguard/REDGUARD.EXE")) {
      qDebug() << "[GameRedguard] Found via Steam registry:" << steamPath;
      return steamPath;
    }
  }

  // Try GOG (common GOG install paths)
  QStringList gogPaths = {
      "C:/Program Files (x86)/GOG Galaxy/Games/Redguard",
      "C:/Program Files/GOG Galaxy/Games/Redguard",
      "C:/Program Files/GOG Galaxy/Games/Redguard"};

  for (const QString& gogPath : gogPaths) {
    if (QDir(gogPath).exists() && QDir(gogPath + "/DOSBOX").exists() &&
        QFile::exists(gogPath + "/DOSBOX/dosbox.exe") &&
        QFile::exists(gogPath + "/dosbox_redguard.conf")) {
      qDebug() << "[GameRedguard] Found via GOG path:" << gogPath;
      return gogPath;
    }
  }

  qDebug() << "[GameRedguard] Could not identify game path";
  return {};
}

QString GameRedguard::findInRegistry(HKEY baseKey, LPCWSTR path, LPCWSTR value) const
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

QDir GameRedguard::savesDirectory() const
{
  QDir gameDir = gameDirectory();

  // Steam version: saves in Redguard/SAVEGAME/
  QDir steamSaves(gameDir.filePath("Redguard/SAVEGAME"));
  if (steamSaves.exists()) {
    return steamSaves;
  }

  // GOG version: saves in SAVE/
  QDir gogSaves(gameDir.filePath("SAVE"));
  if (gogSaves.exists()) {
    return gogSaves;
  }

  // Fallback to Redguard directory
  return gameDir.filePath("Redguard");
}

QString GameRedguard::savegameExtension() const
{
  return "sav";
}

QString GameRedguard::savegameSEExtension() const
{
  return "sav";
}

std::shared_ptr<const XngineSaveGame> GameRedguard::makeSaveGame(QString filepath) const
{
  return std::make_shared<XngineSaveGame>(filepath, this);
}
