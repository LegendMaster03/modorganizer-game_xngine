#include "gamedaggerfall.h"

// Game-specific feature classes disabled - using XnGine base implementations instead
// #include "daggerfallsmoddatachecker.h"
// #include "daggerfallsmoddatacontent.h"
#include "daggerfallsavegame.h"
#if defined(XNGINE_DAGGERFALL_EXE_PATCHING)
#include "daggerfallfallexehacks.h"
#endif

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

#if defined(_DEBUG)
#define DF_TRACE(msg) OutputDebugStringA(msg)
#else
#define DF_TRACE(msg) ((void)0)
#endif

using namespace MOBase;

#if defined(XNGINE_DAGGERFALL_EXE_PATCHING)
namespace
{
void validateUserFallExePatchCatalog(const QDir& gameDir)
{
  if (gameDir.path().isEmpty() || !gameDir.exists()) {
    return;
  }

  const QString catalogPath = gameDir.absoluteFilePath("fall_exe_patches.json");
  if (!QFileInfo::exists(catalogPath)) {
    return;
  }

  QString err;
  if (!DaggerfallFallExeHacks::validatePatchJsonFile(catalogPath, &err)) {
    qWarning().noquote() << "[GameDaggerfall] Invalid fall_exe_patches.json:" << err;
    DF_TRACE("[GameDaggerfall] fall_exe_patches.json validation FAILED\n");
    return;
  }

  qInfo().noquote() << "[GameDaggerfall] Validated user patch catalog:" << catalogPath;
  DF_TRACE("[GameDaggerfall] fall_exe_patches.json validation OK\n");
}
}  // namespace
#endif

GameDaggerfall::GameDaggerfall()
{
  DF_TRACE("[GameDaggerfall] Constructor ENTRY\n");
  DF_TRACE("[GameDaggerfall] Constructor EXIT\n");
}

bool GameDaggerfall::init(IOrganizer* moInfo)
{
  DF_TRACE("[GameDaggerfall] init() ENTRY\n");

  try {
    DF_TRACE("[GameDaggerfall] About to call GameXngine::init()\n");
    if (!GameXngine::init(moInfo)) {
      DF_TRACE("[GameDaggerfall] GameXngine::init() FAILED\n");
      return false;
    }
    DF_TRACE("[GameDaggerfall] GameXngine::init() SUCCESS\n");

    const QString iniForLocalSaves = iniFiles().isEmpty() ? QString{} : iniFiles().first();
    registerFeature(std::make_shared<XngineSaveGameInfo>(this));
    registerFeature(std::make_shared<XngineLocalSavegames>(this, iniForLocalSaves));
    registerFeature(std::make_shared<XngineUnmanagedMods>(this));
#if defined(XNGINE_DAGGERFALL_EXE_PATCHING)
    validateUserFallExePatchCatalog(gameDirectory());
#endif

    DF_TRACE("[GameDaggerfall] init() EXIT SUCCESS\n");
    return true;
  } catch (const std::exception&) {
    DF_TRACE("[GameDaggerfall] EXCEPTION in init()\n");
    return false;
  } catch (...) {
    DF_TRACE("[GameDaggerfall] UNKNOWN EXCEPTION in init()\n");
    return false;
  }
}

QString GameDaggerfall::gameName() const
{
  DF_TRACE("[GameDaggerfall] gameName() called\n");
  return "Daggerfall";
}

QString GameDaggerfall::displayGameName() const
{
  return "The Elder Scrolls Adventures: Daggerfall";
}

QList<ExecutableInfo> GameDaggerfall::executables() const
{
  DF_TRACE("[GameDaggerfall] executables() ENTRY\n");
  QList<ExecutableInfo> executables;
  QDir gameDir = gameDirectory();
  if (gameDir.path().isEmpty() || !gameDir.exists()) {
    DF_TRACE("[GameDaggerfall] executables() - game directory invalid\n");
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
                   .withArgument(R"(-conf "..\dosbox_daggerfall.conf" -conf "..\dosbox_daggerfall_single.conf" -noconsole -c "exit")");
  }

  // Standalone executable if it exists
  QFileInfo daggerExe(gameDir.filePath("DF/DAGGER/DAGGER.EXE"));
  if (daggerExe.exists()) {
    executables << ExecutableInfo("Daggerfall", daggerExe);
  }

  DF_TRACE("[GameDaggerfall] executables() EXIT\n");
  return executables;
}

