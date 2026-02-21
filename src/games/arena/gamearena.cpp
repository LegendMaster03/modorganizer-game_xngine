#include "gamearena.h"

#include <executableinfo.h>
#include <pluginsetting.h>

#include <xnginelocalsavegames.h>
#include <xnginesavegame.h>
#include <xnginesavegameinfo.h>
#include <xngineunmanagedmods.h>

#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QDirIterator>
#include <QRegularExpression>

#include <Windows.h>

#include "utility.h"

#include <memory>

using namespace MOBase;

namespace {
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
  registerFeature(std::make_shared<XngineSaveGameInfo>(this));
  registerFeature(std::make_shared<XngineLocalSavegames>(this, iniForLocalSaves));
  registerFeature(std::make_shared<XngineUnmanagedMods>(this));

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
  return {};
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
  return 0;
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
  return {};
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
  return std::make_shared<XngineSaveGame>(filepath, this);
}

SaveLayout GameArena::saveLayout() const
{
  SaveLayout layout;
  layout.baseRelativePaths = {""};
  layout.slotEntriesAreFiles = true;
  layout.slotFileRegex = QRegularExpression("(?i)^SAVEENGN\\.(\\d{1,2})$");
  layout.slotWidthHint = 2;
  layout.maxSlotHint.reset();
  return layout;
}

QString GameArena::saveGameId() const
{
  return "arena";
}

QString GameArena::saveSlotPrefix() const
{
  return "SAVEGAME.";
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
