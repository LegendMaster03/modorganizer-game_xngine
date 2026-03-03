#include "gameredguard.h"

#include "redguardsmoddatachecker.h"
#include "redguardsmoddatacontent.h"
#include "redguardsavegame.h"
#include "redguardsmapdatabase.h"
#include "redguardsmapchanges.h"
#include "redguardsmapfile.h"
#include "redguardsrtxdatabase.h"
#include "redguardsutils.h"
#include "xngineexepatch.h"

#include <executableinfo.h>
#include <pluginsetting.h>

#include <xnginelocalsavegames.h>
#include <xnginemoddatachecker.h>
#include <xnginemoddatacontent.h>
#include <xnginesavegameinfo.h>
#include <xngineunmanagedmods.h>

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QByteArray>
#include <QStandardPaths>
#include <QSettings>
#include <QFile>
#include <QRegularExpression>
#include <QIcon>
#include <QDir>
#include <QDirIterator>
#include <QTextStream>
#include <QSet>

#include <Windows.h>
#include <winver.h>

#include "utility.h"

#include <exception>
#include <memory>
#include <stdexcept>

using namespace MOBase;

namespace {
constexpr const char* kPatchTempModPrefix = "__redguard_patch_output_";
constexpr const char* kGlobalXdeltaPathKey = "xngine/global_xdelta_exe_path";

QString profileSuffix(const QString& profilePath)
{
  if (profilePath.isEmpty()) {
    return "Default";
  }
  const QString name = QDir(profilePath).dirName();
  return name.isEmpty() ? QString("Default") : name;
}

bool ensureDir(const QString& path)
{
  QDir dir;
  return dir.mkpath(path);
}

bool removeDirRecursive(const QString& path)
{
  QDir dir(path);
  if (!dir.exists()) {
    return true;
  }
  return dir.removeRecursively();
}

QString readGlobalXdeltaPath()
{
  QSettings s;
  return s.value(kGlobalXdeltaPathKey).toString().trimmed();
}

void writeGlobalXdeltaPath(const QString& path)
{
  if (path.trimmed().isEmpty()) {
    return;
  }
  QSettings s;
  if (s.value(kGlobalXdeltaPathKey).toString().trimmed() == path.trimmed()) {
    return;
  }
  s.setValue(kGlobalXdeltaPathKey, path.trimmed());
  s.sync();
}

bool copyDirectoryContents(const QString& sourceDir, const QString& destDir)
{
  QDir src(sourceDir);
  if (!src.exists()) {
    return true;
  }

  if (!ensureDir(destDir)) {
    return false;
  }

  int copiedCount = 0;
  QDirIterator it(sourceDir, QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const QString srcFile = it.next();
    const QString relPath = src.relativeFilePath(srcFile);
    const QString dstFile = QDir(destDir).filePath(relPath);
    const QString dstDirPath = QFileInfo(dstFile).absolutePath();
    if (!ensureDir(dstDirPath)) {
      return false;
    }
    QFile::remove(dstFile);
    if (!QFile::copy(srcFile, dstFile)) {
      return false;
    }
    ++copiedCount;
  }

  qInfo().noquote() << "[GameRedguard] Copied" << copiedCount << "files from"
                    << sourceDir << "to" << destDir;

  return true;
}

bool resolveBaseFilePath(const QString& tempModPath, const QString& gameDir,
                         const QString& fileName, QString& basePath,
                         QString& relativeSubdir)
{
  const QString tempRoot = QDir(tempModPath).filePath(fileName);
  if (QFile::exists(tempRoot)) {
    basePath      = tempRoot;
    relativeSubdir = "";
    return true;
  }

  const QString tempRedguard = QDir(tempModPath).filePath("Redguard/" + fileName);
  if (QFile::exists(tempRedguard)) {
    basePath      = tempRedguard;
    relativeSubdir = "Redguard/";
    return true;
  }

  const QString gameRoot = QDir(gameDir).filePath(fileName);
  if (QFile::exists(gameRoot)) {
    basePath      = gameRoot;
    relativeSubdir = "";
    return true;
  }

  const QString gameRedguard = QDir(gameDir).filePath("Redguard/" + fileName);
  if (QFile::exists(gameRedguard)) {
    basePath      = gameRedguard;
    relativeSubdir = "Redguard/";
    return true;
  }

  return false;
}

QString findSoupPath(const QString& gameDir)
{
  const QStringList candidates = {
      QDir(gameDir).filePath("Redguard/soup386/SOUP386.DEF"),
      QDir(gameDir).filePath("soup386/SOUP386.DEF")
  };

  for (const QString& path : candidates) {
    if (QFile::exists(path)) {
      return path;
    }
  }
  return QString();
}

QString findMapsRoot(const QString& gameDir)
{
  const QStringList candidates = {
      QDir(gameDir).filePath("Redguard/maps"),
      QDir(gameDir).filePath("Redguard/MAPS"),
      QDir(gameDir).filePath("maps"),
      QDir(gameDir).filePath("MAPS")
  };

  for (const QString& path : candidates) {
    if (QDir(path).exists()) {
      return path;
    }
  }
  return QString();
}

QString firstExistingFile(const QDir& root, const QStringList& candidates)
{
  for (const auto& relPath : candidates) {
    const QString fullPath = root.filePath(relPath);
    if (QFileInfo::exists(fullPath)) {
      return fullPath;
    }
  }
  return {};
}

QString detectRedguardVersionFromExeMarker(const QString& exePath)
{
  QFile f(exePath);
  if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
    return {};
  }

  const QByteArray data = f.readAll();
  static const QByteArray marker("SOFTWARE\\Bethesda\\Redguard\\");
  int index = data.indexOf(marker);
  while (index >= 0) {
    const int start = index + marker.size();
    int end = start;
    while (end < data.size()) {
      const char c = data[end];
      if ((c >= '0' && c <= '9') || c == '.') {
        ++end;
      } else {
        break;
      }
    }

    const QByteArray rawVersion = data.mid(start, end - start);
    const QString version = QString::fromLatin1(rawVersion).trimmed();
    static const QRegularExpression versionPattern(R"(^\d+\.\d+\.\d+$)");
    if (versionPattern.match(version).hasMatch()) {
      return version;
    }

    index = data.indexOf(marker, index + 1);
  }

  return {};
}

QString detectRedguardVersionFromText(const QDir& root, const QStringList& candidates)
{
  static const QList<QRegularExpression> patterns = {
      QRegularExpression(R"(\bVersion\s*=\s*([0-9]+(?:\.[0-9]+){1,3})\b)",
                         QRegularExpression::CaseInsensitiveOption),
      QRegularExpression(R"(\bVersion\s+([0-9]+(?:\.[0-9]+){1,3})\b)",
                         QRegularExpression::CaseInsensitiveOption),
  };

  for (const auto& relPath : candidates) {
    QFile f(root.filePath(relPath));
    if (!f.exists() || !f.open(QIODevice::ReadOnly | QIODevice::Text)) {
      continue;
    }

    int linesRead = 0;
    while (!f.atEnd() && linesRead < 128) {
      const QString line = QString::fromLocal8Bit(f.readLine()).trimmed();
      ++linesRead;
      for (const auto& pattern : patterns) {
        const QRegularExpressionMatch match = pattern.match(line);
        if (match.hasMatch()) {
          return match.captured(1);
        }
      }
    }
  }

  return {};
}

