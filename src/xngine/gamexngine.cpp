#include "gamexngine.h"

#include "iprofile.h"
#include "log.h"
#include "registry.h"
#include "scopeguard.h"
#include "utility.h"
#include "vdf_parser.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonValue>

#include <QtDebug>
#include <QtGlobal>

#include <Knownfolders.h>
#include <Shlobj.h>
#include <Windows.h>
#include <winreg.h>
#include <winver.h>

#include <optional>
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
#include <QRegularExpression>
#include <QDateTime>

static bool isPeExecutable(const QString& path)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return false;
  }

  QByteArray header = file.read(64);
  if (header.size() < 64 || header[0] != 'M' || header[1] != 'Z') {
    return false;
  }

  quint32 peOffset = 0;
  memcpy(&peOffset, header.constData() + 0x3C, sizeof(peOffset));
  file.seek(peOffset);
  QByteArray peSig = file.read(4);
  return peSig.size() >= 4 && peSig[0] == 'P' && peSig[1] == 'E' && peSig[2] == '\0' &&
         peSig[3] == '\0';
}

GameXngine::GameXngine() {
  qInfo().noquote() << "[GameXngine] Constructor ENTRY";
  OutputDebugStringA("[GameXngine] Constructor ENTRY\n");
  OutputDebugStringA("[GameXngine] Constructor EXIT\n");
  qInfo().noquote() << "[GameXngine] Constructor EXIT";
}

void GameXngine::detectGame()
{
  qInfo().noquote() << "[GameXngine] detectGame() ENTRY";
  OutputDebugStringA(("[GameXngine] detectGame() ENTRY - gameName='" + gameName().toStdString() + "'\n").c_str());
  m_GamePath    = identifyGamePath();
  qInfo().noquote() << "[GameXngine] detectGame() m_GamePath=" << m_GamePath;
  OutputDebugStringA(("[GameXngine] detectGame() m_GamePath = '" + m_GamePath.toStdString() + "'\n").c_str());
  m_MyGamesPath = determineMyGamesPath(gameName());
  qInfo().noquote() << "[GameXngine] detectGame() EXIT";
  OutputDebugStringA("[GameXngine] detectGame() EXIT\n");
}

bool GameXngine::init(MOBase::IOrganizer* moInfo)
{
  qInfo().noquote() << "[GameXngine] init() ENTRY";
  OutputDebugStringA(("[GameXngine] init() ENTRY - gameName='" + gameName().toStdString() + "'\n").c_str());
  m_Organizer = moInfo;
  if (!m_Organizer) {
    qWarning().noquote() << "[GameXngine] WARNING: m_Organizer is NULL in init()";
    OutputDebugStringA("[GameXngine] WARNING: m_Organizer is NULL in init()\n");
    return false;
  }
  m_Organizer->onAboutToRun([this](const auto& binary) {
    const auto profile = profilePath();
    if (!profile.isEmpty()) {
      const auto layout = saveLayout();
      const auto paths = resolveSaveStorage(profile, saveGameId());
      ensureSaveDirsExist(paths, layout);
    }
    return prepareIni(binary);
  });
  qInfo().noquote() << "[GameXngine] init() EXIT";
  OutputDebugStringA("[GameXngine] init() EXIT\n");
  return true;
}

bool GameXngine::isInstalled() const
{
  qInfo().noquote() << "[GameXngine] isInstalled() called";
  OutputDebugStringA(("[GameXngine] isInstalled() called - gameName='" + gameName().toStdString() + "'\n").c_str());
  OutputDebugStringA(("[GameXngine] m_GamePath = '" + m_GamePath.toStdString() + "'\n").c_str());
  return !m_GamePath.isEmpty();
}

void GameXngine::initializeProfile(const QDir& profile,
                                   MOBase::IPluginGame::ProfileSettings settings) const
{
  qInfo().noquote() << "[GameXngine] initializeProfile() ENTRY";
  // Stub implementation - XnGine games typically don't need special profile initialization
  // Override in derived classes if needed
}

QList<MOBase::ExecutableForcedLoadSetting> GameXngine::executableForcedLoads() const
{
  qInfo().noquote() << "[GameXngine] executableForcedLoads() ENTRY";
  // Stub implementation - XnGine games typically don't use forced loads
  // Override in derived classes if needed
  return QList<MOBase::ExecutableForcedLoadSetting>();
}

