#include "daggerfallfallexehacks.h"
#include "daggerfallformatutils.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSet>

namespace
{
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

bool parsePatchSetObject(const QJsonObject& obj, DaggerfallFallExeHacks::PatchSet& outSet,
                         QString* errorMessage)
{
  outSet = {};
  outSet.id = obj.value("id").toString().trimmed();
  outSet.name = obj.value("name").toString().trimmed();
  outSet.targetVersion = obj.value("targetVersion").toString().trimmed();
  outSet.notes = obj.value("notes").toString().trimmed();

  if (outSet.id.isEmpty()) {
    return Daggerfall::FormatUtil::setError(errorMessage, "Patch set has empty 'id'");
  }
  if (outSet.name.isEmpty()) {
    return Daggerfall::FormatUtil::setError(errorMessage,
                                            QString("Patch set '%1' has empty 'name'").arg(outSet.id));
  }

  const QJsonArray patches = obj.value("patches").toArray();
  if (patches.isEmpty()) {
    return Daggerfall::FormatUtil::setError(errorMessage,
                                            QString("Patch set '%1' has no patches").arg(outSet.id));
  }

  outSet.patches.reserve(patches.size());
  for (int i = 0; i < patches.size(); ++i) {
    const QJsonObject pObj = patches.at(i).toObject();
    DaggerfallFallExeHacks::BytePatch p;

    if (!parseOffsetValue(pObj.value("offset"), p.offset)) {
      return Daggerfall::FormatUtil::setError(
          errorMessage,
          QString("Patch set '%1' patch %2 has invalid 'offset'").arg(outSet.id).arg(i));
    }

    p.description = pObj.value("description").toString().trimmed();
    const QString replHex = pObj.value("replacement").toString().trimmed();
    if (replHex.isEmpty()) {
      return Daggerfall::FormatUtil::setError(
          errorMessage,
          QString("Patch set '%1' patch %2 has empty 'replacement'").arg(outSet.id).arg(i));
    }
    p.replacement = parseHexBytes(replHex);
    if (p.replacement.isEmpty()) {
      return Daggerfall::FormatUtil::setError(
          errorMessage,
          QString("Patch set '%1' patch %2 has invalid replacement bytes").arg(outSet.id).arg(i));
    }

    const QString expHex = pObj.value("expected").toString().trimmed();
    if (!expHex.isEmpty()) {
      p.expected = parseHexBytes(expHex);
      if (p.expected.isEmpty()) {
        return Daggerfall::FormatUtil::setError(
            errorMessage,
            QString("Patch set '%1' patch %2 has invalid expected bytes").arg(outSet.id).arg(i));
      }
    }

    outSet.patches.push_back(p);
  }

  return true;
}
}  // namespace

QVector<DaggerfallFallExeHacks::PatchSet> DaggerfallFallExeHacks::documentedPatchSets()
{
  // Intentionally empty: patch definitions should be user-provided data.
  return {};
}

DaggerfallFallExeHacks::PatchSet DaggerfallFallExeHacks::patchSetById(const QString& id)
{
  Q_UNUSED(id);
  // Deprecated API: hardcoded sets are intentionally disabled.
  return {};
}

bool DaggerfallFallExeHacks::loadPatchSetsFromJsonFile(const QString& jsonPath,
                                                       QVector<PatchSet>& outSets,
                                                       QString* errorMessage)
{
  outSets.clear();
  QFile f(jsonPath);
  if (!f.open(QIODevice::ReadOnly)) {
    return Daggerfall::FormatUtil::setError(errorMessage,
                                            QString("Unable to open patch file: %1").arg(jsonPath));
  }
  return loadPatchSetsFromJsonBytes(f.readAll(), outSets, errorMessage);
}

bool DaggerfallFallExeHacks::loadPatchSetsFromJsonBytes(const QByteArray& jsonData,
                                                        QVector<PatchSet>& outSets,
                                                        QString* errorMessage)
{
  outSets.clear();
  if (jsonData.trimmed().isEmpty()) {
    return Daggerfall::FormatUtil::setError(errorMessage, "Patch JSON is empty");
  }

  QJsonParseError parseError;
  const QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
  if (parseError.error != QJsonParseError::NoError) {
    return Daggerfall::FormatUtil::setError(
        errorMessage, QString("Patch JSON parse error: %1").arg(parseError.errorString()));
  }

  QJsonArray setsArray;
  if (doc.isArray()) {
    setsArray = doc.array();
  } else if (doc.isObject()) {
    const QJsonObject root = doc.object();
    setsArray = root.value("patchSets").toArray();
  }

  if (setsArray.isEmpty()) {
    return Daggerfall::FormatUtil::setError(errorMessage, "Patch JSON has no patch sets");
  }

  outSets.reserve(setsArray.size());
  for (int i = 0; i < setsArray.size(); ++i) {
    const QJsonObject setObj = setsArray.at(i).toObject();
    PatchSet set;
    QString setErr;
    if (!parsePatchSetObject(setObj, set, &setErr)) {
      return Daggerfall::FormatUtil::setError(
          errorMessage, QString("Invalid patch set at index %1: %2").arg(i).arg(setErr));
    }
    outSets.push_back(set);
  }

  return true;
}

