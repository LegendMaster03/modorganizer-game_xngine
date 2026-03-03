#include "xngineexepatch.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QProcess>
#include <QCoreApplication>
#include <QSet>
#include <QStandardPaths>

namespace
{
bool setError(QString* errorMessage, const QString& message)
{
  if (errorMessage != nullptr) {
    *errorMessage = message;
  }
  return false;
}

QByteArray parseHexBytes(const QString& input)
{
  QString s = input;
  s.remove(' ');
  s.remove('\t');
  s.remove('\r');
  s.remove('\n');
  s.remove(',');
  if (s.startsWith("0x", Qt::CaseInsensitive)) {
    s = s.mid(2);
  }
  if ((s.size() % 2) != 0) {
    return {};
  }
  return QByteArray::fromHex(s.toLatin1());
}

bool parseOffsetValue(const QJsonValue& value, qsizetype& outOffset)
{
  if (value.isDouble()) {
    const qint64 n = static_cast<qint64>(value.toDouble(-1));
    if (n < 0) {
      return false;
    }
    outOffset = static_cast<qsizetype>(n);
    return true;
  }
  if (value.isString()) {
    bool ok = false;
    const qlonglong n = value.toString().trimmed().toLongLong(&ok, 0);
    if (!ok || n < 0) {
      return false;
    }
    outOffset = static_cast<qsizetype>(n);
    return true;
  }
  return false;
}

bool parsePatchSetObject(const QJsonObject& obj, XngineExePatch::PatchSet& outSet,
                         QString* errorMessage)
{
  outSet = {};
  outSet.id = obj.value("id").toString().trimmed();
  outSet.name = obj.value("name").toString().trimmed();
  outSet.targetVersion = obj.value("targetVersion").toString().trimmed();
  outSet.notes = obj.value("notes").toString().trimmed();

  if (outSet.id.isEmpty()) {
    return setError(errorMessage, "Patch set has empty 'id'");
  }
  if (outSet.name.isEmpty()) {
    return setError(errorMessage, QString("Patch set '%1' has empty 'name'").arg(outSet.id));
  }

  const QJsonArray patches = obj.value("patches").toArray();
  if (patches.isEmpty()) {
    return setError(errorMessage, QString("Patch set '%1' has no patches").arg(outSet.id));
  }

  outSet.patches.reserve(patches.size());
  for (int i = 0; i < patches.size(); ++i) {
    const QJsonObject pObj = patches.at(i).toObject();
    XngineExePatch::BytePatch patch;

    if (!parseOffsetValue(pObj.value("offset"), patch.offset)) {
      return setError(errorMessage, QString("Patch set '%1' patch %2 has invalid 'offset'")
                                        .arg(outSet.id)
                                        .arg(i));
    }

    patch.description = pObj.value("description").toString().trimmed();
    const QString replacementHex = pObj.value("replacement").toString().trimmed();
    if (replacementHex.isEmpty()) {
      return setError(errorMessage, QString("Patch set '%1' patch %2 has empty 'replacement'")
                                        .arg(outSet.id)
                                        .arg(i));
    }
    patch.replacement = parseHexBytes(replacementHex);
    if (patch.replacement.isEmpty()) {
      return setError(errorMessage, QString("Patch set '%1' patch %2 has invalid replacement")
                                        .arg(outSet.id)
                                        .arg(i));
    }

    const QString expectedHex = pObj.value("expected").toString().trimmed();
    if (!expectedHex.isEmpty()) {
      patch.expected = parseHexBytes(expectedHex);
      if (patch.expected.isEmpty()) {
        return setError(errorMessage, QString("Patch set '%1' patch %2 has invalid expected")
                                          .arg(outSet.id)
                                          .arg(i));
      }
    }

    outSet.patches.push_back(patch);
  }

  return true;
}
}  // namespace

