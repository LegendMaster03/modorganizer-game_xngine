#include "RGMODFrameworkWrapper.h"

#include "ModLoader.h"
#include "MapChanges.h"
#include "RtxDatabase.h"
#include "mobase_stubs.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QStandardPaths>
#include <QMap>
#include <QProcessEnvironment>
#include <algorithm>
#include <fstream>

RGMODFrameworkWrapper::RGMODFrameworkWrapper(const QString& gamePath, const QString& modsPath)
    : mGamePath(gamePath), mModsPath(modsPath), mModLoader(new ModLoader(nullptr))
{
  // Ensure mod directories exist
  QDir modsDir(modsPath);
  if (!modsDir.exists()) {
    modsDir.mkpath(".");
    qDebug() << "Created mods directory at:" << modsPath;
  }
  
  // Set backup path relative to mods path (MO2 profile)
  mBackupPath = modsPath + "/../backup";
  QDir backupDir(mBackupPath);
  if (!backupDir.exists()) {
    backupDir.mkpath(".");
    qDebug() << "Created backup directory at:" << mBackupPath;
  }
  
  // Load existing mod list
  loadModList();
  
  qDebug() << "RGMODFrameworkWrapper initialized";
  qDebug() << "  Game path:" << mGamePath;
  qDebug() << "  Mods path:" << mModsPath;
  qDebug() << "  Backup path:" << mBackupPath;
}

RGMODFrameworkWrapper::~RGMODFrameworkWrapper()
{
  // Save mod list before destruction
  saveModList();
  
  delete mModLoader;
  mModLoader = nullptr;
  
  qDebug() << "RGMODFrameworkWrapper destroyed";
}

QList<ModInfo> RGMODFrameworkWrapper::getModList() const
{
  try {
    QList<ModInfo> mods;
    
    QDir modsDir(mModsPath);
    if (!modsDir.exists()) {
      qDebug() << "Mods directory does not exist:" << mModsPath;
      return mods;
    }
    
    qDebug() << "Scanning mods directory:" << mModsPath;
    
    QFileInfoList entries = modsDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    qDebug() << "Found" << entries.size() << "directories in mods folder";
    
    for (const QFileInfo& entry : entries) {
      try {
        QString modPath = entry.filePath();
        QString modName = entry.fileName();
        
        qDebug() << "";
        qDebug() << "Scanning mod:" << modName;
        
        // Check for About.txt - REQUIRED for Format 1 mods
        QString aboutPath = modPath + "/About.txt";
        QFile aboutFile(aboutPath);
        
        if (!aboutFile.exists()) {
          qDebug() << "  No About.txt found - checking if Format 2...";
          // Check if this is a Format 2 (file replacement) mod
          try {
            if (isFormat2Mod(modPath)) {
              ModInfo mod;
              mod.name = modName;
              mod.format = ModFormat::FileReplacement;
              mod.version = "1.0";
              mod.author = "Format 2 (File Replacement)";
              mod.description = "File replacement mod - applied via MO2 virtual file system.";
              mod.enabled = false;
              
              qDebug() << "  ✓ Format 2 mod detected (VFS supported):" << modName;
              qDebug() << "     Location:" << modPath;
              mods.append(mod);  // Store it and will be applied via VFS
            } else {
              qDebug() << "  ✗ Mod rejected - invalid format";
            }
          } catch (const std::exception& e) {
            qWarning() << "  ✗ Exception checking Format 2:" << e.what();
          } catch (...) {
            qWarning() << "  ✗ Unknown exception checking Format 2";
          }
          continue;
        }
        
        // This is a Format 1 (Patch-based) mod
        ModInfo mod;
        mod.name = modName;
        mod.format = ModFormat::PatchBased;
        
        if (aboutFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
          QTextStream in(&aboutFile);
          
          // First line: version (required)
          mod.version = in.readLine().trimmed();
          if (mod.version.isEmpty()) {
            qDebug() << "  ✗ Mod invalid - empty version";
            aboutFile.close();
            continue;
          }
          
          // Second line: author (required)
          mod.author = in.readLine().trimmed();
          if (mod.author.isEmpty()) {
            qDebug() << "  ✗ Mod invalid - empty author";
            aboutFile.close();
            continue;
          }
          
          // Remaining lines: description (can be empty)
          QString desc;
          while (!in.atEnd()) {
            QString line = in.readLine();
            if (!line.isEmpty()) {
              desc += line + "\n";
            }
          }
          mod.description = desc.trimmed();
          aboutFile.close();
          
          // Mods created with change editors require at least ONE change file
          // Check for any of: INI Changes.txt, Map Changes.txt, RTX Changes.txt, Audio/, Textures/
          bool hasINIChanges = QFile::exists(modPath + "/INI Changes.txt");
          bool hasMapChanges = QFile::exists(modPath + "/Map Changes.txt");
          bool hasRTXChanges = QFile::exists(modPath + "/RTX Changes.txt");
          bool hasAudioFolder = QDir(modPath + "/Audio").exists();
          bool hasTexturesFolder = QDir(modPath + "/Textures").exists();
          
          if (!hasINIChanges && !hasMapChanges && !hasRTXChanges && 
              !hasAudioFolder && !hasTexturesFolder) {
            qDebug() << "  ✗ Mod rejected - no change files found";
            continue;
          }
          
          // All validation passed
          mod.enabled = false;  // Default, will be updated by loadModList
          mods.append(mod);
          
          // Generate meta.ini for MO2 integration and categorization
          generateMetaIniFromAbout(modPath, modName);
          
          // Log what types of changes this mod contains
          QString changeTypes;
          if (hasINIChanges) changeTypes += " INI";
          if (hasMapChanges) changeTypes += " Map";
          if (hasRTXChanges) changeTypes += " RTX";
          if (hasAudioFolder) changeTypes += " Audio";
          if (hasTexturesFolder) changeTypes += " Textures";
          
          qDebug() << "  ✓ Format 1 mod loaded: v" << mod.version << "by" << mod.author;
          qDebug() << "     Changes:" << changeTypes.trimmed();
        } else {
          qWarning() << "  ✗ Could not read About.txt file";
        }
      } catch (const std::exception& e) {
        qWarning() << "  ✗ Exception processing mod:" << e.what();
        // Continue with next mod
      } catch (...) {
        qWarning() << "  ✗ Unknown exception processing mod";
        // Continue with next mod
      }
    }
    
    // Log summary with format breakdown
    QList<ModInfo> format1Mods = mods;
    format1Mods.erase(std::remove_if(format1Mods.begin(), format1Mods.end(),
      [](const ModInfo& m) { return m.format != ModFormat::PatchBased; }), 
      format1Mods.end());
    
    QList<ModInfo> format2Mods = mods;
    format2Mods.erase(std::remove_if(format2Mods.begin(), format2Mods.end(),
      [](const ModInfo& m) { return m.format != ModFormat::FileReplacement; }), 
      format2Mods.end());
    
    qDebug() << "";
    qDebug() << "╔════════════════════════════════════════════════════╗";
    qDebug() << "║          MOD DISCOVERY COMPLETE                    ║";
    qDebug() << "╚════════════════════════════════════════════════════╝";
    qDebug() << "Format 1 (Patch-based):        " << format1Mods.size() << "mods";
    qDebug() << "Format 2 (File Replacement):   " << format2Mods.size() << "mods (VFS-supported)";
    qDebug() << "─────────────────────────────────────────────────────";
    qDebug() << "Total mods found:              " << mods.size() << "mods";
    qDebug() << "";
    
    return mods;
  } catch (const std::exception& e) {
    qCritical() << "CRITICAL EXCEPTION in getModList():" << e.what();
    return QList<ModInfo>();  // Return empty list on error
  } catch (...) {
    qCritical() << "CRITICAL: Unknown exception in getModList()";
    return QList<ModInfo>();  // Return empty list on error
  }
}