bool DaggerfallFallExeHacks::validatePatchJsonFile(const QString& jsonPath, QString* errorMessage)
{
  QVector<PatchSet> sets;
  if (!loadPatchSetsFromJsonFile(jsonPath, sets, errorMessage)) {
    return false;
  }
  return validatePatchSets(sets, errorMessage);
}

bool DaggerfallFallExeHacks::validatePatchJsonBytes(const QByteArray& jsonData,
                                                    QString* errorMessage)
{
  QVector<PatchSet> sets;
  if (!loadPatchSetsFromJsonBytes(jsonData, sets, errorMessage)) {
    return false;
  }
  return validatePatchSets(sets, errorMessage);
}

bool DaggerfallFallExeHacks::validatePatchSets(const QVector<PatchSet>& sets,
                                               QString* errorMessage)
{
  if (sets.isEmpty()) {
    return Daggerfall::FormatUtil::setError(errorMessage, "No patch sets to validate");
  }

  QSet<QString> ids;
  for (const PatchSet& set : sets) {
    if (set.id.trimmed().isEmpty()) {
      return Daggerfall::FormatUtil::setError(errorMessage, "Patch set has empty id");
    }
    if (ids.contains(set.id)) {
      return Daggerfall::FormatUtil::setError(
          errorMessage, QString("Duplicate patch set id '%1'").arg(set.id));
    }
    ids.insert(set.id);

    if (set.patches.isEmpty()) {
      return Daggerfall::FormatUtil::setError(
          errorMessage, QString("Patch set '%1' has no patches").arg(set.id));
    }

    for (int i = 0; i < set.patches.size(); ++i) {
      const BytePatch& p = set.patches.at(i);
      if (p.offset < 0) {
        return Daggerfall::FormatUtil::setError(
            errorMessage, QString("Patch set '%1' patch %2 has negative offset").arg(set.id).arg(i));
      }
      if (p.replacement.isEmpty()) {
        return Daggerfall::FormatUtil::setError(
            errorMessage, QString("Patch set '%1' patch %2 has empty replacement").arg(set.id).arg(i));
      }
      if (!p.expected.isEmpty() && p.expected.size() != p.replacement.size()) {
        return Daggerfall::FormatUtil::setError(
            errorMessage, QString("Patch set '%1' patch %2 expected/replacement size mismatch")
                              .arg(set.id)
                              .arg(i));
      }
    }
  }

  return true;
}

bool DaggerfallFallExeHacks::findPatchSetById(const QVector<PatchSet>& sets, const QString& id,
                                              PatchSet& outSet)
{
  outSet = {};
  for (const PatchSet& set : sets) {
    if (set.id == id) {
      outSet = set;
      return true;
    }
  }
  return false;
}

bool DaggerfallFallExeHacks::verifyPatchSet(const QByteArray& exeData, const PatchSet& set,
                                            QString* errorMessage)
{
  for (const BytePatch& patch : set.patches) {
    if (patch.offset < 0) {
      return Daggerfall::FormatUtil::setError(
          errorMessage, QString("Patch '%1' has negative offset").arg(patch.description));
    }
    if (patch.replacement.isEmpty()) {
      return Daggerfall::FormatUtil::setError(
          errorMessage, QString("Patch '%1' has empty replacement bytes").arg(patch.description));
    }
    if (patch.offset + patch.replacement.size() > exeData.size()) {
      return Daggerfall::FormatUtil::setError(errorMessage,
                                              QString("Patch '%1' exceeds file size").arg(patch.description));
    }
    if (!patch.expected.isEmpty()) {
      if (patch.expected.size() != patch.replacement.size()) {
        return Daggerfall::FormatUtil::setError(
            errorMessage, QString("Patch '%1' expected/replacement size mismatch").arg(patch.description));
      }
      const QByteArray current = exeData.mid(patch.offset, patch.expected.size());
      if (current != patch.expected) {
        return Daggerfall::FormatUtil::setError(
            errorMessage, QString("Patch '%1' expected bytes mismatch at 0x%2")
                              .arg(patch.description)
                              .arg(static_cast<qulonglong>(patch.offset), 0, 16));
      }
    }
  }
  return true;
}

bool DaggerfallFallExeHacks::applyPatchSet(QByteArray& exeData, const PatchSet& set,
                                           QString* errorMessage)
{
  if (!verifyPatchSet(exeData, set, errorMessage)) {
    return false;
  }

  for (const BytePatch& patch : set.patches) {
    for (int i = 0; i < patch.replacement.size(); ++i) {
      exeData[patch.offset + i] = patch.replacement.at(i);
    }
  }
  return true;
}
