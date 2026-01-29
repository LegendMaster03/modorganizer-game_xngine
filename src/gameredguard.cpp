#include "gameRedguard.h"

#include "redguardsavegame.h"
#include "RGMODFrameworkWrapper.h"
#include "RedguardDataChecker.h"
#include <QDebug>
#include <QtGlobal>
#include <QStandardPaths>
#include <QFile>
#include <QFileIconProvider>
#include <QSettings>
#include <versioninfo.h>
#include <pluginsetting.h>
#include <isavegame.h>
#include <iplugingamefeatures.h>
#include <cstdio>
#include <ctime>

using namespace std;

// GameRedguard implementation
GameRedguard::GameRedguard() { qDebug() << "GameRedguard plugin initialized"; }

MOBase::VersionInfo GameRedguard::version() const
{
  // Minimal version info; adjust as needed
  return MOBase::VersionInfo(1, 0, 0, MOBase::VersionInfo::RELEASE_FINAL);
}

bool GameRedguard::init(MOBase::IOrganizer* organizer)
{
  qDebug() << "GameRedguard::init() called";
  mOrganizer = organizer;
  
  // Try to detect the game on initialization
  if (m_GamePath.isEmpty()) {
    detectGame();
  }
  
  qDebug() << "Game Redguard plugin initialized";
  
  // Register hook to call applyChanges before launch
  organizer->onAboutToRun([this, organizer](const QString& executable) {
    qDebug() << "onAboutToRun hook called for:" << executable;
    
    // Detect if we're launching Steam DOSBox
    bool isSteamLaunch = executable.contains("DOSBox-0.73", Qt::CaseInsensitive);
    
    if (isSteamLaunch) {
      qWarning() << "========================================";
      qWarning() << "Steam DOSBox detected!";
      qWarning() << "If the game crashes, this is a known issue with MO2's VFS and Steam DOSBox.";
      qWarning() << "Workaround: Use GOG version for Type 1 mods, or use Type 2 mods only for Steam.";
      qWarning() << "========================================";
      // Continue anyway - let the user try
    }
    
    try {
      // Initialize mod framework if needed
      if (!mRGmodFrameworkWrapper && !m_GamePath.isEmpty()) {
        // Use the MO2 modsPath() method which returns the global mods folder
        QString modsPath = organizer->modsPath();
        qDebug() << "Creating mod framework with mods path:" << modsPath;
        mRGmodFrameworkWrapper = std::make_unique<RGMODFrameworkWrapper>(m_GamePath, modsPath);
        mRGmodFrameworkWrapper->setOrganizer(mOrganizer);
      }
      
      // Get active mods and apply them
      if (mRGmodFrameworkWrapper) {
        QList<QString> activeMods;
        auto* modList = mOrganizer->modList();
        if (modList) {
          QStringList allMods = modList->allModsByProfilePriority();
          for (const QString& modName : allMods) {
            if (modList->state(modName) & MOBase::IModList::STATE_ACTIVE) {
              activeMods.append(modName);
            }
          }
        }
        
        if (!activeMods.isEmpty()) {
          qDebug() << "Applying" << activeMods.size() << "active mods";
          mRGmodFrameworkWrapper->applyChanges(activeMods);
        }
      }
    } catch (const std::exception& e) {
      qCritical() << "Exception in launch hook:" << e.what();
    } catch (...) {
      qCritical() << "Unknown exception in launch hook";
    }
    
    return true;
  });
  
  return true;
}

void GameRedguard::registerFeature(std::shared_ptr<MOBase::GameFeature> feature)
{
  if (!mOrganizer) {
    qWarning() << "Cannot register feature: IOrganizer is null";
    return;
  }
  
  auto gameFeatures = mOrganizer->gameFeatures();
  if (!gameFeatures) {
    qWarning() << "Cannot register feature: gameFeatures() returned null";
    return;
  }
  
  // NOTE: Feature registration is IMPLEMENTED but currently DISABLED
  // The IGameFeatures::registerFeature() method has a forward-declaration issue
  // that prevents us from calling it directly without circular includes.
  // 
  // The RedguardDataChecker is fully functional and ready to be registered.
  // This will work once we properly integrate with MO2's plugin system.
  //
  // For now, the feature is available in the DLL but not active in MO2.
  
  qDebug() << "RedguardDataChecker created but registration disabled due to API integration issues";
}

QList<MOBase::PluginSetting> GameRedguard::settings() const
{
  return {};
}



