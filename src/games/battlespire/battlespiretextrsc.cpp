#include "battlespiretextrsc.h"

#include <xnginerscformat.h>

#include <QFile>

namespace {

bool setError(QString* errorMessage, const QString& text)
{
  if (errorMessage != nullptr) {
    *errorMessage = text;
  }
  return false;
}

}  // namespace

bool BattlespireTextRsc::parseTextRecordBytes(const QByteArray& data, int index,
                                              TextRecord& outRecord,
                                              QString* errorMessage)
{
  outRecord = {};
  outRecord.index = index;
  outRecord.rawBytes = data;

  XngineRscFormat::TextRecord parsed;
  if (!XngineRscFormat::parseTextRecord(data, parsed, errorMessage)) {
    return false;
  }

  outRecord.warning = parsed.warning;
  outRecord.subrecords.reserve(parsed.subrecords.size());
  for (const auto& sub : parsed.subrecords) {
    TextSubrecord o;
    o.rawBytes = sub.raw;
    o.text = sub.text;
    outRecord.subrecords.push_back(o);
  }

  return true;
}

bool BattlespireTextRsc::loadTextRsc(const QString& filePath, Database& outDb,
                                     QString* errorMessage)
{
  outDb = {};

  XngineRscFormat::Document doc;
  XngineRscFormat::Traits traits;
  traits.variant = XngineRscFormat::Variant::TextRecordDatabase;
  if (!XngineRscFormat::readFile(filePath, doc, errorMessage, traits)) {
    return false;
  }

  if (doc.variant != XngineRscFormat::Variant::TextRecordDatabase) {
    return setError(errorMessage, QString("Unexpected RSC variant parsed for TEXT.RSC: %1")
                                      .arg(filePath));
  }

  const auto& parsedDb = doc.textDb;
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
    tr.rawBytes = r.raw;
    tr.warning = r.warning;
    tr.subrecords.reserve(r.subrecords.size());
    for (const auto& sub : r.subrecords) {
      tr.subrecords.push_back({sub.raw, sub.text});
    }
    outDb.records.insert(i, tr);
    outDb.recordsById.insert(tr.textRecordId, tr);
  }

  if (outDb.records.isEmpty()) {
    return setError(errorMessage, "TEXT.RSC parsed, but no usable records were decoded");
  }

  return true;
}