QString GameDaggerfall::steamAPPId() const
{
  DF_TRACE("[GameDaggerfall] steamAPPId() called\n");
  return "275170";
}

QString GameDaggerfall::gogAPPId() const
{
  DF_TRACE("[GameDaggerfall] gogAPPId() called\n");
  return "1435829353";
}

QString GameDaggerfall::binaryName() const
{
  DF_TRACE("[GameDaggerfall] binaryName() called\n");
  return "DAGGER.EXE";
}

QString GameDaggerfall::gameShortName() const
{
  DF_TRACE("[GameDaggerfall] gameShortName() called\n");
  return "Daggerfall";
}

QString GameDaggerfall::gameNexusName() const
{
  DF_TRACE("[GameDaggerfall] gameNexusName() called\n");
  return "daggerfall";
}

QStringList GameDaggerfall::validShortNames() const
{
  DF_TRACE("[GameDaggerfall] validShortNames() called\n");
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
  DF_TRACE("[GameDaggerfall] nexusModOrganizerID() called\n");
  return 0;  // To be determined
}

int GameDaggerfall::nexusGameID() const
{
  DF_TRACE("[GameDaggerfall] nexusGameID() called\n");
  return 232;  // Nexus Game ID for Daggerfall
}

QString GameDaggerfall::name() const
{
  return "The Elder Scrolls Adventures: Daggerfall Support Plugin";
}

QString GameDaggerfall::localizedName() const
{
  DF_TRACE("[GameDaggerfall] localizedName() called\n");
  return tr("The Elder Scrolls Adventures: Daggerfall Support Plugin");
}

QString GameDaggerfall::author() const
{
  DF_TRACE("[GameDaggerfall] author() called\n");
  return "Legend_Master";
}

QString GameDaggerfall::description() const
{
  DF_TRACE("[GameDaggerfall] description() called\n");
  return tr("Adds support for the game The Elder Scrolls Adventures: Daggerfall");
}

VersionInfo GameDaggerfall::version() const
{
  DF_TRACE("[GameDaggerfall] version() called\n");
  return VersionInfo(1, 0, 0, VersionInfo::RELEASE_FINAL);
}

QList<PluginSetting> GameDaggerfall::settings() const
{
  DF_TRACE("[GameDaggerfall] settings() called\n");
  return QList<PluginSetting>();
}

QString GameDaggerfall::identifyGamePath() const
{
  DF_TRACE("[GameDaggerfall] identifyGamePath() ENTRY\n");
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
      DF_TRACE("[GameDaggerfall] Steam path verified\n");
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
      DF_TRACE("[GameDaggerfall] GOG registry path verified\n");
      return gogPath;
    }
  }

  DF_TRACE("[GameDaggerfall] identifyGamePath() EXIT (not found)\n");
  return {};
  } catch (const std::exception&) {
    DF_TRACE("[GameDaggerfall] EXCEPTION in identifyGamePath()\n");
    return {};
  } catch (...) {
    DF_TRACE("[GameDaggerfall] UNKNOWN EXCEPTION in identifyGamePath()\n");
    return {};
  }
}

QDir GameDaggerfall::savesDirectory() const
{
  return GameXngine::savesDirectory();
}

MappingType GameDaggerfall::mappings() const
{
  MappingType out = GameXngine::mappings();

  const auto profile = profilePath();
  if (profile.isEmpty()) {
    return out;
  }

  const auto layout = saveLayout();
  const auto paths = resolveSaveStorage(profile, saveGameId());
  ensureSaveDirsExist(paths, layout, saveSlotPrefix());

  const QDir gameDir = gameDirectory();
  const QString sourceCloudRoot = paths.gameSavesRoot;
  const QString targetCloudRoot = gameDir.absoluteFilePath("cloud_saves");
  out.push_back({sourceCloudRoot, targetCloudRoot, true, true});

  return out;
}

QString GameDaggerfall::savegameExtension() const
{
  DF_TRACE("[GameDaggerfall] savegameExtension() called\n");
  return "sav";
}

QString GameDaggerfall::savegameSEExtension() const
{
  DF_TRACE("[GameDaggerfall] savegameSEExtension() called\n");
  return "sav";
}

std::shared_ptr<const XngineSaveGame> GameDaggerfall::makeSaveGame(QString filepath) const
{
  DF_TRACE("[GameDaggerfall] makeSaveGame() called\n");
  return std::make_shared<DaggerfallsSaveGame>(filepath, this);
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

