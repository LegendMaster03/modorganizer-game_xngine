#ifndef GAMEREDGUARD_H
#define GAMEREDGUARD_H

#include "gamexngine.h"

#include <QObject>
#include <QtPlugin>
#include <QtGlobal>
#include <QIcon>
#include <windows.h>
#include <memory>

class GameRedguard : public GameXngine
{
  Q_OBJECT
  Q_INTERFACES(MOBase::IPlugin MOBase::IPluginGame MOBase::IPluginFileMapper)
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  Q_PLUGIN_METADATA(IID "com.tannin.ModOrganizer.PluginGame/2.0" FILE "gameredguard.json")
#endif

public:
  GameRedguard() = default;

  bool init(MOBase::IOrganizer* moInfo) override;

public:  // IPluginGame interface
  QString gameName() const override;
  QString displayGameName() const override;
  QList<MOBase::ExecutableInfo> executables() const override;
  QString steamAPPId() const override;
  QString gogAPPId() const;
  QString binaryName() const override;
  QString gameShortName() const override;
  QString gameNexusName() const override;
  QStringList validShortNames() const override;
  QStringList iniFiles() const override;
  MappingType mappings() const override;
  int nexusModOrganizerID() const override;
  int nexusGameID() const override;
  QString gameVersion() const override;
  QIcon gameIcon() const override;
  QDir dataDirectory() const override;

public:  // IPlugin interface
  QString name() const override;
  QString localizedName() const override;
  QString author() const override;
  QString description() const override;
  MOBase::VersionInfo version() const override;
  QList<MOBase::PluginSetting> settings() const override;
  bool allowExeModInstall() const;
  bool allowDillon241PatchInstall() const;
  bool showFullSvitInventory() const;

protected:
  QString identifyGamePath() const override;
  bool looksValid(QDir const& path) const override;
  bool prepareIni(const QString& exec) override;
  QDir savesDirectory() const override;
  QString savegameExtension() const override;
  QString savegameSEExtension() const override;
  std::shared_ptr<const XngineSaveGame> makeSaveGame(QString filepath) const override;

  SaveLayout saveLayout() const override;
  QString saveGameId() const override;

private:
  QString findInRegistry(HKEY baseKey, LPCWSTR path, LPCWSTR value) const;
  bool applyPatchMods();
};

#endif  // GAMEREDGUARD_H
