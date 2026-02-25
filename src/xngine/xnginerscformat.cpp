#include "xnginerscformat.h"

#include <QFile>
#include <QtEndian>

#include <algorithm>
#include <cstring>

namespace {

bool setError(QString* errorMessage, const QString& error)
{
  if (errorMessage != nullptr) {
    *errorMessage = error;
  }
  return false;
}

void appendWarning(QString& warning, const QString& part)
{
  if (part.trimmed().isEmpty()) {
    return;
  }
  if (!warning.isEmpty()) {
    warning.append("; ");
  }
  warning.append(part.trimmed());
}

template <typename T>
bool readLittle(const QByteArray& data, qsizetype offset, T& value)
{
  if (offset < 0 || offset + static_cast<qsizetype>(sizeof(T)) > data.size()) {
    return false;
  }
  T raw{};
  std::memcpy(&raw, data.constData() + offset, sizeof(T));
  value = qFromLittleEndian(raw);
  return true;
}

template <typename T>
void appendLittle(QByteArray& out, T value)
{
  T tmp = qToLittleEndian(value);
  out.append(reinterpret_cast<const char*>(&tmp), static_cast<qsizetype>(sizeof(T)));
}

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

XngineRscFormat::Variant detectVariant(const QByteArray& bytes)
{
  if (bytes.size() < 8) {
    return XngineRscFormat::Variant::Auto;
  }

  quint16 headerLen = 0;
  if (!readLittle(bytes, 0, headerLen)) {
    return XngineRscFormat::Variant::Auto;
  }

  // TEXT.RSC family: first word is header list length (excluding that first word),
  // composed of N x 6-byte entries including a sentinel header.
  if (headerLen >= 6 && (headerLen % 6) == 0 &&
      static_cast<qsizetype>(2 + headerLen) <= bytes.size()) {
    quint16 firstId = 0;
    quint32 firstOff = 0;
    if (readLittle(bytes, 2, firstId) && readLittle(bytes, 4, firstOff)) {
      if (firstId != 0xFFFFu && firstOff >= static_cast<quint32>(2 + headerLen) &&
          firstOff < static_cast<quint32>(bytes.size() + 1)) {
        return XngineRscFormat::Variant::TextRecordDatabase;
      }
    }
  }

  return XngineRscFormat::Variant::Auto;
}

}  // namespace

QVector<XngineRscFormat::TextToken>
XngineRscFormat::tokenizeTextSubrecord(const QByteArray& bytes)
{
  QVector<TextToken> out;
  out.reserve(bytes.size());

  for (int i = 0; i < bytes.size(); ++i) {
    const quint8 b = static_cast<quint8>(bytes.at(i));

    if ((b == 0xFC || b == 0xFD) && i + 1 < bytes.size() &&
        static_cast<quint8>(bytes.at(i + 1)) == 0x00) {
      TextToken t;
      t.type = TextTokenType::EndOfLine;
      t.bytes = bytes.mid(i, 2);
      out.push_back(t);
      ++i;
      continue;
    }

    if (b == 0xF9 || b == 0xFA) {
      TextToken t;
      t.type = (b == 0xF9) ? TextTokenType::FontCode : TextTokenType::FontColor;
      t.bytes = bytes.mid(i, (i + 1 < bytes.size()) ? 2 : 1);
      out.push_back(t);
      if (i + 1 < bytes.size()) {
        ++i;
      }
      continue;
    }

    if (b == 0xFB && i >= 1 && i + 2 < bytes.size()) {
      TextToken t;
      t.type = TextTokenType::PositionCode;
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
      TextToken t;
      t.type = TextTokenType::BookImage;
      t.bytes = bytes.mid(i, j - i);
      out.push_back(t);
      i = j - 1;
      continue;
    }

    if (b == 0xF6) {
      TextToken t;
      t.type = TextTokenType::EndOfPage;
      t.bytes = QByteArray(1, static_cast<char>(b));
      out.push_back(t);
      continue;
    }

    if (b >= 0x20 && b <= 0x7F) {
      TextToken t;
      t.type = TextTokenType::Printable;
      t.bytes = QByteArray(1, static_cast<char>(b));
      out.push_back(t);
      continue;
    }

    TextToken t;
    t.type = TextTokenType::RawControl;
    t.bytes = QByteArray(1, static_cast<char>(b));
    out.push_back(t);
  }

  return out;
}