QMap<QString, QMap<QString, QMap<QString, QString>>> parseIniChanges(
    const QString& changesFilePath)
{
  QMap<QString, QMap<QString, QMap<QString, QString>>> allChanges;
  QFile file(changesFilePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return allChanges;
  }

  QTextStream in(&file);
  QString currentIni;
  QString currentSection;

  while (!in.atEnd()) {
    const QString line = in.readLine();
    const QString trimmed = line.trimmed();

    if (trimmed.isEmpty() || trimmed.startsWith(";")) {
      continue;
    }

    if (!line.startsWith(' ') && !line.startsWith('\t')) {
      currentIni = trimmed;
      currentSection.clear();
      continue;
    }

    if (trimmed.startsWith('[') && trimmed.endsWith(']')) {
      currentSection = trimmed.mid(1, trimmed.length() - 2).trimmed();
      continue;
    }

    const int eqPos = trimmed.indexOf('=');
    if (eqPos > 0 && !currentIni.isEmpty()) {
      const QString key = trimmed.left(eqPos).trimmed();
      const QString value = trimmed.mid(eqPos + 1).trimmed();
      allChanges[currentIni][currentSection][key] = value;
    }
  }

  return allChanges;
}

bool applyIniChangesToFile(const QString& iniFileName,
                           const QMap<QString, QMap<QString, QString>>& sectionChanges,
                           const QString& tempModPath,
                           const QString& gameDir)
{
  QString basePath;
  QString relativeSubdir;
  if (!resolveBaseFilePath(tempModPath, gameDir, iniFileName, basePath, relativeSubdir)) {
    qWarning().noquote() << "[GameRedguard] INI base file not found:" << iniFileName;
    return false;
  }

  qInfo().noquote() << "[GameRedguard] INI base:" << basePath;

  QFile sourceFile(basePath);
  if (!sourceFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning().noquote() << "[GameRedguard] Could not read INI:" << basePath;
    return false;
  }
  const QString fileText = QString::fromLatin1(sourceFile.readAll());
  sourceFile.close();

  QStringList lines = fileText.split('\n');

  const auto findSectionRange = [&](const QString& sectionName,
                                    int& startLine, int& endLine) -> bool {
    startLine = -1;
    endLine = lines.size() - 1;
    if (sectionName.isEmpty()) {
      for (int i = 0; i < lines.size(); ++i) {
        const QString trimmed = lines[i].trimmed();
        if (trimmed.startsWith('[') && trimmed.endsWith(']')) {
          endLine = i - 1;
          return true;
        }
      }
      return true;
    }

    for (int i = 0; i < lines.size(); ++i) {
      const QString trimmed = lines[i].trimmed();
      if (trimmed.startsWith('[') && trimmed.endsWith(']')) {
        const QString header = trimmed.mid(1, trimmed.length() - 2).trimmed();
        if (header.compare(sectionName, Qt::CaseInsensitive) == 0) {
          startLine = i;
          for (int j = i + 1; j < lines.size(); ++j) {
            const QString nextTrimmed = lines[j].trimmed();
            if (nextTrimmed.startsWith('[') && nextTrimmed.endsWith(']')) {
              endLine = j - 1;
              return true;
            }
          }
          endLine = lines.size() - 1;
          return true;
        }
      }
    }
    return false;
  };

  for (auto sectionIt = sectionChanges.constBegin();
       sectionIt != sectionChanges.constEnd(); ++sectionIt) {
    const QString sectionName = sectionIt.key();
    const QMap<QString, QString>& keyValues = sectionIt.value();

    int sectionStart = -1;
    int sectionEnd = lines.size() - 1;
    const bool sectionFound = findSectionRange(sectionName, sectionStart, sectionEnd);

    QSet<QString> updatedKeys;
    int searchStart = sectionStart >= 0 ? sectionStart + 1 : 0;
    int searchEnd = sectionEnd;

    for (int i = searchStart; i <= searchEnd && i < lines.size(); ++i) {
      QString trimmedLine = lines[i].trimmed();
      if (trimmedLine.startsWith(';') || trimmedLine.startsWith('[')) {
        continue;
      }

      const int eqPos = trimmedLine.indexOf('=');
      if (eqPos <= 0) {
        continue;
      }

      const QString key = trimmedLine.left(eqPos).trimmed();
      if (keyValues.contains(key)) {
        lines[i] = key + " = " + keyValues.value(key);
        updatedKeys.insert(key);
      }
    }

    QStringList missingLines;
    for (auto kvIt = keyValues.constBegin(); kvIt != keyValues.constEnd(); ++kvIt) {
      if (!updatedKeys.contains(kvIt.key())) {
        missingLines.append(kvIt.key() + " = " + kvIt.value());
      }
    }

    if (!missingLines.isEmpty()) {
      if (sectionFound && sectionStart >= 0) {
        int insertPos = sectionEnd + 1;
        for (const QString& line : missingLines) {
          lines.insert(insertPos, line);
          ++insertPos;
        }
      } else if (!sectionName.isEmpty()) {
        if (!lines.isEmpty() && !lines.last().trimmed().isEmpty()) {
          lines.append("");
        }
        lines.append("[" + sectionName + "]");
        for (const QString& line : missingLines) {
          lines.append(line);
        }
      } else {
        if (!lines.isEmpty() && !lines.last().trimmed().isEmpty()) {
          lines.append("");
        }
        for (const QString& line : missingLines) {
          lines.append(line);
        }
      }
    }
  }

  // Strip "Redguard/" prefix for mod output since mod root is already at the data level
  QString modSubdir = relativeSubdir;
  if (modSubdir.startsWith("Redguard/", Qt::CaseInsensitive)) {
    modSubdir = modSubdir.mid(9);  // Remove "Redguard/" (9 chars)
  }
  const QString destPath = QDir(tempModPath).filePath(modSubdir + iniFileName);
  qInfo().noquote() << "[GameRedguard] Writing INI patch output:" << destPath;
  if (!modSubdir.isEmpty()) {
    if (!ensureDir(QDir(tempModPath).filePath(modSubdir))) {
      qWarning().noquote() << "[GameRedguard] Failed to create INI subdir:"
                           << modSubdir;
      return false;
    }
  }

  QFile destFile(destPath);
  if (!destFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    qWarning().noquote() << "[GameRedguard] Could not write INI:" << destPath;
    return false;
  }

  QString outputText = lines.join("\n");
  outputText = RedguardsUtils::fixUnsupportedCharacters(outputText);
  destFile.write(outputText.toLatin1());
  destFile.close();
  return true;
}

