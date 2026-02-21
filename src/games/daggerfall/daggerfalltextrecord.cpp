#include "daggerfalltextrecord.h"
#include "daggerfallformatutils.h"
#include "daggerfalltextvariables.h"

#include <algorithm>

namespace {

using Daggerfall::FormatUtil::appendWarning;
using Daggerfall::FormatUtil::readLE16U;
using Daggerfall::FormatUtil::readLE32U;
using Daggerfall::FormatUtil::setError;

QVector<quint32> sortedUniqueOffsets(const QVector<quint32>& offsets)
{
  QVector<quint32> out = offsets;
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return out;
}

qsizetype nextOffsetEnd(quint32 start, const QVector<quint32>& sortedOffsets, qsizetype fileSize)
{
  for (quint32 o : sortedOffsets) {
    if (o > start) {
      return static_cast<qsizetype>(o);
    }
  }
  return fileSize;
}

}  // namespace

QVector<DaggerfallTextRecord::Token>
DaggerfallTextRecord::tokenizeSubrecord(const QByteArray& bytes)
{
  QVector<Token> out;
  out.reserve(bytes.size());

  for (int i = 0; i < bytes.size(); ++i) {
    const quint8 b = static_cast<quint8>(bytes.at(i));

    if ((b == 0xFC || b == 0xFD) && i + 1 < bytes.size() &&
        static_cast<quint8>(bytes.at(i + 1)) == 0x00) {
      Token t;
      t.type = TokenType::EndOfLine;
      t.bytes = bytes.mid(i, 2);
      out.push_back(t);
      ++i;
      continue;
    }

    if (b == 0xF9 || b == 0xFA) {
      Token t;
      t.type = (b == 0xF9) ? TokenType::FontCode : TokenType::FontColor;
      t.bytes = bytes.mid(i, (i + 1 < bytes.size()) ? 2 : 1);
      out.push_back(t);
      if (i + 1 < bytes.size()) {
        ++i;
      }
      continue;
    }

    if (b == 0xFB && i >= 1 && i + 2 < bytes.size()) {
      Token t;
      t.type = TokenType::PositionCode;
      t.bytes = bytes.mid(i - 1, 4);
      out.push_back(t);
      i += 2;
      continue;
    }

    if (b == 0xF7) {
      int j = i + 1;
      while (j < bytes.size() && bytes.at(j) != '\0') {
        ++j;
      }
      if (j < bytes.size()) {
        ++j;
      }
      Token t;
      t.type = TokenType::BookImage;
      t.bytes = bytes.mid(i, j - i);
      out.push_back(t);
      i = j - 1;
      continue;
    }

    if (b == 0xF6) {
      Token t;
      t.type = TokenType::EndOfPage;
      t.bytes = QByteArray(1, static_cast<char>(b));
      out.push_back(t);
      continue;
    }

    if (b >= 0x20 && b <= 0x7F) {
      Token t;
      t.type = TokenType::Printable;
      t.bytes = QByteArray(1, static_cast<char>(b));
      out.push_back(t);
      continue;
    }

    Token t;
    t.type = TokenType::RawControl;
    t.bytes = QByteArray(1, static_cast<char>(b));
    out.push_back(t);
  }

  return out;
}

QString DaggerfallTextRecord::decodeSubrecordText(const QByteArray& bytes)
{
  QString out;
  out.reserve(bytes.size());
  const QVector<Token> tokens = tokenizeSubrecord(bytes);
  for (const Token& t : tokens) {
    switch (t.type) {
      case TokenType::Printable:
        out.append(QChar::fromLatin1(t.bytes.at(0)));
        break;
      case TokenType::EndOfLine:
        out.append('\n');
        break;
      case TokenType::EndOfPage:
        out.append('\n');
        break;
      default:
        if (!t.bytes.isEmpty() && static_cast<quint8>(t.bytes.at(0)) == 0x00) {
          out.append('\n');
        }
        break;
    }
  }
  return out;
}