QString XngineRscFormat::decodeTextSubrecordText(const QByteArray& bytes)
{
  QString out;
  out.reserve(bytes.size());

  const QVector<TextToken> tokens = tokenizeTextSubrecord(bytes);
  for (const auto& t : tokens) {
    switch (t.type) {
      case TextTokenType::Printable:
        out.append(QChar::fromLatin1(t.bytes.at(0)));
        break;
      case TextTokenType::EndOfLine:
      case TextTokenType::EndOfPage:
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

bool XngineRscFormat::parseTextRecord(const QByteArray& bytes, TextRecord& outRecord,
                                      QString* errorMessage)
{
  outRecord = {};
  outRecord.raw = bytes;

  QByteArray current;
  bool sawEnd = false;
  for (char c : bytes) {
    const quint8 b = static_cast<quint8>(c);
    if (b == 0xFE) {
      TextSubrecord s;
      s.raw = current;
      s.tokens = tokenizeTextSubrecord(current);
      s.text = decodeTextSubrecordText(current);
      outRecord.subrecords.push_back(s);
      current.clear();
      sawEnd = true;
      break;
    }
    if (b == 0xFF) {
      TextSubrecord s;
      s.raw = current;
      s.tokens = tokenizeTextSubrecord(current);
      s.text = decodeTextSubrecordText(current);
      outRecord.subrecords.push_back(s);
      current.clear();
      continue;
    }
    current.push_back(c);
  }

  if (!sawEnd) {
    if (!current.isEmpty()) {
      TextSubrecord s;
      s.raw = current;
      s.tokens = tokenizeTextSubrecord(current);
      s.text = decodeTextSubrecordText(current);
      outRecord.subrecords.push_back(s);
    }
    appendWarning(outRecord.warning, "Record does not contain 0xFE terminator");
  }

  if (outRecord.subrecords.isEmpty() && !bytes.isEmpty()) {
    return setError(errorMessage, "Text record decoding produced no subrecords");
  }
  return true;
}

bool XngineRscFormat::parseTextRecordDatabase(const QByteArray& bytes, TextRecordDatabase& outDb,
                                              QString* errorMessage, bool strictValidation)
{
  Q_UNUSED(strictValidation);
  outDb = {};
  if (bytes.size() < 8) {
    return setError(errorMessage, "Text record database is too small");
  }

  quint16 headerLength = 0;
  if (!readLittle(bytes, 0, headerLength)) {
    return setError(errorMessage, "Failed to read header length");
  }
  if (headerLength < 6 || (headerLength % 6) != 0) {
    return setError(errorMessage, QString("Invalid header length: %1").arg(headerLength));
  }
  const qsizetype payloadStart = 2 + static_cast<qsizetype>(headerLength);
  if (payloadStart > bytes.size()) {
    return setError(errorMessage, "Header length exceeds file size");
  }

  outDb.headerLength = headerLength;
  outDb.recordCount = (static_cast<int>(headerLength) / 6) - 1;  // sentinel header included
  if (outDb.recordCount <= 0) {
    return setError(errorMessage, "No record headers in database");
  }

  qsizetype cursor = 2;
  QVector<quint32> offsets;
  offsets.reserve(outDb.recordCount);
  outDb.headers.reserve(outDb.recordCount);

  for (int i = 0; i < outDb.recordCount; ++i) {
    TextRecordHeader h;
    if (!readLittle(bytes, cursor, h.id) || !readLittle(bytes, cursor + 2, h.offset)) {
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

  if (!readLittle(bytes, cursor, outDb.sentinelHeader.id) ||
      !readLittle(bytes, cursor + 2, outDb.sentinelHeader.offset)) {
    return setError(errorMessage, "Header list missing sentinel entry");
  }
  if (outDb.sentinelHeader.id != 0xFFFFu) {
    appendWarning(outDb.warning, "Header sentinel id is not 0xFFFF");
  }
  if (static_cast<qsizetype>(outDb.sentinelHeader.offset) > bytes.size()) {
    appendWarning(outDb.warning, "Header sentinel offset exceeds file size");
  }
  if (outDb.headers.isEmpty()) {
    return setError(errorMessage, "No valid record headers");
  }

  offsets.push_back(outDb.sentinelHeader.offset);
  const QVector<quint32> sortedOffsets = sortedUniqueOffsets(offsets);

  outDb.records.reserve(outDb.headers.size());
  for (int i = 0; i < outDb.headers.size(); ++i) {
    const auto& h = outDb.headers.at(i);
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

    TextRecord record;
    QString parseErr;
    if (!parseTextRecord(raw, record, &parseErr)) {
      appendWarning(outDb.warning,
                    QString("Record %1 parse warning: %2").arg(i).arg(parseErr));
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

bool XngineRscFormat::writeTextRecordDatabase(const TextRecordDatabase& db, QByteArray& outBytes,
                                              QString* errorMessage)
{
  if (db.records.isEmpty()) {
    return setError(errorMessage, "Text record database write: no records");
  }

  const int recordCount = db.records.size();
  const quint16 headerLength = static_cast<quint16>((recordCount + 1) * 6);
  const quint32 payloadStart = 2u + static_cast<quint32>(headerLength);

  outBytes.clear();
  outBytes.reserve(payloadStart + 4096);

  appendLittle(outBytes, headerLength);

  quint32 nextOffset = payloadStart;
  for (const auto& r : db.records) {
    if (r.id == 0xFFFFu) {
      return setError(errorMessage, "Text record database write: record id 0xFFFF is reserved");
    }
    appendLittle(outBytes, r.id);
    appendLittle(outBytes, nextOffset);
    nextOffset += static_cast<quint32>(r.raw.size());
  }

  // Sentinel header marks the end of record data.
  appendLittle(outBytes, static_cast<quint16>(0xFFFFu));
  appendLittle(outBytes, nextOffset);

  if (outBytes.size() != payloadStart) {
    return setError(errorMessage, "Text record database write: header size mismatch");
  }

  for (const auto& r : db.records) {
    outBytes.append(r.raw);
  }

  return true;
}

bool XngineRscFormat::readFile(const QString& filePath, Document& outDocument,
                               QString* errorMessage, const Traits& traits)
{
  outDocument = {};

  QFile f(filePath);
  if (!f.open(QIODevice::ReadOnly)) {
    return setError(errorMessage, QString("Unable to open RSC file: %1").arg(filePath));
  }
  const QByteArray bytes = f.readAll();

  Variant variant = traits.variant;
  if (variant == Variant::Auto) {
    variant = detectVariant(bytes);
    if (variant == Variant::Auto) {
      return setError(errorMessage, "Unable to detect RSC variant");
    }
  }

  switch (variant) {
    case Variant::TextRecordDatabase:
      outDocument.variant = Variant::TextRecordDatabase;
      if (!parseTextRecordDatabase(bytes, outDocument.textDb, errorMessage,
                                   traits.strictValidation)) {
        return false;
      }
      outDocument.warning = outDocument.textDb.warning;
      return true;
    case Variant::Auto:
    default:
      return setError(errorMessage, "Unsupported/unknown RSC variant");
  }
}

bool XngineRscFormat::writeFile(const QString& filePath, const Document& document,
                                QString* errorMessage, const Traits& traits)
{
  Variant variant = (traits.variant == Variant::Auto) ? document.variant : traits.variant;
  QByteArray outBytes;

  switch (variant) {
    case Variant::TextRecordDatabase:
      if (!writeTextRecordDatabase(document.textDb, outBytes, errorMessage)) {
        return false;
      }
      break;
    case Variant::Auto:
    default:
      return setError(errorMessage, "RSC write: unspecified variant");
  }

  QFile f(filePath);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return setError(errorMessage, QString("Unable to write RSC file: %1").arg(filePath));
  }
  if (f.write(outBytes) != outBytes.size()) {
    return setError(errorMessage, "Failed writing RSC bytes");
  }
  return true;
}
