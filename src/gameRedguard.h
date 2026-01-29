#pragma once

#include <QString>
#include <QList>
#include <QDir>
#include <QObject>
#include <QIcon>
#include <memory>
#include <iplugingame.h>
#include <iplugin.h>
#include <executableinfo.h>
#include <vector>

// Forward declarations
class redguardSaveGame;
class RGMODFrameworkWrapper;

namespace MOBase {
  class IOrganizer;
  class GameFeature;
}

// Game plugin class - inherits from IPluginGame (which already derives QObject)
class GameRedguard : public MOBase::IPluginGame {
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "com.tannin.ModOrganizer.PluginGame/2.0" FILE "game_redguard.json")
  Q_INTERFACES(MOBase::IPlugin MOBase::IPluginGame)

public:
  GameRedguard();
  ~GameRedguard() = default;

  // IPlugin base
  virtual MOBase::VersionInfo version() const override;
  virtual bool init(MOBase::IOrganizer* organizer) override;
  virtual QList<MOBase::PluginSetting> settings() const override;

  // Virtual methods from IPluginGame
  virtual QString gameName() const override;
  virtual QString gameShortName() const override;
  virtual QString gameNexusName() const override;
  virtual QString name() const override;
  virtual QString author() const override;
  virtual QString description() const override;

  // App IDs
  virtual QString steamAPPId() const override;

  // Nexus info
  virtual int nexusGameID() const override;
  virtual int nexusModOrganizerID() const override;

  // Save games
  virtual std::vector<std::shared_ptr<const MOBase::ISaveGame>> listSaves(QDir saveDir) const override;

  // Primary sources (manual, comic, etc.)
  virtual QStringList primarySources() const override;

  // Required game lifecycle
  virtual void detectGame() override;
  virtual void initializeProfile(const QDir& path, MOBase::IPluginGame::ProfileSettings settings) const override;
  
  // Directory methods
  virtual QDir gameDirectory() const override;
  virtual QDir dataDirectory() const override;
  virtual QDir documentsDirectory() const override;
  virtual QDir savesDirectory() const override;
  virtual bool isInstalled() const override;
  virtual void setGamePath(const QString& path) override;
  virtual bool looksValid(const QDir& path) const override;
  virtual QIcon gameIcon() const override;
  virtual QList<MOBase::ExecutableForcedLoadSetting> executableForcedLoads() const override;
  
  // Executables
  virtual QList<MOBase::ExecutableInfo> executables() const override;
  
  // Game variant and version
  virtual QString gameVersion() const override { return "1.0"; }
  virtual void setGameVariant(const QString& variant) override {}
  
  // Binary name
  virtual QString binaryName() const override { return "dosbox.exe"; }
  virtual QString getLauncherName() const override { return "Redguard (Full Screen).bat"; }
  
  // INI file support
  QStringList listINIFiles() const;
  QString getINIPath(const QString& iniName) const;
  
  // Mod framework access
  RGMODFrameworkWrapper* getModFramework() const { return mRGmodFrameworkWrapper.get(); }

protected:
  // Register a game feature with MO2
  void registerFeature(std::shared_ptr<MOBase::GameFeature> feature);

private:
  QString m_GamePath;
  std::unique_ptr<RGMODFrameworkWrapper> mRGmodFrameworkWrapper;
  MOBase::IOrganizer* mOrganizer = nullptr;
  
  // Known Redguard INI files
  const QStringList REDGUARD_INI_FILES = {
    "COMBAT.INI",
    "ITEM.INI", 
    "KEYS.INI",
    "MENU.INI",
    "REGISTRY.INI",
    "surface.ini",
    "SYSTEM.INI",
    "WORLD.INI"
  };
};