QIcon GameXngine::gameIcon() const
{
  qInfo().noquote() << "[GameXngine] gameIcon() ENTRY";
  OutputDebugStringA("[GameXngine] gameIcon() ENTRY\n");
  if (m_GamePath.isEmpty()) {
    qWarning().noquote() << "[GameXngine] gameIcon() - m_GamePath is EMPTY, returning default icon";
    OutputDebugStringA("[GameXngine] gameIcon() - m_GamePath is EMPTY, returning default icon\n");
    return QIcon();
  }
  QString binPath = gameDirectory().absoluteFilePath(binaryName());
  if (!isPeExecutable(binPath)) {
    qWarning().noquote() << "[GameXngine] gameIcon() - non-PE binary, returning default icon";
    OutputDebugStringA("[GameXngine] gameIcon() - non-PE binary, returning default icon\n");
    return QIcon();
  }
  qInfo().noquote() << "[GameXngine] gameIcon() - calling iconForExecutable:" << binPath;
  OutputDebugStringA("[GameXngine] gameIcon() - calling iconForExecutable\n");
  QIcon icon = MOBase::iconForExecutable(binPath);
  qInfo().noquote() << "[GameXngine] gameIcon() EXIT";
  OutputDebugStringA("[GameXngine] gameIcon() EXIT\n");
  return icon;
}

QDir GameXngine::gameDirectory() const
{
  qInfo().noquote() << "[GameXngine] gameDirectory() called m_GamePath=" << m_GamePath;
  OutputDebugStringA(("[GameXngine] gameDirectory() called - gameName='" + gameName().toStdString() + "'\n").c_str());
  OutputDebugStringA(("[GameXngine] m_GamePath = '" + m_GamePath.toStdString() + "'\n").c_str());
  return QDir(m_GamePath);
}

QDir GameXngine::dataDirectory() const
{
  qInfo().noquote() << "[GameXngine] dataDirectory() called";
  OutputDebugStringA("[GameXngine] dataDirectory() called\n");
  QDir dir = gameDirectory();
  QDir dataDir = dir.absoluteFilePath("data");
  qInfo().noquote() << "[GameXngine] dataDirectory() path=" << dataDir.absolutePath();
  OutputDebugStringA("[GameXngine] dataDirectory() computed\n");
  return dataDir;
}

void GameXngine::setGamePath(const QString& path)
{
  qInfo().noquote() << "[GameXngine] setGamePath() ENTRY";
  m_GamePath = path;
}

QDir GameXngine::documentsDirectory() const
{
  qInfo().noquote() << "[GameXngine] documentsDirectory() called path=" << m_GamePath;
  OutputDebugStringA("[GameXngine] documentsDirectory() called\n");
  return gameDirectory();
}

QDir GameXngine::savesDirectory() const
{
  bool hasProfile = false;
  try {
    if (m_Organizer && m_Organizer->profile()) {
      hasProfile = true;
    }
  } catch (...) {
    hasProfile = false;
  }

  if (!hasProfile) {
    return gameDirectory();
  }

  const auto profile = profilePath();
  if (profile.isEmpty()) {
    return gameDirectory();
  }

  const auto layout = saveLayout();
  const auto paths = resolveSaveStorage(profile, saveGameId());
  if (!layout.baseRelativePaths.empty()) {
    return QDir(QDir(paths.gameSavesRoot).filePath(layout.baseRelativePaths.front()));
  }
  return QDir(paths.gameSavesRoot);
}