namespace XngineExePatch
{
bool loadPatchSetsFromJsonFile(const QString& jsonPath, QVector<PatchSet>& outSets,
                               QString* errorMessage)
{
  outSets.clear();
  QFile f(jsonPath);
  if (!f.open(QIODevice::ReadOnly)) {
    return setError(errorMessage, QString("Unable to open patch file: %1").arg(jsonPath));
  }

  const QByteArray jsonData = f.readAll();
  if (jsonData.trimmed().isEmpty()) {
    return setError(errorMessage, "Patch JSON is empty");
  }

  QJsonParseError parseError;
  const QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
  if (parseError.error != QJsonParseError::NoError) {
    return setError(errorMessage, QString("Patch JSON parse error: %1").arg(parseError.errorString()));
  }

  QJsonArray setsArray;
  if (doc.isArray()) {
    setsArray = doc.array();
  } else if (doc.isObject()) {
    setsArray = doc.object().value("patchSets").toArray();
  }

  if (setsArray.isEmpty()) {
    return setError(errorMessage, "Patch JSON has no patch sets");
  }

  outSets.reserve(setsArray.size());
  for (int i = 0; i < setsArray.size(); ++i) {
    const QJsonObject setObj = setsArray.at(i).toObject();
    PatchSet set;
    QString setErr;
    if (!parsePatchSetObject(setObj, set, &setErr)) {
      return setError(errorMessage, QString("Invalid patch set at index %1: %2").arg(i).arg(setErr));
    }
    outSets.push_back(set);
  }

  return validatePatchSets(outSets, errorMessage);
}

bool validatePatchSets(const QVector<PatchSet>& sets, QString* errorMessage)
{
  if (sets.isEmpty()) {
    return setError(errorMessage, "No patch sets to validate");
  }

  QSet<QString> ids;
  for (const PatchSet& set : sets) {
    if (set.id.trimmed().isEmpty()) {
      return setError(errorMessage, "Patch set has empty id");
    }
    if (ids.contains(set.id)) {
      return setError(errorMessage, QString("Duplicate patch set id '%1'").arg(set.id));
    }
    ids.insert(set.id);

    if (set.patches.isEmpty()) {
      return setError(errorMessage, QString("Patch set '%1' has no patches").arg(set.id));
    }

    for (int i = 0; i < set.patches.size(); ++i) {
      const BytePatch& p = set.patches.at(i);
      if (p.offset < 0) {
        return setError(errorMessage, QString("Patch set '%1' patch %2 has negative offset")
                                          .arg(set.id)
                                          .arg(i));
      }
      if (p.replacement.isEmpty()) {
        return setError(errorMessage, QString("Patch set '%1' patch %2 has empty replacement")
                                          .arg(set.id)
                                          .arg(i));
      }
      if (!p.expected.isEmpty() && p.expected.size() != p.replacement.size()) {
        return setError(errorMessage,
                        QString("Patch set '%1' patch %2 expected/replacement size mismatch")
                            .arg(set.id)
                            .arg(i));
      }
    }
  }

  return true;
}

bool applyPatchSets(QByteArray& exeData, const QVector<PatchSet>& sets, QString* errorMessage)
{
  for (const PatchSet& set : sets) {
    for (const BytePatch& patch : set.patches) {
      if (patch.offset < 0) {
        return setError(errorMessage, QString("Patch '%1' has negative offset").arg(patch.description));
      }
      if (patch.replacement.isEmpty()) {
        return setError(errorMessage, QString("Patch '%1' has empty replacement bytes").arg(patch.description));
      }
      if (patch.offset + patch.replacement.size() > exeData.size()) {
        return setError(errorMessage, QString("Patch '%1' exceeds file size").arg(patch.description));
      }
      if (!patch.expected.isEmpty()) {
        const QByteArray current = exeData.mid(patch.offset, patch.expected.size());
        if (current != patch.expected) {
          return setError(errorMessage, QString("Patch '%1' expected bytes mismatch at 0x%2")
                                            .arg(patch.description)
                                            .arg(static_cast<qulonglong>(patch.offset), 0, 16));
        }
      }
    }

    for (const BytePatch& patch : set.patches) {
      for (int i = 0; i < patch.replacement.size(); ++i) {
        exeData[patch.offset + i] = patch.replacement.at(i);
      }
    }
  }

  return true;
}

QString findFirstExistingFile(const QString& rootPath, const QStringList& relativeCandidates)
{
  const QDir root(rootPath);
  for (const QString& rel : relativeCandidates) {
    const QString abs = root.filePath(rel);
    if (QFileInfo::exists(abs)) {
      return abs;
    }
  }
  return {};
}

QString findFirstMatchingFileRecursive(const QString& rootPath, const QStringList& fileNames)
{
  if (!QDir(rootPath).exists()) {
    return {};
  }
  QSet<QString> names;
  for (const QString& n : fileNames) {
    names.insert(n.toLower());
  }
  QDirIterator it(rootPath, QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const QString abs = it.next();
    const QString base = QFileInfo(abs).fileName().toLower();
    if (names.contains(base)) {
      return abs;
    }
  }
  return {};
}

QString findFirstTool(const QStringList& candidates)
{
  for (const QString& c : candidates) {
    if (c.contains('/') || c.contains('\\')) {
      if (QFileInfo::exists(c)) {
        return c;
      }
      continue;
    }
    const QString found = QStandardPaths::findExecutable(c);
    if (!found.isEmpty()) {
      return found;
    }
  }
  return {};
}

QString findXdeltaTool(const QString& modRootPath, const QString& gameRootPath,
                       const QString& configuredPath)
{
  QStringList candidates;

  if (!configuredPath.trimmed().isEmpty()) {
    candidates << configuredPath.trimmed();
  }

  if (!modRootPath.isEmpty()) {
    const QDir modDir(modRootPath);
    candidates << modDir.filePath("xdelta.exe");
    candidates << modDir.filePath("xdelta3.exe");
    candidates << modDir.filePath("tools/xdelta.exe");
    candidates << modDir.filePath("tools/xdelta3.exe");
  }

  if (!gameRootPath.isEmpty()) {
    const QDir gameDir(gameRootPath);
    candidates << gameDir.filePath("xdelta.exe");
    candidates << gameDir.filePath("xdelta3.exe");
    candidates << gameDir.filePath("tools/xdelta.exe");
    candidates << gameDir.filePath("tools/xdelta3.exe");
  }

  const QString appDirPath = QCoreApplication::applicationDirPath();
  if (!appDirPath.isEmpty()) {
    const QDir appDir(appDirPath);
    candidates << appDir.filePath("xdelta.exe");
    candidates << appDir.filePath("xdelta3.exe");
    candidates << appDir.filePath("tools/xdelta.exe");
    candidates << appDir.filePath("tools/xdelta3.exe");
    candidates << appDir.filePath("tools/xdelta/xdelta.exe");
    candidates << appDir.filePath("tools/xdelta/xdelta3.exe");
    candidates << appDir.filePath("tools/xdelta3/xdelta3.exe");
    candidates << appDir.filePath("tools/xdelta3/xdelta.exe");
    candidates << appDir.filePath("plugins/xdelta.exe");
    candidates << appDir.filePath("plugins/xdelta3.exe");
    candidates << appDir.filePath("plugins/tools/xdelta.exe");
    candidates << appDir.filePath("plugins/tools/xdelta3.exe");
    candidates << appDir.filePath("plugins/xdelta/xdelta.exe");
    candidates << appDir.filePath("plugins/xdelta/xdelta3.exe");
  }

  const QString envTool = qEnvironmentVariable("XDELTA_EXE").trimmed();
  if (!envTool.isEmpty()) {
    candidates << envTool;
  }

  candidates << "xdelta.exe";
  candidates << "xdelta3.exe";
  candidates << "xdelta";
  candidates << "xdelta3";

  return findFirstTool(candidates);
}

bool applyXdeltaPatch(const QString& xdeltaTool, const QString& sourceExe,
                      const QString& patchFile, const QString& outputExe,
                      QString* errorMessage)
{
  if (xdeltaTool.isEmpty()) {
    return setError(errorMessage, "xdelta tool not found");
  }

  QProcess proc;
  proc.setProgram(xdeltaTool);
  proc.setArguments({"-d", "-s", sourceExe, patchFile, outputExe});
  proc.start();
  if (!proc.waitForStarted()) {
    return setError(errorMessage, QString("Failed to launch xdelta tool: %1").arg(xdeltaTool));
  }
  if (!proc.waitForFinished(120000)) {
    proc.kill();
    return setError(errorMessage, "xdelta patch process timed out");
  }
  if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
    const QString stderrText = QString::fromLocal8Bit(proc.readAllStandardError()).trimmed();
    const QString stdoutText = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
    QString msg = QString("xdelta failed with exit code %1").arg(proc.exitCode());
    if (!stderrText.isEmpty()) {
      msg += QString(": %1").arg(stderrText);
    } else if (!stdoutText.isEmpty()) {
      msg += QString(": %1").arg(stdoutText);
    }
    return setError(errorMessage, msg);
  }