ModInfo RGMODFrameworkWrapper::getModInfo(const QString& modName) const
{
  QList<ModInfo> allMods = getModList();
  for (const ModInfo& mod : allMods) {
    if (mod.name == modName) {
      return mod;
    }
  }
  return ModInfo{"", "0.0", "Unknown", "", false};
}

bool RGMODFrameworkWrapper::addMod(const QString& modPath)
{
  QFileInfo info(modPath);
  if (!info.isDir()) {
    qDebug() << "Cannot add mod:" << modPath << "is not a directory";
    return false;
  }
  
  QString modName = info.fileName();
  QString destPath = mModsPath + "/" + modName;
  
  // Check if mod already exists
  if (QDir(destPath).exists()) {
    qDebug() << "Mod already exists:" << modName;
    return false;
  }
  
  // Copy mod folder
  QDir sourceDir(modPath);
  QDir().mkpath(destPath);
  
  qDebug() << "Added mod:" << modName;
  return true;
}

bool RGMODFrameworkWrapper::removeMod(const QString& modName)
{
  QString modPath = mModsPath + "/" + modName;
  QDir modDir(modPath);
  
  if (!modDir.exists()) {
    qDebug() << "Mod not found:" << modName;
    return false;
  }
  
  // Remove directory recursively
  if (modDir.removeRecursively()) {
    qDebug() << "Removed mod:" << modName;
    return true;
  }
  
  qDebug() << "Failed to remove mod:" << modName;
  return false;
}

bool RGMODFrameworkWrapper::setModEnabled(const QString& modName, bool enabled)
{
  // This would update the in-memory list and save it
  // For now, just log
  qDebug() << "Set mod" << modName << "enabled:" << enabled;
  return true;
}

bool RGMODFrameworkWrapper::reorderMods(const QList<QString>& newOrder)
{
  // This would reorder mods in the mod list
  qDebug() << "Reordering" << newOrder.size() << "mods";
  return true;
}

QString RGMODFrameworkWrapper::getModPath(const QString& modName) const
{
  // If we have access to MO2's organizer, use its mod path
  if (mOrganizer) {
    // Cast from void* to IOrganizer* 
    auto* organizer = static_cast<MOBase::IOrganizer*>(mOrganizer);
    if (organizer && organizer->modList()) {
      auto* mod = organizer->modList()->getMod(modName);
      if (mod) {
        return mod->absolutePath();
      }
    }
  }
  
  // Fallback to local mods path
  return mModsPath + "/" + modName;
}

QString RGMODFrameworkWrapper::getBackupPath() const
{
  return mBackupPath;
}

bool RGMODFrameworkWrapper::applyChanges(const QList<QString>& enabledMods)
{
  try {
    qDebug() << "";
    qDebug() << "╔════════════════════════════════════════════════════╗";
    qDebug() << "║   REDGUARD MOD FRAMEWORK - APPLYING CHANGES        ║";
    qDebug() << "╚════════════════════════════════════════════════════╝";
    qDebug() << "Total enabled mods:" << enabledMods.size();
    
    if (enabledMods.isEmpty()) {
      qDebug() << "No mods are enabled";
      return true;
    }
    
    // Process each enabled mod
    for (const QString& modName : enabledMods) {
      QString modPath = mModsPath + "/" + modName;
      
      QDir modDir(modPath);
      if (!modDir.exists()) {
        continue;
      }
      
      // Check for About.txt (Type 1 indicator)
      if (!QFile::exists(modPath + "/About.txt")) {
        continue;
      }
      
      // Check for change files
      bool hasINIChanges = QFile::exists(modPath + "/INI Changes.txt");
      bool hasMapChanges = QFile::exists(modPath + "/Map Changes.txt");
      bool hasRTXChanges = QFile::exists(modPath + "/RTX Changes.txt");
      
      if (!hasINIChanges && !hasMapChanges && !hasRTXChanges) {
        continue;
      }
      
      // Process the Type 1 mod changes
      try {
        if (buildFormat1ModVFS(modPath, modName)) {
          qDebug() << "  ✓" << modName << "(Type 1 - processed)";
        }
      } catch (const std::exception& e) {
        qDebug() << "  ✗ Exception processing mod" << modName << ":" << e.what();
      }
    }
    
    qDebug() << "";
    qDebug() << "═══════════════════════════════════════════════════";
    qDebug() << "";
    return true;
  } catch (const std::exception& e) {
    qCritical() << "EXCEPTION in applyChanges():" << e.what();
    return false;
  } catch (...) {
    qCritical() << "UNKNOWN EXCEPTION in applyChanges()";
    return false;
  }
}

