#include "gamearena.h"
#include "arenasavegame.h"
#include "arenadatachecker.h"
#include "arenamodatacontent.h"
#include "xngineexepatch.h"

#include <executableinfo.h>
#include <pluginsetting.h>

#include <xnginelocalsavegames.h>
#include <xnginesavegameinfo.h>
#include <xngineunmanagedmods.h>

#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QDirIterator>
#include <QRegularExpression>
#include <QTextStream>
#include <QSettings>

#include <Windows.h>

#include "utility.h"

#include <memory>
#include <algorithm>

using namespace MOBase;

namespace {
constexpr const char* kExePatchTempModPrefix = "__arena_exe_patch_output_";
constexpr const char* kGlobalXdeltaPathKey = "xngine/global_xdelta_exe_path";

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

QString firstExistingPath(const QDir& root, const QStringList& relativePaths)
{
  for (const auto& relPath : relativePaths) {
    const QString fullPath = root.filePath(relPath);
    if (QFileInfo::exists(fullPath)) {
      return fullPath;
    }
  }
  return {};
}

QString readArenaInstallCfgValue(const QDir& root, const QString& key)
{
  const QString installCfgPath =
      firstExistingPath(root, {"INSTALL.CFG", "ARENA/INSTALL.CFG", "Arena/INSTALL.CFG"});
  if (installCfgPath.isEmpty()) {
    return {};
  }

  QFile file(installCfgPath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return {};
  }

  const QByteArray keyBytes = key.trimmed().toUtf8().toUpper();
  while (!file.atEnd()) {
    const QByteArray rawLine = file.readLine().trimmed();
    if (rawLine.isEmpty()) {
      continue;
    }
    const int separator = rawLine.indexOf(':');
    if (separator <= 0) {
      continue;
    }
    const QByteArray lineKey = rawLine.left(separator).trimmed().toUpper();
    if (lineKey != keyBytes) {
      continue;
    }
    return QString::fromLocal8Bit(rawLine.mid(separator + 1)).trimmed();
  }

  return {};
}

bool hasArenaExe(const QDir& root)
{
  if (QFile::exists(root.filePath("ARENA.EXE")) ||
      QFile::exists(root.filePath("A.EXE")) ||
      QFile::exists(root.filePath("Arena/ARENA.EXE")) ||
      QFile::exists(root.filePath("ARENA/ARENA.EXE")) ||
      QFile::exists(root.filePath("GAME/ARENA.EXE")) ||
      QFile::exists(root.filePath("ARENA/GAME/ARENA.EXE")) ||
      QFile::exists(root.filePath("Arena/GAME/ARENA.EXE")) ||
      QFile::exists(root.filePath("ARENA/A.EXE")) ||
      QFile::exists(root.filePath("Arena/A.EXE"))) {
    return true;
  }

  QDirIterator it(root.absolutePath(), QStringList() << "ARENA.EXE" << "A.EXE",
                  QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    it.next();
    return true;
  }
  return false;
}

bool hasSteamArenaLayout(const QDir& root)
{
  const bool hasSteamDosbox = QFile::exists(root.filePath("DOSBox-0.74/dosbox.exe")) ||
                              QFile::exists(root.filePath("DOSBox-0.73/dosbox.exe"));
  const bool hasSteamConf =
      QFile::exists(root.filePath("DOSBox-0.74/arena.conf")) ||
      QFile::exists(root.filePath("DOSBox-0.73/arena.conf")) ||
      QFile::exists(root.filePath("DOSBox-0.74/esarena.conf")) ||
      QFile::exists(root.filePath("DOSBox-0.73/esarena.conf"));
  return hasSteamDosbox && (hasSteamConf || hasArenaExe(root));
}

bool hasGogArenaLayout(const QDir& root)
{
  const bool hasGogDosbox = QFile::exists(root.filePath("DOSBOX/dosbox.exe"));
  const bool hasGogConf = QFile::exists(root.filePath("dosbox_arena.conf")) ||
                          QFile::exists(root.filePath("dosbox_arena_single.conf"));
  return hasGogDosbox && (hasGogConf || hasArenaExe(root));
}

QString quoted(const QString& path)
{
  return QString("\"%1\"").arg(QDir::toNativeSeparators(path));
}

QString relativeToDir(const QDir& baseDir, const QString& targetPath)
{
  return QDir::cleanPath(baseDir.relativeFilePath(targetPath));
}

}

GameArena::GameArena() = default;

