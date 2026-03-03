#include "gamedaggerfall.h"

#include "daggerfallsmoddatachecker.h"
#include "daggerfallsmoddatacontent.h"
#include "daggerfallsavegame.h"
#include "xngineexepatch.h"

#include <executableinfo.h>
#include <pluginsetting.h>

#include <xnginelocalsavegames.h>
#include <xnginesavegameinfo.h>
#include <xngineunmanagedmods.h>

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QStandardPaths>
#include <QFile>
#include <QRegularExpression>
#include <QIcon>
#include <QDir>
#include <QTextStream>
#include <QSettings>

#include <Windows.h>

#include "utility.h"

#include <exception>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <optional>

#if defined(_DEBUG)
#define DF_TRACE(msg) OutputDebugStringA(msg)
#else
#define DF_TRACE(msg) ((void)0)
#endif

using namespace MOBase;

namespace
{
constexpr const char* kGlobalXdeltaPathKey = "xngine/global_xdelta_exe_path";

QString readGlobalXdeltaPath()
{
  QSettings s;
  return s.value(kGlobalXdeltaPathKey).toString().trimmed();
}

void writeGlobalXdeltaPath(const QString& path)
{
  if (path.trimmed().isEmpty()) {
    return;
  }
  QSettings s;
  if (s.value(kGlobalXdeltaPathKey).toString().trimmed() == path.trimmed()) {
    return;
  }
  s.setValue(kGlobalXdeltaPathKey, path.trimmed());
  s.sync();
}

#if defined(XNGINE_DAGGERFALL_EXE_PATCHING)
constexpr const char* kExePatchTempModPrefix = "__daggerfall_exe_patch_output_";

QString profileSuffix(const QString& profilePath)
{
  if (profilePath.isEmpty()) {
    return "Default";
  }
  const QString name = QDir(profilePath).dirName();
  return name.isEmpty() ? QString("Default") : name;
}

bool ensureDir(const QString& path)
{
  QDir dir;
  return dir.mkpath(path);
}

bool removeDirRecursive(const QString& path)
{
  QDir dir(path);
  if (!dir.exists()) {
    return true;
  }
  return dir.removeRecursively();
}

struct ExePathMatch
{
  QString absolutePath;
  QString relativePath;
};

std::optional<ExePathMatch> findExistingExe(const QString& rootPath)
{
  const QDir root(rootPath);
  const QStringList candidates = {
      "DF/DAGGER/FALL.EXE",
      "FALL.EXE",
      "DF/DAGGER/DAGGER.EXE",
      "DAGGER.EXE",
  };

  for (const QString& rel : candidates) {
    const QString abs = root.filePath(rel);
    if (QFileInfo::exists(abs)) {
      return ExePathMatch{abs, rel};
    }
  }

  return std::nullopt;
}
#endif
}  // namespace

