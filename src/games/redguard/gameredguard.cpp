#include "gameredguard.h"

#include "redguardsmoddatachecker.h"
#include "redguardsmoddatacontent.h"
#include "redguardsavegame.h"
#include "redguardspatchruntime.h"

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
#include <QByteArray>
#include <QFile>
#include <QRegularExpression>
#include <QIcon>
#include <QDir>

#include <Windows.h>
#include <winver.h>

#include "utility.h"

#include <exception>
#include <memory>
#include <stdexcept>

using namespace MOBase;

namespace {
QString firstExistingFile(const QDir& root, const QStringList& candidates)
{
  for (const auto& relPath : candidates) {
    const QString fullPath = root.filePath(relPath);
    if (QFileInfo::exists(fullPath)) {
      return fullPath;
    }
  }
  return {};
}

QString detectRedguardVersionFromExeMarker(const QString& exePath)
{
  QFile f(exePath);
  if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
    return {};
  }

  const QByteArray data = f.readAll();
  static const QByteArray marker("SOFTWARE\\Bethesda\\Redguard\\");
  int index = data.indexOf(marker);
  while (index >= 0) {
    const int start = index + marker.size();
    int end = start;
    while (end < data.size()) {
      const char c = data[end];
      if ((c >= '0' && c <= '9') || c == '.') {
        ++end;
      } else {
        break;
      }
    }

    const QByteArray rawVersion = data.mid(start, end - start);
    const QString version = QString::fromLatin1(rawVersion).trimmed();
    static const QRegularExpression versionPattern(R"(^\d+\.\d+\.\d+$)");
    if (versionPattern.match(version).hasMatch()) {
      return version;
    }

    index = data.indexOf(marker, index + 1);
  }

  return {};
}

QString detectRedguardVersionFromText(const QDir& root, const QStringList& candidates)
{
  static const QList<QRegularExpression> patterns = {
      QRegularExpression(R"(\bVersion\s*=\s*([0-9]+(?:\.[0-9]+){1,3})\b)",
                         QRegularExpression::CaseInsensitiveOption),
      QRegularExpression(R"(\bVersion\s+([0-9]+(?:\.[0-9]+){1,3})\b)",
                         QRegularExpression::CaseInsensitiveOption),
  };

  for (const auto& relPath : candidates) {
    QFile f(root.filePath(relPath));
    if (!f.exists() || !f.open(QIODevice::ReadOnly | QIODevice::Text)) {
      continue;
    }

    int linesRead = 0;
    while (!f.atEnd() && linesRead < 128) {
      const QString line = QString::fromLocal8Bit(f.readLine()).trimmed();
      ++linesRead;
      for (const auto& pattern : patterns) {
        const QRegularExpressionMatch match = pattern.match(line);
        if (match.hasMatch()) {
          return match.captured(1);
        }
      }
    }
  }

  return {};
}

}  // namespace

GameRedguard::GameRedguard() {
  qInfo().noquote() << "[GameRedguard] Constructor ENTRY";
  OutputDebugStringA("[GameRedguard] Constructor ENTRY\n");
  OutputDebugStringA("[GameRedguard] Constructor EXIT\n");
  qInfo().noquote() << "[GameRedguard] Constructor EXIT";
}

bool GameRedguard::init(IOrganizer* moInfo)
{
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

    // Register save-related features even if optional checker setup fails.
    const QString iniForLocalSaves = iniFiles().isEmpty() ? QString{} : iniFiles().first();
    registerFeature(std::make_shared<XngineSaveGameInfo>(this));
    registerFeature(std::make_shared<XngineLocalSavegames>(this, iniForLocalSaves));
    registerFeature(std::make_shared<XngineUnmanagedMods>(this));

    // Optional checker feature: don't fail plugin init if this breaks.
    OutputDebugStringA("[GameRedguard] About to create RedguardsModDataChecker\n");
    try {
      auto checker = std::make_shared<RedguardsModDataChecker>(this);
      OutputDebugStringA("[GameRedguard] RedguardsModDataChecker created successfully\n");
      OutputDebugStringA("[GameRedguard] About to register RedguardsModDataChecker\n");
      qInfo().noquote() << "[GameRedguard] Registering RedguardsModDataChecker";
      registerFeature(checker);
      OutputDebugStringA("[GameRedguard] RedguardsModDataChecker registered successfully\n");
      qInfo().noquote() << "[GameRedguard] RedguardsModDataChecker registered successfully";
    } catch (const std::exception&) {
      OutputDebugStringA("[GameRedguard] EXCEPTION creating/registering RedguardsModDataChecker (continuing)\n");
      qWarning().noquote() << "[GameRedguard] RedguardsModDataChecker setup failed, continuing";
    } catch (...) {
      OutputDebugStringA("[GameRedguard] UNKNOWN EXCEPTION creating/registering RedguardsModDataChecker (continuing)\n");
      qWarning().noquote() << "[GameRedguard] RedguardsModDataChecker setup failed (unknown), continuing";
    }

    registerFeature(std::make_shared<RedguardsModDataContent>(m_Organizer->gameFeatures()));

    OutputDebugStringA("[GameRedguard] init() EXIT SUCCESS\n");
    qInfo().noquote() << "[GameRedguard] init() EXIT SUCCESS";
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
}