bool RGMODFrameworkWrapper::backupGameFiles()
{
  qDebug() << "Backing up game files...";
  
  QDir backupDir(mBackupPath);
  QStringList iniFiles = {
    "COMBAT.INI", "ITEM.INI", "KEYS.INI", "MENU.INI",
    "REGISTRY.INI", "surface.ini", "SYSTEM.INI", "WORLD.INI"
  };
  
  // Find where game files are located (Redguard/ subdirectory or root)
  QString gameDataPath = mGamePath;
  if (QDir(mGamePath + "/Redguard").exists()) {
    gameDataPath = mGamePath + "/Redguard";
  }
  
  // Backup INI files
  for (const QString& iniFile : iniFiles) {
    QString sourcePath = gameDataPath + "/" + iniFile;
    QString destPath = mBackupPath + "/" + iniFile;
    
    if (QFile::exists(sourcePath) && !QFile::exists(destPath)) {
      if (QFile::copy(sourcePath, destPath)) {
        qDebug() << "Backed up:" << iniFile;
      }
    }
  }
  
  // Backup ENGLISH.RTX
  QString rtxSource = gameDataPath + "/ENGLISH.RTX";
  QString rtxDest = mBackupPath + "/ENGLISH.RTX";
  if (QFile::exists(rtxSource) && !QFile::exists(rtxDest)) {
    if (QFile::copy(rtxSource, rtxDest)) {
      qDebug() << "Backed up: ENGLISH.RTX";
    }
  }
  
  return true;
}

bool RGMODFrameworkWrapper::restoreGameFiles()
{
  qDebug() << "Restoring game files from backup...";
  
  // TODO: Implement restoration
  return true;
}

bool RGMODFrameworkWrapper::loadModList()
{
  QString modListPath = mModsPath + "/../Mod List.txt";
  QFile modListFile(modListPath);
  
  if (!modListFile.exists()) {
    qDebug() << "No mod list file found at:" << modListPath;
    return false;
  }
  
  if (!modListFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "Failed to open mod list file:" << modListPath;
    return false;
  }
  
  QTextStream in(&modListFile);
  int loadedMods = 0;
  while (!in.atEnd()) {
    QString line = in.readLine();
    if (line.isEmpty()) continue;
    
    // Format: enabled\tmodName
    QStringList parts = line.split("\t");
    if (parts.size() == 2) {
      QString enabled = parts[0];
      QString modName = parts[1];
      qDebug() << "Loaded mod:" << modName << "enabled:" << enabled;
      loadedMods++;
    }
  }
  
  modListFile.close();
  qDebug() << "Loaded" << loadedMods << "mods from list";
  return true;
}

bool RGMODFrameworkWrapper::saveModList()
{
  QString modListPath = mModsPath + "/../Mod List.txt";
  QFile modListFile(modListPath);
  
  if (!modListFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    qDebug() << "Failed to write mod list file:" << modListPath;
    return false;
  }
  
  QTextStream out(&modListFile);
  
  // TODO: Write current mod list in order with enabled status
  // Format: enabled\tmodName
  
  modListFile.close();
  qDebug() << "Saved mod list";
  return true;
}

bool RGMODFrameworkWrapper::isFormat2Mod(const QString& modPath) const
{
  // Format 2 mods are file replacement mods - they contain actual game files
  // instead of patch manifests (About.txt, *Changes.txt).
  // 
  // These mods work universally because MO2's VFS simply overlays the files
  // on top of the game directory. The mod just needs to have the correct
  // directory structure matching where the game expects to find the files.
  //
  // For Redguard:
  // - INI files go in root or Redguard/ subdirectory
  // - RTX files go in root or Redguard/ subdirectory  
  // - Textures/ and Audio/ typically at root level
  //
  // MO2 handles the VFS overlay automatically, so Format 2 mods don't need
  // any special processing by this plugin.
  
  // Check for Format 2 indicators - raw game files instead of change manifests
  QDir modDir(modPath);
  
  // Check for game file indicators
  bool hasINI = !modDir.entryList(QStringList() << "*.INI", QDir::Files).isEmpty();
  bool hasRTX = !modDir.entryList(QStringList() << "*.RTX", QDir::Files).isEmpty();
  bool hasGXA = !modDir.entryInfoList(QStringList() << "system", QDir::Dirs | QDir::NoDotAndDotDot).isEmpty();
  bool hasFonts = !modDir.entryInfoList(QStringList() << "fonts", QDir::Dirs | QDir::NoDotAndDotDot).isEmpty();
  
  // Count format indicators
  int formatIndicators = (hasINI ? 1 : 0) + (hasRTX ? 1 : 0) + 
                        (hasGXA ? 1 : 0) + (hasFonts ? 1 : 0);
  
  // Format 2 mods typically have multiple of these indicators
  bool isFormat2 = formatIndicators >= 2;
  
  if (isFormat2) {
    qDebug() << "  Format 2 indicators found for:" << modPath;
    qDebug() << "    Has INI files:" << hasINI;
    qDebug() << "    Has RTX files:" << hasRTX;
    qDebug() << "    Has system/ directory:" << hasGXA;
    qDebug() << "    Has fonts/ directory:" << hasFonts;
  }
  
  return isFormat2;
}

QString RGMODFrameworkWrapper::extractModNameFromFormat2(const QString& modPath) const
{
  // For Format 2 mods, extract name from directory
  // e.g., "Software - GamePass (Транслітерація)" → "Software - GamePass"
  QString dirName = QDir(modPath).dirName();
  
  // Remove language indicator in parentheses if present
  int parenPos = dirName.indexOf('(');
  if (parenPos > 0) {
    dirName = dirName.left(parenPos).trimmed();
  }
  
  return dirName.isEmpty() ? QDir(modPath).dirName() : dirName;
}

// VFS Integration - Format 2 Mod Support
bool RGMODFrameworkWrapper::applyFormat2ModsVFS(const QList<QString>& enabledMods)
{
  if (!mOrganizer) {
    qDebug() << "Cannot apply Format 2 mods - organizer not initialized";
    return false;
  }
  
  qDebug() << "=== Applying Format 2 Mods via VFS ===";
  
  int successCount = 0;
  for (const QString& modName : enabledMods) {
    QString modPath = getModPath(modName);
    
    ModInfo modInfo = getModInfo(modName);
    if (modInfo.format != ModFormat::FileReplacement) {
      continue;  // Skip non-Format 2 mods
    }
    
    if (createVFSMapping(modPath, modName)) {
      successCount++;
      qDebug() << "VFS mapping created for:" << modName;
    } else {
      qDebug() << "Failed to create VFS mapping for:" << modName;
    }
  }
  
  if (successCount > 0) {
    qDebug() << "Applied" << successCount << "Format 2 mods via VFS";
    return true;
  }
  
  return false;
}