#if defined(XNGINE_DAGGERFALL_EXE_PATCHING)
namespace
{
void validateUserFallExePatchCatalog(const QDir& gameDir)
{
  if (gameDir.path().isEmpty() || !gameDir.exists()) {
    return;
  }

  const QStringList catalogs = {
      gameDir.absoluteFilePath("fall_exe_patches.json"),
      gameDir.absoluteFilePath("exe_patches.json"),
  };

  for (const QString& catalogPath : catalogs) {
    if (!QFileInfo::exists(catalogPath)) {
      continue;
    }
    QVector<XngineExePatch::PatchSet> sets;
    QString err;
    if (!XngineExePatch::loadPatchSetsFromJsonFile(catalogPath, sets, &err)) {
      qWarning().noquote() << "[GameDaggerfall] Invalid EXE patch catalog:" << catalogPath
                           << err;
      DF_TRACE("[GameDaggerfall] EXE patch catalog validation FAILED\n");
      continue;
    }
    qInfo().noquote() << "[GameDaggerfall] Validated EXE patch catalog:" << catalogPath;
    DF_TRACE("[GameDaggerfall] EXE patch catalog validation OK\n");
  }
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
    registerFeature(std::make_shared<DaggerfallsModDataChecker>(this));
    registerFeature(std::make_shared<DaggerfallsModDataContent>(m_Organizer->gameFeatures()));
    registerFeature(std::make_shared<XngineSaveGameInfo>(this));
    registerFeature(std::make_shared<XngineLocalSavegames>(this, iniForLocalSaves));
    registerFeature(std::make_shared<XngineUnmanagedMods>(this));
#if defined(XNGINE_DAGGERFALL_EXE_PATCHING)
    validateUserFallExePatchCatalog(gameDirectory());
    ensureExePatchCleanupHook();
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
  return 975;  // Nexus "games.json" id for Daggerfall
}

bool GameDaggerfall::prepareIni(const QString& exec)
{
  if (!GameXngine::prepareIni(exec)) {
    return false;
  }

#if defined(XNGINE_DAGGERFALL_EXE_PATCHING)
  if (allowExeModInstall()) {
    if (!applyExePatchMods()) {
      qWarning().noquote()
          << "[GameDaggerfall] EXE patch staging failed; continuing launch without generated patch output.";
    }
  }
#endif

  return true;
}

QString GameDaggerfall::gameVersion() const
{
  const QString binaryVersion = detectDosVersionFromBinaryStrings(
      gameDirectory(),
      {
          "FALL.EXE",
          "DF/DAGGER/FALL.EXE",
          "DAGGER.EXE",
          "DF/DAGGER/DAGGER.EXE",
      },
      {
          QRegularExpression(R"(\bTES:\s*Daggerfall\s+v([0-9]+(?:\.[0-9]+){1,3})\.?\b)",
                             QRegularExpression::CaseInsensitiveOption),
          QRegularExpression(R"(\bDaggerfall\s+v([0-9]+(?:\.[0-9]+){1,3})\.?\b)",
                             QRegularExpression::CaseInsensitiveOption),
      });
  if (!binaryVersion.isEmpty()) {
    return binaryVersion;
  }

  return detectGameVersion(
      {
          "DF/DAGGER/DAGGER.EXE",
          "DAGGER.EXE",
      },
      {
          "PATCHED.TXT",
          "DF/DAGGER/PATCHED.TXT",
          "README.TXT",
          "DF/DAGGER/README.TXT",
      },
      {
          QRegularExpression(R"(\bversion\s+([0-9]+(?:\.[0-9]+){1,3})\b)",
                             QRegularExpression::CaseInsensitiveOption),
      });
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
  return {
      PluginSetting(
          "allow_json_patch_mod_install",
          tr("Allow JSON binary patch mods. WARNING: this is dangerous and may corrupt saves or game data."),
          false),
      PluginSetting(
          "xdelta_enabled",
          tr("Allow .xdelta binary patch mods. Requires the XNGINE patch tool (xdelta.exe) to be installed with MO2/plugin files. WARNING: this is dangerous and may corrupt saves or game data."),
          false),
      PluginSetting(
          "xdelta_exe_path",
          tr("Optional full path to xdelta.exe/xdelta3.exe. If empty, uses a shared global xdelta path if set, otherwise automatic detection."),
          ""),
  };
}

bool GameDaggerfall::allowExeModInstall() const
{
  return allowJsonPatchInstall() || allowXdeltaPatchInstall();
}

bool GameDaggerfall::allowJsonPatchInstall() const
{
  if (m_Organizer == nullptr) {
    return false;
  }
  return m_Organizer->pluginSetting(name(), "allow_json_patch_mod_install").toBool();
}

bool GameDaggerfall::allowXdeltaPatchInstall() const
{
  if (m_Organizer == nullptr) {
    return false;
  }
  if (!m_Organizer->pluginSetting(name(), "xdelta_enabled").toBool()) {
    return false;
  }

  QString configuredTool =
      m_Organizer->pluginSetting(name(), "xdelta_exe_path").toString().trimmed();
  if (!configuredTool.isEmpty()) {
    writeGlobalXdeltaPath(configuredTool);
  } else {
    configuredTool = readGlobalXdeltaPath();
  }
  const QString resolved =
      XngineExePatch::findXdeltaTool({}, gameDirectory().absolutePath(), configuredTool);
  if (!resolved.isEmpty()) {
    return true;
  }

  static bool warnedMissingXdelta = false;
  if (!warnedMissingXdelta) {
    qWarning().noquote()
        << "[GameDaggerfall] .xdelta patch setting is enabled but no xdelta tool was found."
        << "Set plugin setting 'xdelta_exe_path' or install xdelta.exe in an auto-detected location.";
    warnedMissingXdelta = true;
  }
  return false;
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
       "Raw PCM audio records.", XngineBSAFormat::ArchiveVariant::Snd},
      {"MAPSAVE.SAV", true, XngineBSAFormat::IndexType::NameRecord, true,
       "Automap archive (NameRecord 0x0100; MAPSAVE.0## regional records).",
       XngineBSAFormat::ArchiveVariant::Sav},
  };
}

#if defined(XNGINE_DAGGERFALL_EXE_PATCHING)
void GameDaggerfall::ensureExePatchCleanupHook()
{
  if (m_ExePatchCleanupHookRegistered || !m_Organizer) {
    return;
  }

  m_Organizer->onFinishedRun([this](const QString&, unsigned int) {
    cleanupExePatchOutputMod();
  });
  m_ExePatchCleanupHookRegistered = true;
}