QString GameRedguard::gameName() const
{
  OutputDebugStringA("[GameRedguard] gameName() called\n");
  return "Redguard";
}

QString GameRedguard::displayGameName() const
{
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
  QFileInfo steamConfig(gameDir.filePath("DOSBox-0.73/rg.conf"));
  if (steamDosbox.exists()) {
    executables << ExecutableInfo("Redguard (Steam DOSBox Windowed)", steamDosbox)
                   .withArgument("dosbox.exe -noconsole -conf rg.conf");
    executables << ExecutableInfo("Redguard (Steam DOSBox Fullscreen)", steamDosbox)
                   .withArgument("-noconsole -conf rg.conf -fullscreen");
  }

  // GOG DOSBox launcher
  QFileInfo gogDosbox(gameDir.filePath("DOSBOX/dosbox.exe"));
  if (gogDosbox.exists()) {
    executables << ExecutableInfo("Redguard (GOG DOSBox)", gogDosbox)
                   .withArgument(R"(-conf "..\dosbox_redguard.conf" -conf "..\dosbox_redguard_single.conf" -noconsole -c exit)");
  }

  // Standalone executable if it exists
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
  OutputDebugStringA("[GameRedguard] gogAPPId() called\n");
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

QStringList GameRedguard::iniFiles() const
{
  const QStringList candidates = {
      "COMBAT.INI",
      "Redguard/COMBAT.INI",
      "ITEM.INI",
      "Redguard/ITEM.INI",
      "KEYS.INI",
      "Redguard/KEYS.INI",
      "MENU.INI",
      "Redguard/MENU.INI",
      "REGISTRY.INI",
      "Redguard/REGISTRY.INI",
      "surface.ini",
      "Redguard/surface.ini",
      "SYSTEM.INI",
      "Redguard/SYSTEM.INI",
      "WORLD.INI",
      "Redguard/WORLD.INI",
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

QIcon GameRedguard::gameIcon() const
{
  const QString exePath = gameDirectory().absoluteFilePath("Redguard/REDGUARD.EXE");
  QIcon icon = MOBase::iconForExecutable(exePath);
  return icon.isNull() ? GameXngine::gameIcon() : icon;
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

QString GameRedguard::gameVersion() const
{
  static const QString kFallbackGameVersion = QStringLiteral("1.0.0");
  const QDir root = gameDirectory();

  // Prefer PE metadata for accuracy across releases/editions.
  const QString metadataVersion = detectGameVersion(
      {
          "Redguard/REDGUARD.EXE",
          "REDGUARD.EXE",
      },
      {
          "README.TXT",
          "Redguard/README.TXT",
          "PATCH.TXT",
          "Redguard/PATCH.TXT",
      },
      {
          QRegularExpression(R"(\bv([0-9]+(?:\.[0-9]+){1,3})\b)",
                             QRegularExpression::CaseInsensitiveOption),
          QRegularExpression(R"(\bversion\s+([0-9]+(?:\.[0-9]+){1,3})\b)",
                             QRegularExpression::CaseInsensitiveOption),
      });
  if (!metadataVersion.isEmpty() && metadataVersion != kFallbackGameVersion) {
    return metadataVersion;
  }

  // Fallback to any explicit .VER/text hint if present.
  const QString textVersion = detectRedguardVersionFromText(
      root,
      {"REDGUARD.VER", "Redguard/REDGUARD.VER", "README.TXT", "Redguard/README.TXT",
       "PATCH.TXT", "Redguard/PATCH.TXT"});
  if (!textVersion.isEmpty()) {
    return textVersion;
  }

  // Last-resort fallback: executable embedded engine marker.
  const QString redguardExe = firstExistingFile(
      root, {"Redguard/REDGUARD.EXE", "REDGUARD.EXE"});
  if (!redguardExe.isEmpty()) {
    const QString exeMarkerVersion = detectRedguardVersionFromExeMarker(redguardExe);
    if (!exeMarkerVersion.isEmpty()) {
      return exeMarkerVersion;
    }
  }

  return metadataVersion;
}

QString GameRedguard::name() const
{
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
  return {
      PluginSetting(
          "allow_dillon241_patch_mod_install",
          tr("Allow Dillon241 patch mods (About.txt / INI Changes.txt / Map Changes.txt / RTX Changes.txt)."),
          true),
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

bool GameRedguard::allowExeModInstall() const
{
  return canApplyRedguardExePatchMods(m_Organizer, name(), gameDirectory().absolutePath());
}

bool GameRedguard::allowDillon241PatchInstall() const
{
  if (m_Organizer == nullptr) {
    return false;
  }
  return m_Organizer->pluginSetting(name(), "allow_dillon241_patch_mod_install").toBool();
}

QString GameRedguard::identifyGamePath() const
{
  qInfo().noquote() << "[GameRedguard] identifyGamePath() ENTRY";
  OutputDebugStringA("[GameRedguard] identifyGamePath() ENTRY\n");
  try {
  // Try Steam first (using Steam App ID 1812410)
  QString steamPath = findInRegistry(HKEY_LOCAL_MACHINE,
                                     L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 1812410",
                                     L"InstallLocation");
  if (!steamPath.isEmpty()) {
    // Verify it has the Steam DOSBox structure
    if (QDir(steamPath + "/DOSBox-0.73").exists() &&
        QFile::exists(steamPath + "/DOSBox-0.73/dosbox.exe") &&
        QFile::exists(steamPath + "/Redguard/REDGUARD.EXE")) {
      qInfo().noquote() << "[GameRedguard] Steam path verified";
      OutputDebugStringA("[GameRedguard] Steam path verified\n");
      return steamPath;
    }
  }

  // Try GOG registry (using GOG Game ID 1435829617)
  QString gogPath = findInRegistry(HKEY_LOCAL_MACHINE,
                                   L"Software\\GOG.com\\Games\\1435829617",
                                   L"path");
  if (!gogPath.isEmpty()) {
    // Verify it has the GOG DOSBox structure
    if (QDir(gogPath + "/DOSBOX").exists() &&
        QFile::exists(gogPath + "/DOSBOX/dosbox.exe") &&
        QFile::exists(gogPath + "/Redguard/REDGUARD.EXE")) {
      qInfo().noquote() << "[GameRedguard] GOG registry path verified";
      OutputDebugStringA("[GameRedguard] GOG registry path verified\n");
      return gogPath;
    }
  }

  qWarning().noquote() << "[GameRedguard] identifyGamePath() EXIT (not found)";
  OutputDebugStringA("[GameRedguard] identifyGamePath() EXIT (not found)\n");
  return {};
  } catch (const std::exception&) {
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

bool GameRedguard::looksValid(QDir const& path) const
{
  qInfo().noquote() << "[GameRedguard] looksValid() called";
  OutputDebugStringA("[GameRedguard] looksValid() called\n");
  
  // Redguard has a unique structure - the executable is in a Redguard subdirectory
  // Check for either Steam structure (DOSBox-0.73) or GOG structure (DOSBOX)
  bool valid = (QDir(path.absolutePath() + "/DOSBox-0.73").exists() &&
                QFile::exists(path.absolutePath() + "/Redguard/REDGUARD.EXE")) ||
               (QDir(path.absolutePath() + "/DOSBOX").exists() &&
                QFile::exists(path.absolutePath() + "/Redguard/REDGUARD.EXE"));
  
  return valid;
}

QDir GameRedguard::dataDirectory() const
{
  qInfo().noquote() << "[GameRedguard] dataDirectory() ENTRY";
  OutputDebugStringA("[GameRedguard] dataDirectory() ENTRY\n");
  QDir gameDir = gameDirectory();
  if (gameDir.path().isEmpty() || !gameDir.exists()) {
    qWarning().noquote() << "[GameRedguard] dataDirectory() - game directory invalid:" << gameDir.absolutePath();
    OutputDebugStringA("[GameRedguard] dataDirectory() - game directory invalid\n");
    return QDir();
  }
  QDir redguardDir = gameDir.absoluteFilePath("Redguard");
  qInfo().noquote() << "[GameRedguard] dataDirectory() using Redguard subdirectory:" << redguardDir.absolutePath();
  OutputDebugStringA(("[GameRedguard] dataDirectory() path='" + redguardDir.absolutePath().toStdString() + "'\n").c_str());
  return redguardDir;
}

QDir GameRedguard::savesDirectory() const
{
  return GameXngine::savesDirectory();
}

MappingType GameRedguard::mappings() const
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
  const QString sourceRoot = paths.gameSavesRoot;

  // Redguard writes save slots under SAVEGAME.* folders and some installs
  // place them in the Redguard subdirectory. Map the whole save root there too.
  out.push_back({sourceRoot, gameDir.absoluteFilePath("Redguard"), true, true});
  out.push_back({QDir(sourceRoot).filePath("SAVEGAME"),
                 gameDir.absoluteFilePath("Redguard/SAVEGAME"),
                 true,
                 true});

  // Runtime diagnostic/save logs that should travel with save output and not
  // spill into overwrite as unmanaged files.
  const QStringList logFiles = {
      QStringLiteral("BITMAP.LOG"),
      QStringLiteral("GENERAL.LOG"),
      QStringLiteral("PATH.LOG"),
      QStringLiteral("SAVEFILE.LOG"),
  };
  for (const auto& logName : logFiles) {
    out.push_back({QDir(sourceRoot).filePath(logName),
                   gameDir.absoluteFilePath(logName),
                   false,
                   false});
    out.push_back({QDir(sourceRoot).filePath(logName),
                   gameDir.absoluteFilePath(QStringLiteral("Redguard/") + logName),
                   false,
                   false});
  }

  return out;
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
  return std::make_shared<RedguardsSaveGame>(filepath, this);
}

SaveLayout GameRedguard::saveLayout() const
{
  SaveLayout layout;
  layout.baseRelativePaths = {"", "SAVEGAME"};
  layout.slotDirRegex = QRegularExpression("(?i)^SAVEGAME\\.(\\d+)$");
  layout.slotWidthHint = 3;
  layout.validator = [](const QDir& slotDir) {
    return slotDir.exists() && QFileInfo::exists(slotDir.filePath("SAVEGAME.SAV"));
  };
  return layout;
}

QString GameRedguard::saveGameId() const
{
  return "redguard";
}

bool GameRedguard::prepareIni(const QString& exec)
{
  qInfo().noquote() << "[GameRedguard] prepareIni() ENTRY";
  OutputDebugStringA("[GameRedguard] prepareIni() ENTRY\n");
  
  // First call parent implementation
  if (!GameXngine::prepareIni(exec)) {
    qWarning().noquote() << "[GameRedguard] GameXngine::prepareIni() FAILED";
    return false;
  }
  
  // Apply patch-based mods.
  if (!applyPatchMods()) {
    qWarning().noquote() << "[GameRedguard] applyPatchMods() FAILED";
    // Don't fail launch - file replacement mods should still work
    OutputDebugStringA("[GameRedguard] WARNING: Patch mod application failed but continuing launch\n");
  }
  
  qInfo().noquote() << "[GameRedguard] prepareIni() EXIT SUCCESS";
  OutputDebugStringA("[GameRedguard] prepareIni() EXIT\n");
  return true;
}

bool GameRedguard::applyPatchMods()
{
  qInfo().noquote() << "[GameRedguard] applyPatchMods() ENTRY";
  OutputDebugStringA("[GameRedguard] applyPatchMods() ENTRY\n");
  
  if (!m_Organizer) {
    qWarning().noquote() << "[GameRedguard] m_Organizer is NULL";
    return false;
  }

  const bool allowDillon241 = allowDillon241PatchInstall();
  const bool allowExeMods = allowExeModInstall();
  const bool success =
      applyRedguardPatchMods(m_Organizer, name(), profilePath(),
                             gameDirectory().absolutePath(), allowDillon241, allowExeMods);

  qInfo().noquote() << "[GameRedguard] applyPatchMods() EXIT";
  return success;
}
