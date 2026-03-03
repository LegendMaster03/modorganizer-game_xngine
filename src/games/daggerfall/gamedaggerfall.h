#ifndef GAMEDAGGERFALL_H
#define GAMEDAGGERFALL_H

#include "gamexngine.h"

#include <QObject>
#include <QtPlugin>
#include <QtGlobal>
#include <QIcon>
#include <QStandardPaths>
#include <windows.h>
#include <memory>

class GameDaggerfall : public GameXngine
{
  Q_OBJECT
  Q_INTERFACES(MOBase::IPlugin MOBase::IPluginGame MOBase::IPluginFileMapper)
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  Q_PLUGIN_METADATA(IID "com.tannin.ModOrganizer.PluginGame/2.0" FILE "gamedaggerfall.json")
#endif

public:
  GameDaggerfall();

  virtual bool init(MOBase::IOrganizer* moInfo) override;

public:  // IPluginGame interface
  virtual QString gameName() const override;
  virtual QString displayGameName() const override;
  virtual QList<MOBase::ExecutableInfo> executables() const override;
  virtual QString steamAPPId() const override;
  virtual QString gogAPPId() const;
  virtual QString binaryName() const override;
  virtual QString gameShortName() const override;
  virtual QString gameNexusName() const override;
  virtual QStringList primarySources() const override;
  virtual QStringList validShortNames() const override;
  virtual QStringList iniFiles() const override;
  virtual int nexusModOrganizerID() const override;
  virtual int nexusGameID() const override;
  virtual QString gameVersion() const override;
  virtual QIcon gameIcon() const override;
  virtual MappingType mappings() const override;

public:  // IPlugin interface
  virtual QString name() const override;
  virtual QString localizedName() const override;
  virtual QString author() const override;
  virtual QString description() const override;
  virtual MOBase::VersionInfo version() const override;
  virtual QList<MOBase::PluginSetting> settings() const override;
  int developerSaveDetailsLevel() const;
  bool showDeveloperSaveDetails() const;
  bool allowJsonPatchInstall() const;
  bool allowXdeltaPatchInstall() const;
  bool allowExeModInstall() const;
  bool allowBinaryPatchInstall() const;
  bool allowSavePackModInstall() const;
  bool allowUnityModFormats() const;

protected:
  virtual QString identifyGamePath() const override;
  virtual QDir dataDirectory() const override;
  virtual QDir savesDirectory() const override;
  virtual bool prepareIni(const QString& exec) override;
  virtual QString savegameExtension() const override;
  virtual QString savegameSEExtension() const override;
  virtual std::shared_ptr<const XngineSaveGame> makeSaveGame(QString filepath) const override;

  virtual SaveLayout saveLayout() const override;
  virtual QString saveGameId() const override;
  virtual XngineBSAFormat::Traits bsaTraits() const override;
  virtual QVector<XngineBSAFormat::FileSpec> bsaFileSpecs() const override;

private:
#if defined(XNGINE_DAGGERFALL_EXE_PATCHING)
  bool applyExePatchMods();
  void ensureExePatchCleanupHook();
  void cleanupExePatchOutputMod() const;
#endif
  QString findInRegistry(HKEY baseKey, LPCWSTR path, LPCWSTR value) const;
#if defined(XNGINE_DAGGERFALL_EXE_PATCHING)
  bool m_ExePatchCleanupHookRegistered = false;
#endif
};

#endif  // GAMEDAGGERFALL_H