bool applyIniChanges(const QString& modPath, const QString& tempModPath,
                     const QString& gameDir)
{
  const QString changesFilePath = QDir(modPath).filePath("INI Changes.txt");
  qInfo().noquote() << "[GameRedguard] Applying INI changes from" << changesFilePath;
  const auto allChanges = parseIniChanges(changesFilePath);
  if (allChanges.isEmpty()) {
    qInfo().noquote() << "[GameRedguard] No INI changes parsed";
    return true;
  }

  bool ok = true;
  for (auto iniIt = allChanges.constBegin(); iniIt != allChanges.constEnd(); ++iniIt) {
    if (!applyIniChangesToFile(iniIt.key(), iniIt.value(), tempModPath, gameDir)) {
      ok = false;
    }
  }
  return ok;
}

bool applyRtxChanges(const QString& modPath, const QString& tempModPath,
                     const QString& gameDir)
{
  const QString changesFilePath = QDir(modPath).filePath("RTX Changes.txt");
  qInfo().noquote() << "[GameRedguard] Applying RTX changes from" << changesFilePath;
  QString basePath;
  QString relativeSubdir;
  if (!resolveBaseFilePath(tempModPath, gameDir, "ENGLISH.RTX", basePath, relativeSubdir)) {
    qWarning().noquote() << "[GameRedguard] ENGLISH.RTX not found in game path";
    return false;
  }

  RedguardsRtxDatabase rtxDb;
  if (!rtxDb.readFile(basePath)) {
    qWarning().noquote() << "[GameRedguard] Failed to read RTX:" << basePath;
    return false;
  }

  if (!rtxDb.applyChanges(changesFilePath)) {
    qWarning().noquote() << "[GameRedguard] Failed to apply RTX changes:" << changesFilePath;
    return false;
  }

  // Strip "Redguard/" prefix for mod output since mod root is already at the data level
  QString modSubdir = relativeSubdir;
  if (modSubdir.startsWith("Redguard/", Qt::CaseInsensitive)) {
    modSubdir = modSubdir.mid(9);  // Remove "Redguard/" (9 chars)
  }
  const QString destPath = QDir(tempModPath).filePath(modSubdir + "ENGLISH.RTX");
  qInfo().noquote() << "[GameRedguard] Writing RTX patch output:" << destPath;
  if (!modSubdir.isEmpty()) {
    if (!ensureDir(QDir(tempModPath).filePath(modSubdir))) {
      qWarning().noquote() << "[GameRedguard] Failed to create RTX subdir:"
                           << modSubdir;
      return false;
    }
  }

  if (!rtxDb.writeFile(destPath)) {
    qWarning().noquote() << "[GameRedguard] Failed to write RTX:" << destPath;
    return false;
  }

  return true;
}

bool applyMapChanges(const RedguardsMapChanges& mapChanges, const QString& tempModPath,
                     const QString& gameDir)
{
  if (mapChanges.isEmpty()) {
    qInfo().noquote() << "[GameRedguard] No Map Changes to apply";
    return true;
  }

  QString rtxBasePath;
  QString rtxSubdir;
  if (!resolveBaseFilePath(tempModPath, gameDir, "ENGLISH.RTX", rtxBasePath, rtxSubdir)) {
    qWarning().noquote() << "[GameRedguard] ENGLISH.RTX not found for map pipeline";
    return false;
  }

  RedguardsRtxDatabase rtxDb;
  if (!rtxDb.readFile(rtxBasePath)) {
    qWarning().noquote() << "[GameRedguard] Failed to read RTX for map pipeline:" << rtxBasePath;
    return false;
  }

  RedguardsMapDatabase mapDb(rtxDb);

  QString worldPath;
  QString worldSubdir;
  if (!resolveBaseFilePath(tempModPath, gameDir, "WORLD.INI", worldPath, worldSubdir)) {
    qWarning().noquote() << "[GameRedguard] WORLD.INI not found for map pipeline";
    return false;
  }

  QString itemPath;
  QString itemSubdir;
  if (!resolveBaseFilePath(tempModPath, gameDir, "ITEM.INI", itemPath, itemSubdir)) {
    qWarning().noquote() << "[GameRedguard] ITEM.INI not found for map pipeline";
    return false;
  }

  QString soupPath = findSoupPath(gameDir);
  if (soupPath.isEmpty()) {
    qWarning().noquote() << "[GameRedguard] SOUP386.DEF not found for map pipeline";
    return false;
  }

  if (!mapDb.readWorldFile(worldPath)) {
    qWarning().noquote() << "[GameRedguard] Failed to read WORLD.INI:" << worldPath;
    return false;
  }
  if (!mapDb.readSoupFile(soupPath)) {
    qWarning().noquote() << "[GameRedguard] Failed to read SOUP386.DEF:" << soupPath;
    return false;
  }
  if (!mapDb.readItemsFile(itemPath)) {
    qWarning().noquote() << "[GameRedguard] Failed to read ITEM.INI:" << itemPath;
    return false;
  }

  const QString mapsRoot = findMapsRoot(gameDir);
  if (mapsRoot.isEmpty()) {
    qWarning().noquote() << "[GameRedguard] MAPS directory not found for map pipeline";
    return false;
  }

  const QString relativeMapsSubdir = QDir(gameDir).relativeFilePath(mapsRoot);
  // Strip "Redguard/" prefix for mod output since mod root is already at the data level
  QString modMapsSubdir = relativeMapsSubdir;
  if (modMapsSubdir.startsWith("Redguard/", Qt::CaseInsensitive)) {
    modMapsSubdir = modMapsSubdir.mid(9);  // Remove "Redguard/" (9 chars)
  }
  const QString outputMapsRoot = QDir(tempModPath).filePath(modMapsSubdir);
  if (!ensureDir(outputMapsRoot)) {
    qWarning().noquote() << "[GameRedguard] Failed to create map output directory:" << outputMapsRoot;
    return false;
  }

  bool success = true;
  for (auto* mapFile : mapDb.mapFiles()) {
    if (!mapChanges.hasModifiedMap(mapFile->name())) {
      continue;
    }

    const QString mapPath = QDir(mapsRoot).filePath(mapFile->name() + ".RGM");
    if (!QFile::exists(mapPath)) {
      qWarning().noquote() << "[GameRedguard] Map file not found:" << mapPath;
      success = false;
      continue;
    }

    if (mapFile->isEmpty() && !mapFile->readMap(mapPath)) {
      qWarning().noquote() << "[GameRedguard] Failed to read map file:" << mapPath;
      success = false;
      continue;
    }

    const QString modifiedScript = mapFile->getModifiedScript(mapChanges);
    if (mapFile->name() == "ISLAND") {
      const QString scriptDumpPath = QDir(outputMapsRoot).filePath("ISLAND.script.txt");
      qInfo().noquote() << "[GameRedguard] Script dump target:" << scriptDumpPath;
      QFile scriptDump(scriptDumpPath);
      if (scriptDump.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&scriptDump);
        out << modifiedScript;
        scriptDump.close();
        qInfo().noquote() << "[GameRedguard] Wrote script dump:" << scriptDumpPath;
      } else {
        qWarning().noquote() << "[GameRedguard] Failed to write script dump:" << scriptDumpPath
                             << "error:" << scriptDump.errorString();
        const QString fallbackPath = QDir(tempModPath).filePath("ISLAND.script.txt");
        QFile fallbackDump(fallbackPath);
        if (fallbackDump.open(QIODevice::WriteOnly | QIODevice::Text)) {
          QTextStream out(&fallbackDump);
          out << modifiedScript;
          fallbackDump.close();
          qInfo().noquote() << "[GameRedguard] Wrote fallback script dump:" << fallbackPath;
        } else {
          qWarning().noquote() << "[GameRedguard] Failed to write fallback script dump:" << fallbackPath
                               << "error:" << fallbackDump.errorString();
        }
      }
    }
    const QString outputPath = QDir(outputMapsRoot).filePath(mapFile->name() + ".RGM");
    qInfo().noquote() << "[GameRedguard] Writing patched map:" << outputPath;
    if (!mapFile->writeMap(outputPath, modifiedScript)) {
      qWarning().noquote() << "[GameRedguard] Failed to write patched map:" << outputPath;
      success = false;
    }
  }

  return success;
}
}  // namespace

