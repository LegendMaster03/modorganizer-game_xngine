#include "daggerfalltextrecord.h"
#include "daggerfalltextvariables.h"
#include <xnginerscformat.h>

#include <algorithm>

QVector<DaggerfallTextRecord::Token>
DaggerfallTextRecord::tokenizeSubrecord(const QByteArray& bytes)
{
  QVector<Token> out;
  const auto core = XngineRscFormat::tokenizeTextSubrecord(bytes);
  out.reserve(core.size());
  for (const auto& t : core) {
    Token tt;
    tt.bytes = t.bytes;
    switch (t.type) {
      case XngineRscFormat::TextTokenType::Printable: tt.type = TokenType::Printable; break;
      case XngineRscFormat::TextTokenType::FontCode: tt.type = TokenType::FontCode; break;
      case XngineRscFormat::TextTokenType::FontColor: tt.type = TokenType::FontColor; break;
      case XngineRscFormat::TextTokenType::PositionCode: tt.type = TokenType::PositionCode; break;
      case XngineRscFormat::TextTokenType::BookImage: tt.type = TokenType::BookImage; break;
      case XngineRscFormat::TextTokenType::EndOfPage: tt.type = TokenType::EndOfPage; break;
      case XngineRscFormat::TextTokenType::EndOfLine: tt.type = TokenType::EndOfLine; break;
      case XngineRscFormat::TextTokenType::RawControl:
      default: tt.type = TokenType::RawControl; break;
    }
    out.push_back(tt);
  }
  return out;
}

QString DaggerfallTextRecord::decodeSubrecordText(const QByteArray& bytes)
{
  return XngineRscFormat::decodeTextSubrecordText(bytes);
}

bool DaggerfallTextRecord::parseRecord(const QByteArray& bytes, Record& outRecord,
                                       QString* errorMessage)
{
  XngineRscFormat::TextRecord core;
  if (!XngineRscFormat::parseTextRecord(bytes, core, errorMessage)) {
    return false;
  }

  outRecord = {};
  outRecord.id = core.id;
  outRecord.offset = core.offset;
  outRecord.raw = core.raw;
  outRecord.warning = core.warning;
  outRecord.subrecords.reserve(core.subrecords.size());
  for (const auto& sub : core.subrecords) {
    Subrecord s;
    s.raw = sub.raw;
    s.text = sub.text;
    s.variables = DaggerfallTextVariables::extractVariables(s.text);
    s.tokens = tokenizeSubrecord(sub.raw);
    outRecord.subrecords.push_back(s);
  }

  return true;
}

bool DaggerfallTextRecord::parseDatabase(const QByteArray& bytes, Database& outDb,
                                         QString* errorMessage)
{
  XngineRscFormat::TextRecordDatabase coreDb;
  if (!XngineRscFormat::parseTextRecordDatabase(bytes, coreDb, errorMessage)) {
    return false;
  }

  outDb = {};
  outDb.headerLength = coreDb.headerLength;
  outDb.recordCount = coreDb.recordCount;
  outDb.warning = coreDb.warning;
  outDb.headers.reserve(coreDb.headers.size());
  for (const auto& h : coreDb.headers) {
    outDb.headers.push_back({h.id, h.offset});
  }
  outDb.records.reserve(coreDb.records.size());
  for (const auto& cr : coreDb.records) {
    Record r;
    r.id = cr.id;
    r.offset = cr.offset;
    r.raw = cr.raw;
    r.warning = cr.warning;
    r.subrecords.reserve(cr.subrecords.size());
    for (const auto& cs : cr.subrecords) {
      Subrecord s;
      s.raw = cs.raw;
      s.text = cs.text;
      s.variables = DaggerfallTextVariables::extractVariables(s.text);
      s.tokens = tokenizeSubrecord(cs.raw);
      r.subrecords.push_back(s);
    }
    outDb.records.push_back(r);
  }
  return true;
}