  if (!QFileInfo::exists(outputExe)) {
    return setError(errorMessage, "xdelta reported success but output file was not created");
  }

  return true;
}

bool applyXdeltaPatchToAnyFileInTree(const QString& xdeltaTool, const QString& patchFile,
                                     const QString& baseRoot, const QString& stagedRoot,
                                     QString* matchedRelativePath, QString* errorMessage)
{
  QSet<QString> candidates;

  auto collect = [&](const QString& rootPath) {
    if (!QDir(rootPath).exists()) {
      return;
    }
    QDirIterator it(rootPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
      const QString abs = it.next();
      const QString rel = QDir(rootPath).relativeFilePath(abs);
      if (rel.compare("meta.ini", Qt::CaseInsensitive) == 0) {
        continue;
      }
      candidates.insert(QDir::cleanPath(rel));
    }
  };

  // Prefer files already staged in the temp mod, then fall back to game files.
  collect(stagedRoot);
  collect(baseRoot);

  if (candidates.isEmpty()) {
    return setError(errorMessage, "No candidate files available for xdelta patching");
  }

  for (const QString& relPath : candidates) {
    const QString stagedCandidate = QDir(stagedRoot).filePath(relPath);
    const QString baseCandidate = QDir(baseRoot).filePath(relPath);
    const QString sourcePath = QFileInfo::exists(stagedCandidate) ? stagedCandidate : baseCandidate;
    if (!QFileInfo::exists(sourcePath)) {
      continue;
    }

    const QString outPath = stagedCandidate + ".xdelta.tmp";
    QDir().mkpath(QFileInfo(outPath).absolutePath());
    QFile::remove(outPath);

    QString err;
    if (!applyXdeltaPatch(xdeltaTool, sourcePath, patchFile, outPath, &err)) {
      QFile::remove(outPath);
      continue;
    }

    if (!QFileInfo::exists(stagedCandidate)) {
      QDir().mkpath(QFileInfo(stagedCandidate).absolutePath());
    } else {
      QFile::remove(stagedCandidate);
    }
    if (!QFile::rename(outPath, stagedCandidate)) {
      QFile::remove(outPath);
      return setError(errorMessage,
                      QString("xdelta output produced but could not replace target '%1'")
                          .arg(stagedCandidate));
    }

    if (matchedRelativePath != nullptr) {
      *matchedRelativePath = relPath;
    }
    return true;
  }

  return setError(errorMessage,
                  QString("No compatible source file found for patch '%1'")
                      .arg(QFileInfo(patchFile).fileName()));
}
}  // namespace XngineExePatch
