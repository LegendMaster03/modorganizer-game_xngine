#include "gamebattlespire.h"
#include "battlespiredatachecker.h"
#include "battlespiremodatacontent.h"
#include "battlespiresavegame.h"
#include "xngineexepatch.h"

#include <executableinfo.h>
#include <pluginsetting.h>

#include <xnginelocalsavegames.h>
#include <xnginesavegameinfo.h>
#include <xngineunmanagedmods.h>

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QFile>
#include <QRegularExpression>
#include <QIcon>
#include <QDirIterator>
#include <QTextStream>
#include <QSettings>

#include <Windows.h>

#include "utility.h"

#include <exception>
#include <memory>
#include <stdexcept>
#include <algorithm>

using namespace MOBase;

namespace
{
constexpr const char* kExePatchTempModPrefix = "__battlespire_exe_patch_output_";
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
}  // namespace

GameBattlespire::GameBattlespire()
{
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] Constructor ENTRY";
  OutputDebugStringA("[GameBattlespire] Constructor ENTRY\n");
  OutputDebugStringA("[GameBattlespire] Constructor EXIT\n");
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] Constructor EXIT";
}

void GameBattlespire::detectGame()
{
  GameXngine::detectGame();

  if (!m_MyGamesPath.isEmpty()) {
    return;
  }

  const QString docsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  QString fallbackPath;
  const QString localBase = GameXngine::localAppFolder();
  if (!localBase.isEmpty()) {
    fallbackPath = QDir::cleanPath(localBase + "/Battlespire");
  } else if (!docsPath.isEmpty()) {
    fallbackPath = QDir::cleanPath(docsPath + "/My Games/Battlespire");
  }

  if (!fallbackPath.isEmpty()) {
    QDir().mkpath(fallbackPath);
    m_MyGamesPath = fallbackPath;
    if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] detectGame() fallback myGamesPath:" << m_MyGamesPath;
  }
}

bool GameBattlespire::init(IOrganizer* moInfo)
{
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] init() ENTRY";
  OutputDebugStringA("[GameBattlespire] init() ENTRY\n");

  try {
    OutputDebugStringA("[GameBattlespire] About to call GameXngine::init()\n");
    if (!GameXngine::init(moInfo)) {
      qWarning().noquote() << "[GameBattlespire] GameXngine::init() FAILED";
      OutputDebugStringA("[GameBattlespire] GameXngine::init() FAILED\n");
      return false;
    }
    if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] GameXngine::init() SUCCESS";
    OutputDebugStringA("[GameBattlespire] GameXngine::init() SUCCESS\n");

    const QString iniForLocalSaves = iniFiles().isEmpty() ? QString{} : iniFiles().first();
    registerFeature(std::make_shared<BattlespiresModDataChecker>(this));
    registerFeature(std::make_shared<BattlespireModDataContent>(m_Organizer->gameFeatures()));
    registerFeature(std::make_shared<XngineSaveGameInfo>(this));
    registerFeature(std::make_shared<XngineLocalSavegames>(this, iniForLocalSaves));
    registerFeature(std::make_shared<XngineUnmanagedMods>(this));
    ensureExePatchCleanupHook();

    if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] init() EXIT SUCCESS";
    OutputDebugStringA("[GameBattlespire] init() EXIT SUCCESS\n");
    return true;
  } catch (const std::exception&) {
    qWarning().noquote() << "[GameBattlespire] EXCEPTION in init()";
    OutputDebugStringA("[GameBattlespire] EXCEPTION in init()\n");
    return false;
  } catch (...) {
    qWarning().noquote() << "[GameBattlespire] UNKNOWN EXCEPTION in init()";
    OutputDebugStringA("[GameBattlespire] UNKNOWN EXCEPTION in init()\n");
    return false;
  }
}

std::vector<std::shared_ptr<const MOBase::ISaveGame>>
GameBattlespire::listSaves(QDir folder) const
{
  return GameXngine::listSaves(folder);
}