QString GameRedguard::gameName() const
{
  // Display-friendly full title for MO2 UI
  return "The Elder Scrolls Adventures: Redguard";
}

QString GameRedguard::name() const
{
  return "The Elder Scrolls Adventures: Redguard";
}

QString GameRedguard::author() const
{
  return "Legend_Master";
}

QString GameRedguard::description() const
{
  return "Adds support for the game Redguard";
}

QString GameRedguard::gameShortName() const
{
  return "redguard";
}

QString GameRedguard::steamAPPId() const
{
  return "1812410";
}

QString GameRedguard::gameNexusName() const
{
  return "theelderscrollsadventuresredguard";
}

std::vector<std::shared_ptr<const MOBase::ISaveGame>> GameRedguard::listSaves(QDir saveDir) const
{
  std::vector<std::shared_ptr<const MOBase::ISaveGame>> saves;
  
  // Redguard saves are folders named SAVEGAME.XXX containing SAVEGAME.SAV
  QStringList saveFilters;
  saveFilters << "SAVEGAME.*";
  
  saveDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
  saveDir.setNameFilters(saveFilters);
  
  QFileInfoList saveFolders = saveDir.entryInfoList();
  
  for (const QFileInfo& folderInfo : saveFolders) {
    QString saveFolder = folderInfo.absoluteFilePath();
    QString saveFile = saveFolder + "/SAVEGAME.SAV";
    
    // Verify this folder contains SAVEGAME.SAV
    if (QFile::exists(saveFile)) {
      saves.push_back(std::make_shared<RedguardSaveGame>(saveFolder, this));
    }
  }
  
  return saves;
}

QStringList GameRedguard::primarySources() const
{
  QStringList sources;
  
  // Add game manual if it exists
  QString manualPath = m_GamePath + "/Manual.pdf";
  if (QFile::exists(manualPath)) {
    sources.append(manualPath);
    qDebug() << "Found game manual:" << manualPath;
  }
  
  // Add comic if it exists
  QString comicPath = m_GamePath + "/Redguard_Comic.pdf";
  if (QFile::exists(comicPath)) {
    sources.append(comicPath);
    qDebug() << "Found game comic:" << comicPath;
  }
  
  return sources;
}

void GameRedguard::detectGame()
{
  // Try to detect Redguard installation (including Steam's DOSBox version)
  qDebug() << "Starting game detection...";
  
  // List of possible installation paths
  QStringList searchPaths = {
    "F:/SteamLibrary/steamapps/common/The Elder Scrolls Adventures Redguard",
    "F:\\SteamLibrary\\steamapps\\common\\The Elder Scrolls Adventures Redguard",
    "C:/Program Files/Steam/steamapps/common/The Elder Scrolls Adventures Redguard",
    "C:/Program Files (x86)/Steam/steamapps/common/The Elder Scrolls Adventures Redguard",
    "C:/Program Files/The Elder Scrolls Adventures - Redguard",
    "C:/Program Files (x86)/The Elder Scrolls Adventures - Redguard",
    "D:/SteamLibrary/steamapps/common/The Elder Scrolls Adventures Redguard",
    "D:/Games/The Elder Scrolls Adventures - Redguard",
    "C:/GOG Games/The Elder Scrolls Adventures - Redguard",
    "C:/Program Files (x86)/GOG Galaxy/Games/Redguard",
    "C:/Program Files/GOG Galaxy/Games/Redguard"
  };

  // Try each path
  for (const QString& path : searchPaths) {
    qDebug() << "Checking path:" << path;
    QDir dir(path);
    
    if (dir.exists()) {
      qDebug() << "  Directory exists";
      
      // Check for Steam DOSBox structure (DOSBox-0.73)
      QDir steamDosboxDir(dir.filePath("DOSBox-0.73"));
      if (steamDosboxDir.exists() && QFile::exists(steamDosboxDir.filePath("dosbox.exe"))) {
        qDebug() << "  Found DOSBox-0.73/dosbox.exe";

        QDir rgDir(dir.filePath("Redguard"));
        if (rgDir.exists() && QFile::exists(rgDir.filePath("REDGUARD.EXE"))) {
          m_GamePath = dir.absolutePath();
          qDebug() << "SUCCESS: Detected Steam DOSBox Redguard at:" << m_GamePath;
          return;
        }
      }

      // Check for GOG DOSBox structure (DOSBOX + dosbox_redguard*.conf)
      QDir gogDosboxDir(dir.filePath("DOSBOX"));
      if (gogDosboxDir.exists() && QFile::exists(gogDosboxDir.filePath("dosbox.exe"))) {
        const bool hasPrimaryConf = QFile::exists(dir.filePath("dosbox_redguard.conf"));
        const bool hasSingleConf = QFile::exists(dir.filePath("dosbox_redguard_single.conf"));
        QDir rgDir(dir.filePath("Redguard"));
        const bool hasRgfx = QFile::exists(rgDir.filePath("RGFX.EXE")) || QFile::exists(rgDir.filePath("REDGUARD.EXE"));

        if (hasPrimaryConf && hasSingleConf && rgDir.exists() && hasRgfx) {
          m_GamePath = dir.absolutePath();
          qDebug() << "SUCCESS: Detected GOG DOSBox Redguard at:" << m_GamePath;
          return;
        }
      }

      // Traditional layouts
      if (QFile::exists(dir.filePath("Redguard/REDGUARD.EXE")) || QFile::exists(dir.filePath("Redguard.exe"))) {
        m_GamePath = dir.absolutePath();
        qDebug() << "SUCCESS: Found Redguard executable at:" << m_GamePath;
        return;
      }

      // Steam DOSBox config-only fallback
      if (QFile::exists(steamDosboxDir.filePath("rg.conf"))) {
        m_GamePath = dir.absolutePath();
        qDebug() << "SUCCESS: Found rg.conf at:" << m_GamePath;
        return;
      }
    } else {
      qDebug() << "  Directory does not exist";
    }
  }
  
  qDebug() << "Redguard not found in standard locations";
}

