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
#include <QRegularExpression>
#include <QIcon>

#include <Windows.h>

#include "utility.h"

#include <exception>
#include <memory>
#include <stdexcept>

using namespace MOBase;

GameDaggerfall::GameDaggerfall()
{
  OutputDebugStringA("[GameDaggerfall] Constructor ENTRY\n");
  OutputDebugStringA("[GameDaggerfall] Constructor EXIT\n");
}

bool GameDaggerfall::init(IOrganizer* moInfo)
{
  OutputDebugStringA("[GameDaggerfall] init() ENTRY\n");

  try {
    OutputDebugStringA("[GameDaggerfall] About to call GameXngine::init()\n");
    if (!GameXngine::init(moInfo)) {
      OutputDebugStringA("[GameDaggerfall] GameXngine::init() FAILED\n");
      return false;
    }
    OutputDebugStringA("[GameDaggerfall] GameXngine::init() SUCCESS\n");

    const QString iniForLocalSaves = iniFiles().isEmpty() ? QString{} : iniFiles().first();
    registerFeature(std::make_shared<XngineSaveGameInfo>(this));
    registerFeature(std::make_shared<XngineLocalSavegames>(this, iniForLocalSaves));
    registerFeature(std::make_shared<XngineUnmanagedMods>(this));

    OutputDebugStringA("[GameDaggerfall] init() EXIT SUCCESS\n");
    return true;
  } catch (const std::exception&) {
    OutputDebugStringA("[GameDaggerfall] EXCEPTION in init()\n");
    return false;
  } catch (...) {
    OutputDebugStringA("[GameDaggerfall] UNKNOWN EXCEPTION in init()\n");
    return false;
  }
}

QString GameDaggerfall::gameName() const
{
  OutputDebugStringA("[GameDaggerfall] gameName() called\n");
  return "Daggerfall";
}

QString GameDaggerfall::displayGameName() const
{
  return "The Elder Scrolls Adventures: Daggerfall";
}

QList<ExecutableInfo> GameDaggerfall::executables() const
{
  OutputDebugStringA("[GameDaggerfall] executables() ENTRY\n");
  QList<ExecutableInfo> executables;
  QDir gameDir = gameDirectory();
  if (gameDir.path().isEmpty() || !gameDir.exists()) {
    OutputDebugStringA("[GameDaggerfall] executables() - game directory invalid\n");
    return executables;
  }

  // Steam DOSBox launcher
  QFileInfo steamDosbox(gameDir.filePath("DOSBox-0.73/dosbox.exe"));
  QFileInfo steamConfig(gameDir.filePath("DOSBox-0.73/df.conf"));
  if (steamDosbox.exists()) {
    executables << ExecutableInfo("Daggerfall (Steam DOSBox Windowed)", steamDosbox)
                   .withArgument("-noconsole -conf df.conf");
    executables << ExecutableInfo("Daggerfall (Steam DOSBox Fullscreen)", steamDosbox)
                   .withArgument("-noconsole -conf df.conf -fullscreen");
  }

  // GOG DOSBox launcher
  QFileInfo gogDosbox(gameDir.filePath("DOSBOX/dosbox.exe"));
  if (gogDosbox.exists()) {
    executables << ExecutableInfo("Daggerfall (GOG DOSBox)", gogDosbox)
                   .withArgument(R"(-conf "..\dosbox_daggerfall_single.conf" -noconsole -c "exit")");
  }

  // Standalone executable if it exists
  QFileInfo daggerExe(gameDir.filePath("DF/DAGGER/DAGGER.EXE"));
  if (daggerExe.exists()) {
    executables << ExecutableInfo("Daggerfall", daggerExe);
  }

  OutputDebugStringA("[GameDaggerfall] executables() EXIT\n");
  return executables;
}

QString GameDaggerfall::steamAPPId() const
{
  qInfo().noquote() << "[GameDaggerfall] steamAPPId() called";
  OutputDebugStringA("[GameDaggerfall] steamAPPId() called\n");
  return "275170";
}

QString GameDaggerfall::gogAPPId() const
{
  qInfo().noquote() << "[GameDaggerfall] gogAPPId() called";
  OutputDebugStringA("[GameDaggerfall] gogAPPId() called\n");
  return "1435829353";
}