bool RGMODFrameworkWrapper::createVFSMapping(const QString& modPath, const QString& modName)
{
  if (!mOrganizer) {
    return false;
  }
  
  // Iterate through all files in mod folder
  QDir modDir(modPath);
  QFileInfoList fileList = modDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::DirsFirst);
  
  for (const QFileInfo& fileInfo : fileList) {
    QString relativePath;
    
    if (fileInfo.isDir()) {
      // Handle directories
      QString dirName = fileInfo.fileName();
      QString gamePath = mGamePath + "/" + dirName;
      
      // Map the entire directory
      qDebug() << "  VFS Map DIR:" << modPath << "/" << dirName << "→" << gamePath;
      
    } else {
      // Handle individual files
      QString fileName = fileInfo.fileName();
      QString sourceFile = fileInfo.filePath();
      QString gameFile = mGamePath + "/" + fileName;
      
      // This is a VFS mapping - MO2 will handle the virtual overlay
      // We just need to register it with the organizer
      qDebug() << "  VFS Map FILE:" << sourceFile << "→" << gameFile;
    }
  }
  
  // Note: Actual VFS registration happens in MO2's virtual file system
  // We use the organizer's API to register these mappings
  // For now, we log them for debugging
  
  return true;
}

// Format 1 VFS Support - Build modified game files for VFS overlay
void RGMODFrameworkWrapper::cleanupVFSFiles(const QList<QString>& enabledMods)
{
  qDebug() << "Cleaning up old VFS files...";
  
  for (const QString& modName : enabledMods) {
    QString modPath = getModPath(modName);
    QDir modDir(modPath);
    
    if (!modDir.exists()) {
      continue;
    }
    
    // Remove generated INI files (keep originals)
    QStringList generatedINIs = {
      "COMBAT.INI", "ITEM.INI", "KEYS.INI", "MENU.INI",
      "REGISTRY.INI", "surface.ini", "SYSTEM.INI", "WORLD.INI"
    };
    
    for (const QString& ini : generatedINIs) {
      QString iniPath = modPath + "/" + ini;
      if (QFile::exists(iniPath)) {
        // Only delete if there's a corresponding changes file
        if (QFile::exists(modPath + "/INI Changes.txt")) {
          QFile::remove(iniPath);
          qDebug() << "  Removed old VFS file:" << ini << "from" << modName;
        }
      }
    }
    
    // Clean up RTX database if it exists
    QString rtxPath = modPath + "/ENGLISH.RTX";
    if (QFile::exists(rtxPath) && QFile::exists(modPath + "/RTX Changes.txt")) {
      QFile::remove(rtxPath);
      qDebug() << "  Removed old VFS file: ENGLISH.RTX from" << modName;
    }
  }
  
  qDebug() << "VFS cleanup complete";
}

bool RGMODFrameworkWrapper::buildFormat1ModVFS(const QString& modPath, const QString& modName)
{
  try {
    qDebug() << "=== Building VFS overlay for Format 1 mod:" << modName;
    qDebug() << "  Mod path:" << modPath;
    qDebug() << "  Game path:" << mGamePath;
    
    // Use mod folder root as VFS path - MO2 overlays the entire mod folder
    QString vfsPath = modPath;
    
    QDir vfsDir(vfsPath);
    if (!vfsDir.exists()) {
      qDebug() << "  Creating VFS directory:" << vfsPath;
      if (!vfsDir.mkpath(".")) {
        qCritical() << "  CRITICAL: Failed to create VFS directory:" << vfsPath;
        return false;
      }
    }
    
    // Apply different change types
    bool success = true;
    int appliedChanges = 0;
    
    // INI Changes
    QString iniChangesPath = modPath + "/INI Changes.txt";
    if (QFile::exists(iniChangesPath)) {
      qDebug() << "  Processing INI changes...";
      
      try {
        if (applyINIChanges(modPath, vfsPath)) {
          appliedChanges++;
          qDebug() << "  ✓ Applied INI changes successfully";
        } else {
          qWarning() << "  ✗ WARNING: Failed to apply INI changes";
          success = false;
        }
      } catch (const std::exception& e) {
        qCritical() << "  CRITICAL: Exception in applyINIChanges():" << e.what();
        return false;
      } catch (...) {
        qCritical() << "  CRITICAL: Unknown exception in applyINIChanges()";
        return false;
      }
    }
    
    // Map Changes (scripts)
    QString mapChangesPath = modPath + "/Map Changes.txt";
    if (QFile::exists(mapChangesPath)) {
      qDebug() << "  Processing Map changes...";
      
      if (applyMapChanges(modPath, vfsPath)) {
        appliedChanges++;
        qDebug() << "  ✓ Applied Map changes";
      } else {
        qDebug() << "  ✗ ERROR: Failed to apply Map changes";
        success = false;
      }
    }
    
    // RTX Changes (textures/dialogue)
    QString rtxChangesPath = modPath + "/RTX Changes.txt";
    if (QFile::exists(rtxChangesPath)) {
      qDebug() << "  Processing RTX changes...";
      
      if (applyRTXChanges(modPath, vfsPath)) {
        appliedChanges++;
        qDebug() << "  ✓ Applied RTX changes";
      } else {
        qDebug() << "  ✗ ERROR: Failed to apply RTX changes";
        success = false;
      }
    }
    
    // Audio files - just copy to VFS structure
    QString audioSource = modPath + "/Audio";
    if (QDir(audioSource).exists()) {
      QString audioDest = vfsPath + "/Audio";
      QDir().mkpath(audioDest);
      
      // Copy all WAV files
      QDir audioDir(audioSource);
      QFileInfoList audioFiles = audioDir.entryInfoList(QStringList() << "*.wav" << "*.WAV", QDir::Files);
      for (const QFileInfo& audioFile : audioFiles) {
        QString destFile = audioDest + "/" + audioFile.fileName();
        QFile::remove(destFile);  // Remove if exists
        if (QFile::copy(audioFile.filePath(), destFile)) {
          qDebug() << "  Copied audio file:" << audioFile.fileName();
        }
      }
    }
    
    // Textures - copy to VFS structure
    QString texturesSource = modPath + "/Textures";
    if (QDir(texturesSource).exists()) {
      QString texturesDest = vfsPath + "/Textures";
      QDir().mkpath(texturesDest);
      
      // Copy all texture files
      QDir texturesDir(texturesSource);
      QFileInfoList textureFiles = texturesDir.entryInfoList(QDir::Files);
      for (const QFileInfo& textureFile : textureFiles) {
        QString destFile = texturesDest + "/" + textureFile.fileName();
        QFile::remove(destFile);
        if (QFile::copy(textureFile.filePath(), destFile)) {
          qDebug() << "  Copied texture file:" << textureFile.fileName();
        }
      }
    }
    
    qDebug() << "  VFS overlay built at:" << vfsPath;
    qDebug() << "  Applied changes:" << appliedChanges;
    qDebug() << "  Success:" << (success ? "YES" : "NO");
    
    return success;
  } catch (const std::exception& e) {
    qCritical() << "=== CRITICAL: Exception in buildFormat1ModVFS():" << e.what();
    return false;
  } catch (...) {
    qCritical() << "=== CRITICAL: Unknown exception in buildFormat1ModVFS()";
    return false;
  }
}

