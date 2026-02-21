#ifndef GAMEBATTLESPIRE_H
#define GAMEBATTLESPIRE_H

#include <gamexngine.h>

#include <QObject>
#include <QtPlugin>
#include <QString>
#include <QDir>

class GameBattlespire : public GameXngine
{
  Q_OBJECT
  Q_INTERFACES(MOBase::IPlugin MOBase::IPluginGame MOBase::IPluginFileMapper)
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  Q_PLUGIN_METADATA(IID "com.tannin.ModOrganizer.PluginGame/2.0" FILE "gamebattlespire.json")
#endif

public:
  GameBattlespire();

  virtual bool init(MOBase::IOrganizer* moInfo) override;

  virtual std::vector<std::shared_ptr<const MOBase::ISaveGame>>
  listSaves(QDir folder) const override;

  virtual QString gameName() const override;
    virtual QString displayGameName() const override;
  virtual QList<MOBase::ExecutableInfo> executables() const override;
  virtual QString steamAPPId() const override;
  virtual QString gogAPPId() const;
  virtual QString binaryName() const override;
  virtual QString gameShortName() const override;
  virtual QString gameNexusName() const override;
  virtual QStringList validShortNames() const override;
  virtual QStringList iniFiles() const override;
  virtual int nexusModOrganizerID() const override;
  virtual int nexusGameID() const override;
  virtual bool looksValid(QDir const& path) const override;
  virtual QString gameVersion() const override;
  virtual QIcon gameIcon() const override;
  virtual QDir dataDirectory() const override;
  virtual QDir documentsDirectory() const override;

  virtual QString name() const override;
  virtual QString localizedName() const override;
  virtual QString author() const override;
  virtual QString description() const override;
  virtual MOBase::VersionInfo version() const override;
  virtual QList<MOBase::PluginSetting> settings() const override;

protected:
  virtual void detectGame() override;
  virtual QString identifyGamePath() const override;
  virtual QDir savesDirectory() const override;
  virtual QString savegameExtension() const override;
  virtual QString savegameSEExtension() const override;
  virtual std::shared_ptr<const XngineSaveGame> makeSaveGame(QString filepath) const override;

  virtual SaveLayout saveLayout() const override;
  virtual QString saveGameId() const override;
  virtual XngineBSAFormat::Traits bsaTraits() const override;
  virtual QVector<XngineBSAFormat::FileSpec> bsaFileSpecs() const override;

private:
  QString findInRegistry(HKEY baseKey, LPCWSTR path, LPCWSTR value) const;
};

#endif  // GAMEBATTLESPIRE_H