std::vector<std::shared_ptr<const MOBase::ISaveGame>>
GameXngine::listSaves(QDir folder) const
{
  qInfo().noquote() << "[GameXngine] listSaves() ENTRY folder=" << folder.absolutePath();
  OutputDebugStringA(("[GameXngine] listSaves() ENTRY - gameName='" + gameName().toStdString() + "'\n").c_str());
  OutputDebugStringA(("[GameXngine] listSaves() folder='" + folder.absolutePath().toStdString() + "'\n").c_str());
  const auto profile = profilePath();
  if (profile.isEmpty()) {
    qInfo().noquote() << "[GameXngine] listSaves() - profile path not available";
    return {};
  }
  const auto layout = saveLayout();
  const auto paths = resolveSaveStorage(profile, saveGameId());
  ensureSaveDirsExist(paths, layout);

  QStringList filters;
  const auto ext = savegameExtension();
  if (!ext.isEmpty()) {
    filters << QString("*.") + ext;
  }

  std::vector<std::shared_ptr<const MOBase::ISaveGame>> saves;
  try {
    const auto saveSlots = enumerateSaveSlots(paths, layout);
    for (const auto& slot : saveSlots) {
      QDir slotDir(slot.absolutePath);
      const auto entries = ext.isEmpty() ? slotDir.entryInfoList(QDir::Files)
                  : slotDir.entryInfoList(filters, QDir::Files);
      for (auto info : entries) {
        try {
          saves.push_back(makeSaveGame(info.filePath()));
        } catch (std::exception&) {
          qWarning().noquote() << "[GameXngine] listSaves() - exception parsing save, skipping";
          OutputDebugStringA("[GameXngine] listSaves() - exception parsing save, skipping\n");
          continue;
        }
      }
    }
  } catch (std::exception&) {
    qWarning().noquote() << "[GameXngine] listSaves() - exception listing saves, returning empty";
    OutputDebugStringA("[GameXngine] listSaves() - exception listing saves, returning empty\n");
    return {};
  } catch (...) {
    qWarning().noquote() << "[GameXngine] listSaves() - unknown exception, returning empty";
    OutputDebugStringA("[GameXngine] listSaves() - unknown exception, returning empty\n");
    return {};
  }

  return saves;
}

void GameXngine::setGameVariant(const QString& variant)
{
  qInfo().noquote() << "[GameXngine] setGameVariant() ENTRY";
  m_GameVariant = variant;
}

QString GameXngine::binaryName() const
{
  qInfo().noquote() << "[GameXngine] binaryName() ENTRY";
  return gameShortName() + ".exe";
}

MOBase::IPluginGame::LoadOrderMechanism GameXngine::loadOrderMechanism() const
{
  qInfo().noquote() << "[GameXngine] loadOrderMechanism() ENTRY";
  return LoadOrderMechanism::FileTime;
}

MOBase::IPluginGame::SortMechanism GameXngine::sortMechanism() const
{
  qInfo().noquote() << "[GameXngine] sortMechanism() ENTRY";
  return SortMechanism::LOOT;
}

bool GameXngine::looksValid(QDir const& path) const
{
  qInfo().noquote() << "[GameXngine] looksValid() ENTRY path=" << path.absolutePath();
  // Check for <prog>.exe for now.
  OutputDebugStringA(("[GameXngine] looksValid() ENTRY - gameName='" + gameName().toStdString() + "'\n").c_str());
  OutputDebugStringA(("[GameXngine] looksValid() path='" + path.absolutePath().toStdString() + "'\n").c_str());
  const auto exists = path.exists(binaryName());
  qInfo().noquote() << "[GameXngine] looksValid() binary=" << binaryName() << " exists=" << (exists ? "true" : "false");
  OutputDebugStringA(("[GameXngine] looksValid() binary='" + binaryName().toStdString() + "' exists=" + (exists ? "true" : "false") + "\n").c_str());
  return exists;
}

QString GameXngine::gameVersion() const
{
  // We try the file version, but if it looks invalid (starts with the fallback
  // version), we look the product version instead. If the product version is
  // not empty, we use it.
  QString binaryAbsPath = gameDirectory().absoluteFilePath(binaryName());
  qInfo().noquote() << "[GameXngine] gameVersion() ENTRY binary=" << binaryAbsPath;
  OutputDebugStringA("[GameXngine] gameVersion() ENTRY\n");
  QFileInfo binaryInfo(binaryAbsPath);
  if (!binaryInfo.exists()) {
    qWarning().noquote() << "[GameXngine] gameVersion() - binary not found, using fallback";
    OutputDebugStringA("[GameXngine] gameVersion() - binary not found, using fallback\n");
    return FALLBACK_GAME_VERSION;
  }
  if (!isPeExecutable(binaryAbsPath)) {
    qWarning().noquote() << "[GameXngine] gameVersion() - non-PE binary, using fallback";
    OutputDebugStringA("[GameXngine] gameVersion() - non-PE binary, using fallback\n");
    return FALLBACK_GAME_VERSION;
  }

  QString version       = MOBase::getFileVersion(binaryAbsPath);
  if (version.startsWith(FALLBACK_GAME_VERSION)) {
    QString pversion = MOBase::getProductVersion(binaryAbsPath);
    if (!pversion.isEmpty()) {
      version = pversion;
    }
  }
  return version;
}

