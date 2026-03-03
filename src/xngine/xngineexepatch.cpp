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
#include <algorithm>
#include <limits>
#include <zlib.h>

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

quint16 readBE16(const uchar* p)
{
  return static_cast<quint16>((static_cast<quint16>(p[0]) << 8) | static_cast<quint16>(p[1]));
}

quint32 readBE24(const uchar* p)
{
  return (static_cast<quint32>(p[0]) << 16) | (static_cast<quint32>(p[1]) << 8) |
         static_cast<quint32>(p[2]);
}

bool readBpsNumber(const QByteArray& data, int& pos, quint64& outValue)
{
  outValue = 0;
  quint64 shift = 1;
  while (pos < data.size()) {
    const uchar x = static_cast<uchar>(data.at(pos++));
    outValue += static_cast<quint64>(x & 0x7f) * shift;
    if ((x & 0x80) != 0) {
      return true;
    }
    shift <<= 7;
    outValue += shift;
  }
  return false;
}

bool readBpsSignedNumber(const QByteArray& data, int& pos, qint64& outValue)
{
  quint64 raw = 0;
  if (!readBpsNumber(data, pos, raw)) {
    return false;
  }
  const bool negative = (raw & 1ULL) != 0ULL;
  const qint64 magnitude = static_cast<qint64>(raw >> 1);
  outValue = negative ? -magnitude : magnitude;
  return true;
}

bool readUpsNumber(const QByteArray& data, int& pos, quint64& outValue)
{
  outValue = 0;
  quint64 shift = 1;
  while (pos < data.size()) {
    const quint8 x = static_cast<quint8>(data.at(pos++));
    outValue += static_cast<quint64>(x & 0x7f) * shift;
    if (x & 0x80) {
      return true;
    }
    shift <<= 7;
    outValue += shift;
  }
  return false;
}

QStringList orderedPatchCandidates(const QSet<QString>& candidates, const QString& patchFile)
{
  QStringList ordered = candidates.values();
  ordered.sort(Qt::CaseInsensitive);

  const QFileInfo patchInfo(patchFile);
  const QString patchBase = patchInfo.completeBaseName().toLower();
  const QString patchName = patchInfo.fileName().toLower();

  auto scoreCandidate = [&](const QString& relPath) {
    const QString relLower = relPath.toLower();
    const QString fileLower = QFileInfo(relPath).fileName().toLower();
    const QString fileBase = QFileInfo(relPath).completeBaseName().toLower();

    int score = 0;
    if (!patchBase.isEmpty() && fileBase == patchBase) {
      score += 100;
    }
    if (!patchBase.isEmpty() && patchBase.contains(fileBase)) {
      score += 25;
    }
    if (patchName.contains("fall") && fileLower == "fall.exe") {
      score += 80;
    }
    if (patchName.contains("dagger") && fileLower == "dagger.exe") {
      score += 80;
    }
    if (relLower.contains("df/dagger/")) {
      score += 10;
    }
    return score;
  };

  std::stable_sort(ordered.begin(), ordered.end(), [&](const QString& a, const QString& b) {
    return scoreCandidate(a) > scoreCandidate(b);
  });
  return ordered;
}

quint32 readLE32(const QByteArray& data, int pos)
{
  return static_cast<quint32>(static_cast<quint8>(data.at(pos))) |
         (static_cast<quint32>(static_cast<quint8>(data.at(pos + 1))) << 8) |
         (static_cast<quint32>(static_cast<quint8>(data.at(pos + 2))) << 16) |
         (static_cast<quint32>(static_cast<quint8>(data.at(pos + 3))) << 24);
}

quint32 crc32ForArray(const QByteArray& data)
{
  uLong crc = ::crc32(0L, Z_NULL, 0);
  if (!data.isEmpty()) {
    crc = ::crc32(crc, reinterpret_cast<const Bytef*>(data.constData()),
                  static_cast<uInt>(data.size()));
  }
  return static_cast<quint32>(crc);
}

quint16 readLE16(const QByteArray& data, int pos)
{
  return static_cast<quint16>(static_cast<quint8>(data.at(pos))) |
         static_cast<quint16>(static_cast<quint8>(data.at(pos + 1)) << 8);
}