QString GameBattlespire::gameName() const
{
  return "Battlespire";
}

QString GameBattlespire::displayGameName() const
{
  return "An Elder Scrolls Legend: Battlespire";
}

QList<MOBase::ExecutableInfo> GameBattlespire::executables() const
{
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] executables() ENTRY";
  OutputDebugStringA("[GameBattlespire] executables() ENTRY\n");

  QList<ExecutableInfo> executables;
  QDir gameDir = gameDirectory();
  if (gameDir.path().isEmpty() || !gameDir.exists()) {
    qWarning().noquote() << "[GameBattlespire] executables() EXIT - invalid game directory";
    OutputDebugStringA("[GameBattlespire] executables() EXIT - invalid game directory\n");
    return executables;
  }

  // Steam DOSBox launcher
  QFileInfo steamDosbox(gameDir.filePath("DOSBox-0.73/dosbox.exe"));
  QFileInfo steamConfig(gameDir.filePath("DOSBox-0.73/bs.conf"));
  QFileInfo steamSingleConfig(gameDir.filePath("DOSBox-0.73/bs_single.conf"));
  QFileInfo steamClientConfig(gameDir.filePath("DOSBox-0.73/bs_client.conf"));
  QFileInfo steamServerConfig(gameDir.filePath("DOSBox-0.73/bs_server.conf"));
  if (steamDosbox.exists() && steamConfig.exists() && steamSingleConfig.exists()) {
    executables << ExecutableInfo("Battlespire (Steam DOSBox Single Windowed)", steamDosbox)
                   .withArgument("-noconsole -conf bs.conf -conf bs_single.conf");
    executables << ExecutableInfo("Battlespire (Steam DOSBox Single Fullscreen)", steamDosbox)
                   .withArgument("-noconsole -conf bs.conf -conf bs_single.conf -fullscreen");
  }
  if (steamDosbox.exists() && steamConfig.exists() && steamClientConfig.exists()) {
    executables << ExecutableInfo("Battlespire (Steam DOSBox Client Windowed)", steamDosbox)
                   .withArgument("-noconsole -conf bs.conf -conf bs_client.conf");
    executables << ExecutableInfo("Battlespire (Steam DOSBox Client Fullscreen)", steamDosbox)
                   .withArgument("-noconsole -conf bs.conf -conf bs_client.conf -fullscreen");
  }
  if (steamDosbox.exists() && steamConfig.exists() && steamServerConfig.exists()) {
    executables << ExecutableInfo("Battlespire (Steam DOSBox Server Windowed)", steamDosbox)
                   .withArgument("-noconsole -conf bs.conf -conf bs_server.conf");
    executables << ExecutableInfo("Battlespire (Steam DOSBox Server Fullscreen)", steamDosbox)
                   .withArgument("-noconsole -conf bs.conf -conf bs_server.conf -fullscreen");
  }

  // GOG DOSBox launcher
  QFileInfo gogDosbox(gameDir.filePath("DOSBOX/dosbox.exe"));
  QFileInfo gogConfig(gameDir.filePath("dosbox_battlespire.conf"));
  QFileInfo gogSingleConfig(gameDir.filePath("dosbox_battlespire_single.conf"));
  QFileInfo gogClientConfig(gameDir.filePath("dosbox_battlespire_client.conf"));
  QFileInfo gogServerConfig(gameDir.filePath("dosbox_battlespire_server.conf"));
  if (gogDosbox.exists() && gogConfig.exists() && gogSingleConfig.exists()) {
    executables << ExecutableInfo("Battlespire (GOG DOSBox Single)", gogDosbox)
                   .withArgument(R"(-conf "..\dosbox_battlespire.conf" -conf "..\dosbox_battlespire_single.conf" -noconsole -c "exit")");
  }
  if (gogDosbox.exists() && gogConfig.exists() && gogClientConfig.exists()) {
    executables << ExecutableInfo("Battlespire (GOG DOSBox Client)", gogDosbox)
                   .withArgument(R"(-noconsole -c "exit")");
  }
  if (gogDosbox.exists() && gogConfig.exists() && gogServerConfig.exists()) {
    executables << ExecutableInfo("Battlespire (GOG DOSBox Server)", gogDosbox)
                   .withArgument(R"(-noconsole -c "exit")");
  }

  QFileInfo spireBat(gameDir.filePath("SPIRE.BAT"));
  if (spireBat.exists()) {
    if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] Found SPIRE.BAT executable";
    OutputDebugStringA("[GameBattlespire] Found SPIRE.BAT executable\n");
    executables << ExecutableInfo("Battlespire", spireBat);
  }

  // Standalone executable if it exists
  QFileInfo gameExe(gameDir.filePath("GAME.EXE"));
  if (gameExe.exists()) {
    if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] Found GAME.EXE executable";
    OutputDebugStringA("[GameBattlespire] Found GAME.EXE executable\n");
    executables << ExecutableInfo("Battlespire (GAME.EXE)", gameExe);
  }

  if (executables.empty()) {
    QFileInfo fallbackDosbox(gameDir.filePath("dosbox.exe"));
    if (fallbackDosbox.exists()) {
      if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] Found fallback dosbox.exe";
      OutputDebugStringA("[GameBattlespire] Found fallback dosbox.exe\n");
      executables << ExecutableInfo("Battlespire (DOSBox)", fallbackDosbox)
                     .withArgument("-conf bs_single.conf");
    }
  }

  if (executables.empty()) {
    qWarning().noquote() << "[GameBattlespire] executables() EXIT - no executables found";
    OutputDebugStringA("[GameBattlespire] executables() EXIT - no executables found\n");
  } else {
    if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] executables() EXIT - executables found";
    OutputDebugStringA("[GameBattlespire] executables() EXIT - executables found\n");
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
  return "GAME.EXE";
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
  /*
  return {"battlespire", "an elder scrolls legend", "tesbattlespire"};
  */
  return {"battlespire"};
}