QString GameXngine::getLauncherName() const
{
  qInfo().noquote() << "[GameXngine] getLauncherName() ENTRY";
  return gameShortName() + "Launcher.exe";
}

WORD GameXngine::getArch(QString const& program) const
{
  qInfo().noquote() << "[GameXngine] getArch() ENTRY program=" << program;
  OutputDebugStringA(("[GameXngine] getArch() ENTRY - program='" + program.toStdString() + "'\n").c_str());
  WORD arch = 0;
  // This *really* needs to be factored out
  QString absPath = this->gameDirectory().absoluteFilePath(program);
  qInfo().noquote() << "[GameXngine] getArch() absPath=" << absPath;
  OutputDebugStringA(("[GameXngine] getArch() absPath='" + absPath.toStdString() + "'\n").c_str());
  if (program.isEmpty()) {
    qWarning().noquote() << "[GameXngine] getArch() - empty program, returning 0";
    OutputDebugStringA("[GameXngine] getArch() - empty program, returning 0\n");
    return arch;
  }
  if (!isPeExecutable(absPath)) {
    qWarning().noquote() << "[GameXngine] getArch() - non-PE binary, returning 0";
    OutputDebugStringA("[GameXngine] getArch() - non-PE binary, returning 0\n");
    return arch;
  }
  std::wstring app_name = L"\\?\\" + QDir::toNativeSeparators(absPath).toStdWString();

  WIN32_FIND_DATAW FindFileData;
  HANDLE hFind = ::FindFirstFileW(app_name.c_str(), &FindFileData);

  // exit if the binary was not found
  if (hFind == INVALID_HANDLE_VALUE)
    return arch;

  HANDLE hFile            = INVALID_HANDLE_VALUE;
  HANDLE hMapping         = INVALID_HANDLE_VALUE;
  LPVOID addrHeader       = nullptr;
  PIMAGE_NT_HEADERS peHdr = nullptr;

  hFile = CreateFileW(app_name.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                      OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
  if (hFile == INVALID_HANDLE_VALUE)
    goto cleanup;

  hMapping = CreateFileMappingW(hFile, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0,
                                program.toStdWString().c_str());
  if (hMapping == INVALID_HANDLE_VALUE)
    goto cleanup;

  addrHeader = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
  if (addrHeader == NULL)
    goto cleanup;  // couldn't memory map the file

  peHdr = ImageNtHeader(addrHeader);
  if (peHdr == NULL)
    goto cleanup;  // couldn't read the header

  arch = peHdr->FileHeader.Machine;

cleanup:  // release all of our handles
  FindClose(hFind);
  if (hFile != INVALID_HANDLE_VALUE)
    CloseHandle(hFile);
  if (hMapping != INVALID_HANDLE_VALUE)
    CloseHandle(hMapping);
  return arch;
}

QFileInfo GameXngine::findInGameFolder(const QString& relativePath) const
{
  qInfo().noquote() << "[GameXngine] findInGameFolder() ENTRY";
  return QFileInfo(m_GamePath + "/" + relativePath);
}

QString GameXngine::identifyGamePath() const
{
  qInfo().noquote() << "[GameXngine] identifyGamePath() ENTRY";
  return QString();
}

bool GameXngine::prepareIni(const QString&)
{
  qInfo().noquote() << "[GameXngine] prepareIni() ENTRY";
  return true;
}

QString GameXngine::selectedVariant() const
{
  qInfo().noquote() << "[GameXngine] selectedVariant() ENTRY";
  return m_GameVariant;
}

QString GameXngine::myGamesPath() const
{
  qInfo().noquote() << "[GameXngine] myGamesPath() ENTRY";
  return m_MyGamesPath;
}

bool GameXngine::unpackXngineBsaArchive(const QString& archivePath,
                                        const QString& outputDirectory,
                                        QString* errorMessage) const
{
  return XngineBSAFormat::unpackToDirectory(archivePath, outputDirectory, errorMessage,
                                            bsaTraits());
}

bool GameXngine::packXngineBsaArchive(const QString& inputDirectory,
                                      const QString& archivePath,
                                      XngineBSAFormat::IndexType type,
                                      QString* errorMessage) const
{
  return XngineBSAFormat::packFromDirectory(inputDirectory, archivePath, type,
                                            errorMessage, bsaTraits());
}

bool GameXngine::packXngineBsaArchiveFromManifest(const QString& inputDirectory,
                                                  const QString& manifestFilePath,
                                                  const QString& archivePath,
                                                  XngineBSAFormat::IndexType type,
                                                  QString* errorMessage) const
{
  return XngineBSAFormat::packFromManifestFile(inputDirectory, manifestFilePath,
                                               archivePath, type, errorMessage,
                                               bsaTraits());
}

std::optional<XngineBSAFormat::FileSpec>
GameXngine::bsaFileSpecForArchiveName(const QString& archiveName) const
{
  const QString lookup = QFileInfo(archiveName).fileName();
  for (const auto& spec : bsaFileSpecs()) {
    if (spec.archiveName.compare(lookup, Qt::CaseInsensitive) == 0) {
      return spec;
    }
  }
  return std::nullopt;
}

bool GameXngine::unpackKnownXngineBsaArchive(const QString& archivePath,
                                             const QString& outputDirectory,
                                             QString* errorMessage) const
{
  return unpackXngineBsaArchive(archivePath, outputDirectory, errorMessage);
}

bool GameXngine::packKnownXngineBsaArchive(const QString& inputDirectory,
                                           const QString& archivePath,
                                           QString* errorMessage) const
{
  const auto spec = bsaFileSpecForArchiveName(archivePath);
  if (!spec.has_value() || !spec->indexTypeKnown) {
    if (errorMessage != nullptr) {
      *errorMessage =
          QString("No known BSA index type mapping for archive: %1")
              .arg(QFileInfo(archivePath).fileName());
    }
    return false;
  }

  const QString manifestPath =
      QDir(inputDirectory).filePath("xngine_bsa_manifest.tsv");
  if (QFileInfo::exists(manifestPath)) {
    return packXngineBsaArchiveFromManifest(inputDirectory, manifestPath, archivePath,
                                            spec->indexType, errorMessage);
  }

  return packXngineBsaArchive(inputDirectory, archivePath, spec->indexType,
                              errorMessage);
}

QString GameXngine::profilePath() const
{
  if (!m_Organizer) {
    return {};
  }
  try {
    auto profile = m_Organizer->profile();
    if (!profile) {
      return {};
    }
    return QDir::toNativeSeparators(profile->absolutePath());
  } catch (...) {
    // MO2 can throw when no profile is set; return empty and let caller handle fallback.
    return {};
  }
}

QString GameXngine::saveSlotPrefix() const
{
  return "SAVE";
}

XngineBSAFormat::Traits GameXngine::bsaTraits() const
{
  return {};
}

QVector<XngineBSAFormat::FileSpec> GameXngine::bsaFileSpecs() const
{
  return {};
}

/*static*/ QString GameXngine::getLootPath()
{
  qInfo().noquote() << "[GameXngine] getLootPath() ENTRY";
  return findInRegistry(HKEY_LOCAL_MACHINE, L"Software\\LOOT", L"Installed Path") +
         "/Loot.exe";
}

QString GameXngine::localAppFolder()
{
  qInfo().noquote() << "[GameXngine] localAppFolder() ENTRY";
  QString result = getKnownFolderPath(FOLDERID_LocalAppData, false);
  if (result.isEmpty()) {
    // fallback: try the registry
    result = getSpecialPath("Local AppData");
  }
  return result;
}

void GameXngine::copyToProfile(QString const& sourcePath,
                                 QDir const& destinationDirectory,
                                 QString const& sourceFileName)
{
  qInfo().noquote() << "[GameXngine] copyToProfile() ENTRY (3 args)";
  copyToProfile(sourcePath, destinationDirectory, sourceFileName, sourceFileName);
}

void GameXngine::copyToProfile(QString const& sourcePath,
                                 QDir const& destinationDirectory,
                                 QString const& sourceFileName,
                                 QString const& destinationFileName)
{
  qInfo().noquote() << "[GameXngine] copyToProfile() ENTRY (4 args)";
  QString filePath = destinationDirectory.absoluteFilePath(destinationFileName);
  if (!QFileInfo(filePath).exists()) {
    if (!MOBase::shellCopy(sourcePath + "/" + sourceFileName, filePath)) {
      // if copy file fails, create the file empty
      QFile(filePath).open(QIODevice::WriteOnly);
    }
  }
}

MappingType GameXngine::mappings() const
{
  try {
    qInfo().noquote() << "[GameXngine] mappings() ENTRY";
    const auto profile = profilePath();
    if (profile.isEmpty()) {
      return {};
    }

    const auto layout = saveLayout();
    const auto paths = resolveSaveStorage(profile, saveGameId());
    ensureSaveDirsExist(paths, layout);

    MappingType out;
    const QDir gameDir = gameDirectory();

    if (layout.maxSlotHint) {
      for (int i = 0; i <= *layout.maxSlotHint; ++i) {
        QString slotName;
        if (layout.slotWidthHint > 1) {
          slotName = QString("%1%2").arg(saveSlotPrefix()).arg(i, layout.slotWidthHint, 10, QChar('0'));
        } else {
          slotName = QString("%1%2").arg(saveSlotPrefix()).arg(i);
        }
        QString source = QDir(paths.gameSavesRoot).filePath(slotName);
        QString target = gameDir.absoluteFilePath(slotName);
        out.push_back({source, target, true, true});
      }
    } else {
      for (const auto& baseRel : layout.baseRelativePaths) {
        QString source = QDir(paths.gameSavesRoot).filePath(baseRel);
        QString target = gameDir.absoluteFilePath(baseRel);
        out.push_back({source, target, true, true});
      }
    }

    return out;
  } catch (...) {
    return {};
  }
}

/*static*/ SaveStoragePaths GameXngine::resolveSaveStorage(const QString& profilePath,
                                                           const QString& gameId)
{
  SaveStoragePaths p;
  p.profileRoot = QDir::toNativeSeparators(profilePath);
  QDir profileDir(profilePath);
  QString savesRoot = profileDir.filePath("xngine/saves");
  p.savesRoot = QDir::toNativeSeparators(savesRoot);
  p.gameSavesRoot = QDir::toNativeSeparators(QDir(p.savesRoot).filePath(gameId));

  // ensure directories exist
  QDir().mkpath(p.savesRoot);
  QDir().mkpath(p.gameSavesRoot);

  return p;
}

/*static*/ std::vector<SaveSlot>
GameXngine::enumerateSaveSlots(const SaveStoragePaths& paths, const SaveLayout& layout)
{
  std::vector<SaveSlot> out;
  for (const auto& baseRel : layout.baseRelativePaths) {
    QDir baseDir(QDir(paths.gameSavesRoot).filePath(baseRel));
    if (!baseDir.exists())
      continue;
    const auto entries = baseDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo& fi : entries) {
      QRegularExpressionMatch m = layout.slotDirRegex.match(fi.fileName());
      if (!m.hasMatch())
        continue;
      bool ok = false;
      int slot = m.captured(1).toInt(&ok);
      if (!ok)
        continue;
      QDir slotDir(fi.absoluteFilePath());
      if (layout.validator && !layout.validator(slotDir))
        continue;
      SaveSlot s;
      s.slotNumber = slot;
      s.slotName = fi.fileName();
      s.absolutePath = QDir::toNativeSeparators(fi.absoluteFilePath());
      s.lastModified = fi.lastModified();
      out.push_back(s);
    }
  }

  std::sort(out.begin(), out.end(), [](const SaveSlot& a, const SaveSlot& b) {
    if (a.slotNumber != b.slotNumber)
      return a.slotNumber < b.slotNumber;
    return a.lastModified > b.lastModified;
  });

  return out;
}

