#include "daggerfalltextrsc.h"
#include "daggerfallformatutils.h"
#include "daggerfalltextrecord.h"
#include "daggerfalltextrscindices.h"

#include <QFile>
namespace {

using Daggerfall::FormatUtil::setError;

}  // namespace

bool DaggerfallTextRsc::parseTextRecordBytes(const QByteArray& data, int index,
                                             TextRecord& outRecord,
                                             QString* errorMessage)
{
  outRecord = {};
  outRecord.index = index;
  outRecord.rawBytes = data;
  DaggerfallTextRecord::Record parsed;
  if (!DaggerfallTextRecord::parseRecord(data, parsed, errorMessage)) {
    return false;
  }
  outRecord.warning = parsed.warning;
  outRecord.subrecords.reserve(parsed.subrecords.size());
  for (const auto& sub : parsed.subrecords) {
    TextSubrecord o;
    o.rawBytes = sub.raw;
    o.text = sub.text;
    o.variables = sub.variables;
    outRecord.subrecords.push_back(o);
  }
  return true;
}

QString DaggerfallTextRsc::categoryLabelForRecordId(quint16 textRecordId)
{
  return DaggerfallTextRscIndices::labelForRecordId(textRecordId);
}

QString DaggerfallTextRsc::formatRecordLabel(quint16 textRecordId,
                                             const QString& categoryLabel)
{
  if (categoryLabel.trimmed().isEmpty()) {
    return QString("Record %1").arg(textRecordId);
  }
  return QString("Record %1 (%2)").arg(textRecordId).arg(categoryLabel.trimmed());
}

bool DaggerfallTextRsc::loadTextRsc(const QString& filePath, Database& outDb,
                                    QString* errorMessage)
{
  outDb = {};

  QFile f(filePath);
  if (!f.open(QIODevice::ReadOnly)) {
    return setError(errorMessage, QString("Unable to open TEXT.RSC: %1").arg(filePath));
  }
  const QByteArray data = f.readAll();
  DaggerfallTextRecord::Database parsedDb;
  if (!DaggerfallTextRecord::parseDatabase(data, parsedDb, errorMessage)) {
    return false;
  }
  outDb.textRecordHeaderLength = parsedDb.headerLength;
  outDb.textRecordCount = parsedDb.recordCount;
  outDb.warning = parsedDb.warning;
  outDb.headers.reserve(parsedDb.headers.size());
  for (const auto& h : parsedDb.headers) {
    outDb.headers.push_back({h.id, h.offset});
  }
  for (int i = 0; i < parsedDb.records.size(); ++i) {
    const auto& r = parsedDb.records.at(i);
    TextRecord tr;
    tr.index = i;
    tr.textRecordId = r.id;
    tr.categoryLabel = categoryLabelForRecordId(tr.textRecordId);
    tr.displayLabel = formatRecordLabel(tr.textRecordId, tr.categoryLabel);
    tr.rawBytes = r.raw;
    tr.warning = r.warning;
    tr.subrecords.reserve(r.subrecords.size());
    for (const auto& sub : r.subrecords) {
      tr.subrecords.push_back({sub.raw, sub.text, sub.variables});
    }
    outDb.records.insert(i, tr);
    outDb.recordsById.insert(tr.textRecordId, tr);
  }

  if (outDb.records.isEmpty()) {
    return setError(errorMessage, "TEXT.RSC parsed, but no usable records were decoded");
  }

  return true;
}