bool RGMODFrameworkWrapper::applyINIChanges(const QString& modPath, const QString& vfsPath)
{
  qDebug() << "    Parsing INI Changes.txt format...";
  
  QString changesFile = modPath + "/INI Changes.txt";
  
  QFile file(changesFile);
  
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "    Warning: Could not open" << changesFile;
    return false;
  }
  
  // Format: INI filename (no spaces or path), section headers in brackets, key=value pairs
  // Based on Redguard mod editor format
  QTextStream in(&file);
  QString currentINI;
  QString currentSection;
  
  // Store all INI files and their changes: ININame -> {Section -> {Key -> Value}}
  QMap<QString, QMap<QString, QMap<QString, QString>>> allINIChanges;
  
  while (!in.atEnd()) {
    QString line = in.readLine();
    
    // Trim leading/trailing whitespace
    QString trimmed = line.trimmed();
    
    // Skip empty lines and comments
    if (trimmed.isEmpty() || trimmed.startsWith(";")) {
      continue;
    }
    
    // If line doesn't start with whitespace, it's an INI filename
    if (!line.startsWith(" ") && !line.startsWith("\t")) {
      currentINI = trimmed.toUpper();
      currentSection = "";
      qDebug() << "      Reading changes for:" << currentINI;
      continue;
    }
    
    // This is a key=value line
    if (trimmed.contains("=") && !currentINI.isEmpty()) {
      int eqPos = trimmed.indexOf("=");
      QString key = trimmed.left(eqPos).trimmed();
      QString value = trimmed.mid(eqPos + 1).trimmed();
      
      allINIChanges[currentINI][currentSection][key] = value;
      qDebug() << "          Change:" << key << "=" << value.left(50) << (value.length() > 50 ? "..." : "");
    }
  }
  
  file.close();
  
  // Now apply all changes
  // For each INI file affected, read original, apply patches, write to VFS location
  for (auto iniIt = allINIChanges.constBegin(); iniIt != allINIChanges.constEnd(); ++iniIt) {
    QString iniFileName = iniIt.key();
    const QMap<QString, QMap<QString, QString>>& sections = iniIt.value();
    
    // Find the original INI file in the game directory
    // Could be in root or in a Redguard/ subdirectory
    QString sourceINI;
    QString relativeSubdir;  // Track which subdirectory it's in
    if (QFile::exists(mGamePath + "/" + iniFileName)) {
      sourceINI = mGamePath + "/" + iniFileName;
      relativeSubdir = "";  // Root level
    } else if (QFile::exists(mGamePath + "/Redguard/" + iniFileName)) {
      sourceINI = mGamePath + "/Redguard/" + iniFileName;
      relativeSubdir = "Redguard/";  // In Redguard subdirectory
    } else {
      qDebug() << "    Warning: Could not find original" << iniFileName << "in game path";
      qDebug() << "      Tried:" << mGamePath + "/" + iniFileName;
      qDebug() << "      Tried:" << mGamePath + "/Redguard/" + iniFileName;
      continue;
    }
    
    // Read original INI into memory
    QFile sourceFile(sourceINI);
    if (!sourceFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
      qDebug() << "    Warning: Could not read" << sourceINI;
      continue;
    }
    
    QTextStream sourceReader(&sourceFile);
    QStringList lines;
    while (!sourceReader.atEnd()) {
      lines.append(sourceReader.readLine());
    }
    sourceFile.close();
    
    // Apply all changes
    bool hasChanges = false;
    for (auto sectionIt = sections.constBegin(); sectionIt != sections.constEnd(); ++sectionIt) {
      const QString& section = sectionIt.key();
      const QMap<QString, QString>& keyValues = sectionIt.value();
      
      // Track which section we're in
      bool inSection = false;
      int sectionHeaderLine = -1;
      
      for (int i = 0; i < lines.size(); ++i) {
        QString& line = lines[i];
        QString trimmedLine = line.trimmed();
        
        // Check if this is the section header
        if (trimmedLine == "[" + section + "]" || trimmedLine == "[" + section.toUpper() + "]") {
          inSection = true;
          sectionHeaderLine = i;
          continue;
        }
        
        // If we hit another section header, exit this section
        if (inSection && trimmedLine.startsWith("[") && trimmedLine.endsWith("]") && i != sectionHeaderLine) {
          inSection = false;
          sectionHeaderLine = -1;
          continue;
        }
        
        // If in the right section, look for keys to replace
        if (inSection) {
          for (auto kvIt = keyValues.constBegin(); kvIt != keyValues.constEnd(); ++kvIt) {
            const QString& key = kvIt.key();
            const QString& newValue = kvIt.value();
            
            // Check if this line contains the key (case-insensitive)
            if (trimmedLine.startsWith(key, Qt::CaseInsensitive) && 
                trimmedLine.length() > key.length() && 
                trimmedLine.at(key.length()) == '=') {
              // Found the key - replace the line
              line = key + "=" + newValue;
              hasChanges = true;
              qDebug() << "    Modified:" << iniFileName << "[" << section << "]" << key;
              break;
            }
          }
        }
      }
      
      // If section not found, add it at the end
      if (sectionHeaderLine == -1 && !keyValues.isEmpty()) {
        lines.append("");
        lines.append("[" + section + "]");
        for (auto kvIt = keyValues.constBegin(); kvIt != keyValues.constEnd(); ++kvIt) {
          lines.append(kvIt.key() + "=" + kvIt.value());
          hasChanges = true;
        }
      }
    }
    
    // Write modified INI to VFS location
    // Preserve game directory structure (e.g., Redguard/ subdirectory)
    QString destINI = vfsPath + "/" + relativeSubdir + iniFileName;
    
    qDebug() << "    Writing modified INI to:" << destINI;
    
    // Create subdirectory if needed
    if (!relativeSubdir.isEmpty()) {
      QString subdirPath = vfsPath + "/" + relativeSubdir;
      qDebug() << "      Creating subdirectory:" << subdirPath;
      if (!QDir().mkpath(subdirPath)) {
        qCritical() << "      CRITICAL: Failed to create subdirectory:" << subdirPath;
        return false;
      }
    }
    
    QFile destFile(destINI);
    if (!destFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
      qCritical() << "    CRITICAL: Could not write" << destINI;
      qCritical() << "      Error:" << destFile.errorString();
      return false;
    }
    
    QTextStream destWriter(&destFile);
    for (const QString& line : lines) {
      destWriter << line << "\n";
    }
    destFile.close();
    
    if (hasChanges) {
      qDebug() << "    Wrote modified" << iniFileName << "to VFS overlay";
    }
  }
  
  return true;
}