bool GameArena::init(IOrganizer* moInfo)
{
  if (!GameXngine::init(moInfo)) {
    return false;
  }

  const QString iniForLocalSaves = iniFiles().isEmpty() ? QString{} : iniFiles().first();
  registerFeature(std::make_shared<ArenaModDataChecker>(this));
  registerFeature(std::make_shared<ArenaModDataContent>(m_Organizer->gameFeatures()));
  registerFeature(std::make_shared<XngineSaveGameInfo>(this));
  registerFeature(std::make_shared<XngineLocalSavegames>(this, iniForLocalSaves));
  registerFeature(std::make_shared<XngineUnmanagedMods>(this));
  ensureExePatchCleanupHook();

  return true;
}

QString GameArena::gameName() const
{
  return "Arena";
}

QString GameArena::displayGameName() const
{
  return "The Elder Scrolls: Arena";
}

QList<ExecutableInfo> GameArena::executables() const
{
  QList<ExecutableInfo> exes;
  const QDir gameDir = gameDirectory();
  if (gameDir.path().isEmpty() || !gameDir.exists()) {
    return exes;
  }

  const QString steamDosbox = firstExistingPath(
      gameDir, {"DOSBox-0.74/dosbox.exe", "DOSBox-0.73/dosbox.exe"});
  if (!steamDosbox.isEmpty()) {
    const QString steamMainConf = firstExistingPath(
        gameDir,
        {"DOSBox-0.74/arena.conf", "DOSBox-0.73/arena.conf", "DOSBox-0.74/esarena.conf", "DOSBox-0.73/esarena.conf"});
    const QString steamSingleConf = firstExistingPath(
        gameDir,
        {"DOSBox-0.74/arena_single.conf", "DOSBox-0.73/arena_single.conf",
         "DOSBox-0.74/esarena_single.conf", "DOSBox-0.73/esarena_single.conf"});
    if (!steamMainConf.isEmpty()) {
      const QDir steamDosboxDir = QFileInfo(steamDosbox).absoluteDir();
      QString steamArgs =
          QString("-noconsole -conf %1")
              .arg(quoted(relativeToDir(steamDosboxDir, QFileInfo(steamMainConf).absoluteFilePath())));
      if (!steamSingleConf.isEmpty()) {
        steamArgs +=
            QString(" -conf %1")
                .arg(quoted(relativeToDir(steamDosboxDir, QFileInfo(steamSingleConf).absoluteFilePath())));
      }
      exes << ExecutableInfo("Arena (Steam DOSBox Windowed)", QFileInfo(steamDosbox))
                 .withWorkingDirectory(steamDosboxDir)
                 .withArgument(steamArgs);
      exes << ExecutableInfo("Arena (Steam DOSBox Fullscreen)", QFileInfo(steamDosbox))
                 .withWorkingDirectory(steamDosboxDir)
                 .withArgument(steamArgs + " -fullscreen");
    }
  }

  const QFileInfo gogDosbox(gameDir.filePath("DOSBOX/dosbox.exe"));
  if (gogDosbox.exists()) {
    const QString gogMainConf = gameDir.filePath("dosbox_arena.conf");
    const QString gogSingleConf = gameDir.filePath("dosbox_arena_single.conf");
    if (QFile::exists(gogMainConf) && QFile::exists(gogSingleConf)) {
      const QDir gogDosboxDir = gogDosbox.absoluteDir();
      const QString gogArgs = QString("-conf %1 -conf %2 -noconsole -c \"exit\"")
                                  .arg(quoted(relativeToDir(gogDosboxDir, gogMainConf)),
                                       quoted(relativeToDir(gogDosboxDir, gogSingleConf)));
      exes << ExecutableInfo("Arena (GOG DOSBox)", gogDosbox)
                 .withWorkingDirectory(gogDosboxDir)
                 .withArgument(gogArgs);
    }
  }

  const QString nativeExePath = firstExistingPath(
      gameDir,
      {
          "Arena/ARENA.EXE",
          "ARENA/ARENA.EXE",
          "ARENA.EXE",
          "A.EXE",
      });
  if (!nativeExePath.isEmpty()) {
    exes << ExecutableInfo("Arena", QFileInfo(nativeExePath));
  }

  const QString arenaBatPath = firstExistingPath(
      gameDir,
      {
          "Arena/ARENA.BAT",
          "ARENA/ARENA.BAT",
          "ARENA.BAT",
      });
  if (!arenaBatPath.isEmpty()) {
    exes << ExecutableInfo("Arena (ARENA.BAT)", QFileInfo(arenaBatPath));
  }

  return exes;
}

