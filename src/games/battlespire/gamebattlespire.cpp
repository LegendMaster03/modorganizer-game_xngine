#include "gamebattlespire.h"

// Game-specific feature classes disabled - using XnGine base implementations instead
// #include "battlespiredatachecker.h"
// #include "battlespiremodatacontent.h"
// #include "battlespireafegame.h"
#include "battlespiresavegame.h"

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
#include <QProcessEnvironment>
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

namespace
{
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
    registerFeature(std::make_shared<XngineSaveGameInfo>(this));
    registerFeature(std::make_shared<XngineLocalSavegames>(this, iniForLocalSaves));
    registerFeature(std::make_shared<XngineUnmanagedMods>(this));

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
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] gameName() called";
  OutputDebugStringA("[GameBattlespire] gameName() called\n");
  return "Battlespire";
}

QString GameBattlespire::displayGameName() const
{
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] displayGameName() called";
  OutputDebugStringA("[GameBattlespire] displayGameName() called\n");
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
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] steamAPPId() called";
  OutputDebugStringA("[GameBattlespire] steamAPPId() called\n");
  return "1812420";
}

QString GameBattlespire::gogAPPId() const
{
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] gogAPPId() called";
  OutputDebugStringA("[GameBattlespire] gogAPPId() called\n");
  return "1435829464";
}

QString GameBattlespire::binaryName() const
{
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] binaryName() called";
  OutputDebugStringA("[GameBattlespire] binaryName() called\n");
  return "GAME.EXE";
}

QString GameBattlespire::gameShortName() const
{
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] gameShortName() called";
  OutputDebugStringA("[GameBattlespire] gameShortName() called\n");
  return "Battlespire";
}

QString GameBattlespire::gameNexusName() const
{
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] gameNexusName() called";
  OutputDebugStringA("[GameBattlespire] gameNexusName() called\n");
  return "anelderscrollslegendbattlespire";
}

QStringList GameBattlespire::validShortNames() const
{
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] validShortNames() called";
  OutputDebugStringA("[GameBattlespire] validShortNames() called\n");
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
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] nexusModOrganizerID() called";
  OutputDebugStringA("[GameBattlespire] nexusModOrganizerID() called\n");
  return 0;  // To be determined
}

int GameBattlespire::nexusGameID() const
{
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] nexusGameID() called";
  OutputDebugStringA("[GameBattlespire] nexusGameID() called\n");
  return 3495;  // Nexus "games.json" id for An Elder Scrolls Legend: Battlespire
}

bool GameBattlespire::looksValid(QDir const& path) const
{
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] looksValid() called";
  OutputDebugStringA("[GameBattlespire] looksValid() called\n");
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
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] localizedName() called";
  OutputDebugStringA("[GameBattlespire] localizedName() called\n");
  return tr("An Elder Scrolls Legend: Battlespire Support Plugin");
}

QString GameBattlespire::author() const
{
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] author() called";
  OutputDebugStringA("[GameBattlespire] author() called\n");
  return "Legend_Master";
}

QString GameBattlespire::description() const
{
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] description() called";
  OutputDebugStringA("[GameBattlespire] description() called\n");
  return tr("Adds support for the game An Elder Scrolls Legend: Battlespire");
}

MOBase::VersionInfo GameBattlespire::version() const
{
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] version() called";
  OutputDebugStringA("[GameBattlespire] version() called\n");
  return VersionInfo(1, 0, 0, VersionInfo::RELEASE_FINAL);
}

QList<PluginSetting> GameBattlespire::settings() const
{
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] settings() called";
  OutputDebugStringA("[GameBattlespire] settings() called\n");
  return QList<PluginSetting>();
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
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] savegameExtension() called (disabled for isolation)";
  OutputDebugStringA("[GameBattlespire] savegameExtension() called\n");
  return {};
}

QString GameBattlespire::savegameSEExtension() const
{
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] savegameSEExtension() called (disabled for isolation)";
  OutputDebugStringA("[GameBattlespire] savegameSEExtension() called\n");
  return {};
}

std::shared_ptr<const XngineSaveGame> GameBattlespire::makeSaveGame(QString filepath) const
{
  if (shouldLogForCurrentProfile()) qInfo().noquote() << "[GameBattlespire] makeSaveGame() called";
  OutputDebugStringA("[GameBattlespire] makeSaveGame() called\n");
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