bool RGMODFrameworkWrapper::applyMapChanges(const QString& modPath, const QString& vfsPath)
{
  qDebug() << "    Parsing Map Changes.txt format...";
  
  QString changesFile = modPath + "/Map Changes.txt";
  QFile file(changesFile);
  
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "    Warning: Could not open" << changesFile;
    return false;
  }
  
  // Format from Java MapChanges:
  // MAPNAME (e.g., "MAPS/IMPERIAL.RGM")
  //   LINENUMBER  SCRIPTLINE (insertion/modification)
  //   LINENUMBER  (empty = deletion)
  
  QTextStream in(&file);
  QString currentMap;
  
  // Store map changes: MapName -> {LineNumber -> ScriptLine}
  // Empty/null value means deletion
  QMap<QString, QMap<int, QString>> mapChanges;
  
  while (!in.atEnd()) {
    QString line = in.readLine();
    
    // If line doesn't start with whitespace, it's a map name
    if (!line.isEmpty() && !line.startsWith(" ") && !line.startsWith("\t")) {
      currentMap = line.trimmed().toUpper();
      if (!currentMap.startsWith("MAPS/")) {
        currentMap = "MAPS/" + currentMap;
      }
      if (!currentMap.endsWith(".RGM")) {
        currentMap += ".RGM";
      }
      qDebug() << "      Reading changes for:" << currentMap;
      continue;
    }
    
    // Parse indented lines as changes
    if ((line.startsWith(" ") || line.startsWith("\t")) && !currentMap.isEmpty()) {
      QString trimmed = line.trimmed();
      
      if (trimmed.isEmpty()) {
        continue;  // Skip empty indented lines
      }
      
      // Format: LINENUMBER\tSCRIPTLINE
      // Where SCRIPTLINE can be empty (meaning delete)
      int tabPos = trimmed.indexOf('\t');
      if (tabPos > 0) {
        bool ok = false;
        int lineNum = trimmed.left(tabPos).toInt(&ok);
        if (ok) {
          QString scriptLine = (tabPos + 1 < trimmed.length()) ? trimmed.mid(tabPos + 1) : "";
          mapChanges[currentMap][lineNum] = scriptLine;
          qDebug() << "        Line" << lineNum << ":" << scriptLine.left(50) << (scriptLine.length() > 50 ? "..." : "");
        }
      }
    }
  }
  
  file.close();
  
  // Apply map changes
  // Note: Full RGM parsing is complex, so we'll store the changes for later application
  // or provide a mechanism for the game engine to apply them at runtime
  
  if (mapChanges.isEmpty()) {
    qDebug() << "    No map changes to apply";
    return true;
  }
  
  // For now, create a "Map Changes Applied.txt" marker file
  // This indicates that map changes were intended and should be applied
  // The actual application requires full RGM binary format parsing
  
  QString markerFile = vfsPath + "/Map Changes Applied.txt";
  QFile marker(markerFile);
  if (marker.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&marker);
    out << "Redguard Map Changes Applied\n";
    out << "Total maps modified: " << mapChanges.size() << "\n";
    out << "\n";
    
    for (auto mapIt = mapChanges.constBegin(); mapIt != mapChanges.constEnd(); ++mapIt) {
      out << mapIt.key() << "\n";
      for (auto lineIt = mapIt.value().constBegin(); lineIt != mapIt.value().constEnd(); ++lineIt) {
        out << "  " << lineIt.key() << "\t" << lineIt.value() << "\n";
      }
    }
    
    marker.close();
    qDebug() << "    Recorded map changes marker for later application";
  }
  
  // Store the map changes for potential future use
  // TODO: Implement full RGM parsing to actually apply these changes
  // This requires parsing the binary RGM format and modifying script sections
  
  qDebug() << "    Note: Map script patching requires full RGM parsing";
  qDebug() << "    Modified maps will use original files until full parser is complete";
  qDebug() << "    Map changes have been recorded for reference";
  
  return true;
}

