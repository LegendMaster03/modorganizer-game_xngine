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
#include <QMetaType>

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
constexpr const char* kDefaultUnityNexusName = "daggerfallunity";

QString firstExistingRelativePath(const QDir& root, const QStringList& candidates)
{
  for (const QString& candidate : candidates) {
    if (QFileInfo::exists(root.filePath(candidate))) {
      return candidate;
    }
  }
  return {};
}

QString readDaggerfallSetupValue(const QDir& root, const QString& key)
{
  const QString setupRelative =
      firstExistingRelativePath(root, {"SETUP.INI", "DF/DAGGER/SETUP.INI"});
  if (setupRelative.isEmpty()) {
    return {};
  }

  QSettings settings(root.filePath(setupRelative), QSettings::IniFormat);
  const QString value = settings.value(QStringLiteral("PROGRAM/%1").arg(key)).toString().trimmed();
  return value;
}

QString readDaggerfallZCfgValue(const QDir& root, const QString& key)
{
  const QString zCfgRelative = firstExistingRelativePath(root, {"Z.CFG", "DF/DAGGER/Z.CFG"});
  if (zCfgRelative.isEmpty()) {
    return {};
  }

  QFile file(root.filePath(zCfgRelative));
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return {};
  }

  const QByteArray keyBytes = key.trimmed().toUtf8().toLower();
  while (!file.atEnd()) {
    const QByteArray rawLine = file.readLine().trimmed();
    if (rawLine.isEmpty()) {
      continue;
    }
    const QList<QByteArray> parts = rawLine.split(' ');
    if (parts.isEmpty()) {
      continue;
    }
    if (parts.first().trimmed().toLower() != keyBytes) {
      continue;
    }
    const QByteArray valueBytes = rawLine.mid(parts.first().size()).trimmed();
    return QString::fromLocal8Bit(valueBytes).trimmed();
  }

  return {};
}

bool hasDaggerfallExeLayout(const QDir& root)
{
  return QFileInfo::exists(root.filePath("DF/DAGGER/DAGGER.EXE")) ||
         QFileInfo::exists(root.filePath("DAGGER.EXE"));
}

bool hasDaggerfallDosboxLayout(const QDir& root)
{
  return (QDir(root.filePath("DOSBox-0.74")).exists() &&
          QFileInfo::exists(root.filePath("DOSBox-0.74/dosbox.exe"))) ||
         (QDir(root.filePath("DOSBOX")).exists() &&
          QFileInfo::exists(root.filePath("DOSBOX/dosbox.exe")));
}

bool hasDaggerfallDeclaredFiles(const QDir& root)
{
  const QString setupRelative =
      firstExistingRelativePath(root, {"SETUP.INI", "DF/DAGGER/SETUP.INI"});
  const QString zCfgRelative = firstExistingRelativePath(root, {"Z.CFG", "DF/DAGGER/Z.CFG"});
  if (setupRelative.isEmpty() || zCfgRelative.isEmpty()) {
    return false;
  }

  const QFileInfo setupInfo(root.filePath(setupRelative));
  if (!setupInfo.exists()) {
    return false;
  }
  const QDir setupDir = setupInfo.dir();

  const QString configFile = readDaggerfallSetupValue(root, QStringLiteral("ConfigFile"));
  const QString digitalTest = readDaggerfallSetupValue(root, QStringLiteral("DigitalTest"));
  const QString midiTest = readDaggerfallSetupValue(root, QStringLiteral("MIDITest"));
  const QString midiMelodic = readDaggerfallSetupValue(root, QStringLiteral("MIDIMelodic"));
  const QString midiDrum = readDaggerfallSetupValue(root, QStringLiteral("MIDIDrum"));

  const QStringList requiredRelative = {
      setupRelative,
      zCfgRelative,
      configFile,
      digitalTest,
      midiTest,
      midiMelodic,
      midiDrum,
  };

  for (const QString& entry : requiredRelative) {
    if (entry.trimmed().isEmpty()) {
      return false;
    }
    if (!QFileInfo::exists(setupDir.filePath(entry.trimmed()))) {
      return false;
    }
  }

  return true;
}