void GameRedguard::initializeProfile(const QDir& path, MOBase::IPluginGame::ProfileSettings settings) const
{
  Q_UNUSED(path);
  Q_UNUSED(settings);
}

QIcon GameRedguard::gameIcon() const
{
  if (m_GamePath.isEmpty()) {
    return QIcon();
  }
  
  // Try to extract icon from game executables in order of preference
  QStringList exePaths = {
    m_GamePath + "/Redguard.exe",                    // Standalone install
    m_GamePath + "/Redguard/REDGUARD.EXE",           // Subdirectory install
    m_GamePath + "/DOSBox-0.73/dosbox.exe"           // Steam DOSBox version
  };
  
  for (const QString& exePath : exePaths) {
    QFileInfo exeFile(exePath);
    if (exeFile.exists()) {
      // Use Windows icon extraction via Qt
      QFileIconProvider iconProvider;
      QIcon icon = iconProvider.icon(exeFile);
      
      if (!icon.isNull()) {
        qDebug() << "Extracted game icon from:" << exePath;
        return icon;
      }
    }
  }
  
  qDebug() << "No game executable found for icon extraction";
  return QIcon();
}

QList<MOBase::ExecutableForcedLoadSetting> GameRedguard::executableForcedLoads() const
{
  // Inject VFS into DOSBox (Windows app) but not into the DOS exe.
  // DOSBox here is 32-bit, so we must load the 32-bit VFS DLL.
  QList<MOBase::ExecutableForcedLoadSetting> settings;

  MOBase::ExecutableForcedLoadSetting dosboxWithVFS("dosbox.exe", "usvfs_x86.dll");
  dosboxWithVFS.withEnabled(true);   // enable VFS for DOSBox
  dosboxWithVFS.withForced(true);    // force injection even if not selected
  settings.append(dosboxWithVFS);

  qDebug() << "VFS enabled for DOSBox.exe via usvfs_x86.dll";
  qDebug() << "Child DOS program (REDGUARD.EXE) runs without direct VFS injection";
  return settings;
}