QStringList GameBattlespire::iniFiles() const
{
  const QStringList candidates = {"SPIRE.CFG",
                                };

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

QVector<XngineBSAFormat::FileSpec> GameBattlespire::bsaFileSpecs() const
{
    return {
        {"3D.BSA", false, XngineBSAFormat::IndexType::NameRecord, false,
         "3D asset archive used by Battlespire."},
        {"BS6.BSA", false, XngineBSAFormat::IndexType::NameRecord, false,
         "BS6 level/model data archive."},
        {"BSI.BSA", false, XngineBSAFormat::IndexType::NameRecord, false,
         "Battlespire support data archive (exact record schema varies)."},
        {"TXT.BSA", false, XngineBSAFormat::IndexType::NameRecord, false,
         "Text payloads used by game systems (e.g. magical items list)."},
        {"FLC.BSA", false, XngineBSAFormat::IndexType::NameRecord, false,
         "Conversation animation frames (some files may contain two leading unknown bytes)."},
        {"SPIRE.SND", true, XngineBSAFormat::IndexType::NumberRecord, false,
         "RIFF/WAVE audio records.", XngineBSAFormat::ArchiveVariant::Snd},
        {"WAVES.BSA", false, XngineBSAFormat::IndexType::NameRecord, true,
         "CD-only headerless 8-bit mono PCM at 11025 Hz; absent from GOG release."},
    };
  }

XngineBSAFormat::Traits GameBattlespire::bsaTraits() const
{
  XngineBSAFormat::Traits traits;
  traits.allowCompressed = true;
  traits.allowCompressedPassthroughWrite = true;
  traits.compressionMode = XngineBSAFormat::CompressionMode::BattlespireLzss;
  traits.allowMissingTypeHeader = true;
  traits.writeTypeHeader = true;
  return traits;
}

int GameBattlespire::nexusModOrganizerID() const
{
  return 0;  // To be determined
}

int GameBattlespire::nexusGameID() const
{
  return 3495;  // Nexus "games.json" id for An Elder Scrolls Legend: Battlespire
}

bool GameBattlespire::looksValid(QDir const& path) const
{
  return path.exists("GAME.EXE") || path.exists("SPIRE.BAT");
}

QString GameBattlespire::gameVersion() const
{
  const QString binaryVersion = detectDosVersionFromBinaryStrings(
      gameDirectory(),
      {
          "GAME.EXE",
      },
      {
          QRegularExpression(R"(\bBattlespire\s+V([0-9]+(?:\.[0-9]+){1,3})\b)",
                             QRegularExpression::CaseInsensitiveOption),
      });
  if (!binaryVersion.isEmpty()) {
    if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] gameVersion() detected" << binaryVersion;
    return binaryVersion;
  }

  const QString version = detectGameVersion(
      {
          "GAME.EXE",
      },
      {
          "patch.txt",
          "PATCH.TXT",
          "README.TXT",
          "GAMEDATA/README.TXT",
      },
      {
          QRegularExpression(R"(\bv([0-9]+(?:\.[0-9]+){1,3})\b)",
                             QRegularExpression::CaseInsensitiveOption),
          QRegularExpression(R"(\bversion\s+([0-9]+(?:\.[0-9]+){1,3})\b)",
                             QRegularExpression::CaseInsensitiveOption),
      });

  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] gameVersion() detected" << version;
  return version;
}

