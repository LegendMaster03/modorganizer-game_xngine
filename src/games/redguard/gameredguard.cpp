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

GameRedguard::GameRedguard() {
  qInfo().noquote() << "[GameRedguard] Constructor ENTRY";
  OutputDebugStringA("[GameRedguard] Constructor ENTRY\n");
  OutputDebugStringA("[GameRedguard] Constructor EXIT\n");
  qInfo().noquote() << "[GameRedguard] Constructor EXIT";
}

bool GameRedguard::init(IOrganizer* moInfo)
{
  qWarning().noquote() << "[GameRedguard] init() DISABLED for crash isolation";
  return false;

  /*
  qInfo().noquote() << "[GameRedguard] init() ENTRY";
  OutputDebugStringA("[GameRedguard] init() ENTRY\n");
  
  try {
    OutputDebugStringA("[GameRedguard] About to call GameXngine::init()\n");
    if (!GameXngine::init(moInfo)) {
      OutputDebugStringA("[GameRedguard] GameXngine::init() FAILED\n");
      qWarning().noquote() << "[GameRedguard] GameXngine::init() FAILED";
      return false;
    }
    OutputDebugStringA("[GameRedguard] GameXngine::init() SUCCESS\n");
    qInfo().noquote() << "[GameRedguard] GameXngine::init() SUCCESS";

    // Test ModDataChecker first
    OutputDebugStringA("[GameRedguard] About to create RedguardsModDataChecker\n");
    try {
      auto checker = std::make_shared<RedguardsModDataChecker>(this);
      OutputDebugStringA("[GameRedguard] RedguardsModDataChecker created successfully\n");
      OutputDebugStringA("[GameRedguard] About to register RedguardsModDataChecker\n");
      qInfo().noquote() << "[GameRedguard] Registering RedguardsModDataChecker";
      registerFeature(checker);
      OutputDebugStringA("[GameRedguard] RedguardsModDataChecker registered successfully\n");
      qInfo().noquote() << "[GameRedguard] RedguardsModDataChecker registered successfully";
    } catch (const std::exception& e) {
      OutputDebugStringA("[GameRedguard] EXCEPTION creating/registering RedguardsModDataChecker\n");
      return false;
    } catch (...) {
      OutputDebugStringA("[GameRedguard] UNKNOWN EXCEPTION creating/registering RedguardsModDataChecker\n");
      return false;
    }
    
    // Keep these disabled for now
    // OutputDebugStringA("[GameRedguard] Registering RedguardsModDataContent\n");
    // registerFeature(std::make_shared<RedguardsModDataContent>(m_Organizer->gameFeatures()));
    
    // registerFeature(std::make_shared<XngineSaveGameInfo>(this));
    // registerFeature(std::make_shared<XngineLocalSavegames>(this));
    // registerFeature(std::make_shared<XngineUnmanagedMods>(this));

    OutputDebugStringA("[GameRedguard] init() EXIT SUCCESS (features disabled for testing)\n");
    qInfo().noquote() << "[GameRedguard] init() EXIT SUCCESS (features disabled for testing)";
    return true;
  } catch (const std::exception& e) {
    OutputDebugStringA("[GameRedguard] EXCEPTION in init()\n");
    qWarning().noquote() << "[GameRedguard] EXCEPTION in init()";
    return false;
  } catch (...) {
    OutputDebugStringA("[GameRedguard] UNKNOWN EXCEPTION in init()\n");
    qWarning().noquote() << "[GameRedguard] UNKNOWN EXCEPTION in init()";
    return false;
  }
  */
}

QString GameRedguard::gameName() const
{
  OutputDebugStringA("[GameRedguard] gameName() called\n");
  return "Redguard";
}

QString GameRedguard::displayGameName() const
{
  qInfo().noquote() << "[GameRedguard] displayGameName() called";
  return "The Elder Scrolls Adventures: Redguard";
}

QList<ExecutableInfo> GameRedguard::executables() const
{
  OutputDebugStringA("[GameRedguard] executables() ENTRY\n");
  QList<ExecutableInfo> executables;
  QDir gameDir = gameDirectory();
  if (gameDir.path().isEmpty() || !gameDir.exists()) {
    OutputDebugStringA("[GameRedguard] executables() - game directory invalid\n");
    return executables;
  }
  
  // Steam DOSBox launcher
  QFileInfo steamDosbox(gameDir.filePath("DOSBox-0.73/dosbox.exe"));
  if (steamDosbox.exists()) {
    executables << ExecutableInfo("Redguard (Steam DOSBox)", steamDosbox)
                   .withArgument(R"(-conf rg.conf -c "cd Redguard" -c "REDGUARD.EXE")");
  }

  // GOG DOSBox launcher
  QFileInfo gogDosbox(gameDir.filePath("DOSBOX/dosbox.exe"));
  if (gogDosbox.exists()) {
    executables << ExecutableInfo("Redguard (GOG DOSBox)", gogDosbox)
                   .withArgument(R"(-conf dosbox_redguard.conf)");
  }

  // Standalone Redguard exe if it exists
  QFileInfo redguardExe(gameDir.filePath("Redguard/REDGUARD.EXE"));
  if (redguardExe.exists()) {
    executables << ExecutableInfo("Redguard", redguardExe);
  }

  OutputDebugStringA("[GameRedguard] executables() EXIT\n");
  return executables;
}

QString GameRedguard::steamAPPId() const
{
  OutputDebugStringA("[GameRedguard] steamAPPId() called\n");
  return "1812410";
}

QString GameRedguard::gogAPPId() const
{
  qInfo().noquote() << "[GameRedguard] gogAPPId() called";
  return "1435829617";
}