/*static*/ bool GameXngine::ensureSaveDirsExist(const SaveStoragePaths& paths, const SaveLayout& layout)
{
  bool ok = true;
  QDir gameRoot(paths.gameSavesRoot);
  if (!gameRoot.exists())
    ok = QDir().mkpath(paths.gameSavesRoot);

  // For layouts with maxSlotHint, create empty slot dirs
  if (layout.maxSlotHint) {
    for (int i = 0; i <= *layout.maxSlotHint; ++i) {
      QString name;
      if (layout.slotWidthHint > 1) {
        name = QString("SAVE%1").arg(i, layout.slotWidthHint, 10, QChar('0'));
      } else {
        name = QString("SAVE%1").arg(i);
      }
      QDir slotDir(gameRoot.filePath(name));
      if (!slotDir.exists()) {
        if (!QDir().mkpath(slotDir.absolutePath()))
          ok = false;
      }
    }
  } else {
    // Ensure base relative paths exist
    for (const auto& baseRel : layout.baseRelativePaths) {
      QDir d(gameRoot.filePath(baseRel));
      if (!d.exists()) {
        if (!QDir().mkpath(d.absolutePath()))
          ok = false;
      }
    }
  }

  return ok;
}

std::unique_ptr<BYTE[]> GameXngine::getRegValue(HKEY key, LPCWSTR path, LPCWSTR value,
                                                  DWORD flags, LPDWORD type = nullptr)
{
  DWORD size = 0;
  HKEY subKey;
  LONG res = ::RegOpenKeyExW(key, path, 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY, &subKey);
  if (res != ERROR_SUCCESS) {
    res = ::RegOpenKeyExW(key, path, 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &subKey);
    if (res != ERROR_SUCCESS)
      return std::unique_ptr<BYTE[]>();
  }
  res = ::RegGetValueW(subKey, L"", value, flags, type, nullptr, &size);
  if (res == ERROR_FILE_NOT_FOUND || res == ERROR_UNSUPPORTED_TYPE) {
    return std::unique_ptr<BYTE[]>();
  }
  if (res != ERROR_SUCCESS && res != ERROR_MORE_DATA) {
    throw MOBase::MyException(
        QObject::tr("failed to query registry path (preflight): %1").arg(res, 0, 16));
  }

  std::unique_ptr<BYTE[]> result(new BYTE[size]);
  res = ::RegGetValueW(subKey, L"", value, flags, type, result.get(), &size);

  if (res != ERROR_SUCCESS) {
    throw MOBase::MyException(
        QObject::tr("failed to query registry path (read): %1").arg(res, 0, 16));
  }

  return result;
}

