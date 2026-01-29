#pragma once

#include <QString>
#include <QList>
#include <QDir>

// Forward declaration
class ModLoader;

// Enum for mod format types
enum class ModFormat {
  PatchBased,          // Format 1: About.txt + *Changes.txt
  FileReplacement,     // Format 2: Raw game files, no About.txt
  Unknown              // Format unknown/invalid
};

// Struct to represent mod metadata
struct ModInfo {
  QString name;
  QString version;
  QString author;
  QString description;
  bool enabled;
  ModFormat format = ModFormat::PatchBased;  // Mod format type
};

// Mod loading framework wrapper - handles all mod-related operations
class RGMODFrameworkWrapper {
public:
  explicit RGMODFrameworkWrapper(const QString& gamePath, const QString& modsPath);
  ~RGMODFrameworkWrapper();

  // Mod discovery and listing
  QList<ModInfo> getModList() const;
  ModInfo getModInfo(const QString& modName) const;
  
  // Mod management
  bool addMod(const QString& modPath);
  bool removeMod(const QString& modName);
  bool setModEnabled(const QString& modName, bool enabled);
  bool reorderMods(const QList<QString>& newOrder);
  
  // File locations
  QString getModPath(const QString& modName) const;
  QString getBackupPath() const;
  
  // Change application
  bool applyChanges(const QList<QString>& enabledMods);
  
  // Backup/restore
  bool backupGameFiles();
  bool restoreGameFiles();
  
  // Direct mod application (writes to game folder, no VFS)
  bool applyModDirectly(const QString& modPath, const QString& modName, const QString& gameDataPath);
  bool applyDirectINIChanges(const QString& modPath, const QString& gameDataPath);
  bool applyDirectMapChanges(const QString& modPath, const QString& gameDataPath);
  bool applyDirectRTXChanges(const QString& modPath, const QString& gameDataPath);
  
  // VFS integration (legacy, kept for compatibility)
  void setOrganizer(void* organizer) { mOrganizer = organizer; }
  bool applyFormat2ModsVFS(const QList<QString>& enabledMods);
  bool createVFSMapping(const QString& modPath, const QString& modName);
  
  // Format 1 VFS support - build modified game files for VFS overlay
  bool buildFormat1ModVFS(const QString& modPath, const QString& modName);
  void cleanupVFSFiles(const QList<QString>& enabledMods);
  bool applyINIChanges(const QString& modPath, const QString& vfsPath);
  bool applyMapChanges(const QString& modPath, const QString& vfsPath);
  bool applyRTXChanges(const QString& modPath, const QString& vfsPath);
  
  // Mod metadata conversion - About.txt to meta.ini
  bool generateMetaIniFromAbout(const QString& modPath, const QString& modName) const;

private:
  QString mGamePath;
  QString mModsPath;
  QString mBackupPath;
  void* mOrganizer = nullptr;
  
  // Format detection methods
  bool isFormat2Mod(const QString& modPath) const;
  QString extractModNameFromFormat2(const QString& modPath) const;
  ModLoader* mModLoader;
  
  bool loadModList();
  bool saveModList();
};