GameRedguard::GameRedguard() {
  qInfo().noquote() << "[GameRedguard] Constructor ENTRY";
  OutputDebugStringA("[GameRedguard] Constructor ENTRY\n");
  OutputDebugStringA("[GameRedguard] Constructor EXIT\n");
  qInfo().noquote() << "[GameRedguard] Constructor EXIT";
}

bool GameRedguard::init(IOrganizer* moInfo)
{
  qInfo().noquote() << "[GameRedguard] init() ENTRY";
  OutputDebugStringA("[GameRedguard] init() ENTRY\n");
  
  try {
    OutputDebugStringA("[GameRedguard] About to call GameXngine::init()\n");
    if (!GameXngine::init(moInfo)) {
      OutputDebugStringA("[GameRedguard] GameXngine::init() FAILED\n");
      qWarning().noquote() << "[GameRedguard] GameXngine::init() FAILED";
      return false;
    }
    OutputDebugStringA("[GameRedguard] GameXngine::init() SUCCESS\n");
    qInfo().noquote() << "[GameRedguard] GameXngine::init() SUCCESS";

    // Register save-related features even if optional checker setup fails.
    const QString iniForLocalSaves = iniFiles().isEmpty() ? QString{} : iniFiles().first();
    registerFeature(std::make_shared<XngineSaveGameInfo>(this));
    registerFeature(std::make_shared<XngineLocalSavegames>(this, iniForLocalSaves));
    registerFeature(std::make_shared<XngineUnmanagedMods>(this));

    // Optional checker feature: don't fail plugin init if this breaks.
    OutputDebugStringA("[GameRedguard] About to create RedguardsModDataChecker\n");
    try {
      auto checker = std::make_shared<RedguardsModDataChecker>(this);
      OutputDebugStringA("[GameRedguard] RedguardsModDataChecker created successfully\n");
      OutputDebugStringA("[GameRedguard] About to register RedguardsModDataChecker\n");
      qInfo().noquote() << "[GameRedguard] Registering RedguardsModDataChecker";
      registerFeature(checker);
      OutputDebugStringA("[GameRedguard] RedguardsModDataChecker registered successfully\n");
      qInfo().noquote() << "[GameRedguard] RedguardsModDataChecker registered successfully";
    } catch (const std::exception&) {
      OutputDebugStringA("[GameRedguard] EXCEPTION creating/registering RedguardsModDataChecker (continuing)\n");
      qWarning().noquote() << "[GameRedguard] RedguardsModDataChecker setup failed, continuing";
    } catch (...) {
      OutputDebugStringA("[GameRedguard] UNKNOWN EXCEPTION creating/registering RedguardsModDataChecker (continuing)\n");
      qWarning().noquote() << "[GameRedguard] RedguardsModDataChecker setup failed (unknown), continuing";
    }

    registerFeature(std::make_shared<RedguardsModDataContent>(m_Organizer->gameFeatures()));

    OutputDebugStringA("[GameRedguard] init() EXIT SUCCESS\n");
    qInfo().noquote() << "[GameRedguard] init() EXIT SUCCESS";
    return true;
  } catch (const std::exception& e) {
    OutputDebugStringA("[GameRedguard] EXCEPTION in init()\n");
    qWarning().noquote() << "[GameRedguard] EXCEPTION in init()";
    return false;
  } catch (...) {
    OutputDebugStringA("[GameRedguard] UNKNOWN EXCEPTION in init()\n");
    qWarning().noquote() << "[GameRedguard] UNKNOWN EXCEPTION in init()";
    return false;
  }
}

QString GameRedguard::gameName() const
{
  OutputDebugStringA("[GameRedguard] gameName() called\n");
  return "Redguard";
}

QString GameRedguard::displayGameName() const
{
  return "The Elder Scrolls Adventures: Redguard";
}

QList<ExecutableInfo> GameRedguard::executables() const
{
  OutputDebugStringA("[GameRedguard] executables() ENTRY\n");
  QList<ExecutableInfo> executables;
  QDir gameDir = gameDirectory();
  if (gameDir.path().isEmpty() || !gameDir.exists()) {
    OutputDebugStringA("[GameRedguard] executables() - game directory invalid\n");
    return executables;
  }
  
  // Steam DOSBox launcher
  QFileInfo steamDosbox(gameDir.filePath("DOSBox-0.73/dosbox.exe"));
  QFileInfo steamConfig(gameDir.filePath("DOSBox-0.73/rg.conf"));
  if (steamDosbox.exists()) {
    executables << ExecutableInfo("Redguard (Steam DOSBox Windowed)", steamDosbox)
                   .withArgument("dosbox.exe -noconsole -conf rg.conf");
    executables << ExecutableInfo("Redguard (Steam DOSBox Fullscreen)", steamDosbox)
                   .withArgument("-noconsole -conf rg.conf -fullscreen");
  }

  // GOG DOSBox launcher
  QFileInfo gogDosbox(gameDir.filePath("DOSBOX/dosbox.exe"));
  if (gogDosbox.exists()) {
    executables << ExecutableInfo("Redguard (GOG DOSBox)", gogDosbox)
                   .withArgument(R"(-conf "..\dosbox_redguard.conf" -conf "..\dosbox_redguard_single.conf" -noconsole -c exit)");
  }

  // Standalone executable if it exists
  QFileInfo redguardExe(gameDir.filePath("Redguard/REDGUARD.EXE"));
  if (redguardExe.exists()) {
    executables << ExecutableInfo("Redguard", redguardExe);
  }

  OutputDebugStringA("[GameRedguard] executables() EXIT\n");
  return executables;
}