QString GameXngine::findInRegistry(HKEY baseKey, LPCWSTR path, LPCWSTR value)
{
  try {
    std::unique_ptr<BYTE[]> buffer =
        getRegValue(baseKey, path, value, RRF_RT_REG_SZ | RRF_NOEXPAND);
    if (!buffer) {
      return {};
    }
    const auto* raw = reinterpret_cast<const char16_t*>(buffer.get());
    if (!raw) {
      return {};
    }
    return QString::fromUtf16(raw);
  } catch (const std::exception&) {
    qWarning().noquote() << "[GameXngine] findInRegistry() exception";
    OutputDebugStringA("[GameXngine] findInRegistry() exception\n");
    return {};
  } catch (...) {
    qWarning().noquote() << "[GameXngine] findInRegistry() unknown exception";
    OutputDebugStringA("[GameXngine] findInRegistry() unknown exception\n");
    return {};
  }
}

QString GameXngine::getKnownFolderPath(REFKNOWNFOLDERID folderId, bool useDefault)
{
  PWSTR path = nullptr;
  ON_BLOCK_EXIT([&]() {
    if (path != nullptr)
      ::CoTaskMemFree(path);
  });

  if (::SHGetKnownFolderPath(folderId, useDefault ? KF_FLAG_DEFAULT_PATH : 0, NULL,
                             &path) == S_OK) {
    return QDir::fromNativeSeparators(QString::fromWCharArray(path));
  } else {
    return QString();
  }
}

