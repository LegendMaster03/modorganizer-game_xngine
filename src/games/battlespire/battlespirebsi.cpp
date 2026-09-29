#include "battlespirebsi.h"

#include <QFile>
#include <QtEndian>

#include <algorithm>
#include <cstring>
#include <limits>

namespace {

bool setError(QString* errorMessage, const QString& text)
{
  if (errorMessage != nullptr) {
    *errorMessage = text;
  }
  return false;
}

bool hasRange(const QByteArray& data, qsizetype offset, qsizetype size)
{
  if (offset < 0 || size < 0 || offset > data.size()) {
    return false;
  }
  return size <= data.size() - offset;
}

bool readBE32(const QByteArray& data, qsizetype offset, quint32& value)
{
  if (!hasRange(data, offset, 4)) {
    return false;
  }
  quint32 raw = 0;
  std::memcpy(&raw, data.constData() + offset, sizeof(raw));
  value = qFromBigEndian(raw);
  return true;
}

bool readLE16(const QByteArray& data, qsizetype offset, quint16& value)
{
  if (!hasRange(data, offset, 2)) {
    return false;
  }
  quint16 raw = 0;
  std::memcpy(&raw, data.constData() + offset, sizeof(raw));
  value = qFromLittleEndian(raw);
  return true;
}

quint32 readLE24(const QByteArray& data, qsizetype offset)
{
  const auto b0 = static_cast<quint8>(data.at(offset));
  const auto b1 = static_cast<quint8>(data.at(offset + 1));
  const auto b2 = static_cast<quint8>(data.at(offset + 2));
  return static_cast<quint32>(b0) |
         (static_cast<quint32>(b1) << 8) |
         (static_cast<quint32>(b2) << 16);
}

QString readCString(const QByteArray& bytes)
{
  const qsizetype nullPos = bytes.indexOf('\0');
  const qsizetype length = nullPos >= 0 ? nullPos : bytes.size();
  return QString::fromLatin1(bytes.constData(), length);
}

BattlespireBsi::Rgb decode15Bit(quint16 value)
{
  BattlespireBsi::Rgb color;
  color.r = static_cast<quint8>(((value >> 10) & 0x1F) * 8);
  color.g = static_cast<quint8>(((value >> 5) & 0x1F) * 8);
  color.b = static_cast<quint8>((value & 0x1F) * 8);
  return color;
}

bool decode15BitPalette(const QByteArray& raw, QVector<BattlespireBsi::Rgb>& out,
                        QString* errorMessage)
{
  if ((raw.size() % 2) != 0) {
    return setError(errorMessage, "15-bit BSI palette has an odd byte length");
  }
  out.clear();
  out.reserve(raw.size() / 2);
  for (qsizetype off = 0; off < raw.size(); off += 2) {
    quint16 value = 0;
    if (!readLE16(raw, off, value)) {
      return setError(errorMessage, "Failed reading BSI 15-bit palette entry");
    }
    out.push_back(decode15Bit(value));
  }
  return true;
}

bool decodeCmap(const QByteArray& raw, QVector<BattlespireBsi::Rgb>& out,
                QString* errorMessage)
{
  if ((raw.size() % 3) != 0) {
    return setError(errorMessage, "BSI CMAP length is not divisible by three");
  }
  out.clear();
  out.reserve(raw.size() / 3);
  for (qsizetype off = 0; off < raw.size(); off += 3) {
    // The reverse-engineered reader identifies file order as G, B, R. Values are 6-bit.
    const quint8 g = static_cast<quint8>(raw.at(off));
    const quint8 b = static_cast<quint8>(raw.at(off + 1));
    const quint8 r = static_cast<quint8>(raw.at(off + 2));
    out.push_back({static_cast<quint8>(std::min<int>(255, r * 4)),
                   static_cast<quint8>(std::min<int>(255, g * 4)),
                   static_cast<quint8>(std::min<int>(255, b * 4))});
  }
  return true;
}

bool decodePixels(const BattlespireBsi::Header& header, const QByteArray& encoded,
                  QByteArray& outPixels, QStringList& diagnostics,
                  QString* errorMessage)
{
  const quint64 rows64 = static_cast<quint64>(header.height) * header.frames;
  const quint64 pixels64 = rows64 * header.width;
  if (rows64 > static_cast<quint64>(std::numeric_limits<qsizetype>::max()) ||
      pixels64 > static_cast<quint64>(std::numeric_limits<qsizetype>::max())) {
    return setError(errorMessage, "BSI image dimensions overflow addressable memory");
  }

  const qsizetype rows = static_cast<qsizetype>(rows64);
  const qsizetype expectedPixels = static_cast<qsizetype>(pixels64);
  outPixels.clear();
  outPixels.reserve(expectedPixels);

  if (header.compression == 0) {
    if (!hasRange(encoded, 0, expectedPixels)) {
      return setError(errorMessage,
                      QString("Uncompressed BSI DATA has %1 bytes; expected at least %2")
                          .arg(encoded.size())
                          .arg(expectedPixels));
    }
    outPixels = encoded.left(expectedPixels);
    if (encoded.size() > expectedPixels) {
      diagnostics.push_back(
          QString("BSI DATA contains %1 trailing bytes after uncompressed pixels")
              .arg(encoded.size() - expectedPixels));
    }
  } else if (header.compression == 4 || header.compression == 6) {
    if (rows > std::numeric_limits<qsizetype>::max() / 4) {
      return setError(errorMessage, "BSI scanline table size overflows");
    }
    const qsizetype tableBytes = rows * 4;
    if (!hasRange(encoded, 0, tableBytes)) {
      return setError(errorMessage, "Compressed BSI DATA is smaller than its scanline table");
    }

    for (qsizetype row = 0; row < rows; ++row) {
      const qsizetype tableOffset = row * 4;
      const quint32 sourceOffset = readLE24(encoded, tableOffset);
      const quint8 rowCompression = static_cast<quint8>(encoded.at(tableOffset + 3));
      if (sourceOffset >= static_cast<quint64>(encoded.size())) {
        return setError(errorMessage,
                        QString("BSI scanline %1 points outside DATA at offset %2")
                            .arg(row)
                            .arg(sourceOffset));
      }

      qsizetype pos = static_cast<qsizetype>(sourceOffset);
      qsizetype written = 0;
      if (rowCompression == 0) {
        if (!hasRange(encoded, pos, header.width)) {
          return setError(errorMessage,
                          QString("Raw BSI scanline %1 is truncated").arg(row));
        }
        outPixels.append(encoded.constData() + pos, header.width);
        continue;
      }

      if (rowCompression != 0x80) {
        return setError(errorMessage,
                        QString("BSI scanline %1 uses unsupported compression byte 0x%2")
                            .arg(row)
                            .arg(rowCompression, 2, 16, QChar('0')));
      }

      while (written < header.width) {
        if (!hasRange(encoded, pos, 1)) {
          return setError(errorMessage,
                          QString("RLE BSI scanline %1 ends before reaching its width")
                              .arg(row));
        }
        const quint8 control = static_cast<quint8>(encoded.at(pos++));
        const qsizetype count = static_cast<qsizetype>(control & 0x7F);
        if (count == 0) {
          return setError(errorMessage,
                          QString("RLE BSI scanline %1 contains a zero-length run")
                              .arg(row));
        }
        if (count > static_cast<qsizetype>(header.width) - written) {
          return setError(errorMessage,
                          QString("RLE BSI scanline %1 overruns its declared width")
                              .arg(row));
        }

        if ((control & 0x80) != 0) {
          if (!hasRange(encoded, pos, 1)) {
            return setError(errorMessage,
                            QString("RLE BSI scanline %1 is missing a run pixel")
                                .arg(row));
          }
          const char pixel = encoded.at(pos++);
          outPixels.append(count, pixel);
        } else {
          if (!hasRange(encoded, pos, count)) {
            return setError(errorMessage,
                            QString("RLE BSI scanline %1 literal run is truncated")
                                .arg(row));
          }
          outPixels.append(encoded.constData() + pos, count);
          pos += count;
        }
        written += count;
      }
    }
  } else {
    return setError(errorMessage,
                    QString("Unsupported BSI compression mode %1").arg(header.compression));
  }

  bool hasHighIndices = false;
  for (const char pixel : outPixels) {
    if (static_cast<quint8>(pixel) > 127) {
      hasHighIndices = true;
      break;
    }
  }
  if (hasHighIndices) {
    diagnostics.push_back(
        "Decoded BSI pixels contain palette indices above 127; their palette semantics remain unresolved");
  }
  return true;
}

bool parseHeaderChunk(const QByteArray& data, BattlespireBsi::Header& header,
                      QString* errorMessage)
{
  if (data.size() != 26) {
    return setError(errorMessage,
                    QString("BSI BHDR must be 26 bytes, got %1").arg(data.size()));
  }
  if (!readLE16(data, 0, header.xOffset) || !readLE16(data, 2, header.yOffset) ||
      !readLE16(data, 4, header.width) || !readLE16(data, 6, header.height) ||
      !readLE16(data, 14, header.frames) || !readLE16(data, 16, header.unknown3) ||
      !readLE16(data, 18, header.unknown4) || !readLE16(data, 20, header.unknown5) ||
      !readLE16(data, 22, header.unknown6Value) || !readLE16(data, 24, header.compression)) {
    return setError(errorMessage, "Failed reading BSI BHDR fields");
  }
  header.unknown6 = data.mid(8, 6);
  return true;
}

}  // namespace