QList<MOBase::ExecutableInfo> GameRedguard::executables() const
{
  QList<MOBase::ExecutableInfo> executables;

  if (m_GamePath.isEmpty()) {
    qDebug() << "Cannot create executables list - game path is empty";
    return executables;
  }

  // --- GOG ---
  QFileInfo gogDosbox(m_GamePath + "/DOSBOX/dosbox.exe");
  if (gogDosbox.exists()) {
    // Official GOG launcher with known good arguments
    MOBase::ExecutableInfo gogLaunch("Launch Redguard (GOG)", gogDosbox);
    gogLaunch.withWorkingDirectory(QDir(m_GamePath + "/DOSBOX"));
    gogLaunch.withArgument("-conf");
    gogLaunch.withArgument("..\\dosbox_redguard.conf");
    gogLaunch.withArgument("-conf");
    gogLaunch.withArgument("..\\dosbox_redguard_single.conf");
    gogLaunch.withArgument("-noconsole");
    gogLaunch.withArgument("-c");
    gogLaunch.withArgument("exit");
    executables.append(gogLaunch);

    // Plain DOSBox (no args)
    MOBase::ExecutableInfo gogDosboxNoArgs("DOSBox (GOG, no args)", gogDosbox);
    gogDosboxNoArgs.withWorkingDirectory(QDir(m_GamePath + "/DOSBOX"));
    executables.append(gogDosboxNoArgs);
  }

  // --- Steam ---
  QFileInfo steamDosbox(m_GamePath + "/DOSBox-0.73/dosbox.exe");
  if (steamDosbox.exists()) {
    // Steam DOSBox with Redguard config (matches official Steam launcher)
    MOBase::ExecutableInfo steamLaunch("Launch Redguard (Steam)", steamDosbox);
    steamLaunch.withWorkingDirectory(QDir(m_GamePath + "/DOSBox-0.73"));
    steamLaunch.withArgument("-noconsole");
    steamLaunch.withArgument("-conf");
    steamLaunch.withArgument("rg.conf");  // Relative path, working dir is DOSBox-0.73
    executables.append(steamLaunch);
    
    // Steam DOSBox fullscreen variant  
    MOBase::ExecutableInfo steamLaunchFullscreen("Launch Redguard (Steam, Fullscreen)", steamDosbox);
    steamLaunchFullscreen.withWorkingDirectory(QDir(m_GamePath + "/DOSBox-0.73"));
    steamLaunchFullscreen.withArgument("-noconsole");
    steamLaunchFullscreen.withArgument("-conf");
    steamLaunchFullscreen.withArgument("rg.conf");
    steamLaunchFullscreen.withArgument("-fullscreen");
    executables.append(steamLaunchFullscreen);

    // Plain DOSBox
    MOBase::ExecutableInfo steamDosboxNoArgs("DOSBox (Steam, no args)", steamDosbox);
    steamDosboxNoArgs.withWorkingDirectory(QDir(m_GamePath + "/DOSBox-0.73"));
    executables.append(steamDosboxNoArgs);
  }

  // --- Redguard direct ---
  QFileInfo redguardExePath1(m_GamePath + "/Redguard.exe");
  QFileInfo redguardExePath2(m_GamePath + "/Redguard/REDGUARD.EXE");

  if (redguardExePath1.exists()) {
    MOBase::ExecutableInfo redguard("Redguard", redguardExePath1);
    redguard.withWorkingDirectory(QDir(m_GamePath));
    executables.append(redguard);
  } else if (redguardExePath2.exists()) {
    MOBase::ExecutableInfo redguard("Redguard", redguardExePath2);
    redguard.withWorkingDirectory(QDir(m_GamePath + "/Redguard"));
    executables.append(redguard);
  }

  if (executables.isEmpty()) {
    qDebug() << "No valid Redguard executables found at:" << m_GamePath;
  }

  return executables;
}