QString GameDaggerfall::binaryName() const
{
  OutputDebugStringA("[GameDaggerfall] binaryName() called\n");
  return "DAGGER.EXE";
}

QString GameDaggerfall::gameShortName() const
{
  OutputDebugStringA("[GameDaggerfall] gameShortName() called\n");
  return "Daggerfall";
}

QString GameDaggerfall::gameNexusName() const
{
  OutputDebugStringA("[GameDaggerfall] gameNexusName() called\n");
  return "daggerfall";
}

QStringList GameDaggerfall::validShortNames() const
{
  OutputDebugStringA("[GameDaggerfall] validShortNames() called\n");
  return {"daggerfall", "df"};
}

QStringList GameDaggerfall::iniFiles() const
{
  const QStringList candidates = {
      "SETUP.INI",
      "DF/DAGGER/SETUP.INI",
      // CASTER.CFG is a possible candidate but does not open in a text editor and may not be a true CFG file - excluding for now
      //"CASTER.CFG",
      //"DF/DAGGER/CASTER.CFG",
      "HMISET.CFG",
      "DF/DAGGER/HMISET.CFG",
      "Z.CFG",
      "DF/DAGGER/Z.CFG"};

  QStringList ordered;
  const QDir root = gameDirectory();
  for (const auto& candidate : candidates) {
    if (QFileInfo::exists(root.filePath(candidate))) {
      ordered.push_back(candidate);
    }
  }
  for (const auto& candidate : candidates) {
    if (!ordered.contains(candidate)) {
      ordered.push_back(candidate);
    }
  }
  return ordered;
}

QIcon GameDaggerfall::gameIcon() const
{
  QDir dir = gameDirectory();
  
  // Try .ico files first
  QStringList icoCandidates = {
      "DF/DAGGER/DAGGER.ICO",
      "DAGGER.ICO",
      "goggame-1435829353.ico"
  };
  
  for (const QString& relPath : icoCandidates) {
    QFileInfo iconFile(dir.filePath(relPath));
    if (iconFile.exists()) {
      return QIcon(iconFile.absoluteFilePath());
    }
  }
  
  // Fallback to EXE icon
  const QString exePath = dir.absoluteFilePath("DF/DAGGER/DAGGER.EXE");
  QIcon icon = MOBase::iconForExecutable(exePath);
  return icon.isNull() ? GameXngine::gameIcon() : icon;
}

int GameDaggerfall::nexusModOrganizerID() const
{
  OutputDebugStringA("[GameDaggerfall] nexusModOrganizerID() called\n");
  return 0;  // To be determined
}

int GameDaggerfall::nexusGameID() const
{
  OutputDebugStringA("[GameDaggerfall] nexusGameID() called\n");
  return 232;  // Nexus Game ID for Daggerfall
}

QString GameDaggerfall::name() const
{
  return "The Elder Scrolls Adventures: Daggerfall Support Plugin";
}

QString GameDaggerfall::localizedName() const
{
  OutputDebugStringA("[GameDaggerfall] localizedName() called\n");
  return tr("The Elder Scrolls Adventures: Daggerfall Support Plugin");
}

QString GameDaggerfall::author() const
{
  OutputDebugStringA("[GameDaggerfall] author() called\n");
  return "Legend_Master";
}

QString GameDaggerfall::description() const
{
  OutputDebugStringA("[GameDaggerfall] description() called\n");
  return tr("Adds support for the game The Elder Scrolls Adventures: Daggerfall");
}

VersionInfo GameDaggerfall::version() const
{
  OutputDebugStringA("[GameDaggerfall] version() called\n");
  return VersionInfo(1, 0, 0, VersionInfo::RELEASE_FINAL);
}

QList<PluginSetting> GameDaggerfall::settings() const
{
  OutputDebugStringA("[GameDaggerfall] settings() called\n");
  return QList<PluginSetting>();
}