QStringList findMissingDaggerfallDeclaredFiles(const QDir& root)
{
  QStringList missing;

  const QString setupRelative =
      firstExistingRelativePath(root, {"SETUP.INI", "DF/DAGGER/SETUP.INI"});
  const QString zCfgRelative = firstExistingRelativePath(root, {"Z.CFG", "DF/DAGGER/Z.CFG"});
  if (setupRelative.isEmpty()) {
    missing.push_back(QStringLiteral("SETUP.INI"));
    return missing;
  }
  if (zCfgRelative.isEmpty()) {
    missing.push_back(QStringLiteral("Z.CFG"));
    return missing;
  }

  const QFileInfo setupInfo(root.filePath(setupRelative));
  const QDir setupDir = setupInfo.dir();

  const QStringList declared = {
      readDaggerfallSetupValue(root, QStringLiteral("ConfigFile")),
      readDaggerfallSetupValue(root, QStringLiteral("DigitalTest")),
      readDaggerfallSetupValue(root, QStringLiteral("MIDITest")),
      readDaggerfallSetupValue(root, QStringLiteral("MIDIMelodic")),
      readDaggerfallSetupValue(root, QStringLiteral("MIDIDrum")),
  };

  for (const QString& entry : declared) {
    if (entry.trimmed().isEmpty() || !QFileInfo::exists(setupDir.filePath(entry.trimmed()))) {
      missing.push_back(entry.trimmed().isEmpty() ? QStringLiteral("(empty declared path)") : entry.trimmed());
    }
  }

  return missing;
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

    if (!shouldRegisterManagedGameFeatures(QStringLiteral("Daggerfall"))) {
      return true;
    }

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

QStringList GameDaggerfall::primarySources() const
{
  QStringList sources{"daggerfall"};
  if (allowUnityModFormats() &&
      !sources.contains(QString::fromLatin1(kDefaultUnityNexusName), Qt::CaseInsensitive)) {
    sources.push_back(QString::fromLatin1(kDefaultUnityNexusName));
  }
  return sources;
}

QStringList GameDaggerfall::validShortNames() const
{
  DF_TRACE("[GameDaggerfall] validShortNames() called\n");
  QStringList names{"daggerfall", "df"};
  if (allowUnityModFormats() &&
      !names.contains(QString::fromLatin1(kDefaultUnityNexusName), Qt::CaseInsensitive)) {
    names.push_back(QString::fromLatin1(kDefaultUnityNexusName));
  }
  return names;
}

QStringList GameDaggerfall::iniFiles() const
{
  QStringList ordered;
  const QDir root = gameDirectory();
  const QString setupRelative =
      firstExistingRelativePath(root, {"SETUP.INI", "DF/DAGGER/SETUP.INI"});
  const QString zCfgRelative = firstExistingRelativePath(root, {"Z.CFG", "DF/DAGGER/Z.CFG"});

  if (!setupRelative.isEmpty()) {
    ordered.push_back(setupRelative);
  }
  if (!zCfgRelative.isEmpty()) {
    ordered.push_back(zCfgRelative);
  }

  const QString configFile = readDaggerfallSetupValue(root, QStringLiteral("ConfigFile"));
  if (!configFile.isEmpty()) {
    const QString configRelative =
        QDir(QFileInfo(root.filePath(setupRelative)).path()).relativeFilePath(
            QFileInfo(root.filePath(setupRelative)).dir().filePath(configFile));
    if (!ordered.contains(configRelative, Qt::CaseInsensitive)) {
      ordered.push_back(QDir::cleanPath(configRelative));
    }
  }

  const QStringList fallbackCandidates = {
      "SETUP.INI",
      "DF/DAGGER/SETUP.INI",
      "HMISET.CFG",
      "DF/DAGGER/HMISET.CFG",
      "Z.CFG",
      "DF/DAGGER/Z.CFG"};
  for (const auto& candidate : fallbackCandidates) {
    if (QFileInfo::exists(root.filePath(candidate)) &&
        !ordered.contains(candidate, Qt::CaseInsensitive)) {
      ordered.push_back(candidate);
    }
  }
  for (const auto& candidate : fallbackCandidates) {
    if (!ordered.contains(candidate, Qt::CaseInsensitive)) {
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
          "show_developer_save_details",
          tr("Developer save details level: 0=Off, 1=Compact, 2=Fit-to-pane, 3=Full."),
          0),
      PluginSetting(
          "daggerfall_unity_mod_formats",
          tr("Allow Daggerfall Unity mod formats (.dfmod and dfmod.json marker files). Limited support only: this plugin is for classic Daggerfall and cannot execute Unity runtime code."),
          false),
      PluginSetting(
          "allow_save_pack_mod_install",
          tr("Allow save-pack mods (SAVE* folders and MAPSAVE.SAV). WARNING: these can overwrite existing save-slot data."),
          true),
      PluginSetting(
          "allow_json_patch_mod_install",
          tr("Allow JSON binary patch mods. WARNING: this is dangerous and may corrupt saves or game data."),
          false),
      PluginSetting(
          "xdelta_enabled",
          tr("Allow binary patch mods (.xdelta/.xdelta3/.vcdiff, .ips, .bps, .ups, and .ppf). xdelta-family patches require the XNGINE patch tool (xdelta.exe) to be installed with MO2/plugin files. WARNING: this is dangerous and may corrupt saves or game data."),
          false),
      PluginSetting(
          "xdelta_exe_path",
          tr("Optional full path to xdelta.exe/xdelta3.exe. If empty, uses a shared global xdelta path if set, otherwise automatic detection."),
          ""),
  };
}

int GameDaggerfall::developerSaveDetailsLevel() const
{
  if (m_Organizer == nullptr) {
    return 0;
  }

  const QVariant value = m_Organizer->pluginSetting(name(), "show_developer_save_details");

  // Backward compatibility with existing bool setting values.
  if (value.metaType().id() == QMetaType::Bool) {
    return value.toBool() ? 1 : 0;
  }

  bool ok = false;
  int level = value.toInt(&ok);
  if (!ok) {
    level = value.toString().trimmed().toInt(&ok);
  }
  if (!ok) {
    return 0;
  }

  return std::clamp(level, 0, 3);
}

bool GameDaggerfall::showDeveloperSaveDetails() const
{
  return developerSaveDetailsLevel() > 0;
}

bool GameDaggerfall::allowExeModInstall() const
{
  return allowJsonPatchInstall() || allowBinaryPatchInstall();
}

bool GameDaggerfall::allowBinaryPatchInstall() const
{
  if (m_Organizer == nullptr) {
    return false;
  }
  return m_Organizer->pluginSetting(name(), "xdelta_enabled").toBool();
}

bool GameDaggerfall::allowSavePackModInstall() const
{
  if (m_Organizer == nullptr) {
    return true;
  }
  return m_Organizer->pluginSetting(name(), "allow_save_pack_mod_install").toBool();
}

bool GameDaggerfall::allowUnityModFormats() const
{
  if (m_Organizer == nullptr) {
    return false;
  }
  const bool enabled =
      m_Organizer->pluginSetting(name(), "daggerfall_unity_mod_formats").toBool();
  if (enabled) {
    static bool warnedLimitedUnitySupport = false;
    if (!warnedLimitedUnitySupport) {
      qWarning().noquote()
          << "[GameDaggerfall] Daggerfall Unity mod format support is enabled in compatibility mode."
          << "Support is limited: this plugin manages classic Daggerfall and cannot run Unity runtime .dfmod code.";
      warnedLimitedUnitySupport = true;
    }
  }
  return enabled;
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
  if (!allowBinaryPatchInstall()) {
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

bool GameDaggerfall::looksValid(QDir const& path) const
{
  if (!path.exists()) {
    return false;
  }

  if (hasDaggerfallExeLayout(path)) {
    return true;
  }

  if (hasDaggerfallDosboxLayout(path)) {
    if (!hasDaggerfallDeclaredFiles(path)) {
      qWarning().noquote()
          << "[GameDaggerfall] Config-declared files are missing, but the install still looks valid:"
          << findMissingDaggerfallDeclaredFiles(path).join(", ");
    }
    return true;
  }

  return false;
}

QDir GameDaggerfall::savesDirectory() const
{
  return GameXngine::savesDirectory();
}

QDir GameDaggerfall::dataDirectory() const
{
  return gameDirectory();
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
  const QString configuredMapSave =
      readDaggerfallZCfgValue(gameDirectory(), QStringLiteral("maps"));
  const QString mapSaveArchive =
      configuredMapSave.isEmpty() ? QStringLiteral("MAPSAVE.SAV")
                                  : QFileInfo(configuredMapSave).fileName();

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
      {mapSaveArchive, true, XngineBSAFormat::IndexType::NameRecord, true,
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
  const bool allowLegacyBinaryPatches = allowBinaryPatchInstall();
  if (!allowJson && !allowXdelta && !allowLegacyBinaryPatches) {
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
    QStringList ipsPatchFiles;
    QStringList bpsPatchFiles;
    QStringList upsPatchFiles;
    QStringList ppfPatchFiles;
  };
  QList<ModPatchInput> patchInputs;

  struct PatchSummary
  {
    int replacementExeCount = 0;
    int replacementExeFailCount = 0;
    int jsonCatalogCount = 0;
    int jsonCatalogFailCount = 0;
    int xdeltaCount = 0;
    int xdeltaFailCount = 0;
    int ipsCount = 0;
    int ipsFailCount = 0;
    int bpsCount = 0;
    int bpsFailCount = 0;
    int upsCount = 0;
    int upsFailCount = 0;
    int ppfCount = 0;
    int ppfFailCount = 0;
    QStringList details;
  };
  PatchSummary summary;

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
      QDirIterator xdeltaIt(modPath, QStringList() << "*.xdelta" << "*.xdelta3" << "*.vcdiff", QDir::Files,
                            QDirIterator::Subdirectories);
      while (xdeltaIt.hasNext()) {
        input.xdeltaPatchFiles.push_back(xdeltaIt.next());
      }
      input.xdeltaPatchFiles.sort(Qt::CaseInsensitive);
    }
    if (allowLegacyBinaryPatches) {
      QDirIterator ipsIt(modPath, QStringList() << "*.ips", QDir::Files,
                        QDirIterator::Subdirectories);
      while (ipsIt.hasNext()) {
        input.ipsPatchFiles.push_back(ipsIt.next());
      }
      input.ipsPatchFiles.sort(Qt::CaseInsensitive);
      QDirIterator bpsIt(modPath, QStringList() << "*.bps", QDir::Files,
                        QDirIterator::Subdirectories);
      while (bpsIt.hasNext()) {
        input.bpsPatchFiles.push_back(bpsIt.next());
      }
      input.bpsPatchFiles.sort(Qt::CaseInsensitive);
      QDirIterator upsIt(modPath, QStringList() << "*.ups", QDir::Files,
                        QDirIterator::Subdirectories);
      while (upsIt.hasNext()) {
        input.upsPatchFiles.push_back(upsIt.next());
      }
      input.upsPatchFiles.sort(Qt::CaseInsensitive);
      QDirIterator ppfIt(modPath, QStringList() << "*.ppf", QDir::Files,
                        QDirIterator::Subdirectories);
      while (ppfIt.hasNext()) {
        input.ppfPatchFiles.push_back(ppfIt.next());
      }
      input.ppfPatchFiles.sort(Qt::CaseInsensitive);
    }

    if (!input.catalogPath.isEmpty() || !input.replacementExePath.isEmpty() ||
        !input.xdeltaPatchFiles.isEmpty() || !input.ipsPatchFiles.isEmpty() ||
        !input.bpsPatchFiles.isEmpty() || !input.upsPatchFiles.isEmpty() ||
        !input.ppfPatchFiles.isEmpty()) {
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
        ++summary.replacementExeFailCount;
        overallSuccess = false;
      } else {
        QFile::remove(destPath);
        if (!QFile::copy(input.replacementExePath, destPath)) {
          qWarning().noquote()
              << "[GameDaggerfall] Failed to stage replacement executable from mod"
              << input.modName << "->" << destPath;
          ++summary.replacementExeFailCount;
          overallSuccess = false;
        } else {
          qInfo().noquote()
              << "[GameDaggerfall] Staged replacement executable from mod" << input.modName
              << "to" << destPath;
          ++summary.replacementExeCount;
          summary.details.push_back(
              QString("mod=%1 type=replacement target=%2")
                  .arg(input.modName, QDir(tempModPath).relativeFilePath(destPath)));
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
      summary.jsonCatalogFailCount += input.catalogPath.isEmpty() ? 0 : 1;
      summary.xdeltaFailCount += input.xdeltaPatchFiles.size();
      summary.ipsFailCount += input.ipsPatchFiles.size();
      summary.bpsFailCount += input.bpsPatchFiles.size();
      summary.upsFailCount += input.upsPatchFiles.size();
      summary.ppfFailCount += input.ppfPatchFiles.size();
      overallSuccess = false;
      continue;
    }

    if (!input.catalogPath.isEmpty()) {
      QVector<XngineExePatch::PatchSet> sets;
      QString loadError;
      if (!XngineExePatch::loadPatchSetsFromJsonFile(input.catalogPath, sets, &loadError)) {
        qWarning().noquote() << "[GameDaggerfall] Failed to load patch catalog from mod"
                             << input.modName << ":" << loadError;
        ++summary.jsonCatalogFailCount;
        overallSuccess = false;
        continue;
      }
      if (sets.isEmpty()) {
        qWarning().noquote() << "[GameDaggerfall] Empty patch catalog in mod" << input.modName;
        ++summary.jsonCatalogFailCount;
        overallSuccess = false;
        continue;
      }

      QFile exeFile(workingExe->absolutePath);
      if (!exeFile.open(QIODevice::ReadOnly)) {
        qWarning().noquote() << "[GameDaggerfall] Failed to read base executable for patching:"
                             << workingExe->absolutePath;
        ++summary.jsonCatalogFailCount;
        overallSuccess = false;
        continue;
      }
      QByteArray exeData = exeFile.readAll();
      exeFile.close();

      if (!XngineExePatch::applyPatchSets(exeData, sets, &loadError)) {
        qWarning().noquote() << "[GameDaggerfall] Failed to apply patch catalog from mod"
                             << input.modName << ":" << loadError;
        ++summary.jsonCatalogFailCount;
        overallSuccess = false;
        continue;
      }

      const QString destPath = QDir(tempModPath).filePath(workingExe->relativePath);
      const QString destDir = QFileInfo(destPath).absolutePath();
      if (!ensureDir(destDir)) {
        qWarning().noquote()
            << "[GameDaggerfall] Failed to create destination directory for patched EXE:"
            << destDir;
        ++summary.jsonCatalogFailCount;
        overallSuccess = false;
        continue;
      }

      QFile out(destPath);
      if (!out.open(QIODevice::WriteOnly)) {
        qWarning().noquote() << "[GameDaggerfall] Failed to write patched executable:"
                             << destPath;
        ++summary.jsonCatalogFailCount;
        overallSuccess = false;
        continue;
      }
      if (out.write(exeData) != exeData.size()) {
        qWarning().noquote() << "[GameDaggerfall] Incomplete write for patched executable:"
                             << destPath;
        ++summary.jsonCatalogFailCount;
        overallSuccess = false;
      }
      out.close();

      qInfo().noquote() << "[GameDaggerfall] Staged patched executable from catalog in mod"
                        << input.modName << "to" << destPath;
      ++summary.jsonCatalogCount;
      summary.details.push_back(
          QString("mod=%1 type=json_catalog target=%2")
              .arg(input.modName, QDir(tempModPath).relativeFilePath(destPath)));
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
        summary.xdeltaFailCount += input.xdeltaPatchFiles.size();
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
          ++summary.xdeltaFailCount;
          overallSuccess = false;
          break;
        }
        qInfo().noquote() << "[GameDaggerfall] Applied .xdelta patch to" << matchedRel
                          << "from mod" << input.modName << ":" << patchFile;
        ++summary.xdeltaCount;
        summary.details.push_back(
            QString("mod=%1 type=xdelta target=%2 patch=%3")
                .arg(input.modName, matchedRel, QFileInfo(patchFile).fileName()));
      }
    }

    if (!input.ipsPatchFiles.isEmpty()) {
      for (const QString& patchFile : input.ipsPatchFiles) {
        QString err;
        QString matchedRel;
        if (!XngineExePatch::applyIpsPatchToAnyFileInTree(patchFile, gameDirPath, tempModPath,
                                                          &matchedRel, &err)) {
          qWarning().noquote() << "[GameDaggerfall] Failed to apply .ips patch"
                               << patchFile << "from mod" << input.modName << ":" << err;
          ++summary.ipsFailCount;
          overallSuccess = false;
          break;
        }
        qInfo().noquote() << "[GameDaggerfall] Applied .ips patch to" << matchedRel
                          << "from mod" << input.modName << ":" << patchFile;
        ++summary.ipsCount;
        summary.details.push_back(
            QString("mod=%1 type=ips target=%2 patch=%3")
                .arg(input.modName, matchedRel, QFileInfo(patchFile).fileName()));
      }
    }

    if (!input.bpsPatchFiles.isEmpty()) {
      for (const QString& patchFile : input.bpsPatchFiles) {
        QString err;
        QString matchedRel;
        if (!XngineExePatch::applyBpsPatchToAnyFileInTree(patchFile, gameDirPath, tempModPath,
                                                          &matchedRel, &err)) {
          qWarning().noquote() << "[GameDaggerfall] Failed to apply .bps patch"
                               << patchFile << "from mod" << input.modName << ":" << err;
          ++summary.bpsFailCount;
          overallSuccess = false;
          break;
        }
        qInfo().noquote() << "[GameDaggerfall] Applied .bps patch to" << matchedRel
                          << "from mod" << input.modName << ":" << patchFile;
        ++summary.bpsCount;
        summary.details.push_back(
            QString("mod=%1 type=bps target=%2 patch=%3")
                .arg(input.modName, matchedRel, QFileInfo(patchFile).fileName()));
      }
    }

    if (!input.upsPatchFiles.isEmpty()) {
      for (const QString& patchFile : input.upsPatchFiles) {
        QString err;
        QString matchedRel;
        if (!XngineExePatch::applyUpsPatchToAnyFileInTree(patchFile, gameDirPath, tempModPath,
                                                          &matchedRel, &err)) {
          qWarning().noquote() << "[GameDaggerfall] Failed to apply .ups patch"
                               << patchFile << "from mod" << input.modName << ":" << err;
          ++summary.upsFailCount;
          overallSuccess = false;
          break;
        }
        qInfo().noquote() << "[GameDaggerfall] Applied .ups patch to" << matchedRel
                          << "from mod" << input.modName << ":" << patchFile;
        ++summary.upsCount;
        summary.details.push_back(
            QString("mod=%1 type=ups target=%2 patch=%3")
                .arg(input.modName, matchedRel, QFileInfo(patchFile).fileName()));
      }
    }

    if (!input.ppfPatchFiles.isEmpty()) {
      for (const QString& patchFile : input.ppfPatchFiles) {
        QString err;
        QString matchedRel;
        if (!XngineExePatch::applyPpfPatchToAnyFileInTree(patchFile, gameDirPath, tempModPath,
                                                          &matchedRel, &err)) {
          qWarning().noquote() << "[GameDaggerfall] Failed to apply .ppf patch"
                               << patchFile << "from mod" << input.modName << ":" << err;
          ++summary.ppfFailCount;
          overallSuccess = false;
          break;
        }
        qInfo().noquote() << "[GameDaggerfall] Applied .ppf patch to" << matchedRel
                          << "from mod" << input.modName << ":" << patchFile;
        ++summary.ppfCount;
        summary.details.push_back(
            QString("mod=%1 type=ppf target=%2 patch=%3")
                .arg(input.modName, matchedRel, QFileInfo(patchFile).fileName()));
      }
    }
  }

  qInfo().noquote()
      << "[GameDaggerfall] EXE patch summary:"
      << "replacementExe=" << summary.replacementExeCount
      << "replacementExeFailed=" << summary.replacementExeFailCount
      << "jsonCatalog=" << summary.jsonCatalogCount
      << "jsonCatalogFailed=" << summary.jsonCatalogFailCount
      << "xdelta=" << summary.xdeltaCount
      << "xdeltaFailed=" << summary.xdeltaFailCount
      << "ips=" << summary.ipsCount
      << "ipsFailed=" << summary.ipsFailCount
      << "bps=" << summary.bpsCount
      << "bpsFailed=" << summary.bpsFailCount
      << "ups=" << summary.upsCount
      << "upsFailed=" << summary.upsFailCount
      << "ppf=" << summary.ppfCount
      << "ppfFailed=" << summary.ppfFailCount
      << "overallSuccess=" << (overallSuccess ? "true" : "false");
  for (const QString& line : summary.details) {
    qInfo().noquote() << "[GameDaggerfall]  -" << line;
  }

  if (!overallSuccess) {
    qWarning().noquote()
        << "[GameDaggerfall] EXE patch staging completed with failures."
        << "Review warnings above. Common fixes: verify patch/base-game version match,"
        << "enable required plugin settings, and confirm xdelta path/tool availability.";
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