quint64 readLE64(const QByteArray& data, int pos)
{
  quint64 v = 0;
  for (int i = 0; i < 8; ++i) {
    v |= static_cast<quint64>(static_cast<quint8>(data.at(pos + i))) << (8 * i);
  }
  return v;
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

    // Also search common MO2 parent install roots (e.g. C:/Modding).
    const QDir parentDir = QDir(appDir.absolutePath() + "/..");
    candidates << parentDir.filePath("xdelta.exe");
    candidates << parentDir.filePath("xdelta3.exe");
    candidates << parentDir.filePath("tools/xdelta.exe");
    candidates << parentDir.filePath("tools/xdelta3.exe");
    candidates << parentDir.filePath("MO2/plugins/xdelta.exe");
    candidates << parentDir.filePath("MO2/plugins/xdelta3.exe");

    // Recursive checks for user-requested locations:
    // 1) MO2 plugin folder
    // 2) Modding root folder
    const QStringList toolNames = {"xdelta.exe", "xdelta3.exe"};
    const QString pluginTreeHit =
        findFirstMatchingFileRecursive(appDir.filePath("plugins"), toolNames);
    if (!pluginTreeHit.isEmpty()) {
      candidates << pluginTreeHit;
    }
    const QString moddingRootHit =
        findFirstMatchingFileRecursive(parentDir.absolutePath(), toolNames);
    if (!moddingRootHit.isEmpty()) {
      candidates << moddingRootHit;
    }
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

bool applyIpsPatch(const QString& sourceFile, const QString& ipsPatchFile, const QString& outputFile,
                   QString* errorMessage)
{
  QFile src(sourceFile);
  if (!src.open(QIODevice::ReadOnly)) {
    return setError(errorMessage, QString("Unable to open source file for IPS patch: %1")
                                      .arg(sourceFile));
  }
  QByteArray outData = src.readAll();
  src.close();

  QFile ips(ipsPatchFile);
  if (!ips.open(QIODevice::ReadOnly)) {
    return setError(errorMessage, QString("Unable to open IPS patch file: %1").arg(ipsPatchFile));
  }
  const QByteArray patchData = ips.readAll();
  ips.close();

  if (patchData.size() < 8 || !patchData.startsWith("PATCH")) {
    return setError(errorMessage, QString("Invalid IPS patch header: %1").arg(ipsPatchFile));
  }

  int pos = 5;
  while (pos + 3 <= patchData.size()) {
    const uchar* cur = reinterpret_cast<const uchar*>(patchData.constData() + pos);
    if (cur[0] == 'E' && cur[1] == 'O' && cur[2] == 'F') {
      pos += 3;
      break;
    }

    const quint32 offset = readBE24(cur);
    pos += 3;
    if (pos + 2 > patchData.size()) {
      return setError(errorMessage, QString("Truncated IPS patch near offset record in %1")
                                        .arg(ipsPatchFile));
    }

    const uchar* sizePtr = reinterpret_cast<const uchar*>(patchData.constData() + pos);
    const quint16 size = readBE16(sizePtr);
    pos += 2;

    if (size == 0) {
      // RLE record: [offset:3][size:2=0][rle_size:2][value:1]
      if (pos + 3 > patchData.size()) {
        return setError(errorMessage, QString("Truncated IPS RLE record in %1").arg(ipsPatchFile));
      }
      const uchar* rlePtr = reinterpret_cast<const uchar*>(patchData.constData() + pos);
      const quint16 rleSize = readBE16(rlePtr);
      const char value = patchData.at(pos + 2);
      pos += 3;

      const qsizetype endPos = static_cast<qsizetype>(offset) + static_cast<qsizetype>(rleSize);
      if (endPos < 0) {
        return setError(errorMessage, QString("Invalid IPS RLE target range in %1")
                                          .arg(ipsPatchFile));
      }
      if (outData.size() < endPos) {
        outData.resize(endPos);
      }
      for (qsizetype i = 0; i < static_cast<qsizetype>(rleSize); ++i) {
        outData[static_cast<qsizetype>(offset) + i] = value;
      }
      continue;
    }

    if (pos + size > patchData.size()) {
      return setError(errorMessage, QString("Truncated IPS data record in %1").arg(ipsPatchFile));
    }
    const qsizetype endPos = static_cast<qsizetype>(offset) + static_cast<qsizetype>(size);
    if (endPos < 0) {
      return setError(errorMessage, QString("Invalid IPS target range in %1").arg(ipsPatchFile));
    }
    if (outData.size() < endPos) {
      outData.resize(endPos);
    }
    for (qsizetype i = 0; i < static_cast<qsizetype>(size); ++i) {
      outData[static_cast<qsizetype>(offset) + i] = patchData.at(pos + static_cast<int>(i));
    }
    pos += size;
  }

  QDir().mkpath(QFileInfo(outputFile).absolutePath());
  QFile::remove(outputFile);
  QFile out(outputFile);
  if (!out.open(QIODevice::WriteOnly)) {
    return setError(errorMessage, QString("Unable to open IPS output file for write: %1")
                                      .arg(outputFile));
  }
  if (out.write(outData) != outData.size()) {
    out.close();
    return setError(errorMessage,
                    QString("Incomplete write for IPS output file: %1").arg(outputFile));
  }
  out.close();
  return true;
}

bool applyIpsPatchToAnyFileInTree(const QString& patchFile, const QString& baseRoot,
                                  const QString& stagedRoot, QString* matchedRelativePath,
                                  QString* errorMessage)
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

  collect(stagedRoot);
  collect(baseRoot);
  if (candidates.isEmpty()) {
    return setError(errorMessage, "No candidate files available for IPS patching");
  }

  const QStringList orderedCandidates = orderedPatchCandidates(candidates, patchFile);
  for (const QString& relPath : orderedCandidates) {
    const QString stagedCandidate = QDir(stagedRoot).filePath(relPath);
    const QString baseCandidate = QDir(baseRoot).filePath(relPath);
    const QString sourcePath = QFileInfo::exists(stagedCandidate) ? stagedCandidate : baseCandidate;
    if (!QFileInfo::exists(sourcePath)) {
      continue;
    }

    const QString outPath = stagedCandidate + ".ips.tmp";
    QFile::remove(outPath);
    QString err;
    if (!applyIpsPatch(sourcePath, patchFile, outPath, &err)) {
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
                      QString("IPS output produced but could not replace target '%1'")
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

bool applyBpsPatch(const QString& sourceFile, const QString& bpsPatchFile, const QString& outputFile,
                   QString* errorMessage)
{
  QFile src(sourceFile);
  if (!src.open(QIODevice::ReadOnly)) {
    return setError(errorMessage, QString("Unable to open source file for BPS patch: %1")
                                      .arg(sourceFile));
  }
  const QByteArray sourceData = src.readAll();
  src.close();

  QFile bps(bpsPatchFile);
  if (!bps.open(QIODevice::ReadOnly)) {
    return setError(errorMessage, QString("Unable to open BPS patch file: %1").arg(bpsPatchFile));
  }
  const QByteArray patchData = bps.readAll();
  bps.close();

  if (patchData.size() < 4 + 12 || !patchData.startsWith("BPS1")) {
    return setError(errorMessage, QString("Invalid BPS patch header: %1").arg(bpsPatchFile));
  }

  // BPS footer contains source CRC32, target CRC32, patch CRC32.
  const int actionEnd = patchData.size() - 12;
  const quint32 expectedSourceCrc = readLE32(patchData, actionEnd);
  const quint32 expectedTargetCrc = readLE32(patchData, actionEnd + 4);
  const quint32 expectedPatchCrc = readLE32(patchData, actionEnd + 8);

  const quint32 actualSourceCrc = crc32ForArray(sourceData);
  if (actualSourceCrc != expectedSourceCrc) {
    return setError(errorMessage,
                    QString("BPS source CRC mismatch for %1 (expected 0x%2, got 0x%3)")
                        .arg(bpsPatchFile)
                        .arg(expectedSourceCrc, 8, 16, QLatin1Char('0'))
                        .arg(actualSourceCrc, 8, 16, QLatin1Char('0')));
  }

  QByteArray patchWithoutPatchCrc = patchData.left(patchData.size() - 4);
  const quint32 actualPatchCrc = crc32ForArray(patchWithoutPatchCrc);
  if (actualPatchCrc != expectedPatchCrc) {
    return setError(errorMessage,
                    QString("BPS patch CRC mismatch for %1 (expected 0x%2, got 0x%3)")
                        .arg(bpsPatchFile)
                        .arg(expectedPatchCrc, 8, 16, QLatin1Char('0'))
                        .arg(actualPatchCrc, 8, 16, QLatin1Char('0')));
  }

  int pos = 4;

  quint64 sourceSize = 0;
  quint64 targetSize = 0;
  quint64 metadataSize = 0;
  if (!readBpsNumber(patchData, pos, sourceSize) ||
      !readBpsNumber(patchData, pos, targetSize) ||
      !readBpsNumber(patchData, pos, metadataSize)) {
    return setError(errorMessage, QString("Corrupt BPS size header: %1").arg(bpsPatchFile));
  }
  if (sourceSize != static_cast<quint64>(sourceData.size())) {
    return setError(errorMessage,
                    QString("BPS source size mismatch for %1 (expected %2, got %3)")
                        .arg(bpsPatchFile)
                        .arg(sourceSize)
                        .arg(sourceData.size()));
  }
  if (metadataSize > static_cast<quint64>((actionEnd - pos))) {
    return setError(errorMessage, QString("Invalid BPS metadata size in %1").arg(bpsPatchFile));
  }
  pos += static_cast<int>(metadataSize);

  QByteArray outputData;
  outputData.reserve(static_cast<int>(targetSize));
  qint64 sourceRelativeOffset = 0;
  qint64 targetRelativeOffset = 0;

  while (outputData.size() < static_cast<int>(targetSize)) {
    if (pos >= actionEnd) {
      return setError(errorMessage,
                      QString("BPS stream ended before target completed: %1").arg(bpsPatchFile));
    }

    quint64 command = 0;
    if (!readBpsNumber(patchData, pos, command)) {
      return setError(errorMessage, QString("Corrupt BPS action command in %1").arg(bpsPatchFile));
    }

    const int action = static_cast<int>(command & 3ULL);
    const quint64 length64 = (command >> 2) + 1ULL;
    if (length64 > static_cast<quint64>(std::numeric_limits<int>::max())) {
      return setError(errorMessage, QString("BPS action length too large in %1").arg(bpsPatchFile));
    }
    const int length = static_cast<int>(length64);
    if (outputData.size() + length > static_cast<int>(targetSize)) {
      return setError(errorMessage, QString("BPS action exceeds target size in %1").arg(bpsPatchFile));
    }

    switch (action) {
      case 0: {  // SourceRead
        const int srcPos = outputData.size();
        if (srcPos < 0 || srcPos + length > sourceData.size()) {
          return setError(errorMessage, QString("BPS SourceRead out of range in %1").arg(bpsPatchFile));
        }
        outputData.append(sourceData.constData() + srcPos, length);
        break;
      }
      case 1: {  // TargetRead (literal bytes)
        if (pos + length > actionEnd) {
          return setError(errorMessage, QString("BPS TargetRead out of patch range in %1")
                                            .arg(bpsPatchFile));
        }
        outputData.append(patchData.constData() + pos, length);
        pos += length;
        break;
      }
      case 2: {  // SourceCopy
        qint64 delta = 0;
        if (!readBpsSignedNumber(patchData, pos, delta)) {
          return setError(errorMessage, QString("Corrupt BPS SourceCopy delta in %1")
                                            .arg(bpsPatchFile));
        }
        sourceRelativeOffset += delta;
        if (sourceRelativeOffset < 0 ||
            sourceRelativeOffset + static_cast<qint64>(length) > sourceData.size()) {
          return setError(errorMessage, QString("BPS SourceCopy out of range in %1").arg(bpsPatchFile));
        }
        outputData.append(sourceData.constData() + sourceRelativeOffset, length);
        sourceRelativeOffset += length;
        break;
      }
      case 3: {  // TargetCopy
        qint64 delta = 0;
        if (!readBpsSignedNumber(patchData, pos, delta)) {
          return setError(errorMessage, QString("Corrupt BPS TargetCopy delta in %1")
                                            .arg(bpsPatchFile));
        }
        targetRelativeOffset += delta;
        if (targetRelativeOffset < 0 ||
            targetRelativeOffset >= outputData.size()) {
          return setError(errorMessage, QString("BPS TargetCopy start out of range in %1")
                                            .arg(bpsPatchFile));
        }

        // Copy byte-by-byte to preserve overlap semantics.
        for (int i = 0; i < length; ++i) {
          const qint64 readPos = targetRelativeOffset + i;
          if (readPos < 0 || readPos >= outputData.size()) {
            return setError(errorMessage, QString("BPS TargetCopy out of range in %1")
                                              .arg(bpsPatchFile));
          }
          outputData.push_back(outputData.at(static_cast<int>(readPos)));
        }
        targetRelativeOffset += length;
        break;
      }
      default:
        return setError(errorMessage, QString("Unsupported BPS action in %1").arg(bpsPatchFile));
    }
  }

  const quint32 actualTargetCrc = crc32ForArray(outputData);
  if (actualTargetCrc != expectedTargetCrc) {
    return setError(errorMessage,
                    QString("BPS target CRC mismatch for %1 (expected 0x%2, got 0x%3)")
                        .arg(bpsPatchFile)
                        .arg(expectedTargetCrc, 8, 16, QLatin1Char('0'))
                        .arg(actualTargetCrc, 8, 16, QLatin1Char('0')));
  }

  QDir().mkpath(QFileInfo(outputFile).absolutePath());
  QFile::remove(outputFile);
  QFile out(outputFile);
  if (!out.open(QIODevice::WriteOnly)) {
    return setError(errorMessage, QString("Unable to open BPS output file for write: %1")
                                      .arg(outputFile));
  }
  if (out.write(outputData) != outputData.size()) {
    out.close();
    return setError(errorMessage,
                    QString("Incomplete write for BPS output file: %1").arg(outputFile));
  }
  out.close();
  return true;
}

bool applyBpsPatchToAnyFileInTree(const QString& patchFile, const QString& baseRoot,
                                  const QString& stagedRoot, QString* matchedRelativePath,
                                  QString* errorMessage)
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

  collect(stagedRoot);
  collect(baseRoot);
  if (candidates.isEmpty()) {
    return setError(errorMessage, "No candidate files available for BPS patching");
  }

  const QStringList orderedCandidates = orderedPatchCandidates(candidates, patchFile);
  for (const QString& relPath : orderedCandidates) {
    const QString stagedCandidate = QDir(stagedRoot).filePath(relPath);
    const QString baseCandidate = QDir(baseRoot).filePath(relPath);
    const QString sourcePath = QFileInfo::exists(stagedCandidate) ? stagedCandidate : baseCandidate;
    if (!QFileInfo::exists(sourcePath)) {
      continue;
    }

    const QString outPath = stagedCandidate + ".bps.tmp";
    QFile::remove(outPath);
    QString err;
    if (!applyBpsPatch(sourcePath, patchFile, outPath, &err)) {
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
                      QString("BPS output produced but could not replace target '%1'")
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

bool applyUpsPatch(const QString& sourceFile, const QString& upsPatchFile, const QString& outputFile,
                   QString* errorMessage)
{
  QFile src(sourceFile);
  if (!src.open(QIODevice::ReadOnly)) {
    return setError(errorMessage, QString("Unable to open source file for UPS patch: %1")
                                      .arg(sourceFile));
  }
  const QByteArray sourceData = src.readAll();
  src.close();

  QFile ups(upsPatchFile);
  if (!ups.open(QIODevice::ReadOnly)) {
    return setError(errorMessage, QString("Unable to open UPS patch file: %1").arg(upsPatchFile));
  }
  const QByteArray patchData = ups.readAll();
  ups.close();

  if (patchData.size() < 4 + 12 || !patchData.startsWith("UPS1")) {
    return setError(errorMessage, QString("Invalid UPS patch header: %1").arg(upsPatchFile));
  }

  // Footer is 12 bytes of checksums (input/output/patch CRC32).
  const int actionEnd = patchData.size() - 12;
  const quint32 expectedInputCrc = readLE32(patchData, actionEnd);
  const quint32 expectedOutputCrc = readLE32(patchData, actionEnd + 4);
  const quint32 expectedPatchCrc = readLE32(patchData, actionEnd + 8);

  const quint32 actualInputCrc = crc32ForArray(sourceData);
  if (actualInputCrc != expectedInputCrc) {
    return setError(errorMessage,
                    QString("UPS input CRC mismatch for %1 (expected 0x%2, got 0x%3)")
                        .arg(upsPatchFile)
                        .arg(expectedInputCrc, 8, 16, QLatin1Char('0'))
                        .arg(actualInputCrc, 8, 16, QLatin1Char('0')));
  }

  QByteArray patchWithoutPatchCrc = patchData.left(patchData.size() - 4);
  const quint32 actualPatchCrc = crc32ForArray(patchWithoutPatchCrc);
  if (actualPatchCrc != expectedPatchCrc) {
    return setError(errorMessage,
                    QString("UPS patch CRC mismatch for %1 (expected 0x%2, got 0x%3)")
                        .arg(upsPatchFile)
                        .arg(expectedPatchCrc, 8, 16, QLatin1Char('0'))
                        .arg(actualPatchCrc, 8, 16, QLatin1Char('0')));
  }

  int pos = 4;

  quint64 inputSize = 0;
  quint64 outputSize = 0;
  if (!readUpsNumber(patchData, pos, inputSize) || !readUpsNumber(patchData, pos, outputSize)) {
    return setError(errorMessage, QString("Corrupt UPS size header: %1").arg(upsPatchFile));
  }
  if (inputSize != static_cast<quint64>(sourceData.size())) {
    return setError(errorMessage,
                    QString("UPS source size mismatch for %1 (expected %2, got %3)")
                        .arg(upsPatchFile)
                        .arg(inputSize)
                        .arg(sourceData.size()));
  }
  if (outputSize > static_cast<quint64>(std::numeric_limits<int>::max())) {
    return setError(errorMessage, QString("UPS output size too large in %1").arg(upsPatchFile));
  }

  QByteArray outputData(static_cast<int>(outputSize), '\0');
  const int copyLen = std::min(outputData.size(), sourceData.size());
  if (copyLen > 0) {
    std::copy_n(sourceData.constData(), copyLen, outputData.begin());
  }

  qint64 sourceOffset = 0;
  qint64 outputOffset = 0;

  while (pos < actionEnd) {
    quint64 relativeOffset = 0;
    if (!readUpsNumber(patchData, pos, relativeOffset)) {
      return setError(errorMessage, QString("Corrupt UPS relative offset in %1").arg(upsPatchFile));
    }
    sourceOffset += static_cast<qint64>(relativeOffset);
    outputOffset += static_cast<qint64>(relativeOffset);

    if (outputOffset < 0 || outputOffset >= outputData.size()) {
      return setError(errorMessage, QString("UPS output offset out of range in %1")
                                        .arg(upsPatchFile));
    }

    while (true) {
      if (pos >= actionEnd) {
        return setError(errorMessage, QString("Unexpected end of UPS action stream in %1")
                                          .arg(upsPatchFile));
      }
      const quint8 patchByte = static_cast<quint8>(patchData.at(pos++));

      quint8 srcByte = 0;
      if (sourceOffset >= 0 && sourceOffset < sourceData.size()) {
        srcByte = static_cast<quint8>(sourceData.at(static_cast<int>(sourceOffset)));
      }
      outputData[static_cast<int>(outputOffset)] = static_cast<char>(srcByte ^ patchByte);
      ++sourceOffset;
      ++outputOffset;

      if (patchByte == 0) {
        break;
      }
      if (outputOffset < 0 || outputOffset >= outputData.size()) {
        return setError(errorMessage, QString("UPS output write exceeded target in %1")
                                          .arg(upsPatchFile));
      }
    }
  }

  const quint32 actualOutputCrc = crc32ForArray(outputData);
  if (actualOutputCrc != expectedOutputCrc) {
    return setError(errorMessage,
                    QString("UPS output CRC mismatch for %1 (expected 0x%2, got 0x%3)")
                        .arg(upsPatchFile)
                        .arg(expectedOutputCrc, 8, 16, QLatin1Char('0'))
                        .arg(actualOutputCrc, 8, 16, QLatin1Char('0')));
  }

  QDir().mkpath(QFileInfo(outputFile).absolutePath());
  QFile::remove(outputFile);
  QFile out(outputFile);
  if (!out.open(QIODevice::WriteOnly)) {
    return setError(errorMessage, QString("Unable to open UPS output file for write: %1")
                                      .arg(outputFile));
  }
  if (out.write(outputData) != outputData.size()) {
    out.close();
    return setError(errorMessage,
                    QString("Incomplete write for UPS output file: %1").arg(outputFile));
  }
  out.close();
  return true;
}

bool applyUpsPatchToAnyFileInTree(const QString& patchFile, const QString& baseRoot,
                                  const QString& stagedRoot, QString* matchedRelativePath,
                                  QString* errorMessage)
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

  collect(stagedRoot);
  collect(baseRoot);
  if (candidates.isEmpty()) {
    return setError(errorMessage, "No candidate files available for UPS patching");
  }

  const QStringList orderedCandidates = orderedPatchCandidates(candidates, patchFile);
  for (const QString& relPath : orderedCandidates) {
    const QString stagedCandidate = QDir(stagedRoot).filePath(relPath);
    const QString baseCandidate = QDir(baseRoot).filePath(relPath);
    const QString sourcePath = QFileInfo::exists(stagedCandidate) ? stagedCandidate : baseCandidate;
    if (!QFileInfo::exists(sourcePath)) {
      continue;
    }

    const QString outPath = stagedCandidate + ".ups.tmp";
    QFile::remove(outPath);
    QString err;
    if (!applyUpsPatch(sourcePath, patchFile, outPath, &err)) {
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
                      QString("UPS output produced but could not replace target '%1'")
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

bool applyPpfPatch(const QString& sourceFile, const QString& ppfPatchFile, const QString& outputFile,
                   QString* errorMessage)
{
  QFile src(sourceFile);
  if (!src.open(QIODevice::ReadOnly)) {
    return setError(errorMessage, QString("Unable to open source file for PPF patch: %1")
                                      .arg(sourceFile));
  }
  QByteArray outData = src.readAll();
  src.close();

  QFile ppf(ppfPatchFile);
  if (!ppf.open(QIODevice::ReadOnly)) {
    return setError(errorMessage, QString("Unable to open PPF patch file: %1").arg(ppfPatchFile));
  }
  const QByteArray patchData = ppf.readAll();
  ppf.close();

  if (patchData.size() < 60 || !patchData.startsWith("PPF")) {
    return setError(errorMessage, QString("Invalid PPF patch header: %1").arg(ppfPatchFile));
  }

  int version = 0;
  if (patchData.size() >= 4 && patchData.mid(0, 4) == "PPF1") {
    version = 1;
  } else if (patchData.size() >= 4 && patchData.mid(0, 4) == "PPF2") {
    version = 2;
  } else if (patchData.size() >= 4 && patchData.mid(0, 4) == "PPF3") {
    version = 3;
  } else {
    return setError(errorMessage, QString("Unsupported PPF version in %1").arg(ppfPatchFile));
  }

  int dataStart = 0;
  int dataEnd = patchData.size();
  bool undo = false;
  bool blockCheck = false;

  auto computeDizFooterStart = [&](int lenBytes) -> int {
    if (patchData.size() < lenBytes + 4) {
      return -1;
    }
    if (patchData.mid(patchData.size() - 4, 4) != ".DIZ") {
      return -1;
    }
    int idLenPos = patchData.size() - 4 - lenBytes;
    quint64 idLen = 0;
    if (lenBytes == 2) {
      idLen = readLE16(patchData, idLenPos);
    } else {
      idLen = readLE32(patchData, idLenPos);
    }
    const qint64 footerLen = static_cast<qint64>(idLen) + 16 + lenBytes + 4;
    const qint64 start = static_cast<qint64>(patchData.size()) - footerLen;
    if (start < 0 || start > patchData.size()) {
      return -1;
    }
    return static_cast<int>(start);
  };

  if (version == 1) {
    dataStart = 56;
  } else if (version == 2) {
    if (patchData.size() < 1084) {
      return setError(errorMessage, QString("PPF2 patch too small (missing 1024-byte binblock): %1")
                                        .arg(ppfPatchFile));
    }
    dataStart = 1084;
    const int footerStart = computeDizFooterStart(4);
    if (footerStart >= 0) {
      dataEnd = footerStart;
    }
  } else {
    if (patchData.size() < 60) {
      return setError(errorMessage, QString("PPF3 header too small in %1").arg(ppfPatchFile));
    }
    blockCheck = static_cast<quint8>(patchData.at(57)) != 0;
    undo = (static_cast<quint8>(patchData.at(58)) != 0);
    if (blockCheck && patchData.size() < 1084) {
      return setError(errorMessage, QString("PPF3 patch declares blockcheck but is too small: %1")
                                        .arg(ppfPatchFile));
    }
    dataStart = blockCheck ? 1084 : 60;
    const int footerStart = computeDizFooterStart(2);
    if (footerStart >= 0) {
      dataEnd = footerStart;
    }
  }

  if (dataStart < 0 || dataStart > dataEnd || dataEnd > patchData.size()) {
    return setError(errorMessage, QString("Invalid PPF data section in %1").arg(ppfPatchFile));
  }
  if (version == 2 && outData.size() < 1024) {
    return setError(errorMessage, QString("PPF2 patch requires source file >= 1024 bytes: %1")
                                      .arg(sourceFile));
  }
  if (version == 3 && blockCheck && outData.size() < 1024) {
    return setError(errorMessage, QString("PPF3 blockcheck patch requires source file >= 1024 bytes: %1")
                                      .arg(sourceFile));
  }

  int pos = dataStart;
  while (pos < dataEnd) {
    quint64 offset = 0;
    quint8 count = 0;

    if (version == 3) {
      if (pos + 9 > dataEnd) {
        return setError(errorMessage, QString("Truncated PPF3 record header in %1")
                                          .arg(ppfPatchFile));
      }
      offset = readLE64(patchData, pos);
      pos += 8;
      count = static_cast<quint8>(patchData.at(pos++));
    } else {
      if (pos + 5 > dataEnd) {
        return setError(errorMessage, QString("Truncated PPF record header in %1")
                                          .arg(ppfPatchFile));
      }
      offset = readLE32(patchData, pos);
      pos += 4;
      count = static_cast<quint8>(patchData.at(pos++));
    }

    if (count == 0) {
      continue;
    }
    if (pos + count > dataEnd) {
      return setError(errorMessage, QString("Truncated PPF record payload in %1")
                                        .arg(ppfPatchFile));
    }
    if (offset > static_cast<quint64>(std::numeric_limits<qsizetype>::max())) {
      return setError(errorMessage, QString("PPF record offset too large in %1")
                                        .arg(ppfPatchFile));
    }

    const qsizetype qOffset = static_cast<qsizetype>(offset);
    const qsizetype qCount = static_cast<qsizetype>(count);
    const qsizetype endPos = qOffset + qCount;
    if (qOffset < 0 || endPos < qOffset) {
      return setError(errorMessage, QString("Invalid PPF record range in %1").arg(ppfPatchFile));
    }
    if (outData.size() < endPos) {
      outData.resize(endPos);
    }

    for (qsizetype i = 0; i < qCount; ++i) {
      outData[qOffset + i] = patchData.at(pos + static_cast<int>(i));
    }
    pos += count;

    if (version == 3 && undo) {
      if (pos + count > dataEnd) {
        return setError(errorMessage, QString("Truncated PPF3 undo payload in %1")
                                          .arg(ppfPatchFile));
      }
      pos += count;
    }
  }

  QDir().mkpath(QFileInfo(outputFile).absolutePath());
  QFile::remove(outputFile);
  QFile out(outputFile);
  if (!out.open(QIODevice::WriteOnly)) {
    return setError(errorMessage, QString("Unable to open PPF output file for write: %1")
                                      .arg(outputFile));
  }
  if (out.write(outData) != outData.size()) {
    out.close();
    return setError(errorMessage,
                    QString("Incomplete write for PPF output file: %1").arg(outputFile));
  }
  out.close();
  return true;
}

bool applyPpfPatchToAnyFileInTree(const QString& patchFile, const QString& baseRoot,
                                  const QString& stagedRoot, QString* matchedRelativePath,
                                  QString* errorMessage)
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

  collect(stagedRoot);
  collect(baseRoot);
  if (candidates.isEmpty()) {
    return setError(errorMessage, "No candidate files available for PPF patching");
  }

  const QStringList orderedCandidates = orderedPatchCandidates(candidates, patchFile);
  for (const QString& relPath : orderedCandidates) {
    const QString stagedCandidate = QDir(stagedRoot).filePath(relPath);
    const QString baseCandidate = QDir(baseRoot).filePath(relPath);
    const QString sourcePath = QFileInfo::exists(stagedCandidate) ? stagedCandidate : baseCandidate;
    if (!QFileInfo::exists(sourcePath)) {
      continue;
    }

    const QString outPath = stagedCandidate + ".ppf.tmp";
    QFile::remove(outPath);
    QString err;
    if (!applyPpfPatch(sourcePath, patchFile, outPath, &err)) {
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
                      QString("PPF output produced but could not replace target '%1'")
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