QString GameRedguard::binaryName() const
{
  OutputDebugStringA("[GameRedguard] binaryName() called\n");
  return "REDGUARD.EXE";
}

QString GameRedguard::gameShortName() const
{
  OutputDebugStringA("[GameRedguard] gameShortName() called\n");
  return "Redguard";
}

QString GameRedguard::gameNexusName() const
{
  OutputDebugStringA("[GameRedguard] gameNexusName() called\n");
  return "theelderscrollsadventuresredguard";
}

QStringList GameRedguard::validShortNames() const
{
  OutputDebugStringA("[GameRedguard] validShortNames() called\n");
  return {"redguard", "rg"};
}

int GameRedguard::nexusModOrganizerID() const
{
  OutputDebugStringA("[GameRedguard] nexusModOrganizerID() called\n");
  return 6220;  // Nexus MO Organizer ID for Redguard
}

int GameRedguard::nexusGameID() const
{
  OutputDebugStringA("[GameRedguard] nexusGameID() called\n");
  return 4462;  // Nexus Game ID for Redguard
}

QString GameRedguard::name() const
{
  qInfo().noquote() << "[GameRedguard] name() called";
  return "The Elder Scrolls Adventures: Redguard Support Plugin";
}

QString GameRedguard::localizedName() const
{
  OutputDebugStringA("[GameRedguard] localizedName() called\n");
  return tr("The Elder Scrolls Adventures: Redguard Support Plugin");
}

QString GameRedguard::author() const
{
  OutputDebugStringA("[GameRedguard] author() called\n");
  return "Legend_Master";
}

QString GameRedguard::description() const
{
  OutputDebugStringA("[GameRedguard] description() called\n");
  return tr("Adds support for the game The Elder Scrolls Adventures: Redguard");
}

VersionInfo GameRedguard::version() const
{
  OutputDebugStringA("[GameRedguard] version() called\n");
  return VersionInfo(1, 0, 0, VersionInfo::RELEASE_FINAL);
}

QList<PluginSetting> GameRedguard::settings() const
{
  OutputDebugStringA("[GameRedguard] settings() called\n");
  return QList<PluginSetting>();
}

QString GameRedguard::identifyGamePath() const
{
  OutputDebugStringA("[GameRedguard] identifyGamePath() ENTRY\n");
  
  try {
    OutputDebugStringA("[GameRedguard] About to call findInRegistry for Steam\n");
    // Try Steam first (using Steam App ID 1812410)
    QString steamPath = findInRegistry(HKEY_LOCAL_MACHINE,
                                       L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 1812410",
                                       L"InstallLocation");
    OutputDebugStringA("[GameRedguard] findInRegistry returned\n");
    
    if (!steamPath.isEmpty()) {
      OutputDebugStringA("[GameRedguard] Steam path found, verifying structure\n");
      // Verify it has the Steam DOSBox structure
      if (QDir(steamPath + "/DOSBox-0.73").exists() &&
          QFile::exists(steamPath + "/DOSBox-0.73/dosbox.exe") &&
          QFile::exists(steamPath + "/Redguard/REDGUARD.EXE")) {
        OutputDebugStringA("[GameRedguard] Steam path verified\n");
        return steamPath;
      }
      OutputDebugStringA("[GameRedguard] Steam path found but structure invalid\n");
    }

    OutputDebugStringA("[GameRedguard] Checking GOG paths\n");
    // Try GOG (common GOG install paths)
    QStringList gogPaths = {
        "C:/Program Files (x86)/GOG Galaxy/Games/Redguard",
        "C:/Program Files/GOG Galaxy/Games/Redguard",
        "C:/Program Files/GOG Galaxy/Games/Redguard"};

    for (const QString& gogPath : gogPaths) {
      OutputDebugStringA("[GameRedguard] Checking GOG path\n");
      if (QDir(gogPath).exists() && QDir(gogPath + "/DOSBOX").exists() &&
          QFile::exists(gogPath + "/DOSBOX/dosbox.exe") &&
          QFile::exists(gogPath + "/dosbox_redguard.conf")) {
        OutputDebugStringA("[GameRedguard] GOG path verified\n");
        return gogPath;
      }
    }

    OutputDebugStringA("[GameRedguard] identifyGamePath() EXIT (not found)\n");
    return {};
  } catch (const std::exception& e) {
    OutputDebugStringA("[GameRedguard] EXCEPTION in identifyGamePath()\n");
    return {};
  } catch (...) {
    OutputDebugStringA("[GameRedguard] UNKNOWN EXCEPTION in identifyGamePath()\n");
    return {};
  }
}

QString GameRedguard::findInRegistry(HKEY baseKey, LPCWSTR path, LPCWSTR value) const
{
  qInfo().noquote() << "[GameRedguard] findInRegistry() ENTRY";
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
  qInfo().noquote() << "[GameRedguard] savesDirectory() ENTRY";
  QDir gameDir = gameDirectory();
  if (gameDir.path().isEmpty() || !gameDir.exists()) {
    return gameDir;
  }

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
  OutputDebugStringA("[GameRedguard] savegameExtension() called\n");
  return "sav";
}

QString GameRedguard::savegameSEExtension() const
{
  OutputDebugStringA("[GameRedguard] savegameSEExtension() called\n");
  return "sav";
}

std::shared_ptr<const XngineSaveGame> GameRedguard::makeSaveGame(QString filepath) const
{
  OutputDebugStringA("[GameRedguard] makeSaveGame() called\n");
  return std::make_shared<XngineSaveGame>(filepath, this);
}
