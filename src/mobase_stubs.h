#pragma once

#include <QString>
#include <QDir>
#include <functional>
#include <memory>

namespace MOBase {

// Forward declarations
class IModList;
class IPluginGame;

// Minimal IOrganizer interface (stub)
class IOrganizer {
public:
  virtual ~IOrganizer() = default;
  virtual QString basePath() const = 0;
  virtual QString dataPath() const = 0;
  virtual IModList* modList() const = 0;
  virtual void onAboutToRun(std::function<bool(const QString&)> func) = 0;
  virtual void onFinishedRun(std::function<void(const QString&, unsigned int)> func) = 0;
};

// Minimal IModInterface
class IModInterface {
public:
  virtual ~IModInterface() = default;
  virtual QString name() const = 0;
  virtual QString absolutePath() const = 0;
  virtual bool isActive() const = 0;
};

// Minimal IModList
class IModList {
public:
  virtual ~IModList() = default;
  virtual int rowCount() const = 0;
  virtual IModInterface* getMod(int index) = 0;
  virtual IModInterface* getMod(const QString& name) = 0;
};

// IPluginGame interface - game plugins must implement this
class IPluginGame {
public:
  virtual ~IPluginGame() = default;

  // Game information
  virtual QString gameName() const = 0;
  virtual QString gameShortName() const = 0;
  virtual QString gameNexusName() const = 0;
  virtual QString name() const = 0;
  virtual QString author() const = 0;
  virtual QString description() const = 0;

  // App IDs
  virtual QString steamAPPId() const = 0;
  virtual QString gogAPPId() const = 0;
  virtual QString epicAPPId() const { return ""; }
  virtual QString eaDesktopContentId() const { return ""; }

  // Nexus info
  virtual int nexusGameID() const = 0;
  virtual int nexusModOrganizerID() const = 0;

  // Save games
  virtual QString savegameExtension() const = 0;
  
  // Additional required methods with default implementations
  virtual QString binaryName() const { return ""; }
  virtual QString getLauncherName() const { return ""; }
  virtual QString getSupportURL() const { return ""; }
  
  // Directory methods
  virtual QDir gameDirectory() const { return QDir(); }
  virtual QDir dataDirectory() const { return QDir(); }
  virtual QDir documentsDirectory() const { return QDir(); }
  virtual QDir savesDirectory() const { return QDir(); }
  
  // Game variant and version
  virtual QString gameVersion() const { return ""; }
  virtual void setGameVariant(const QString& variant) {}
  
  // Game validation
  virtual bool isInstalled() const { return true; }
  virtual void setGamePath(const QString& path) {}
  
  // Features and executables - would need more complex types
  // For now we'll skip these
};

}  // namespace MOBase

Q_DECLARE_INTERFACE(MOBase::IPluginGame, "com.modorganizer.plugins.IPluginGame")