QIcon GameBattlespire::gameIcon() const
{
  QDir dir = gameDirectory();
  
  // Try .ico files first
  QStringList icoCandidates = {
      "SPIRE.ico",
      "SPIRE.ICO",
      "goggame-1435829464.ico"
  };
  
  for (const QString& relPath : icoCandidates) {
    QFileInfo iconFile(dir.filePath(relPath));
    if (iconFile.exists()) {
      return QIcon(iconFile.absoluteFilePath());
    }
  }
  
  // Fallback to EXE icon
  const QString exePath = dir.absoluteFilePath("GAME.EXE");
  QIcon icon = MOBase::iconForExecutable(exePath);
  return icon.isNull() ? GameXngine::gameIcon() : icon;
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
  return {
      PluginSetting(
          "show_developer_save_details",
          tr("Show internal Battlespire save debug details (record offsets, flags, validation notes) in save info."),
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

bool GameBattlespire::showDeveloperSaveDetails() const
{
  if (m_Organizer == nullptr) {
    return false;
  }
  return m_Organizer->pluginSetting(name(), "show_developer_save_details").toBool();
}

bool GameBattlespire::allowExeModInstall() const
{
  return allowJsonPatchInstall() || allowXdeltaPatchInstall();
}

bool GameBattlespire::allowJsonPatchInstall() const
{
  return false;
}

bool GameBattlespire::allowXdeltaPatchInstall() const
{
  if (m_Organizer == nullptr) {
    return false;
  }
  if (!m_Organizer->pluginSetting(name(), "xdelta_enabled").toBool()) {
    return false;
  }

  QString configuredTool =
      m_Organizer->pluginSetting(name(), "xdelta_exe_path").toString().trimmed();
  bool usingGlobalFallback = false;
  if (!configuredTool.isEmpty()) {
    writeGlobalXdeltaPath(configuredTool);
  } else {
    const QString globalConfigured = readGlobalXdeltaPath();
    if (!globalConfigured.isEmpty()) {
      configuredTool = globalConfigured;
      usingGlobalFallback = true;
    }
  }
  const QString gameDirPath = gameDirectory().absolutePath();

  // Prefer explicit user-configured path if provided.
  if (!configuredTool.isEmpty()) {
    const QFileInfo configuredInfo(configuredTool);
    const bool configuredExists = configuredInfo.exists() && configuredInfo.isFile();
    if (configuredExists) {
      const QString resolvedConfigured =
          XngineExePatch::findXdeltaTool({}, gameDirPath, configuredTool);
      if (!resolvedConfigured.isEmpty()) {
        if (usingGlobalFallback) {
          logXdeltaToolStatusOnce(
              QString("global_ok|%1|%2").arg(configuredTool, resolvedConfigured));
        } else {
          logXdeltaToolStatusOnce(QString("configured_ok|%1").arg(resolvedConfigured));
        }
        return true;
      }

      if (usingGlobalFallback) {
        logXdeltaToolStatusOnce(QString("global_unusable|%1").arg(configuredTool));
      } else {
        logXdeltaToolStatusOnce(QString("configured_unusable|%1").arg(configuredTool));
      }
    } else {
      const QString resolvedAuto = XngineExePatch::findXdeltaTool({}, gameDirPath, {});
      if (!resolvedAuto.isEmpty()) {
        if (usingGlobalFallback) {
          logXdeltaToolStatusOnce(
              QString("global_missing_fallback|%1|%2").arg(configuredTool, resolvedAuto));
        } else {
          logXdeltaToolStatusOnce(
              QString("configured_missing_fallback|%1|%2").arg(configuredTool, resolvedAuto));
        }
        return true;
      }

      if (usingGlobalFallback) {
        logXdeltaToolStatusOnce(QString("global_missing_no_tool|%1").arg(configuredTool));
      } else {
        logXdeltaToolStatusOnce(QString("configured_missing_no_tool|%1").arg(configuredTool));
      }
      return false;
    }
  }

  const QString resolvedAuto = XngineExePatch::findXdeltaTool({}, gameDirPath, {});
  if (!resolvedAuto.isEmpty()) {
    logXdeltaToolStatusOnce(QString("auto_ok|%1").arg(resolvedAuto));
    return true;
  }

  logXdeltaToolStatusOnce("no_tool");
  return false;
}

bool GameBattlespire::prepareIni(const QString& exec)
{
  if (!GameXngine::prepareIni(exec)) {
    return false;
  }

  if (allowExeModInstall()) {
    if (!applyExePatchMods()) {
      qWarning().noquote()
          << "[GameBattlespire] EXE patch staging failed; continuing launch without generated patch output.";
    }
  }

  return true;
}

QString GameBattlespire::identifyGamePath() const
{
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] identifyGamePath() ENTRY";
  OutputDebugStringA("[GameBattlespire] identifyGamePath() ENTRY\n");
  try {
  // Try Steam first (using Steam App ID 1812420)
  QString steamPath = findInRegistry(HKEY_LOCAL_MACHINE,
                                     L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 1812420",
                                     L"InstallLocation");
  if (!steamPath.isEmpty()) {
    if (QDir(steamPath).exists() && QDir(steamPath + "/GAMEDATA").exists()) {
      if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] Steam path verified";
      OutputDebugStringA("[GameBattlespire] Steam path verified\n");
      return steamPath;
    }
  }

  // Try GOG registry (using GOG Game ID 1435829464)
  QString gogPath = findInRegistry(HKEY_LOCAL_MACHINE,
                                   L"Software\\GOG.com\\Games\\1435829464",
                                   L"path");
  if (!gogPath.isEmpty()) {
    if (QDir(gogPath).exists() && QDir(gogPath + "/GAMEDATA").exists()) {
      if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] GOG registry path verified";
      OutputDebugStringA("[GameBattlespire] GOG registry path verified\n");
      return gogPath;
    }
  }

  qWarning().noquote() << "[GameBattlespire] identifyGamePath() EXIT (not found)";
  OutputDebugStringA("[GameBattlespire] identifyGamePath() EXIT (not found)\n");
  return {};
  } catch (const std::exception&) {
    qWarning().noquote() << "[GameBattlespire] EXCEPTION in identifyGamePath()";
    OutputDebugStringA("[GameBattlespire] EXCEPTION in identifyGamePath()\n");
    return {};
  } catch (...) {
    qWarning().noquote() << "[GameBattlespire] UNKNOWN EXCEPTION in identifyGamePath()";
    OutputDebugStringA("[GameBattlespire] UNKNOWN EXCEPTION in identifyGamePath()\n");
    return {};
  }
}