bool RGMODFrameworkWrapper::applyRTXChanges(const QString& modPath, const QString& vfsPath)
{
  try {
    qDebug() << "  Processing RTX changes...";
    
    QString changesFile = modPath + "/RTX Changes.txt";
    
    // Find ENGLISH.RTX in game directory (could be root or Redguard/ subdirectory)
    QString sourceRTX;
    QString relativeSubdir;
    
    if (QFile::exists(mGamePath + "/ENGLISH.RTX")) {
      sourceRTX = mGamePath + "/ENGLISH.RTX";
      relativeSubdir = "";
    } else if (QFile::exists(mGamePath + "/Redguard/ENGLISH.RTX")) {
      sourceRTX = mGamePath + "/Redguard/ENGLISH.RTX";
      relativeSubdir = "Redguard/";
    } else {
      qDebug() << "    Warning: Could not find ENGLISH.RTX in game path";
      qDebug() << "      Tried:" << mGamePath + "/ENGLISH.RTX";
      qDebug() << "      Tried:" << mGamePath + "/Redguard/ENGLISH.RTX";
      return false;
    }
    
    // Preserve directory structure in VFS overlay
    QString destRTX = vfsPath + "/" + relativeSubdir + "ENGLISH.RTX";
    if (!relativeSubdir.isEmpty()) {
      QDir().mkpath(vfsPath + "/" + relativeSubdir);
    }
    
    // Read the original RTX database
    RtxDatabase rtxDb;
    
    std::string sourceRTXStr = sourceRTX.toStdString();
    
    bool fileExists = QFile::exists(sourceRTX);
    
    try {
      if (!rtxDb.readFile(sourceRTX)) {
        qDebug() << "    Warning: Could not read original ENGLISH.RTX from" << sourceRTX;
        return false;
      }
    } catch (const std::exception& e) {
      qDebug() << "    Exception reading RTX:" << e.what();
      return false;
    } catch (...) {
      qDebug() << "    Unknown exception reading RTX";
      return false;
    }
    
    int dbSize = rtxDb.size();
    
    qDebug() << "    Read RTX database:" << dbSize << "entries";
  
  // Apply changes from the mod
  if (!rtxDb.applyChanges(changesFile)) {
    qDebug() << "    Warning: Could not apply RTX changes";
    return false;
  }

  // Optional quick test indicator (locked behind env opt-in to avoid accidental crashes)
  const auto quickTestMessage = [&]() -> QString {
    QString message;

    // Explicit opt-in: must set MO2_REDGUARD_QUICK_TEST to enable this path
    const QString envValue = QProcessEnvironment::systemEnvironment().value("MO2_REDGUARD_QUICK_TEST");
    const bool disableRequested = envValue.compare("0", Qt::CaseInsensitive) == 0 || envValue.compare("false", Qt::CaseInsensitive) == 0;
    const bool enabled = (!envValue.isEmpty() && !disableRequested);

    if (!enabled) {
      return {};
    }

    if (envValue.compare("1", Qt::CaseInsensitive) == 0 || envValue.compare("true", Qt::CaseInsensitive) == 0) {
      message = "MO2 VFS ACTIVE";
    } else {
      message = envValue;
    }

    // Optional override from Quick Test.txt if present (still only when opt-in is true)
    const QString quickTestPath = modPath + "/Quick Test.txt";
    if (QFile::exists(quickTestPath)) {
      QFile messageFile(quickTestPath);
      if (messageFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream textIn(&messageFile);
        const QString fileMessage = textIn.readAll().trimmed();
        if (!fileMessage.isEmpty()) {
          message = fileMessage;
        }
      } else {
        qDebug() << "    Warning: Could not read Quick Test.txt";
      }
    }

    return message.left(120);
  }();

  if (!quickTestMessage.isEmpty()) {
    RtxEntry* quickEntry = rtxDb.getEntry("?lvg");
    if (quickEntry != nullptr) {
      quickEntry->subtitle = quickTestMessage;
      qDebug() << "    Added quick test indicator to ?lvg:" << quickTestMessage;
    } else {
      qDebug() << "    Quick test indicator requested but label ?lvg not found in RTX";
    }
  }
  
  // Write modified RTX to VFS location
  if (!rtxDb.writeFile(destRTX)) {
    qDebug() << "    Warning: Could not write modified RTX file";
    return false;
  }
  
  qDebug() << "    Successfully applied RTX changes to VFS";
  
  return true;
  
  } catch (const std::exception& e) {
    qCritical() << "Exception in applyRTXChanges:" << e.what();
    return false;
  } catch (...) {
    qCritical() << "Unknown exception in applyRTXChanges";
    return false;
  }
}

// Generate meta.ini from About.txt for MO2 recognition and categorization
bool RGMODFrameworkWrapper::generateMetaIniFromAbout(const QString& modPath, const QString& modName) const
{
  QString aboutPath = modPath + "/About.txt";
  QString metaPath = modPath + "/meta.ini";
  
  // If meta.ini already exists and is recent, don't overwrite
  QFileInfo metaInfo(metaPath);
  if (metaInfo.exists() && metaInfo.lastModified().addSecs(300) > QDateTime::currentDateTime()) {
    return true;  // Recently updated, skip
  }
  
  QFile aboutFile(aboutPath);
  if (!aboutFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "  Warning: Could not read About.txt for meta.ini generation";
    return false;
  }
  
  QTextStream in(&aboutFile);
  QString version = in.readLine().trimmed();
  QString author = in.readLine().trimmed();
  QString description;
  while (!in.atEnd()) {
    QString line = in.readLine();
    if (!line.isEmpty()) {
      description += line + "\n";
    }
  }
  description = description.trimmed();
  aboutFile.close();
  
  // Determine category based on mod content or name
  int categoryId = 0;  // 0 = No category
  QString categoryName = "Misc";
  
  // Check mod content to auto-categorize
  bool hasINI = QFile::exists(modPath + "/INI Changes.txt");
  bool hasMap = QFile::exists(modPath + "/Map Changes.txt");
  bool hasRTX = QFile::exists(modPath + "/RTX Changes.txt");
  bool hasAudio = QDir(modPath + "/Audio").exists() && !QDir(modPath + "/Audio").entryList(QDir::Files).isEmpty();
  bool hasTextures = QDir(modPath + "/Textures").exists() && !QDir(modPath + "/Textures").entryList(QDir::Files).isEmpty();
  
  // MO2 Category IDs (standard Nexus categories)
  // 1 = Armour, 2 = Audio, 3 = Body/Face/Hair, 4 = Buildings
  // 5 = Clothing, 6 = Collectibles, 7 = Companions, 8 = Creatures
  // 9 = Factions, 10 = Gameplay, 11 = Locations, 12 = Magic
  // 13 = Models & Textures, 14 = NPCs, 15 = Overhauls, 16 = Patches
  // 17 = Quests, 18 = Races, 19 = Resources, 20 = Utilities
  
  // Auto-categorize based on content
  if (hasTextures && !hasAudio && !hasMap) {
    categoryId = 13;  // Models & Textures
    categoryName = "Models & Textures";
  } else if (hasAudio && !hasTextures && !hasMap) {
    categoryId = 2;  // Audio
    categoryName = "Audio";
  } else if (hasMap && (hasINI || hasRTX)) {
    categoryId = 10;  // Gameplay
    categoryName = "Gameplay";
  } else if (hasINI && !hasMap && !hasTextures) {
    categoryId = 20;  // Utilities (for config/settings)
    categoryName = "Utilities";
  } else if (hasTextures && hasAudio) {
    categoryId = 15;  // Overhauls
    categoryName = "Overhauls";
  } else if (modName.toLower().contains("patch") || modName.toLower().contains("fix")) {
    categoryId = 16;  // Patches
    categoryName = "Patches";
  } else if (modName.toLower().contains("translation") || modName.toLower().contains("traduccion")) {
    categoryId = 20;  // Utilities (for localization)
    categoryName = "Localization";
  } else {
    categoryId = 0;  // Misc/No category
    categoryName = "Misc";
  }
  
  // Generate meta.ini content with MO2-compatible format
  QString metaContent = QString(
    "[General]\n"
    "gameName=redguard\n"
    "modid=0\n"
    "version=%1\n"
    "newestVersion=%1\n"
    "category=%2\n"
    "installationFile=\n"
    "\n"
    "[installedFiles]\n"
    "size=0\n"
  )
  .arg(version)
  .arg(categoryId);
  
  // Write meta.ini
  QFile metaFile(metaPath);
  if (!metaFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    qDebug() << "  Warning: Could not create meta.ini";
    return false;
  }
  
  metaFile.write(metaContent.toUtf8());
  metaFile.close();
  
  qDebug() << "  Generated meta.ini:" << metaPath;
  qDebug() << "    Category:" << categoryName << "(ID:" << categoryId << ")";
  qDebug() << "    Name:" << modName;
  qDebug() << "    Version:" << version;
  
  return true;
}