QString GameRedguard::steamAPPId() const
{
  OutputDebugStringA("[GameRedguard] steamAPPId() called\n");
  return "1812410";
}

QString GameRedguard::gogAPPId() const
{
  OutputDebugStringA("[GameRedguard] gogAPPId() called\n");
  return "1435829617";
}

QString GameRedguard::binaryName() const
{
  OutputDebugStringA("[GameRedguard] binaryName() called\n");
  return "REDGUARD.EXE";
}

QString GameRedguard::gameShortName() const
{
  OutputDebugStringA("[GameRedguard] gameShortName() called\n");
  return "Redguard";
}

QString GameRedguard::gameNexusName() const
{
  OutputDebugStringA("[GameRedguard] gameNexusName() called\n");
  return "theelderscrollsadventuresredguard";
}

QStringList GameRedguard::validShortNames() const
{
  OutputDebugStringA("[GameRedguard] validShortNames() called\n");
  return {"redguard", "rg"};
}

QStringList GameRedguard::iniFiles() const
{
  const QStringList candidates = {
      "COMBAT.INI",
      "Redguard/COMBAT.INI",
      "ITEM.INI",
      "Redguard/ITEM.INI",
      "KEYS.INI",
      "Redguard/KEYS.INI",
      "MENU.INI",
      "Redguard/MENU.INI",
      "REGISTRY.INI",
      "Redguard/REGISTRY.INI",
      "surface.ini",
      "Redguard/surface.ini",
      "SYSTEM.INI",
      "Redguard/SYSTEM.INI",
      "WORLD.INI",
      "Redguard/WORLD.INI",
  };

  QStringList ordered;
  const QDir root = gameDirectory();
  for (const auto& candidate : candidates) {
    if (QFileInfo::exists(root.filePath(candidate))) {
      ordered.push_back(candidate);
    }
  }
  for (const auto& candidate : candidates) {
    if (!ordered.contains(candidate)) {
      ordered.push_back(candidate);
    }
  }
  return ordered;
}

QIcon GameRedguard::gameIcon() const
{
  const QString exePath = gameDirectory().absoluteFilePath("Redguard/REDGUARD.EXE");
  QIcon icon = MOBase::iconForExecutable(exePath);
  return icon.isNull() ? GameXngine::gameIcon() : icon;
}

int GameRedguard::nexusModOrganizerID() const
{
  OutputDebugStringA("[GameRedguard] nexusModOrganizerID() called\n");
  return 6220;  // Nexus MO Organizer ID for Redguard
}

int GameRedguard::nexusGameID() const
{
  OutputDebugStringA("[GameRedguard] nexusGameID() called\n");
  return 4462;  // Nexus Game ID for Redguard
}

QString GameRedguard::gameVersion() const
{
  static const QString kFallbackGameVersion = QStringLiteral("1.0.0");
  const QDir root = gameDirectory();

  // Prefer PE metadata for accuracy across releases/editions.
  const QString metadataVersion = detectGameVersion(
      {
          "Redguard/REDGUARD.EXE",
          "REDGUARD.EXE",
      },
      {
          "README.TXT",
          "Redguard/README.TXT",
          "PATCH.TXT",
          "Redguard/PATCH.TXT",
      },
      {
          QRegularExpression(R"(\bv([0-9]+(?:\.[0-9]+){1,3})\b)",
                             QRegularExpression::CaseInsensitiveOption),
          QRegularExpression(R"(\bversion\s+([0-9]+(?:\.[0-9]+){1,3})\b)",
                             QRegularExpression::CaseInsensitiveOption),
      });
  if (!metadataVersion.isEmpty() && metadataVersion != kFallbackGameVersion) {
    return metadataVersion;
  }

  // Fallback to any explicit .VER/text hint if present.
  const QString textVersion = detectRedguardVersionFromText(
      root,
      {"REDGUARD.VER", "Redguard/REDGUARD.VER", "README.TXT", "Redguard/README.TXT",
       "PATCH.TXT", "Redguard/PATCH.TXT"});
  if (!textVersion.isEmpty()) {
    return textVersion;
  }

  // Last-resort fallback: executable embedded engine marker.
  const QString redguardExe = firstExistingFile(
      root, {"Redguard/REDGUARD.EXE", "REDGUARD.EXE"});
  if (!redguardExe.isEmpty()) {
    const QString exeMarkerVersion = detectRedguardVersionFromExeMarker(redguardExe);
    if (!exeMarkerVersion.isEmpty()) {
      return exeMarkerVersion;
    }
  }

  return metadataVersion;
}

QString GameRedguard::name() const
{
  return "The Elder Scrolls Adventures: Redguard Support Plugin";
}

QString GameRedguard::localizedName() const
{
  OutputDebugStringA("[GameRedguard] localizedName() called\n");
  return tr("The Elder Scrolls Adventures: Redguard Support Plugin");
}

QString GameRedguard::author() const
{
  OutputDebugStringA("[GameRedguard] author() called\n");
  return "Legend_Master";
}

QString GameRedguard::description() const
{
  OutputDebugStringA("[GameRedguard] description() called\n");
  return tr("Adds support for the game The Elder Scrolls Adventures: Redguard");
}

VersionInfo GameRedguard::version() const
{
  OutputDebugStringA("[GameRedguard] version() called\n");
  return VersionInfo(1, 0, 0, VersionInfo::RELEASE_FINAL);
}

QList<PluginSetting> GameRedguard::settings() const
{
  OutputDebugStringA("[GameRedguard] settings() called\n");
  return {
      PluginSetting(
          "allow_dillon241_patch_mod_install",
          tr("Allow Dillon241 patch mods (About.txt / INI Changes.txt / Map Changes.txt / RTX Changes.txt)."),
          true),
      PluginSetting(
          "xdelta_enabled",
          tr("Allow .xdelta binary patch mods. Requires the XNGINE patch tool (xdelta.exe) to be installed with MO2/plugin files. WARNING: this is dangerous and may corrupt saves or game data."),
          false),
      PluginSetting(
          "xdelta_exe_path",
          tr("Optional full path to xdelta.exe/xdelta3.exe. If empty, uses a shared global xdelta path if set, otherwise automatic detection."),
          ""),
  };
}