bool GameRedguard::looksValid(const QDir& path) const
{
  qDebug() << "looksValid() called on:" << path.path();
  
  // First, check if this is a Redguard game installation
  // Check for Steam DOSBox structure (DOSBox-0.73/dosbox.exe and Redguard/ with game files)
  QDir dosboxDir(path.filePath("DOSBox-0.73"));
  if (dosboxDir.exists() && QFile::exists(dosboxDir.filePath("dosbox.exe"))) {
    QDir rgDir(path.filePath("Redguard"));
    if (rgDir.exists() && QFile::exists(rgDir.filePath("REDGUARD.EXE"))) {
      qDebug() << "Valid: Found Steam DOSBox Redguard installation";
      return true;
    }
  }

  // Check for GOG DOSBox structure
  // NOTE: GOG is not supported - it's managed by GOG Galaxy and works perfectly without MO2.
  // Users with GOG should use GOG's native launcher.
  // This plugin is for Steam and standalone installations where mod management is needed.
  
  // Check for traditional Redguard.exe in root
  if (QFile::exists(path.filePath("Redguard.exe"))) {
    qDebug() << "Valid: Found traditional Redguard.exe";
    return true;
  }
  
  // Check for traditional Redguard/REDGUARD.EXE (subdirectory variant)
  if (QFile::exists(path.filePath("Redguard/REDGUARD.EXE"))) {
    qDebug() << "Valid: Found Redguard/REDGUARD.EXE";
    return true;
  }
  
  // Check for DOSBox config files
  if (QFile::exists(dosboxDir.filePath("rg.conf"))) {
    qDebug() << "Valid: Found rg.conf in DOSBox directory";
    return true;
  }
  
  // MOD VALIDATION STARTS HERE
  
  // Check if current directory is a valid Format 1 mod (has About.txt)
  if (QFile::exists(path.filePath("About.txt"))) {
    qDebug() << "Valid: Found Format 1 mod (About.txt) in current directory";
    
    // Validate that it has at least one change file
    bool hasChanges = QFile::exists(path.filePath("INI Changes.txt")) ||
                      QFile::exists(path.filePath("Map Changes.txt")) ||
                      QFile::exists(path.filePath("RTX Changes.txt")) ||
                      QDir(path.filePath("Audio")).exists() ||
                      QDir(path.filePath("Textures")).exists();
    
    if (hasChanges) {
      qDebug() << "Valid: Format 1 mod has change files";
    } else {
      qDebug() << "Warning: Format 1 mod has About.txt but no change files (still valid)";
    }
    return true;
  }
  
  // Check if current directory is Format 2 mod (has game files, no About.txt)
  QStringList gameFiles = {"*.RGM", "*.EXE", "*.CFG", "ENGLISH.RTX"};
  QDir modDir(path);
  if (!modDir.isEmpty()) {
    for (const QString& filter : gameFiles) {
      if (!modDir.entryList(QStringList(filter), QDir::Files).isEmpty()) {
        qDebug() << "Valid: Found Format 2 mod (game files)";
        return true;
      }
    }
  }
  
  // IMPORTANT: Check for nested mod structure
  // This handles cases where the ZIP contains a parent folder with the mod inside
  // Example: archive.zip/<the elder scrolls adventures redguard>/Unofficial Redguard Patch/About.txt
  QDir dirContents(path);
  dirContents.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
  QStringList subdirs = dirContents.entryList();
  
  qDebug() << "========== NESTED STRUCTURE CHECK ==========";
  qDebug() << "Path:" << path.path();
  qDebug() << "Found" << subdirs.size() << "subdirectories";
  qDebug() << "Subdirectories:" << subdirs;
  
  // Check ALL subdirectories (not just when there's exactly one)
  for (const QString& subdir : subdirs) {
    QDir nestedPath(path.filePath(subdir));
    QString aboutPath = nestedPath.filePath("About.txt");
    qDebug() << "  Checking subdirectory:" << subdir;
    qDebug() << "    Full path:" << nestedPath.path();
    qDebug() << "    About.txt path:" << aboutPath;
    qDebug() << "    About.txt exists:" << QFile::exists(aboutPath);
    
    // Check if this subdirectory contains About.txt
    if (QFile::exists(aboutPath)) {
      qDebug() << "  ✓ VALID: Found Format 1 mod in subdirectory:" << subdir;
      qDebug() << "========================================";
      return true;
    }
    
    // Check if this subdirectory contains Format 2 game files
    for (const QString& filter : gameFiles) {
      QStringList matches = nestedPath.entryList(QStringList(filter), QDir::Files);
      if (!matches.isEmpty()) {
        qDebug() << "  ✓ VALID: Found Format 2 mod files in subdirectory:" << subdir;
        qDebug() << "    Matching files:" << matches;
        qDebug() << "========================================";
        return true;
      }
    }
    qDebug() << "    No valid mod data in this subdirectory";
  }
  qDebug() << "========================================";
  
  qDebug() << "Invalid: Not a valid Redguard game or mod directory";
  qDebug() << "  Path checked:" << path.path();
  qDebug() << "  Subdirectories:" << subdirs;
  return false;
}

int GameRedguard::nexusGameID() const
{
  return 4462;
}

int GameRedguard::nexusModOrganizerID() const
{
  return 6220;
}

QDir GameRedguard::gameDirectory() const
{
  if (m_GamePath.isEmpty()) {
    qDebug() << "WARNING: Game path is empty";
    return QDir();
  }
  return QDir(m_GamePath);
}

QDir GameRedguard::dataDirectory() const
{
  // Like Gamebryo games install to Data\, Redguard installs to Redguard\
  // This tells MO2 to automatically put mod files in Redguard subfolder
  return QDir(m_GamePath.isEmpty() ? "" : m_GamePath + "/Redguard");
}