QString GameXngine::getSpecialPath(const QString& name)
{
  QString base = findInRegistry(
      HKEY_CURRENT_USER,
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
      name.toStdWString().c_str());

  WCHAR temp[MAX_PATH];
  if (::ExpandEnvironmentStringsW(base.toStdWString().c_str(), temp, MAX_PATH) != 0) {
    return QString::fromWCharArray(temp);
  } else {
    return base;
  }
}

QString GameXngine::determineMyGamesPath(const QString& gameName)
{
  const QString pattern = "%1/My Games/" + gameName;

  auto tryDir = [&](const QString& dir) -> std::optional<QString> {
    if (dir.isEmpty()) {
      return {};
    }

    const auto path = pattern.arg(dir);
    if (!QFileInfo(path).exists()) {
      return {};
    }

    return path;
  };

  // a) this is the way it should work. get the configured My Documents directory
  if (auto d = tryDir(getKnownFolderPath(FOLDERID_Documents, false))) {
    return *d;
  }

  // b) if there is no <game> directory there, look in the default directory
  if (auto d = tryDir(getKnownFolderPath(FOLDERID_Documents, true))) {
    return *d;
  }

  // c) finally, look in the registry. This is discouraged
  if (auto d = tryDir(getSpecialPath("Personal"))) {
    return *d;
  }

  return {};
}

