#ifndef GAMEXNGINE_H
#define GAMEXNGINE_H

#include "iplugingame.h"

class XngineBSAInvalidation;
class XngineDataArchives;
class XngineLocalSavegames;
class XngineSaveGameInfo;
class XngineScriptExtender;
class XngineGamePlugins;
class XngineUnmanagedMods;

#include <QObject>
#include <QtPlugin>
#include <QString>
#include <ShlObj.h>
#include <dbghelp.h>
#include <ipluginfilemapper.h>
#include <iplugingame.h>
#include <memory>

#include "xnginesavegame.h"
#include "xnginesaves.h"
#include "igamefeatures.h"

class GameXngine : public MOBase::IPluginGame, public MOBase::IPluginFileMapper
{
  friend class XngineScriptExtender;
  friend class XngineSaveGameInfo;
  friend class XngineSaveGameInfoWidget;
  friend class XngineSaveGame;

  /**
   * Some Bethesda games do not have a valid file version but a valid product
   * version. If the file version starts with FALLBACK_GAME_VERSION, the product
   * version will be tried.
   */
  static constexpr const char* FALLBACK_GAME_VERSION = "1.0.0";

public:
  GameXngine();

  void detectGame() override;
  bool init(MOBase::IOrganizer* moInfo) override;

public:  // IPluginGame interface
  // getName
  virtual void initializeProfile(const QDir& profile,
                                 MOBase::IPluginGame::ProfileSettings settings) const override;
  virtual std::vector<std::shared_ptr<const MOBase::ISaveGame>>
  listSaves(QDir folder) const override;
  virtual QList<MOBase::ExecutableForcedLoadSetting> executableForcedLoads() const override;

  virtual bool isInstalled() const override;
  virtual QIcon gameIcon() const override;
  virtual QDir gameDirectory() const override;
  virtual QDir dataDirectory() const override;
  // secondaryDataDirectories
  virtual void setGamePath(const QString& path) override;
  virtual QDir documentsDirectory() const override;
  virtual QDir savesDirectory() const override;
  // executables
  // steamAPPId
  // primaryPlugins
  // enabledPlugins
  // gameVariants
  virtual void setGameVariant(const QString& variant) override;
  virtual QString binaryName() const override;
  // gameShortName
  // primarySources
  // validShortNames
  // iniFiles
  // DLCPlugins
  // CCPlugins
  virtual LoadOrderMechanism loadOrderMechanism() const override;
  virtual SortMechanism sortMechanism() const override;
  // nexusModOrganizerID
  // nexusGameID
  virtual bool looksValid(QDir const&) const override;
  virtual QString gameVersion() const override;
  virtual QString getLauncherName() const override;

public:  // IPluginFileMapper interface
  virtual MappingType mappings() const;

public:  // Other (e.g. for game features)
  QString myGamesPath() const;

  static SaveStoragePaths resolveSaveStorage(const QString& profilePath, const QString& gameId);

  static std::vector<SaveSlot> enumerateSaveSlots(const SaveStoragePaths& paths,
                                                  const SaveLayout& layout);

  static bool ensureSaveDirsExist(const SaveStoragePaths& paths, const SaveLayout& layout);

protected:
  virtual SaveLayout saveLayout() const = 0;
  virtual QString saveGameId() const = 0;
  virtual QString saveSlotPrefix() const;

  QString profilePath() const;

  // Retrieve the saves extension for the game.
  virtual QString savegameExtension() const   = 0;
  virtual QString savegameSEExtension() const = 0;

  // Create a save game.
  virtual std::shared_ptr<const XngineSaveGame>
  makeSaveGame(QString filepath) const = 0;

  QFileInfo findInGameFolder(const QString& relativePath) const;
  QString selectedVariant() const;
  WORD getArch(QString const& program) const;

  static QString localAppFolder();
  // Arguably this shouldn't really be here but every xngine program seems to
  // use it
  static QString getLootPath();

  // This function is not terribly well named as it copies exactly where it's told
  // to, irrespective of whether it's in the profile...
  static void copyToProfile(const QString& sourcePath, const QDir& destinationDirectory,
                            const QString& sourceFileName);

  static void copyToProfile(const QString& sourcePath, const QDir& destinationDirectory,
                            const QString& sourceFileName,
                            const QString& destinationFileName);

  virtual QString identifyGamePath() const;

  virtual bool prepareIni(const QString& exec);

  static std::unique_ptr<BYTE[]> getRegValue(HKEY key, LPCWSTR path, LPCWSTR value,
                                             DWORD flags, LPDWORD type);

  static QString findInRegistry(HKEY baseKey, LPCWSTR path, LPCWSTR value);

  static QString getKnownFolderPath(REFKNOWNFOLDERID folderId, bool useDefault);

  static QString getSpecialPath(const QString& name);

  static QString determineMyGamesPath(const QString& gameName);

  static QString parseEpicGamesLocation(const QStringList& manifests);

  static QString parseSteamLocation(const QString& appid, const QString& directoryName);

protected:
  void registerFeature(std::shared_ptr<MOBase::GameFeature> feature);

protected:
  // to access organizer for game features, avoid having to pass it to all saves since
  // we already pass the game
  friend class XngineSaveGame;

  QString m_GamePath;
  QString m_MyGamesPath;
  QString m_GameVariant;
  MOBase::IOrganizer* m_Organizer;
};

#endif  // GAMEXNGINE_H