QString GameArena::steamAPPId() const
{
  return "1812290";
}

QString GameArena::gogAPPId() const
{
  return "1435828767";
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
  // Nexus game domain for Arena NXM links:
  // https://www.nexusmods.com/games/tesarena
  return "tesarena";
}

QStringList GameArena::validShortNames() const
{
  return {"arena", "tesarena"};
}

QStringList GameArena::iniFiles() const
{
  const QStringList candidates = {"ULTRAMID.INI",
                                  "ARENA/ULTRAMID.INI",
                                  "INSTALL.CFG",
                                  "ARENA/INSTALL.CFG",
                                  "SOUND.CFG",
                                  "ARENA/SOUND.CFG",};

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

QIcon GameArena::gameIcon() const
{
  const QDir dir = gameDirectory();
  const QString iconPath = firstExistingPath(
      dir,
      {
          "ARENA.ICO",
          "Arena/ARENA.ICO",
          "ARENA/ARENA.ICO",
          "goggame-1435828767.ico",
      });

  if (!iconPath.isEmpty()) {
    return QIcon(iconPath);
  }

  const QString exePath = firstExistingPath(
      dir,
      {"Arena/ARENA.EXE", "ARENA/ARENA.EXE", "ARENA.EXE", "A.EXE"});
  if (!exePath.isEmpty()) {
    const QIcon exeIcon = MOBase::iconForExecutable(exePath);
    if (!exeIcon.isNull()) {
      return exeIcon;
    }
  }

  return GameXngine::gameIcon();
}

int GameArena::nexusModOrganizerID() const
{
  return 0;
}

int GameArena::nexusGameID() const
{
  // Nexus "games.json" id for "The Elder Scrolls: Arena".
  return 940;
}

QString GameArena::gameVersion() const
{
  const QString binaryVersion = detectDosVersionFromBinaryStrings(
      gameDirectory(),
      {
          "Arena/ARENA.EXE",
          "ARENA/ARENA.EXE",
          "ARENA.EXE",
          "ARENA/A.EXE",
          "Arena/A.EXE",
          "A.EXE",
      },
      {
          QRegularExpression(R"(\bTES:\s*Arena\s+v([0-9]+(?:\.[0-9]+){1,3})\.?\b)",
                             QRegularExpression::CaseInsensitiveOption),
          QRegularExpression(R"(\bArena\s+v([0-9]+(?:\.[0-9]+){1,3})\.?\b)",
                             QRegularExpression::CaseInsensitiveOption),
      });
  if (!binaryVersion.isEmpty()) {
    return binaryVersion;
  }

  return detectGameVersion(
      {
          "Arena/ARENA.EXE",
          "ARENA/ARENA.EXE",
          "ARENA.EXE",
          "A.EXE",
      },
      {
          "ARENA/README.TXT",
          "Arena/README.TXT",
          "README.TXT",
          "ARENA/READ.ME",
          "Arena/READ.ME",
          "READ.ME",
          "ARENA/README",
          "Arena/README",
          "README",
      },
      {
          QRegularExpression(R"(\bVersion\s+([0-9]+(?:\.[0-9]+){1,3})\b)",
                             QRegularExpression::CaseInsensitiveOption),
      });
}

QDir GameArena::dataDirectory() const
{
  const QDir gameDir = gameDirectory();
  if (gameDir.path().isEmpty() || !gameDir.exists()) {
    return {};
  }

  for (const auto& subdir : {QString("Arena"), QString("ARENA"), QString("GAME"), QString("GAMEDATA")}) {
    const QDir candidate(gameDir.filePath(subdir));
    if (candidate.exists()) {
      return candidate;
    }
  }

  // Fallback to game root for DOS-era layouts that don't use a dedicated data folder.
  return gameDir;
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
  return tr("Adds basic support for The Elder Scrolls: Arena");
}

VersionInfo GameArena::version() const
{
  return VersionInfo(1, 0, 0, VersionInfo::RELEASE_FINAL);
}

QList<PluginSetting> GameArena::settings() const
{
  return {
      PluginSetting(
          "show_developer_save_details",
          tr("Show internal Arena save debug details (quest flags, raw values) in save info."),
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

bool GameArena::showDeveloperSaveDetails() const
{
  if (m_Organizer == nullptr) {
    return false;
  }
  return m_Organizer->pluginSetting(name(), "show_developer_save_details").toBool();
}

bool GameArena::allowExeModInstall() const
{
  return allowJsonPatchInstall() || allowXdeltaPatchInstall();
}

bool GameArena::allowJsonPatchInstall() const
{
  return false;
}

bool GameArena::allowXdeltaPatchInstall() const
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
        << "[GameArena] .xdelta patch setting is enabled but no xdelta tool was found."
        << "Set plugin setting 'xdelta_exe_path' or install xdelta.exe in an auto-detected location.";
    warnedMissingXdelta = true;
  }
  return false;
}

bool GameArena::prepareIni(const QString& exec)
{
  if (!GameXngine::prepareIni(exec)) {
    return false;
  }

  if (allowExeModInstall()) {
    if (!applyExePatchMods()) {
      qWarning().noquote()
          << "[GameArena] EXE patch staging failed; continuing launch without generated patch output.";
    }
  }

  return true;
}

QString GameArena::identifyGamePath() const
{
  // Steam (The Elder Scrolls: Arena)
  QString steamPath = findInRegistry(HKEY_LOCAL_MACHINE,
                                     L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 1812290",
                                     L"InstallLocation");
  if (!steamPath.isEmpty()) {
    const QDir dir(steamPath);
    if (dir.exists() && looksValid(dir)) {
      return steamPath;
    }
  }

  // Steam library fallback (some installs do not expose the uninstall key)
  const QStringList steamDirs = {
      "The Elder Scrolls Arena",
      "The Elder Scrolls: Arena",
      "Arena",
      "TESArena",
  };
  for (const auto& dirName : steamDirs) {
    const QString parsed = parseSteamLocation(steamAPPId(), dirName);
    if (!parsed.isEmpty() && looksValid(QDir(parsed))) {
      return parsed;
    }
  }

  // GOG (The Elder Scrolls: Arena)
  QString gogPath = findInRegistry(HKEY_LOCAL_MACHINE,
                                   L"Software\\GOG.com\\Games\\1435828767",
                                   L"path");
  if (!gogPath.isEmpty()) {
    const QDir dir(gogPath);
    if (dir.exists() && looksValid(dir)) {
      return gogPath;
    }
  }

  // GOG registry fallback in case value casing differs
  gogPath = findInRegistry(HKEY_LOCAL_MACHINE,
                           L"Software\\GOG.com\\Games\\1435828767",
                           L"Path");
  if (!gogPath.isEmpty()) {
    const QDir dir(gogPath);
    if (dir.exists() && looksValid(dir)) {
      return gogPath;
    }
  }

  return {};
}

bool GameArena::looksValid(QDir const& path) const
{
  if (!path.exists()) {
    return false;
  }

  if (hasArenaExe(path)) {
    return true;
  }

  if (hasSteamArenaLayout(path) || hasGogArenaLayout(path)) {
    return true;
  }

  return false;
}

QDir GameArena::savesDirectory() const
{
  return GameXngine::savesDirectory();
}

MappingType GameArena::mappings() const
{
  MappingType out;

  const auto profile = profilePath();
  if (profile.isEmpty()) {
    return out;
  }

  const auto layout = saveLayout();
  const auto paths = resolveSaveStorage(profile, saveGameId());
  ensureSaveDirsExist(paths, layout, saveSlotPrefix());

  const QDir gameDir = gameDirectory();
  const QString sourceRoot = paths.gameSavesRoot;
  const QString targetRoot = gameDir.absolutePath();

  // Flat Arena save bundle files.
  const QStringList stems = {"AUTOMAP", "IN", "LOG", "SAVEENGN", "SAVEGAME",
                             "SPELLS", "STATES", "WILDPAL"};
  for (int slot = 0; slot <= 9; ++slot) {
    const QString suffix2 = QString("%1").arg(slot, 2, 10, QChar('0'));
    const QString suffix1 = QString::number(slot);
    for (const auto& stem : stems) {
      out.push_back({QDir(sourceRoot).filePath(stem + "." + suffix2),
                     QDir(targetRoot).filePath(stem + "." + suffix2),
                     false,
                     false});
      out.push_back({QDir(sourceRoot).filePath(stem + "." + suffix1),
                     QDir(targetRoot).filePath(stem + "." + suffix1),
                     false,
                     false});
    }
  }

  // Optional slot names table.
  const QString namesDataFile = readArenaInstallCfgValue(gameDir, "DATAFILE");
  const QString namesFileName = namesDataFile.isEmpty() ? QStringLiteral("NAMES.DAT")
                                                        : QFileInfo(namesDataFile).fileName();
  out.push_back({QDir(sourceRoot).filePath(namesFileName),
                 QDir(targetRoot).filePath(namesFileName),
                 false,
                 false});

  // Compatibility: profile slot directories SAVE0..SAVE9 should project
  // their *contents* directly into game root.
  for (int slot = 0; slot <= 9; ++slot) {
    out.push_back({QDir(sourceRoot).filePath(QString("SAVE%1").arg(slot)),
                   targetRoot,
                   true,
                   true});
  }

  return out;
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
  return std::make_shared<ArenaSaveGame>(filepath, this);
}

SaveLayout GameArena::saveLayout() const
{
  SaveLayout layout;
  layout.baseRelativePaths = {""};
  layout.slotEntriesAreFiles = false;
  layout.slotDirRegex = QRegularExpression("(?i)^SAVE(\\d+)$");
  layout.slotWidthHint = 1;
  layout.maxSlotHint = 9;
  layout.validator = [](const QDir&) { return true; };
  return layout;
}

QString GameArena::saveGameId() const
{
  return "arena";
}

void GameArena::ensureExePatchCleanupHook()
{
  if (m_ExePatchCleanupHookRegistered || !m_Organizer) {
    return;
  }
  m_Organizer->onFinishedRun([this](const QString&, unsigned int) {
    cleanupExePatchOutputMod();
  });
  m_ExePatchCleanupHookRegistered = true;
}

void GameArena::cleanupExePatchOutputMod() const
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

bool GameArena::applyExePatchMods()
{
  if (!m_Organizer) {
    return false;
  }
  auto* modList = m_Organizer->modList();
  if (!modList) {
    return false;
  }

  const bool allowXdelta = allowXdeltaPatchInstall();
  if (!allowXdelta) {
    return true;
  }

  const QStringList exeCandidates = {
      "Arena/ARENA.EXE", "ARENA/ARENA.EXE", "ARENA.EXE", "ARENA/A.EXE", "Arena/A.EXE", "A.EXE"};
  const QStringList exeNames = {"ARENA.EXE", "A.EXE"};

  const QStringList allMods = modList->allModsByProfilePriority();
  if (allMods.isEmpty()) {
    return true;
  }

  const QString modsPath = m_Organizer->modsPath();
  const QString gameDirPath = gameDirectory().absolutePath();
  const QString tempModName = QString(kExePatchTempModPrefix) + profileSuffix(profilePath());
  const QString tempModPath = QDir(modsPath).filePath(tempModName);

  struct ModPatchInput
  {
    QString modName;
    QString modPath;
    QString replacementExePath;
    QStringList xdeltaPatchFiles;
  };

  QList<ModPatchInput> patchInputs;
  int lastPatchPriority = -1;
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

    input.replacementExePath = XngineExePatch::findFirstExistingFile(modPath, exeCandidates);
    if (input.replacementExePath.isEmpty()) {
      input.replacementExePath =
          XngineExePatch::findFirstMatchingFileRecursive(modPath, exeNames);
    }

    QDirIterator xdeltaIt(modPath, QStringList() << "*.xdelta", QDir::Files,
                          QDirIterator::Subdirectories);
    while (xdeltaIt.hasNext()) {
      input.xdeltaPatchFiles.push_back(xdeltaIt.next());
    }

    if (!input.replacementExePath.isEmpty() || !input.xdeltaPatchFiles.isEmpty()) {
      patchInputs.push_back(input);
      lastPatchPriority = (std::max)(lastPatchPriority, modList->priority(modName));
    }
  }

  if (patchInputs.isEmpty()) {
    return true;
  }

  if (!removeDirRecursive(tempModPath)) {
    qWarning().noquote() << "[GameArena] Failed to clean temp mod path:" << tempModPath;
  }
  if (!modList->getMod(tempModName)) {
    MOBase::GuessedValue<QString> guessedName(tempModName);
    m_Organizer->createMod(guessedName);
  }
  if (!ensureDir(tempModPath)) {
    qWarning().noquote() << "[GameArena] Failed to create temp mod path:" << tempModPath;
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
    out << "description=Temporary Arena executable patch output\n";
    metaFile.close();
  }

  if (modList->getMod(tempModName)) {
    modList->setActive(tempModName, true);
    if (lastPatchPriority >= 0) {
      modList->setPriority(tempModName, lastPatchPriority + 1);
    }
  }

  bool success = true;
  for (const ModPatchInput& input : patchInputs) {
    QString workingExePath = XngineExePatch::findFirstExistingFile(tempModPath, exeCandidates);
    if (workingExePath.isEmpty()) {
      workingExePath = XngineExePatch::findFirstExistingFile(gameDirPath, exeCandidates);
    }
    if (workingExePath.isEmpty()) {
      qWarning().noquote() << "[GameArena] Could not find base EXE for patching";
      success = false;
      continue;
    }
    QString relExePath = QDir(tempModPath).relativeFilePath(workingExePath);
    if (QDir(tempModPath).relativeFilePath(workingExePath).startsWith("..")) {
      relExePath = QDir(gameDirPath).relativeFilePath(workingExePath);
    }

    const QString stagedExePath = QDir(tempModPath).filePath(relExePath);
    ensureDir(QFileInfo(stagedExePath).absolutePath());

    if (!input.replacementExePath.isEmpty()) {
      QFile::remove(stagedExePath);
      if (!QFile::copy(input.replacementExePath, stagedExePath)) {
        qWarning().noquote() << "[GameArena] Failed staging replacement EXE from mod"
                             << input.modName;
        success = false;
        continue;
      }
    } else if (!QFileInfo::exists(stagedExePath)) {
      QFile::copy(workingExePath, stagedExePath);
    }

    if (!input.xdeltaPatchFiles.isEmpty()) {
      QString configuredTool =
          m_Organizer->pluginSetting(name(), "xdelta_exe_path").toString().trimmed();
      if (!configuredTool.isEmpty()) {
        writeGlobalXdeltaPath(configuredTool);
      } else {
        configuredTool = readGlobalXdeltaPath();
      }
      QString xdeltaTool =
          XngineExePatch::findXdeltaTool(input.modPath, gameDirPath, configuredTool);
      if (xdeltaTool.isEmpty()) {
        qWarning().noquote() << "[GameArena] xdelta tool not found for mod" << input.modName
                             << "- checked mod/game folders, MO2 folder/tools, XDELTA_EXE, and PATH.";
        success = false;
        continue;
      }
      for (const QString& patchFile : input.xdeltaPatchFiles) {
        QString err;
        QString matchedRel;
        if (!XngineExePatch::applyXdeltaPatchToAnyFileInTree(
                xdeltaTool, patchFile, gameDirPath, tempModPath, &matchedRel, &err)) {
          qWarning().noquote() << "[GameArena] Failed applying .xdelta patch" << patchFile
                               << "from mod" << input.modName << ":" << err;
          success = false;
          break;
        }
        qInfo().noquote() << "[GameArena] Applied .xdelta patch to" << matchedRel
                          << "from mod" << input.modName;
      }
    }
  }

  return success;
}

QString GameArena::saveSlotPrefix() const
{
  return "SAVE";
}

QVector<XngineBSAFormat::FileSpec> GameArena::bsaFileSpecs() const
{
  return {
      {"GLOBAL.BSA", false, XngineBSAFormat::IndexType::NameRecord, false,
       "Primary Arena data archive (record type varies by build)."},
  };
}

QString GameArena::findInRegistry(HKEY baseKey, LPCWSTR path, LPCWSTR value) const
{
  DWORD size = 0;
  HKEY subKey;
  LONG res = ::RegOpenKeyExW(baseKey, path, 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY, &subKey);
  if (res != ERROR_SUCCESS) {
    res = ::RegOpenKeyExW(baseKey, path, 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &subKey);
    if (res != ERROR_SUCCESS) {
      return {};
    }
  }

  res = ::RegQueryValueExW(subKey, value, nullptr, nullptr, nullptr, &size);
  if (res != ERROR_SUCCESS) {
    ::RegCloseKey(subKey);
    return {};
  }

  std::unique_ptr<wchar_t[]> buffer = std::make_unique<wchar_t[]>(size / sizeof(wchar_t) + 1);
  res = ::RegQueryValueExW(subKey, value, nullptr, nullptr,
                           reinterpret_cast<LPBYTE>(buffer.get()), &size);
  ::RegCloseKey(subKey);

  if (res != ERROR_SUCCESS) {
    return {};
  }

  return QString::fromWCharArray(buffer.get());
}