QString GameXngine::parseEpicGamesLocation(const QStringList& manifests)
{
  // Use the registry entry to find the EGL Data dir first, just in case something
  // changes
  QString manifestDir = findInRegistry(
      HKEY_LOCAL_MACHINE, L"Software\\Epic Games\\EpicGamesLauncher", L"AppDataPath");
  if (manifestDir.isEmpty())
    manifestDir = getKnownFolderPath(FOLDERID_ProgramData, false) +
                  "\\Epic\\EpicGamesLauncher\\Data\\";
  manifestDir += "Manifests";
  QDir epicManifests(manifestDir, "*.item",
                     QDir::SortFlags(QDir::Name | QDir::IgnoreCase), QDir::Files);
  if (epicManifests.exists()) {
    QDirIterator it(epicManifests);
    while (it.hasNext()) {
      QString manifestFile = it.next();
      QFile manifest(manifestFile);

      if (!manifest.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open Epic Games manifest file.");
        continue;
      }

      QByteArray manifestData = manifest.readAll();

      QJsonDocument manifestJson(QJsonDocument::fromJson(manifestData));

      if (manifests.contains(manifestJson["AppName"].toString())) {
        return manifestJson["InstallLocation"].toString();
      }
    }
  }
  return "";
}

QString GameXngine::parseSteamLocation(const QString& appid,
                                         const QString& directoryName)
{
  QString path = "Software\\Valve\\Steam";
  QString steamLocation =
      findInRegistry(HKEY_CURRENT_USER, path.toStdWString().c_str(), L"SteamPath");
  if (!steamLocation.isEmpty()) {
    QString steamLibraryLocation;
    QString steamLibraries(steamLocation + "\\" + "config" + "\\" +
                           "libraryfolders.vdf");
    if (QFile(steamLibraries).exists()) {
      std::ifstream file(steamLibraries.toStdString());
      auto root = tyti::vdf::read(file);
      for (auto child : root.childs) {
        tyti::vdf::object* library = child.second.get();
        auto apps                  = library->childs["apps"];
        if (apps->attribs.contains(appid.toStdString())) {
          steamLibraryLocation = QString::fromStdString(library->attribs["path"]);
          break;
        }
      }
    }
    if (!steamLibraryLocation.isEmpty()) {
      QString gameLocation = steamLibraryLocation + "\\" + "steamapps" + "\\" +
                             "common" + "\\" + directoryName;
      if (QDir(gameLocation).exists())
        return gameLocation;
    }
  }
  return "";
}

void GameXngine::registerFeature(std::shared_ptr<MOBase::GameFeature> feature)
{
  // priority does not matter, this is a game plugin so will get lowest priority in MO2
  if (!m_Organizer) {
    qWarning().noquote() << "[GameXngine] registerFeature() called with null organizer";
    OutputDebugStringA("[GameXngine] registerFeature() called with null organizer\n");
    return;
  }
  qInfo().noquote() << "[GameXngine] registerFeature() ENTRY";
  m_Organizer->gameFeatures()->registerFeature(this, feature, 0, true);
  qInfo().noquote() << "[GameXngine] registerFeature() EXIT";
}