bool GameRedguard::allowExeModInstall() const
{
  if (m_Organizer == nullptr) {
    return false;
  }
  if (!m_Organizer->pluginSetting(name(), "xdelta_enabled").toBool()) {
    return false;
  }

  QString configuredTool =
      m_Organizer->pluginSetting(name(), "xdelta_exe_path").toString().trimmed();
  if (!configuredTool.isEmpty()) {
    writeGlobalXdeltaPath(configuredTool);
  } else {
    configuredTool = readGlobalXdeltaPath();
  }
  const QString resolved =
      XngineExePatch::findXdeltaTool({}, gameDirectory().absolutePath(), configuredTool);
  if (!resolved.isEmpty()) {
    return true;
  }

  static bool warnedMissingXdelta = false;
  if (!warnedMissingXdelta) {
    qWarning().noquote()
        << "[GameRedguard] .xdelta patch setting is enabled but no xdelta tool was found."
        << "Set plugin setting 'xdelta_exe_path' or install xdelta.exe in an auto-detected location.";
    warnedMissingXdelta = true;
  }
  return false;
}

bool GameRedguard::allowDillon241PatchInstall() const
{
  if (m_Organizer == nullptr) {
    return false;
  }
  return m_Organizer->pluginSetting(name(), "allow_dillon241_patch_mod_install").toBool();
}

QString GameRedguard::identifyGamePath() const
{
  qInfo().noquote() << "[GameRedguard] identifyGamePath() ENTRY";
  OutputDebugStringA("[GameRedguard] identifyGamePath() ENTRY\n");
  try {
  // Try Steam first (using Steam App ID 1812410)
  QString steamPath = findInRegistry(HKEY_LOCAL_MACHINE,
                                     L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 1812410",
                                     L"InstallLocation");
  if (!steamPath.isEmpty()) {
    // Verify it has the Steam DOSBox structure
    if (QDir(steamPath + "/DOSBox-0.73").exists() &&
        QFile::exists(steamPath + "/DOSBox-0.73/dosbox.exe") &&
        QFile::exists(steamPath + "/Redguard/REDGUARD.EXE")) {
      qInfo().noquote() << "[GameRedguard] Steam path verified";
      OutputDebugStringA("[GameRedguard] Steam path verified\n");
      return steamPath;
    }
  }

  // Try GOG registry (using GOG Game ID 1435829617)
  QString gogPath = findInRegistry(HKEY_LOCAL_MACHINE,
                                   L"Software\\GOG.com\\Games\\1435829617",
                                   L"path");
  if (!gogPath.isEmpty()) {
    // Verify it has the GOG DOSBox structure
    if (QDir(gogPath + "/DOSBOX").exists() &&
        QFile::exists(gogPath + "/DOSBOX/dosbox.exe") &&
        QFile::exists(gogPath + "/Redguard/REDGUARD.EXE")) {
      qInfo().noquote() << "[GameRedguard] GOG registry path verified";
      OutputDebugStringA("[GameRedguard] GOG registry path verified\n");
      return gogPath;
    }
  }

  qWarning().noquote() << "[GameRedguard] identifyGamePath() EXIT (not found)";
  OutputDebugStringA("[GameRedguard] identifyGamePath() EXIT (not found)\n");
  return {};
  } catch (const std::exception&) {
    OutputDebugStringA("[GameRedguard] EXCEPTION in identifyGamePath()\n");
    return {};
  } catch (...) {
    OutputDebugStringA("[GameRedguard] UNKNOWN EXCEPTION in identifyGamePath()\n");
    return {};
  }
}

QString GameRedguard::findInRegistry(HKEY baseKey, LPCWSTR path, LPCWSTR value) const
{
  qInfo().noquote() << "[GameRedguard] findInRegistry() ENTRY";
  DWORD size = 0;
  HKEY subKey;
  LONG res = ::RegOpenKeyExW(baseKey, path, 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY, &subKey);
  if (res != ERROR_SUCCESS) {
    res = ::RegOpenKeyExW(baseKey, path, 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &subKey);
    if (res != ERROR_SUCCESS)
      return QString();
  }

  std::unique_ptr<wchar_t[]> buffer;
  res = ::RegQueryValueExW(subKey, value, nullptr, nullptr, nullptr, &size);
  if (res != ERROR_SUCCESS)
    return QString();

  buffer = std::make_unique<wchar_t[]>(size / sizeof(wchar_t) + 1);
  res = ::RegQueryValueExW(subKey, value, nullptr, nullptr,
                           reinterpret_cast<LPBYTE>(buffer.get()), &size);
  ::RegCloseKey(subKey);

  if (res != ERROR_SUCCESS)
    return QString();

  return QString::fromWCharArray(buffer.get());
}

bool GameRedguard::looksValid(QDir const& path) const
{
  qInfo().noquote() << "[GameRedguard] looksValid() called";
  OutputDebugStringA("[GameRedguard] looksValid() called\n");
  
  // Redguard has a unique structure - the executable is in a Redguard subdirectory
  // Check for either Steam structure (DOSBox-0.73) or GOG structure (DOSBOX)
  bool valid = (QDir(path.absolutePath() + "/DOSBox-0.73").exists() &&
                QFile::exists(path.absolutePath() + "/Redguard/REDGUARD.EXE")) ||
               (QDir(path.absolutePath() + "/DOSBOX").exists() &&
                QFile::exists(path.absolutePath() + "/Redguard/REDGUARD.EXE"));
  
  return valid;
}

QDir GameRedguard::dataDirectory() const
{
  qInfo().noquote() << "[GameRedguard] dataDirectory() ENTRY";
  OutputDebugStringA("[GameRedguard] dataDirectory() ENTRY\n");
  QDir gameDir = gameDirectory();
  if (gameDir.path().isEmpty() || !gameDir.exists()) {
    qWarning().noquote() << "[GameRedguard] dataDirectory() - game directory invalid:" << gameDir.absolutePath();
    OutputDebugStringA("[GameRedguard] dataDirectory() - game directory invalid\n");
    return QDir();
  }
  QDir redguardDir = gameDir.absoluteFilePath("Redguard");
  qInfo().noquote() << "[GameRedguard] dataDirectory() using Redguard subdirectory:" << redguardDir.absolutePath();
  OutputDebugStringA(("[GameRedguard] dataDirectory() path='" + redguardDir.absolutePath().toStdString() + "'\n").c_str());
  return redguardDir;
}

QDir GameRedguard::savesDirectory() const
{
  return GameXngine::savesDirectory();
}

MappingType GameRedguard::mappings() const
{
  MappingType out = GameXngine::mappings();

  const auto profile = profilePath();
  if (profile.isEmpty()) {
    return out;
  }

  const auto layout = saveLayout();
  const auto paths = resolveSaveStorage(profile, saveGameId());
  ensureSaveDirsExist(paths, layout, saveSlotPrefix());

  const QDir gameDir = gameDirectory();
  const QString sourceRoot = paths.gameSavesRoot;

  // Redguard writes save slots under SAVEGAME.* folders and some installs
  // place them in the Redguard subdirectory. Map the whole save root there too.
  out.push_back({sourceRoot, gameDir.absoluteFilePath("Redguard"), true, true});
  out.push_back({QDir(sourceRoot).filePath("SAVEGAME"),
                 gameDir.absoluteFilePath("Redguard/SAVEGAME"),
                 true,
                 true});

  // Runtime diagnostic/save logs that should travel with save output and not
  // spill into overwrite as unmanaged files.
  const QStringList logFiles = {
      QStringLiteral("BITMAP.LOG"),
      QStringLiteral("GENERAL.LOG"),
      QStringLiteral("PATH.LOG"),
      QStringLiteral("SAVEFILE.LOG"),
  };
  for (const auto& logName : logFiles) {
    out.push_back({QDir(sourceRoot).filePath(logName),
                   gameDir.absoluteFilePath(logName),
                   false,
                   false});
    out.push_back({QDir(sourceRoot).filePath(logName),
                   gameDir.absoluteFilePath(QStringLiteral("Redguard/") + logName),
                   false,
                   false});
  }

  return out;
}