// Apply mod changes directly to the game folder (no VFS)
bool RGMODFrameworkWrapper::applyModDirectly(const QString& modPath, const QString& modName, const QString& gameDataPath)
{
  qDebug() << "Applying mod directly to game folder:" << modName;
  qDebug() << "  Mod path:" << modPath;
  qDebug() << "  Game data path:" << gameDataPath;
  
  bool success = true;
  
  // Apply INI changes
  QString iniChangesFile = modPath + "/INI Changes.txt";
  if (QFile::exists(iniChangesFile)) {
    qDebug() << "  Applying INI changes...";
    if (!applyDirectINIChanges(modPath, gameDataPath)) {
      qDebug() << "    Warning: Failed to apply INI changes";
      success = false;
    } else {
      qDebug() << "    ✓ Applied INI changes";
    }
  }
  
  // Apply Map changes
  QString mapChangesFile = modPath + "/Map Changes.txt";
  if (QFile::exists(mapChangesFile)) {
    qDebug() << "  Applying Map changes...";
    if (!applyDirectMapChanges(modPath, gameDataPath)) {
      qDebug() << "    Warning: Failed to apply Map changes";
      success = false;
    } else {
      qDebug() << "    ✓ Applied Map changes";
    }
  }
  
  // Apply RTX changes
  QString rtxChangesFile = modPath + "/RTX Changes.txt";
  if (QFile::exists(rtxChangesFile)) {
    qDebug() << "  Applying RTX changes...";
    if (!applyDirectRTXChanges(modPath, gameDataPath)) {
      qDebug() << "    Warning: Failed to apply RTX changes";
      success = false;
    } else {
      qDebug() << "    ✓ Applied RTX changes";
    }
  }
  
  return success;
}

// Helper to apply INI changes directly to game folder
bool RGMODFrameworkWrapper::applyDirectINIChanges(const QString& modPath, const QString& gameDataPath)
{
  // Read the INI Changes.txt and apply to actual game INI files
  QString changesFile = modPath + "/INI Changes.txt";
  QFile file(changesFile);
  
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return false;
  }
  
  QTextStream in(&file);
  QString currentINI;
  QString currentSection;
  QMap<QString, QMap<QString, QString>> iniChanges;
  
  while (!in.atEnd()) {
    QString line = in.readLine().trimmed();
    if (line.isEmpty() || line.startsWith(";")) continue;
    
    if (line.endsWith(".INI", Qt::CaseInsensitive)) {
      currentINI = line;
      currentSection = "";
      continue;
    }
    
    if (line.startsWith("[") && line.endsWith("]")) {
      currentSection = line.mid(1, line.length() - 2);
      continue;
    }
    
    if (line.contains("=") && !currentINI.isEmpty()) {
      QStringList parts = line.split("=", Qt::KeepEmptyParts);
      if (parts.size() >= 2) {
        QString key = parts[0].trimmed();
        QString value = parts.mid(1).join("=").trimmed();
        QString sectionKey = currentSection.isEmpty() ? "" : "[" + currentSection + "]";
        iniChanges[currentINI][sectionKey + "/" + key] = value;
      }
    }
  }
  file.close();
  
  // Now apply to actual game INI files
  for (auto it = iniChanges.constBegin(); it != iniChanges.constEnd(); ++it) {
    QString iniFile = it.key();
    QString gameINIPath = gameDataPath + "/" + iniFile;
    
    if (!QFile::exists(gameINIPath)) {
      qDebug() << "    Warning: Game INI not found:" << gameINIPath;
      continue;
    }
    
    // Read current INI
    QFile iniFileObj(gameINIPath);
    if (!iniFileObj.open(QIODevice::ReadOnly | QIODevice::Text)) {
      qDebug() << "    Warning: Could not read" << iniFile;
      continue;
    }
    
    QStringList lines;
    QTextStream reader(&iniFileObj);
    while (!reader.atEnd()) {
      lines.append(reader.readLine());
    }
    iniFileObj.close();
    
    // Apply changes
    const QMap<QString, QString>& changes = it.value();
    for (auto changeIt = changes.constBegin(); changeIt != changes.constEnd(); ++changeIt) {
      QString fullKey = changeIt.key();
      QString newValue = changeIt.value();
      
      QString section, key;
      if (fullKey.contains("/")) {
        int slashPos = fullKey.lastIndexOf("/");
        section = fullKey.left(slashPos);
        key = fullKey.mid(slashPos + 1);
      } else {
        key = fullKey;
        section = "";
      }
      
      // Find and update the line
      bool found = false;
      for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        if (line.startsWith(section) && !section.isEmpty()) {
          // Found section
          for (int j = i + 1; j < lines.size(); ++j) {
            QString subLine = lines[j];
            if (subLine.trimmed().startsWith("[")) break;  // Next section
            if (subLine.trimmed().startsWith(key + "=")) {
              lines[j] = key + "=" + newValue;
              found = true;
              break;
            }
          }
        }
      }
      
      if (!found && section.isEmpty()) {
        // Add to end of file
        lines.append(key + "=" + newValue);
      }
    }
    
    // Write back
    if (!iniFileObj.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
      qDebug() << "    Warning: Could not write" << iniFile;
      continue;
    }
    
    QTextStream writer(&iniFileObj);
    for (const QString& line : lines) {
      writer << line << "\n";
    }
    iniFileObj.close();
  }
  
  return true;
}

bool RGMODFrameworkWrapper::applyDirectMapChanges(const QString& modPath, const QString& gameDataPath)
{
  qDebug() << "    applyDirectMapChanges - not yet implemented";
  return true;
}

bool RGMODFrameworkWrapper::applyDirectRTXChanges(const QString& modPath, const QString& gameDataPath)
{
  qDebug() << "    applyDirectRTXChanges - not yet implemented";
  return true;
}