QDir GameBattlespire::dataDirectory() const
{
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] dataDirectory() ENTRY";
  QDir gameDir = gameDirectory();
  if (gameDir.path().isEmpty() || !gameDir.exists()) {
    qWarning().noquote() << "[GameBattlespire] dataDirectory() - game directory invalid:" << gameDir.absolutePath();
    return QDir();
  }
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] dataDirectory() using game root:" << gameDir.absolutePath();
  return gameDir;
}

QDir GameBattlespire::documentsDirectory() const
{
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] documentsDirectory() using game install path";
  return gameDirectory();
}

QDir GameBattlespire::savesDirectory() const
{
  return GameXngine::savesDirectory();
}

QString GameBattlespire::savegameExtension() const
{
  return {};
}

QString GameBattlespire::savegameSEExtension() const
{
  return {};
}

std::shared_ptr<const XngineSaveGame> GameBattlespire::makeSaveGame(QString filepath) const
{
  return std::make_shared<BattlespireSaveGame>(filepath, this);
}

SaveLayout GameBattlespire::saveLayout() const
{
  SaveLayout layout;
  layout.baseRelativePaths = {""};
  layout.slotDirRegex = QRegularExpression("^SAVE(\\d+)$");
  layout.slotWidthHint = 1;
  layout.maxSlotHint = 9;
  layout.validator = [](const QDir& slotDir) {
    return slotDir.exists();
  };
  return layout;
}