QString GameRedguard::savegameExtension() const
{
  OutputDebugStringA("[GameRedguard] savegameExtension() called\n");
  return "sav";
}

QString GameRedguard::savegameSEExtension() const
{
  OutputDebugStringA("[GameRedguard] savegameSEExtension() called\n");
  return "sav";
}

std::shared_ptr<const XngineSaveGame> GameRedguard::makeSaveGame(QString filepath) const
{
  OutputDebugStringA("[GameRedguard] makeSaveGame() called\n");
  return std::make_shared<RedguardsSaveGame>(filepath, this);
}

SaveLayout GameRedguard::saveLayout() const
{
  SaveLayout layout;
  layout.baseRelativePaths = {"", "SAVEGAME"};
  layout.slotDirRegex = QRegularExpression("(?i)^SAVEGAME\\.(\\d+)$");
  layout.slotWidthHint = 3;
  layout.validator = [](const QDir& slotDir) {
    return slotDir.exists() && QFileInfo::exists(slotDir.filePath("SAVEGAME.SAV"));
  };
  return layout;
}

QString GameRedguard::saveGameId() const
{
  return "redguard";
}

bool GameRedguard::prepareIni(const QString& exec)
{
  qInfo().noquote() << "[GameRedguard] prepareIni() ENTRY";
  OutputDebugStringA("[GameRedguard] prepareIni() ENTRY\n");
  
  // First call parent implementation
  if (!GameXngine::prepareIni(exec)) {
    qWarning().noquote() << "[GameRedguard] GameXngine::prepareIni() FAILED";
    return false;
  }
  
  // Apply patch-based mods.
  if (!applyPatchMods()) {
    qWarning().noquote() << "[GameRedguard] applyPatchMods() FAILED";
    // Don't fail launch - file replacement mods should still work
    OutputDebugStringA("[GameRedguard] WARNING: Patch mod application failed but continuing launch\n");
  }
  
  qInfo().noquote() << "[GameRedguard] prepareIni() EXIT SUCCESS";
  OutputDebugStringA("[GameRedguard] prepareIni() EXIT\n");
  return true;
}