bool BattlespireBsi::parseData(const QByteArray& data, Document& outDocument,
                               QString* errorMessage)
{
  outDocument = {};
  if (data.size() < 8) {
    return setError(errorMessage, "BSI file is too small to contain a chunk header");
  }

  qsizetype pos = 0;
  if (QString::fromLatin1(data.constData(), 4) == "BSIF") {
    quint32 bsifLength = 0;
    if (!readBE32(data, 4, bsifLength)) {
      return setError(errorMessage, "Failed reading BSI BSIF preamble length");
    }
    outDocument.hasBsifPreamble = true;
    outDocument.bsifDeclaredLength = bsifLength;
    pos = 8;  // The original engine and reference decoder skip only the BSIF header.
  }

  bool sawEnd = false;
  while (pos < data.size()) {
    if (!hasRange(data, pos, 8)) {
      return setError(errorMessage,
                      QString("BSI trailing bytes at offset %1 are smaller than a chunk header")
                          .arg(pos));
    }

    const QString tag = QString::fromLatin1(data.constData() + pos, 4);
    quint32 length = 0;
    if (!readBE32(data, pos + 4, length)) {
      return setError(errorMessage, QString("Failed reading BSI %1 length").arg(tag));
    }
    const qsizetype payloadOffset = pos + 8;
    const qsizetype remaining = data.size() - payloadOffset;
    if (length > static_cast<quint64>(remaining)) {
      return setError(errorMessage,
                      QString("BSI chunk %1 declares %2 bytes but only %3 remain")
                          .arg(tag)
                          .arg(length)
                          .arg(remaining));
    }
    const QByteArray payload = data.mid(payloadOffset, static_cast<qsizetype>(length));

    if (tag == "IFHD") {
      if (payload.size() != 44) {
        return setError(errorMessage,
                        QString("BSI IFHD must be 44 bytes, got %1").arg(payload.size()));
      }
      outDocument.ifhd = payload;
    } else if (tag == "NAME") {
      outDocument.name = readCString(payload);
      if (!payload.contains('\0')) {
        outDocument.diagnostics.push_back("BSI NAME chunk has no NUL terminator");
      }
    } else if (tag == "BHDR") {
      if (!parseHeaderChunk(payload, outDocument.header, errorMessage)) {
        return false;
      }
      outDocument.hasHeader = true;
    } else if (tag == "HICL") {
      if (payload.size() != 256) {
        return setError(errorMessage,
                        QString("BSI HICL must be 256 bytes, got %1").arg(payload.size()));
      }
      outDocument.hiclRaw = payload;
    } else if (tag == "HTBL") {
      if ((payload.size() % 256) != 0) {
        return setError(errorMessage,
                        QString("BSI HTBL length %1 is not divisible by 256")
                            .arg(payload.size()));
      }
      outDocument.htblRaw = payload;
      if (payload.size() != 8192) {
        outDocument.diagnostics.push_back(
            QString("BSI HTBL has %1 bytes; the documented common size is 8192")
                .arg(payload.size()));
      }
    } else if (tag == "CMAP") {
      if ((payload.size() % 3) != 0) {
        return setError(errorMessage,
                        QString("BSI CMAP length %1 is not divisible by three")
                            .arg(payload.size()));
      }
      outDocument.cmapRaw = payload;
      if (payload.size() != 768) {
        outDocument.diagnostics.push_back(
            QString("BSI CMAP has %1 bytes; the documented common size is 768")
                .arg(payload.size()));
      }
    } else if (tag == "DATA") {
      outDocument.encodedPixels = payload;
    } else if (tag == "END ") {
      if (length != 0) {
        return setError(errorMessage, "BSI END chunk must have zero length");
      }
      sawEnd = true;
    } else {
      outDocument.unknownChunks.push_back({tag, length, payload});
    }

    pos = payloadOffset + static_cast<qsizetype>(length);
    if (tag == "END ") {
      if (pos < data.size()) {
        outDocument.diagnostics.push_back(
            QString("BSI contains %1 bytes after END marker").arg(data.size() - pos));
      }
      break;
    }
  }

  if (!sawEnd) {
    outDocument.diagnostics.push_back("BSI END marker is absent");
  }

  if (!outDocument.hiclRaw.isEmpty() &&
      !decode15BitPalette(outDocument.hiclRaw, outDocument.hiclPalette, errorMessage)) {
    return false;
  }

  outDocument.lightingPalettes.clear();
  for (qsizetype off = 0; off + 256 <= outDocument.htblRaw.size(); off += 256) {
    QVector<Rgb> palette;
    if (!decode15BitPalette(outDocument.htblRaw.mid(off, 256), palette, errorMessage)) {
      return false;
    }
    outDocument.lightingPalettes.push_back(std::move(palette));
  }

  if (!outDocument.cmapRaw.isEmpty() &&
      !decodeCmap(outDocument.cmapRaw, outDocument.cmapPalette, errorMessage)) {
    return false;
  }

  if (outDocument.hasHeader && !outDocument.encodedPixels.isEmpty()) {
    if (!decodePixels(outDocument.header, outDocument.encodedPixels,
                      outDocument.decodedPixels, outDocument.diagnostics, errorMessage)) {
      return false;
    }
  }

  return true;
}

bool BattlespireBsi::readFile(const QString& filePath, Document& outDocument,
                              QString* errorMessage)
{
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return setError(errorMessage, QString("Unable to open BSI file: %1").arg(filePath));
  }
  return parseData(file.readAll(), outDocument, errorMessage);
}