void GameDaggerfall::cleanupExePatchOutputMod() const
{
  if (!m_Organizer) {
    return;
  }

  auto* modList = m_Organizer->modList();
  if (!modList) {
    return;
  }

  const QString tempModName =
      QString(kExePatchTempModPrefix) + profileSuffix(profilePath());
  const QString tempModPath = QDir(m_Organizer->modsPath()).filePath(tempModName);

  if (modList->getMod(tempModName)) {
    modList->setActive(tempModName, false);
  }
  removeDirRecursive(tempModPath);
}

bool GameDaggerfall::applyExePatchMods()
{
  if (!m_Organizer) {
    return false;
  }
  const bool allowJson = allowJsonPatchInstall();
  const bool allowXdelta = allowXdeltaPatchInstall();
  if (!allowJson && !allowXdelta) {
    return true;
  }

  auto* modList = m_Organizer->modList();
  if (!modList) {
    return false;
  }

  const QStringList allMods = modList->allModsByProfilePriority();
  if (allMods.isEmpty()) {
    return true;
  }

  const QString modsPath = m_Organizer->modsPath();
  const QString gameDirPath = gameDirectory().absolutePath();
  const QString tempModName =
      QString(kExePatchTempModPrefix) + profileSuffix(profilePath());
  const QString tempModPath = QDir(modsPath).filePath(tempModName);

  bool foundWork = false;
  int lastPatchPriority = -1;

  struct ModPatchInput
  {
    QString modName;
    QString modPath;
    QString catalogPath;
    QString replacementExePath;
    QString replacementExeRelPath;
    QStringList xdeltaPatchFiles;
  };
  QList<ModPatchInput> patchInputs;

  for (const QString& modName : allMods) {
    if (!(modList->state(modName) & IModList::STATE_ACTIVE)) {
      continue;
    }

    const QString modPath = QDir(modsPath).filePath(modName);
    if (!QDir(modPath).exists()) {
      continue;
    }

    ModPatchInput input;
    input.modName = modName;
    input.modPath = modPath;

    if (allowJson) {
      const QStringList catalogCandidates = {
          "fall_exe_patches.json",
          "DF/DAGGER/fall_exe_patches.json",
          "exe_patches.json",
          "DF/DAGGER/exe_patches.json",
      };
      for (const QString& rel : catalogCandidates) {
        const QString abs = QDir(modPath).filePath(rel);
        if (QFileInfo::exists(abs)) {
          input.catalogPath = abs;
          break;
        }
      }
      const auto replacement = findExistingExe(modPath);
      if (replacement.has_value()) {
        input.replacementExePath = replacement->absolutePath;
        input.replacementExeRelPath = replacement->relativePath;
      }
    }
    if (allowXdelta) {
      QDirIterator xdeltaIt(modPath, QStringList() << "*.xdelta", QDir::Files,
                            QDirIterator::Subdirectories);
      while (xdeltaIt.hasNext()) {
        input.xdeltaPatchFiles.push_back(xdeltaIt.next());
      }
    }

    if (!input.catalogPath.isEmpty() || !input.replacementExePath.isEmpty() ||
        !input.xdeltaPatchFiles.isEmpty()) {
      patchInputs.push_back(input);
      foundWork = true;
      lastPatchPriority = (std::max)(lastPatchPriority, modList->priority(modName));
    }
  }

  if (!foundWork) {
    return true;
  }

  qWarning().noquote() << "[GameDaggerfall] EXE patch mods detected; staging via temporary VFS mod:"
                       << tempModName;

  if (!removeDirRecursive(tempModPath)) {
    qWarning().noquote() << "[GameDaggerfall] Failed to clean temp mod path:" << tempModPath;
  }

  if (!modList->getMod(tempModName)) {
    MOBase::GuessedValue<QString> guessedName(tempModName);
    m_Organizer->createMod(guessedName);
  }

  if (!ensureDir(tempModPath)) {
    qWarning().noquote() << "[GameDaggerfall] Failed to create temp mod path:" << tempModPath;
    return false;
  }

  const QString metaIniPath = QDir(tempModPath).filePath("meta.ini");
  QFile metaFile(metaIniPath);
  if (!metaFile.exists() && metaFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&metaFile);
    out << "[General]\n";
    out << "name=" << tempModName << "\n";
    out << "version=1.0\n";
    out << "author=Mod Organizer\n";
    out << "description=Temporary Daggerfall executable patch output\n";
    metaFile.close();
  }

  if (modList->getMod(tempModName)) {
    modList->setActive(tempModName, true);
    if (lastPatchPriority >= 0) {
      modList->setPriority(tempModName, lastPatchPriority + 1);
    }
  }

  bool overallSuccess = true;
  for (const ModPatchInput& input : patchInputs) {
    if (!input.replacementExePath.isEmpty()) {
      const QString destPath = QDir(tempModPath).filePath(input.replacementExeRelPath);
      const QString destDir = QFileInfo(destPath).absolutePath();
      if (!ensureDir(destDir)) {
        qWarning().noquote()
            << "[GameDaggerfall] Failed to create destination directory for EXE replacement:"
            << destDir;
        overallSuccess = false;
      } else {
        QFile::remove(destPath);
        if (!QFile::copy(input.replacementExePath, destPath)) {
          qWarning().noquote()
              << "[GameDaggerfall] Failed to stage replacement executable from mod"
              << input.modName << "->" << destPath;
          overallSuccess = false;
        } else {
          qInfo().noquote()
              << "[GameDaggerfall] Staged replacement executable from mod" << input.modName
              << "to" << destPath;
        }
      }
    }

    auto workingExe = findExistingExe(tempModPath);
    if (!workingExe.has_value()) {
      workingExe = findExistingExe(gameDirPath);
    }
    if (!workingExe.has_value()) {
      qWarning().noquote() << "[GameDaggerfall] Could not locate FALL.EXE/DAGGER.EXE base file for"
                              " patching";
      overallSuccess = false;
      continue;
    }

    if (!input.catalogPath.isEmpty()) {
      QVector<XngineExePatch::PatchSet> sets;
      QString loadError;
      if (!XngineExePatch::loadPatchSetsFromJsonFile(input.catalogPath, sets, &loadError)) {
        qWarning().noquote() << "[GameDaggerfall] Failed to load patch catalog from mod"
                             << input.modName << ":" << loadError;
        overallSuccess = false;
        continue;
      }
      if (sets.isEmpty()) {
        qWarning().noquote() << "[GameDaggerfall] Empty patch catalog in mod" << input.modName;
        overallSuccess = false;
        continue;
      }

      QFile exeFile(workingExe->absolutePath);
      if (!exeFile.open(QIODevice::ReadOnly)) {
        qWarning().noquote() << "[GameDaggerfall] Failed to read base executable for patching:"
                             << workingExe->absolutePath;
        overallSuccess = false;
        continue;
      }
      QByteArray exeData = exeFile.readAll();
      exeFile.close();

      if (!XngineExePatch::applyPatchSets(exeData, sets, &loadError)) {
        qWarning().noquote() << "[GameDaggerfall] Failed to apply patch catalog from mod"
                             << input.modName << ":" << loadError;
        overallSuccess = false;
        continue;
      }

      const QString destPath = QDir(tempModPath).filePath(workingExe->relativePath);
      const QString destDir = QFileInfo(destPath).absolutePath();
      if (!ensureDir(destDir)) {
        qWarning().noquote()
            << "[GameDaggerfall] Failed to create destination directory for patched EXE:"
            << destDir;
        overallSuccess = false;
        continue;
      }

      QFile out(destPath);
      if (!out.open(QIODevice::WriteOnly)) {
        qWarning().noquote() << "[GameDaggerfall] Failed to write patched executable:"
                             << destPath;
        overallSuccess = false;
        continue;
      }
      if (out.write(exeData) != exeData.size()) {
        qWarning().noquote() << "[GameDaggerfall] Incomplete write for patched executable:"
                             << destPath;
        overallSuccess = false;
      }
      out.close();

      qInfo().noquote() << "[GameDaggerfall] Staged patched executable from catalog in mod"
                        << input.modName << "to" << destPath;
    }

    if (!input.xdeltaPatchFiles.isEmpty()) {
      QString configuredTool =
          m_Organizer->pluginSetting(name(), "xdelta_exe_path").toString().trimmed();
      if (!configuredTool.isEmpty()) {
        writeGlobalXdeltaPath(configuredTool);
      } else {
        configuredTool = readGlobalXdeltaPath();
      }
      QString tool =
          XngineExePatch::findXdeltaTool(input.modPath, gameDirPath, configuredTool);
      if (tool.isEmpty()) {
        qWarning().noquote() << "[GameDaggerfall] .xdelta patches present in mod"
                             << input.modName
                             << "but no xdelta tool was found (checked mod/game folders,"
                                " MO2 folder/tools, XDELTA_EXE, and PATH).";
        overallSuccess = false;
        continue;
      }

      for (const QString& patchFile : input.xdeltaPatchFiles) {
        QString err;
        QString matchedRel;
        if (!XngineExePatch::applyXdeltaPatchToAnyFileInTree(
                tool, patchFile, gameDirPath, tempModPath, &matchedRel, &err)) {
          qWarning().noquote() << "[GameDaggerfall] Failed to apply .xdelta patch"
                               << patchFile << "from mod" << input.modName << ":" << err;
          overallSuccess = false;
          break;
        }
        qInfo().noquote() << "[GameDaggerfall] Applied .xdelta patch to" << matchedRel
                          << "from mod" << input.modName << ":" << patchFile;
      }
    }
  }

  return overallSuccess;
}
#endif

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