bool GameRedguard::applyPatchMods()
{
  qInfo().noquote() << "[GameRedguard] applyPatchMods() ENTRY";
  OutputDebugStringA("[GameRedguard] applyPatchMods() ENTRY\n");
  
  if (!m_Organizer) {
    qWarning().noquote() << "[GameRedguard] m_Organizer is NULL";
    return false;
  }
  
  // Get the mod list to find enabled mods
  auto* modList = m_Organizer->modList();
  if (!modList) {
    qWarning().noquote() << "[GameRedguard] modList is NULL";
    return false;
  }
  
  // Get all mods ordered by priority (load order matters for patches)
  QStringList allMods = modList->allModsByProfilePriority();
  if (allMods.isEmpty()) {
    qInfo().noquote() << "[GameRedguard] No mods in profile, nothing to apply";
    return true;
  }
  
  // Get the mods directory path
  QString modsPath = m_Organizer->modsPath();
  QString gameDir = gameDirectory().absolutePath();

  const QString tempModName =
      QString(kPatchTempModPrefix) + profileSuffix(profilePath());
  const QString tempModPath = QDir(modsPath).filePath(tempModName);
  
  qInfo().noquote() << "[GameRedguard] Scanning" << allMods.size() << "mods for patch files";
  qInfo().noquote() << "[GameRedguard] Mods path:" << modsPath;
  qInfo().noquote() << "[GameRedguard] Game directory:" << gameDir;
  
  int patchModCount = 0;
  QStringList patchFileTypes = {"INI Changes.txt", "Map Changes.txt", "RTX Changes.txt"};
  const bool allowDillon241 = allowDillon241PatchInstall();
  
  // Scan enabled mods for patch files
  int lastPatchPriority = -1;
  QList<QString> patchModsInOrder;
  for (const QString& modName : allMods) {
    // Check if mod is active
    if (!(modList->state(modName) & IModList::STATE_ACTIVE)) {
      continue;
    }
    
    QString modPath = modsPath + "/" + modName;
    QDir modDir(modPath);
    if (!modDir.exists()) {
      continue;
    }
    
    // Check for text/map patch files
    bool hasPatchFiles = false;
    for (const QString& patchFile : patchFileTypes) {
      if (QFile::exists(modDir.absoluteFilePath(patchFile))) {
        if (!hasPatchFiles) {
          qInfo().noquote() << "[GameRedguard] Found patch mod:" << modName;
          hasPatchFiles = true;
        }
        qInfo().noquote() << "[GameRedguard]   - Has" << patchFile;
      }
    }
    bool hasXdelta = false;
    QDirIterator xdeltaIt(modPath, QStringList() << "*.xdelta", QDir::Files,
                          QDirIterator::Subdirectories);
    while (xdeltaIt.hasNext()) {
      hasXdelta = true;
      if (!hasPatchFiles) {
        qInfo().noquote() << "[GameRedguard] Found xdelta patch mod:" << modName;
      }
      qInfo().noquote() << "[GameRedguard]   - Has xdelta:" << xdeltaIt.next();
    }

    if ((hasPatchFiles && allowDillon241) || (hasXdelta && allowExeModInstall())) {
      lastPatchPriority = modList->priority(modName);
      patchModCount++;
      patchModsInOrder.append(modName);
    }
  }
  
  if (patchModCount == 0) {
    qInfo().noquote() << "[GameRedguard] No patch-based mods found";
    return true;
  }
  
  // Log status
  qWarning().noquote() << "[GameRedguard] ========================================";
  qWarning().noquote() << "[GameRedguard] PATCH MODS DETECTED:" << patchModCount << "mod(s)";
  qWarning().noquote() << "[GameRedguard] ========================================";
  qWarning().noquote() << "[GameRedguard] Building temporary patch output mod:" << tempModName;
  qInfo().noquote() << "[GameRedguard] Temp mod path:" << tempModPath;
  qWarning().noquote() << "[GameRedguard] ========================================";

  if (!removeDirRecursive(tempModPath)) {
    qWarning().noquote() << "[GameRedguard] Failed to clean existing temp mod:" << tempModPath;
  }

  // Create mod entry FIRST - let MO2 create the folder
  if (!modList->getMod(tempModName)) {
    MOBase::GuessedValue<QString> guessedName(tempModName);
    auto* created = m_Organizer->createMod(guessedName);
    if (!created) {
      qWarning().noquote() << "[GameRedguard] Failed to create temp mod entry:" << tempModName;
      qWarning().noquote() << "[GameRedguard] Patches will be generated but may not be loaded by MO2";
      // Even if createMod failed, ensure the folder exists for patch writing
      if (!ensureDir(tempModPath)) {
        qWarning().noquote() << "[GameRedguard] Failed to create temp mod path:" << tempModPath;
        return false;
      }
    }
  }

  // Now ensure the folder exists (in case createMod succeeded)
  if (!ensureDir(tempModPath)) {
    qWarning().noquote() << "[GameRedguard] Failed to create temp mod path:" << tempModPath;
    return false;
  }

  // Create meta.ini for MO2 to recognize this folder as a mod
  QString metaIniPath = QDir(tempModPath).filePath("meta.ini");
  QFile metaFile(metaIniPath);
  if (!metaFile.exists()) {
    if (metaFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
      QTextStream out(&metaFile);
      out << "[General]\n";
      out << "name=" << tempModName << "\n";
      out << "version=1.0\n";
      out << "author=Mod Organizer\n";
      out << "description=Temporary patched mod output\n";
      metaFile.close();
      qInfo().noquote() << "[GameRedguard] Created meta.ini for temp mod";
    }
  }

  // Try to activate if mod is now in the mod list
  if (modList->getMod(tempModName)) {
    modList->setActive(tempModName, true);
    qInfo().noquote() << "[GameRedguard] Set temp mod active:" << tempModName;
    
    if (lastPatchPriority >= 0) {
      modList->setPriority(tempModName, lastPatchPriority + 1);
      qInfo().noquote() << "[GameRedguard] Set temp mod priority:" << (lastPatchPriority + 1);
    }
  } else {
    qWarning().noquote() << "[GameRedguard] Temp mod not found in mod list - cannot activate";
  }

  static bool cleanupRegistered = false;
  if (!cleanupRegistered) {
    cleanupRegistered = true;
    m_Organizer->onFinishedRun([tempModPath, tempModName, modList](const QString&, unsigned int) {
      if (modList) {
        modList->setActive(tempModName, false);
      }
      removeDirRecursive(tempModPath);
      qInfo().noquote() << "[GameRedguard] Cleanup complete - temp mod deleted";
    });
  }

  bool success = true;
  RedguardsMapChanges combinedMapChanges;
  bool hasMapChanges = false;
  for (const QString& modName : patchModsInOrder) {
    const QString modPath = QDir(modsPath).filePath(modName);
    qInfo().noquote() << "[GameRedguard] Applying patch mod:" << modName
                      << "from" << modPath;

    if (allowDillon241 && QFile::exists(QDir(modPath).filePath("INI Changes.txt"))) {
      if (!applyIniChanges(modPath, tempModPath, gameDir)) {
        success = false;
      }
    }

    if (allowDillon241 && QFile::exists(QDir(modPath).filePath("Map Changes.txt"))) {
      const QString changesPath = QDir(modPath).filePath("Map Changes.txt");
      qInfo().noquote() << "[GameRedguard] Parsing Map Changes from" << changesPath;
      if (!combinedMapChanges.readChanges(changesPath)) {
        qWarning().noquote() << "[GameRedguard] Failed to read Map Changes:" << changesPath;
        success = false;
      } else {
        hasMapChanges = true;
      }
    }

    if (allowDillon241 && QFile::exists(QDir(modPath).filePath("RTX Changes.txt"))) {
      if (!applyRtxChanges(modPath, tempModPath, gameDir)) {
        success = false;
      }
    }

    const QString audioSource = QDir(modPath).filePath("Audio");
    if (QDir(audioSource).exists()) {
      const QString audioDest = QDir(tempModPath).filePath("Audio");
      qInfo().noquote() << "[GameRedguard] Staging Audio ->" << audioDest;
      if (!copyDirectoryContents(audioSource, audioDest)) {
        qWarning().noquote() << "[GameRedguard] Failed to stage Audio for mod:" << modName;
        success = false;
      }
    }

    const QString texturesSource = QDir(modPath).filePath("Textures");
    if (QDir(texturesSource).exists()) {
      const QString texturesDest = QDir(tempModPath).filePath("Textures");
      qInfo().noquote() << "[GameRedguard] Staging Textures ->" << texturesDest;
      if (!copyDirectoryContents(texturesSource, texturesDest)) {
        qWarning().noquote() << "[GameRedguard] Failed to stage Textures for mod:" << modName;
        success = false;
      }
    }

    if (allowExeModInstall()) {
      QDirIterator xdeltaProbe(modPath, QStringList() << "*.xdelta", QDir::Files,
                               QDirIterator::Subdirectories);
      if (xdeltaProbe.hasNext()) {
        const QString configuredTool =
            m_Organizer->pluginSetting(name(), "xdelta_exe_path").toString().trimmed();
        QString resolvedConfiguredTool = configuredTool;
        if (!resolvedConfiguredTool.isEmpty()) {
            writeGlobalXdeltaPath(resolvedConfiguredTool);
        } else {
            resolvedConfiguredTool = readGlobalXdeltaPath();
        }
        const QString xdeltaTool =
            XngineExePatch::findXdeltaTool(modPath, gameDir, resolvedConfiguredTool);
        if (xdeltaTool.isEmpty()) {
          qWarning().noquote()
              << "[GameRedguard] xdelta tool not found for mod:" << modName
              << "- checked mod/game folders, MO2 folder/tools, XDELTA_EXE, and PATH.";
          success = false;
        } else {
          QDirIterator xdeltaIt(modPath, QStringList() << "*.xdelta", QDir::Files,
                                QDirIterator::Subdirectories);
          while (xdeltaIt.hasNext()) {
            const QString patchFile = xdeltaIt.next();
            QString matchedRel;
            QString err;
            if (!XngineExePatch::applyXdeltaPatchToAnyFileInTree(
                    xdeltaTool, patchFile, gameDir, tempModPath, &matchedRel, &err)) {
              qWarning().noquote() << "[GameRedguard] Failed applying .xdelta patch" << patchFile
                                   << "from mod" << modName << ":" << err;
              success = false;
              break;
            }
            qInfo().noquote() << "[GameRedguard] Applied .xdelta patch to" << matchedRel
                              << "from mod" << modName;
          }
        }
      }
    }
  }

  if (hasMapChanges) {
    if (!applyMapChanges(combinedMapChanges, tempModPath, gameDir)) {
      success = false;
    }
  }

  qInfo().noquote() << "[GameRedguard] applyPatchMods() EXIT";
  return success;
}