QDir GameRedguard::documentsDirectory() const
{
  return QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
}

QDir GameRedguard::savesDirectory() const
{
  if (!m_GamePath.isEmpty()) {
    // Check for Steam DOSBox savegame location (Redguard/SAVEGAME/)
    QDir savePath(m_GamePath + "/Redguard/SAVEGAME");
    if (savePath.exists()) {
      return savePath;
    }
    
    // Check for traditional /SAVE location
    savePath = QDir(m_GamePath + "/SAVE");
    if (savePath.exists()) {
      return savePath;
    }
  }
  return documentsDirectory();
}

void GameRedguard::setGamePath(const QString& path)
{
  m_GamePath = path;
  qDebug() << "Game path set to:" << m_GamePath;
  qDebug() << "Game directory exists:" << QDir(m_GamePath).exists();
  qDebug() << "Redguard.exe exists:" << QFile::exists(m_GamePath + "/Redguard.exe");
}

bool GameRedguard::isInstalled() const
{
  if (m_GamePath.isEmpty()) {
    qDebug() << "isInstalled: No game path set";
    return false;
  }
  
  QDir gameDir(m_GamePath);
  
  // Check for Steam DOSBox variant (DOSBox-0.73/dosbox.exe + Redguard/REDGUARD.EXE)
  QDir dosboxDir(gameDir.filePath("DOSBox-0.73"));
  if (dosboxDir.exists() && QFile::exists(dosboxDir.filePath("dosbox.exe"))) {
    QDir rgDir(gameDir.filePath("Redguard"));
    if (rgDir.exists() && QFile::exists(rgDir.filePath("REDGUARD.EXE"))) {
      qDebug() << "isInstalled: Found Steam DOSBox Redguard at" << m_GamePath;
      return true;
    }
  }
  
  // Check for traditional Redguard.exe in root
  if (QFile::exists(gameDir.filePath("Redguard.exe"))) {
    qDebug() << "isInstalled: Found traditional Redguard.exe at" << m_GamePath;
    return true;
  }
  
  // Check for Redguard/REDGUARD.EXE variant
  if (QFile::exists(gameDir.filePath("Redguard/REDGUARD.EXE"))) {
    qDebug() << "isInstalled: Found Redguard/REDGUARD.EXE at" << m_GamePath;
    return true;
  }
  
  qDebug() << "isInstalled: Game not found at" << m_GamePath;
  return false;
}

QStringList GameRedguard::listINIFiles() const
{
  QStringList foundINIs;
  
  if (m_GamePath.isEmpty()) {
    qDebug() << "listINIFiles: Game path not set";
    return foundINIs;
  }
  
  // Check Steam DOSBox structure first
  QDir rgDir(m_GamePath + "/Redguard");
  if (rgDir.exists()) {
    for (const QString& iniName : REDGUARD_INI_FILES) {
      if (QFile::exists(rgDir.filePath(iniName))) {
        foundINIs.append(iniName);
        qDebug() << "Found INI:" << iniName << "at" << rgDir.filePath(iniName);
      }
    }
  } else {
    // Check root directory for traditional installation
    QDir rootDir(m_GamePath);
    for (const QString& iniName : REDGUARD_INI_FILES) {
      if (QFile::exists(rootDir.filePath(iniName))) {
        foundINIs.append(iniName);
        qDebug() << "Found INI:" << iniName << "at" << rootDir.filePath(iniName);
      }
    }
  }
  
  qDebug() << "listINIFiles: Found" << foundINIs.size() << "INI files";
  return foundINIs;
}

QString GameRedguard::getINIPath(const QString& iniName) const
{
  if (m_GamePath.isEmpty()) {
    qDebug() << "getINIPath: Game path not set";
    return "";
  }
  
  // Check Steam DOSBox structure first
  QString dosboxPath = m_GamePath + "/Redguard/" + iniName;
  if (QFile::exists(dosboxPath)) {
    qDebug() << "getINIPath:" << iniName << "found at" << dosboxPath;
    return dosboxPath;
  }
  
  // Check root directory
  QString rootPath = m_GamePath + "/" + iniName;
  if (QFile::exists(rootPath)) {
    qDebug() << "getINIPath:" << iniName << "found at" << rootPath;
    return rootPath;
  }
  
  qDebug() << "getINIPath:" << iniName << "not found";
  return "";
}