QString GameBattlespire::saveGameId() const
{
  return "battlespire";
}

void GameBattlespire::ensureExePatchCleanupHook()
{
  if (m_ExePatchCleanupHookRegistered || !m_Organizer) {
    return;
  }
  m_Organizer->onFinishedRun([this](const QString&, unsigned int) {
    cleanupExePatchOutputMod();
  });
  m_ExePatchCleanupHookRegistered = true;
}

void GameBattlespire::cleanupExePatchOutputMod() const
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
  const bool removed = removeDirRecursive(tempModPath);
  if (!removed) {
    qWarning().noquote() << "[GameBattlespire] Failed to remove temp patch mod directory:"
                         << tempModPath;
  }
}

void GameBattlespire::logXdeltaToolStatusOnce(const QString& status) const
{
  if (m_LastXdeltaToolStatus == status) {
    return;
  }
  m_LastXdeltaToolStatus = status;

  const QStringList parts = status.split('|');
  const QString kind = parts.value(0);

  if (kind == "configured_ok") {
    qInfo().noquote() << "[GameBattlespire] xdelta tool path is valid (plugin setting):"
                      << parts.value(1);
    return;
  }
  if (kind == "global_ok") {
    qInfo().noquote() << "[GameBattlespire] Using global xdelta path from shared settings:"
                      << parts.value(1)
                      << "(resolved to:" << parts.value(2) << ")";
    return;
  }
  if (kind == "configured_unusable") {
    qWarning().noquote() << "[GameBattlespire] xdelta_exe_path is set but could not be used:"
                         << parts.value(1);
    return;
  }
  if (kind == "global_unusable") {
    qWarning().noquote()
        << "[GameBattlespire] Global xdelta path is set but could not be used:"
        << parts.value(1);
    return;
  }
  if (kind == "configured_missing_fallback") {
    qWarning().noquote() << "[GameBattlespire] xdelta_exe_path does not exist:"
                         << parts.value(1)
                         << "- using auto-detected xdelta tool:"
                         << parts.value(2);
    return;
  }
  if (kind == "global_missing_fallback") {
    qWarning().noquote() << "[GameBattlespire] Global xdelta path does not exist:"
                         << parts.value(1)
                         << "- using auto-detected xdelta tool:"
                         << parts.value(2);
    return;
  }
  if (kind == "configured_missing_no_tool") {
    qWarning().noquote() << "[GameBattlespire] xdelta_exe_path does not exist:"
                         << parts.value(1)
                         << "- and no auto-detected xdelta tool was found.";
    return;
  }
  if (kind == "global_missing_no_tool") {
    qWarning().noquote() << "[GameBattlespire] Global xdelta path does not exist:"
                         << parts.value(1)
                         << "- and no auto-detected xdelta tool was found.";
    return;
  }
  if (kind == "auto_ok") {
    qInfo().noquote() << "[GameBattlespire] xdelta tool auto-detected at:" << parts.value(1);
    return;
  }

  qWarning().noquote()
      << "[GameBattlespire] .xdelta patch setting is enabled but no xdelta tool was found."
      << "Set plugin setting 'xdelta_exe_path' or install xdelta.exe in an auto-detected location.";
}