bool DaggerfallTextRecord::parseRecord(const QByteArray& bytes, Record& outRecord,
                                       QString* errorMessage)
{
  outRecord = {};
  outRecord.raw = bytes;

  QByteArray current;
  bool sawEnd = false;
  for (char c : bytes) {
    const quint8 b = static_cast<quint8>(c);
    if (b == 0xFE) {
      Subrecord s;
      s.raw = current;
      s.tokens = tokenizeSubrecord(current);
      s.text = decodeSubrecordText(current);
      s.variables = DaggerfallTextVariables::extractVariables(s.text);
      outRecord.subrecords.push_back(s);
      current.clear();
      sawEnd = true;
      break;
    }
    if (b == 0xFF) {
      Subrecord s;
      s.raw = current;
      s.tokens = tokenizeSubrecord(current);
      s.text = decodeSubrecordText(current);
      s.variables = DaggerfallTextVariables::extractVariables(s.text);
      outRecord.subrecords.push_back(s);
      current.clear();
      continue;
    }
    current.push_back(c);
  }

  if (!sawEnd) {
    if (!current.isEmpty()) {
      Subrecord s;
      s.raw = current;
      s.tokens = tokenizeSubrecord(current);
      s.text = decodeSubrecordText(current);
      s.variables = DaggerfallTextVariables::extractVariables(s.text);
      outRecord.subrecords.push_back(s);
    }
    appendWarning(outRecord.warning, "Record does not contain 0xFE terminator");
  }

  if (outRecord.subrecords.isEmpty() && !bytes.isEmpty()) {
    return setError(errorMessage, "Text record decoding produced no subrecords");
  }
  return true;
}

bool DaggerfallTextRecord::parseDatabase(const QByteArray& bytes, Database& outDb,
                                         QString* errorMessage)
{
  outDb = {};
  if (bytes.size() < 10) {
    return setError(errorMessage, "Text record database is too small");
  }

  quint16 headerLength = 0;
  if (!readLE16U(bytes, 0, headerLength)) {
    return setError(errorMessage, "Failed to read header length");
  }
  if (headerLength < 8 || (headerLength % 6) != 0) {
    return setError(errorMessage, QString("Invalid header length: %1").arg(headerLength));
  }
  if (headerLength > bytes.size()) {
    return setError(errorMessage, "Header length exceeds file size");
  }

  outDb.headerLength = headerLength;
  outDb.recordCount = (static_cast<int>(headerLength) / 6) - 1;
  if (outDb.recordCount <= 0) {
    return setError(errorMessage, "No record headers in database");
  }

  qsizetype cursor = 2;
  QVector<quint32> offsets;
  offsets.reserve(outDb.recordCount);
  outDb.headers.reserve(outDb.recordCount);
  for (int i = 0; i < outDb.recordCount; ++i) {
    Header h;
    if (!readLE16U(bytes, cursor, h.id) || !readLE32U(bytes, cursor + 2, h.offset)) {
      return setError(errorMessage, "Header list is truncated");
    }
    cursor += 6;
    if (h.id == 0xFFFFu) {
      appendWarning(outDb.warning, QString("Header %1 has invalid id 0xFFFF").arg(i));
      continue;
    }
    if (static_cast<qsizetype>(h.offset) >= bytes.size()) {
      appendWarning(outDb.warning,
                    QString("Header %1 offset %2 is outside file").arg(i).arg(h.offset));
      continue;
    }
    outDb.headers.push_back(h);
    offsets.push_back(h.offset);
  }

  quint16 terminator = 0;
  if (!readLE16U(bytes, cursor, terminator)) {
    return setError(errorMessage, "Header list missing terminator");
  }
  if (terminator != 0xFFFFu) {
    appendWarning(outDb.warning, "Header list terminator is not 0xFFFF");
  }
  if (outDb.headers.isEmpty()) {
    return setError(errorMessage, "No valid record headers");
  }

  const QVector<quint32> sortedOffsets = sortedUniqueOffsets(offsets);
  outDb.records.reserve(outDb.headers.size());
  for (int i = 0; i < outDb.headers.size(); ++i) {
    const Header& h = outDb.headers.at(i);
    const qsizetype end = nextOffsetEnd(h.offset, sortedOffsets, bytes.size());
    if (end <= static_cast<qsizetype>(h.offset)) {
      continue;
    }
    QByteArray raw = bytes.mid(static_cast<qsizetype>(h.offset),
                               end - static_cast<qsizetype>(h.offset));
    const int recordEnd = raw.indexOf(static_cast<char>(0xFE));
    if (recordEnd >= 0) {
      raw.truncate(recordEnd + 1);
    }

    Record record;
    QString parseErr;
    if (!parseRecord(raw, record, &parseErr)) {
      appendWarning(outDb.warning, QString("Record %1 parse warning: %2").arg(i).arg(parseErr));
      continue;
    }
    record.id = h.id;
    record.offset = h.offset;
    outDb.records.push_back(record);
  }

  if (outDb.records.isEmpty()) {
    return setError(errorMessage, "Database parsed, but no records were decoded");
  }
  return true;
}
