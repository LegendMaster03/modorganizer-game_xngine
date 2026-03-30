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

bool GameRedguard::init(IOrganizer* moInfo)
{
  try {
    if (!GameXngine::init(moInfo)) {
      qWarning().noquote() << "Redguard: GameXngine::init() failed";
      return false;
    }

    const QString iniForLocalSaves = iniFiles().isEmpty() ? QString{} : iniFiles().first();
    registerFeature(std::make_shared<XngineSaveGameInfo>(this));
    registerFeature(std::make_shared<XngineLocalSavegames>(this, iniForLocalSaves));
    registerFeature(std::make_shared<XngineUnmanagedMods>(this));

    try {
      auto checker = std::make_shared<RedguardsModDataChecker>(this);
      registerFeature(checker);
    } catch (const std::exception&) {
      qWarning().noquote() << "Redguard: RedguardsModDataChecker setup failed, continuing";
    } catch (...) {
      qWarning().noquote() << "Redguard: RedguardsModDataChecker setup failed, continuing";
    }

    registerFeature(std::make_shared<RedguardsModDataContent>(m_Organizer->gameFeatures()));

    return true;
  } catch (const std::exception& e) {
    qWarning().noquote() << "Redguard: init() failed:" << e.what();
    return false;
  } catch (...) {
    qWarning().noquote() << "Redguard: init() failed";
    return false;
  }
}

QString GameRedguard::gameName() const
{
  return "Redguard";
}

QString GameRedguard::displayGameName() const
{
  return "The Elder Scrolls Adventures: Redguard";
}

QList<ExecutableInfo> GameRedguard::executables() const
{
  QList<ExecutableInfo> executables;
  QDir gameDir = gameDirectory();
  if (gameDir.path().isEmpty() || !gameDir.exists()) {
    return executables;
  }
  
  // Steam DOSBox launcher
  QFileInfo steamDosbox(gameDir.filePath("DOSBox-0.73/dosbox.exe"));
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

  return executables;
}

QString GameRedguard::steamAPPId() const
{
  return "1812410";
}

QString GameRedguard::gogAPPId() const
{
  return "1435829617";
}

QString GameRedguard::binaryName() const
{
  return "REDGUARD.EXE";
}

QString GameRedguard::gameShortName() const
{
  return "Redguard";
}

QString GameRedguard::gameNexusName() const
{
  return "theelderscrollsadventuresredguard";
}

QStringList GameRedguard::validShortNames() const
{
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
  return 6220;  // Nexus MO Organizer ID for Redguard
}

int GameRedguard::nexusGameID() const
{
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
  return tr("The Elder Scrolls Adventures: Redguard Support Plugin");
}

QString GameRedguard::author() const
{
  return "Legend_Master";
}

QString GameRedguard::description() const
{
  return tr("Adds support for the game The Elder Scrolls Adventures: Redguard");
}

VersionInfo GameRedguard::version() const
{
  return VersionInfo(1, 0, 0, VersionInfo::RELEASE_FINAL);
}

QList<PluginSetting> GameRedguard::settings() const
{
  return {
      PluginSetting(
          "show_developer_save_details",
          tr("Show internal Redguard save debug details (raw area tokens and parse evidence) in save info."),
          false),
      PluginSetting(
          "allow_dillon241_patch_mod_install",
          tr("Allow Dillon241 patch mods (About.txt / INI Changes.txt / Map Changes.txt / RTX Changes.txt)."),
          true),
      PluginSetting(
          "show_full_svit_inventory",
          tr("Show the full Redguard SAVEGAME.SAV SVIT inventory table in save info (developer/debug view)."),
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

bool GameRedguard::showDeveloperSaveDetails() const
{
  if (m_Organizer == nullptr) {
    return false;
  }
  return m_Organizer->pluginSetting(name(), "show_developer_save_details").toBool();
}

bool GameRedguard::showFullSvitInventory() const
{
  if (m_Organizer == nullptr) {
    return false;
  }
  return m_Organizer->pluginSetting(name(), "show_full_svit_inventory").toBool();
}

QString GameRedguard::identifyGamePath() const
{
  try {
    const QString steamPath = findInRegistry(
        HKEY_LOCAL_MACHINE,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 1812410",
        L"InstallLocation");
    if (!steamPath.isEmpty()) {
      if (QDir(steamPath + "/DOSBox-0.73").exists() &&
          QFile::exists(steamPath + "/DOSBox-0.73/dosbox.exe") &&
          QFile::exists(steamPath + "/Redguard/REDGUARD.EXE")) {
        return steamPath;
      }
    }

    const QString gogPath = findInRegistry(HKEY_LOCAL_MACHINE,
                                           L"Software\\GOG.com\\Games\\1435829617",
                                           L"path");
    if (!gogPath.isEmpty()) {
      if (QDir(gogPath + "/DOSBOX").exists() &&
          QFile::exists(gogPath + "/DOSBOX/dosbox.exe") &&
          QFile::exists(gogPath + "/Redguard/REDGUARD.EXE")) {
        return gogPath;
      }
    }

    return {};
  } catch (const std::exception&) {
    return {};
  } catch (...) {
    return {};
  }
}

QString GameRedguard::findInRegistry(HKEY baseKey, LPCWSTR path, LPCWSTR value) const
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

bool GameRedguard::looksValid(QDir const& path) const
{
  return (QDir(path.absolutePath() + "/DOSBox-0.73").exists() &&
          QFile::exists(path.absolutePath() + "/Redguard/REDGUARD.EXE")) ||
         (QDir(path.absolutePath() + "/DOSBOX").exists() &&
          QFile::exists(path.absolutePath() + "/Redguard/REDGUARD.EXE"));
}

QDir GameRedguard::dataDirectory() const
{
  QDir gameDir = gameDirectory();
  if (gameDir.path().isEmpty() || !gameDir.exists()) {
    return QDir();
  }
  return gameDir.absoluteFilePath("Redguard");
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
  return "sav";
}

QString GameRedguard::savegameSEExtension() const
{
  return "sav";
}

std::shared_ptr<const XngineSaveGame> GameRedguard::makeSaveGame(QString filepath) const
{
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
  if (!GameXngine::prepareIni(exec)) {
    qWarning().noquote() << "Redguard: GameXngine::prepareIni() failed";
    return false;
  }

  if (!applyPatchMods()) {
    qWarning().noquote() << "Redguard: patch mod application failed, continuing launch";
  }

  return true;
}

bool GameRedguard::applyPatchMods()
{
  if (!m_Organizer) {
    qWarning().noquote() << "Redguard: organizer is null";
    return false;
  }

  const bool allowDillon241 = allowDillon241PatchInstall();
  const bool allowExeMods = allowExeModInstall();
  const bool success =
      applyRedguardPatchMods(m_Organizer, name(), profilePath(),
                             gameDirectory().absolutePath(), allowDillon241, allowExeMods);

  return success;
}