bool GameBattlespire::applyExePatchMods()
{
  if (!m_Organizer) {
    return false;
  }
  const bool allowXdelta = allowXdeltaPatchInstall();
  if (!allowXdelta) {
    return true;
  }
  auto* modList = m_Organizer->modList();
  if (!modList) {
    return false;
  }

  const QStringList exeCandidates = {"GAME.EXE"};
  const QStringList exeNames = {"GAME.EXE"};
  const QStringList allMods = modList->allModsByProfilePriority();
  if (allMods.isEmpty()) {
    return true;
  }

  const QString modsPath = m_Organizer->modsPath();
  const QString gameDirPath = gameDirectory().absolutePath();
  const QString tempModName = QString(kExePatchTempModPrefix) + profileSuffix(profilePath());
  const QString tempModPath = QDir(modsPath).filePath(tempModName);
  qInfo().noquote() << "[GameBattlespire] applyExePatchMods() start:"
                    << "allowXdelta=" << allowXdelta
                    << "gameDir=" << gameDirPath
                    << "modsPath=" << modsPath
                    << "tempModPath=" << tempModPath;

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

    if (allowXdelta) {
      QDirIterator xdeltaIt(modPath, QStringList() << "*.xdelta", QDir::Files,
                            QDirIterator::Subdirectories);
      while (xdeltaIt.hasNext()) {
        input.xdeltaPatchFiles.push_back(xdeltaIt.next());
      }
    }

    if (!input.replacementExePath.isEmpty() || !input.xdeltaPatchFiles.isEmpty()) {
      patchInputs.push_back(input);
      lastPatchPriority = (std::max)(lastPatchPriority, modList->priority(modName));
      qInfo().noquote() << "[GameBattlespire] Patch input detected:"
                        << "mod=" << modName
                        << "replacementExe="
                        << (input.replacementExePath.isEmpty() ? "<none>"
                                                               : input.replacementExePath)
                        << "xdeltaCount=" << input.xdeltaPatchFiles.size();
      for (const QString& patchFile : input.xdeltaPatchFiles) {
        qInfo().noquote() << "[GameBattlespire]  - xdelta patch file:" << patchFile;
      }
    }
  }

  if (patchInputs.isEmpty()) {
    qInfo().noquote() << "[GameBattlespire] applyExePatchMods(): no eligible patch mods found.";
    return true;
  }

  removeDirRecursive(tempModPath);
  if (!modList->getMod(tempModName)) {
    MOBase::GuessedValue<QString> guessedName(tempModName);
    m_Organizer->createMod(guessedName);
  }
  if (!ensureDir(tempModPath)) {
    qWarning().noquote() << "[GameBattlespire] Failed to create temp mod path:" << tempModPath;
    return false;
  }
  qInfo().noquote() << "[GameBattlespire] Temp patch mod directory ready:" << tempModPath;

  const QString metaIniPath = QDir(tempModPath).filePath("meta.ini");
  QFile metaFile(metaIniPath);
  if (!metaFile.exists() && metaFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&metaFile);
    out << "[General]\n";
    out << "name=" << tempModName << "\n";
    out << "version=1.0\n";
    out << "author=Mod Organizer\n";
    out << "description=Temporary Battlespire executable patch output\n";
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
    qInfo().noquote() << "[GameBattlespire] Applying patch mod:" << input.modName;
    QString workingExePath = XngineExePatch::findFirstExistingFile(tempModPath, exeCandidates);
    if (workingExePath.isEmpty()) {
      workingExePath = XngineExePatch::findFirstExistingFile(gameDirPath, exeCandidates);
    }
    if (workingExePath.isEmpty()) {
      qWarning().noquote() << "[GameBattlespire] Could not find GAME.EXE for patching";
      success = false;
      continue;
    }

    QString relExePath = QDir(tempModPath).relativeFilePath(workingExePath);
    if (relExePath.startsWith("..")) {
      relExePath = QDir(gameDirPath).relativeFilePath(workingExePath);
    }
    const QString stagedExePath = QDir(tempModPath).filePath(relExePath);
    ensureDir(QFileInfo(stagedExePath).absolutePath());
    qInfo().noquote() << "[GameBattlespire]  workingExePath:" << workingExePath;
    qInfo().noquote() << "[GameBattlespire]  stagedExePath :" << stagedExePath;

    if (!input.replacementExePath.isEmpty()) {
      QFile::remove(stagedExePath);
      if (!QFile::copy(input.replacementExePath, stagedExePath)) {
        qWarning().noquote() << "[GameBattlespire] Failed staging replacement EXE from mod"
                             << input.modName;
        success = false;
        continue;
      }
      qInfo().noquote() << "[GameBattlespire]  staged replacement EXE from:"
                        << input.replacementExePath;
    } else if (!QFileInfo::exists(stagedExePath)) {
      QFile::copy(workingExePath, stagedExePath);
      qInfo().noquote() << "[GameBattlespire]  seeded staged EXE from game file.";
    }

    if (!input.xdeltaPatchFiles.isEmpty()) {
      QString configuredTool =
          m_Organizer->pluginSetting(name(), "xdelta_exe_path").toString().trimmed();
      if (configuredTool.isEmpty()) {
        configuredTool = readGlobalXdeltaPath();
      } else {
        writeGlobalXdeltaPath(configuredTool);
      }
      QString xdeltaTool =
          XngineExePatch::findXdeltaTool(input.modPath, gameDirPath, configuredTool);
      if (xdeltaTool.isEmpty()) {
        qWarning().noquote()
            << "[GameBattlespire] xdelta tool not found for mod" << input.modName
            << "- checked mod/game folders, MO2 folder/tools, XDELTA_EXE, and PATH.";
        success = false;
        continue;
      }
      qInfo().noquote() << "[GameBattlespire]  xdelta tool resolved to:" << xdeltaTool;
      for (const QString& patchFile : input.xdeltaPatchFiles) {
        qInfo().noquote() << "[GameBattlespire]  applying xdelta patch:" << patchFile;
        QString err;
        QString matchedRel;
        if (!XngineExePatch::applyXdeltaPatchToAnyFileInTree(
                xdeltaTool, patchFile, gameDirPath, tempModPath, &matchedRel, &err)) {
          qWarning().noquote() << "[GameBattlespire] Failed applying .xdelta patch" << patchFile
                               << "from mod" << input.modName << ":" << err;
          success = false;
          break;
        }
        qInfo().noquote() << "[GameBattlespire] Applied .xdelta patch to" << matchedRel
                          << "from mod" << input.modName;
      }
    }
  }

  qInfo().noquote() << "[GameBattlespire] applyExePatchMods() done. success=" << success
                    << "tempModPath=" << tempModPath;
  return success;
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