QString GameDaggerfall::identifyGamePath() const
{
  OutputDebugStringA("[GameDaggerfall] identifyGamePath() ENTRY\n");
  try {
  // Try Steam first (using Steam App ID 275170)
  QString steamPath = findInRegistry(HKEY_LOCAL_MACHINE,
                                     L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 275170",
                                     L"InstallLocation");
  if (!steamPath.isEmpty()) {
    // Verify it has the Steam DOSBox structure
    if (QDir(steamPath + "/DOSBox-0.74").exists() &&
        QFile::exists(steamPath + "/DOSBox-0.74/dosbox.exe") &&
        QFile::exists(steamPath + "/DF/DAGGER/DAGGER.EXE")) {
      OutputDebugStringA("[GameDaggerfall] Steam path verified\n");
      return steamPath;
    }
  }

  // Try GOG registry (using GOG Game ID 1435829353)
  QString gogPath = findInRegistry(HKEY_LOCAL_MACHINE,
                                   L"Software\\GOG.com\\Games\\1435829353",
                                   L"path");
  if (!gogPath.isEmpty()) {
    // Verify it has the GOG DOSBox structure
    if (QDir(gogPath + "/DOSBOX").exists() &&
        QFile::exists(gogPath + "/DOSBOX/dosbox.exe") &&
        (QFile::exists(gogPath + "/dosbox_daggerfall.conf") ||
         QFile::exists(gogPath + "/DF/DAGGER/DAGGER.EXE"))) {
      OutputDebugStringA("[GameDaggerfall] GOG registry path verified\n");
      return gogPath;
    }
  }

  OutputDebugStringA("[GameDaggerfall] identifyGamePath() EXIT (not found)\n");
  return {};
  } catch (const std::exception&) {
    OutputDebugStringA("[GameDaggerfall] EXCEPTION in identifyGamePath()\n");
    return {};
  } catch (...) {
    OutputDebugStringA("[GameDaggerfall] UNKNOWN EXCEPTION in identifyGamePath()\n");
    return {};
  }
}

QDir GameDaggerfall::savesDirectory() const
{
  return GameXngine::savesDirectory();
}

QString GameDaggerfall::savegameExtension() const
{
  OutputDebugStringA("[GameDaggerfall] savegameExtension() called\n");
  return "sav";
}

QString GameDaggerfall::savegameSEExtension() const
{
  OutputDebugStringA("[GameDaggerfall] savegameSEExtension() called\n");
  return "sav";
}

std::shared_ptr<const XngineSaveGame> GameDaggerfall::makeSaveGame(QString filepath) const
{
  OutputDebugStringA("[GameDaggerfall] makeSaveGame() called\n");
  return std::make_shared<XngineSaveGame>(filepath, this);
}

SaveLayout GameDaggerfall::saveLayout() const
{
  SaveLayout layout;
  layout.baseRelativePaths = {""};
  layout.slotDirRegex = QRegularExpression("^SAVE(\\d+)$");
  layout.slotWidthHint = 1;
  layout.maxSlotHint = 5;
  layout.validator = [](const QDir& slotDir) {
    return slotDir.exists();
  };
  return layout;
}

QString GameDaggerfall::saveGameId() const
{
  return "daggerfall";
}

XngineBSAFormat::Traits GameDaggerfall::bsaTraits() const
{
  XngineBSAFormat::Traits traits;
  traits.allowCompressed = false;
  traits.enforceDos83Names = true;
  traits.normalizeNameCase = true;
  return traits;
}

QVector<XngineBSAFormat::FileSpec> GameDaggerfall::bsaFileSpecs() const
{
  return {
      {"ARCH3D.BSA", true, XngineBSAFormat::IndexType::NumberRecord, false,
       "3D object/mesh records."},
      {"BLOCKS.BSA", true, XngineBSAFormat::IndexType::NameRecord, false,
       "RMB/RDB/RDI block records."},
      {"MAPS.BSA", true, XngineBSAFormat::IndexType::NameRecord, false,
       "Region/location records (MAPNAMES/MAPTABLE/MAPPITEM/MAPDITEM)."},
      {"MONSTER.BSA", true, XngineBSAFormat::IndexType::NameRecord, false,
       "Monster config and animation references."},
      {"MIDI.BSA", true, XngineBSAFormat::IndexType::NameRecord, false,
       "Music records."},
      {"DAGGER.SND", true, XngineBSAFormat::IndexType::NumberRecord, false,
       "Raw PCM audio records."},
      {"MAPSAVE.SAV", true, XngineBSAFormat::IndexType::NameRecord, true,
       "Automap save data records."},
  };
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
