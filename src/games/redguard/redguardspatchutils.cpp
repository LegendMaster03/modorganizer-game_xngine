#include "redguardspatchutils.h"

#include "redguardsmapchanges.h"
#include "redguardsmapdatabase.h"
#include "redguardsmapfile.h"
#include "redguardsrtxdatabase.h"
#include "redguardsutils.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QSet>
#include <QTextStream>
#include <QDebug>

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
    basePath       = tempRoot;
    relativeSubdir = "";
    return true;
  }

  const QString tempRedguard = QDir(tempModPath).filePath("Redguard/" + fileName);
  if (QFile::exists(tempRedguard)) {
    basePath       = tempRedguard;
    relativeSubdir = "Redguard/";
    return true;
  }

  const QString gameRoot = QDir(gameDir).filePath(fileName);
  if (QFile::exists(gameRoot)) {
    basePath       = gameRoot;
    relativeSubdir = "";
    return true;
  }

  const QString gameRedguard = QDir(gameDir).filePath("Redguard/" + fileName);
  if (QFile::exists(gameRedguard)) {
    basePath       = gameRedguard;
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

QString resolveConfiguredSystemFilename(const QString& tempModPath, const QString& gameDir,
                                        const QString& key, const QString& fallback)
{
  QString systemIniPath;
  QString systemIniSubdir;
  if (resolveBaseFilePath(tempModPath, gameDir, "SYSTEM.INI", systemIniPath, systemIniSubdir)) {
    QSettings settings(systemIniPath, QSettings::IniFormat);
    const QString configuredFileName =
        settings.value(QStringLiteral("system/%1").arg(key)).toString().trimmed();
    if (!configuredFileName.isEmpty()) {
      return QFileInfo(configuredFileName).fileName();
    }
  }

  return fallback;
}

QString resolveConfiguredRtxFilename(const QString& tempModPath, const QString& gameDir)
{
  return resolveConfiguredSystemFilename(tempModPath, gameDir, QStringLiteral("rtx_filename"),
                                         QStringLiteral("ENGLISH.RTX"));
}

QString resolveConfiguredWorldIniFilename(const QString& tempModPath, const QString& gameDir)
{
  return resolveConfiguredSystemFilename(tempModPath, gameDir, QStringLiteral("world_ini"),
                                         QStringLiteral("WORLD.INI"));
}

QString resolveConfiguredItemIniFilename(const QString& tempModPath, const QString& gameDir)
{
  return resolveConfiguredSystemFilename(tempModPath, gameDir, QStringLiteral("item_ini"),
                                         QStringLiteral("ITEM.INI"));
}

QMap<QString, QMap<QString, QMap<QString, QString>>>
parseIniChanges(const QString& changesFilePath)
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

  QString modSubdir = relativeSubdir;
  if (modSubdir.startsWith("Redguard/", Qt::CaseInsensitive)) {
    modSubdir = modSubdir.mid(9);
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
  const QString rtxFileName = resolveConfiguredRtxFilename(tempModPath, gameDir);
  QString basePath;
  QString relativeSubdir;
  if (!resolveBaseFilePath(tempModPath, gameDir, rtxFileName, basePath, relativeSubdir)) {
    qWarning().noquote() << "[GameRedguard]" << rtxFileName
                         << "not found in game path";
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

  QString modSubdir = relativeSubdir;
  if (modSubdir.startsWith("Redguard/", Qt::CaseInsensitive)) {
    modSubdir = modSubdir.mid(9);
  }
  const QString destPath = QDir(tempModPath).filePath(modSubdir + rtxFileName);
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

  const QString rtxFileName = resolveConfiguredRtxFilename(tempModPath, gameDir);
  QString rtxBasePath;
  QString rtxSubdir;
  if (!resolveBaseFilePath(tempModPath, gameDir, rtxFileName, rtxBasePath, rtxSubdir)) {
    qWarning().noquote() << "[GameRedguard]" << rtxFileName
                         << "not found for map pipeline";
    return false;
  }

  RedguardsRtxDatabase rtxDb;
  if (!rtxDb.readFile(rtxBasePath)) {
    qWarning().noquote() << "[GameRedguard] Failed to read RTX for map pipeline:" << rtxBasePath;
    return false;
  }

  RedguardsMapDatabase mapDb(rtxDb);

  const QString worldFileName = resolveConfiguredWorldIniFilename(tempModPath, gameDir);
  QString worldPath;
  QString worldSubdir;
  if (!resolveBaseFilePath(tempModPath, gameDir, worldFileName, worldPath, worldSubdir)) {
    qWarning().noquote() << "[GameRedguard]" << worldFileName
                         << "not found for map pipeline";
    return false;
  }

  const QString itemFileName = resolveConfiguredItemIniFilename(tempModPath, gameDir);
  QString itemPath;
  QString itemSubdir;
  if (!resolveBaseFilePath(tempModPath, gameDir, itemFileName, itemPath, itemSubdir)) {
    qWarning().noquote() << "[GameRedguard]" << itemFileName
                         << "not found for map pipeline";
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
  QString modMapsSubdir = relativeMapsSubdir;
  if (modMapsSubdir.startsWith("Redguard/", Qt::CaseInsensitive)) {
    modMapsSubdir = modMapsSubdir.mid(9);
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
