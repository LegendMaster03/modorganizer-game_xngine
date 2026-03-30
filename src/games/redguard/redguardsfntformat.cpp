#include "redguardsfntformat.h"

#include <QFile>
#include <QtEndian>

#include <cstring>

namespace
{
bool setError(QString* errorMessage, const QString& text)
{
  if (errorMessage != nullptr) {
    *errorMessage = text;
  }
  return false;
}

bool readFileBytes(const QString& filePath, QByteArray& outBytes,
                   QString* errorMessage)
{
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return setError(errorMessage,
                    QString("Unable to open FNT file: %1").arg(filePath));
  }

  outBytes = file.readAll();
  return true;
}

bool readLE16(const QByteArray& data, qsizetype offset, quint16& outValue)
{
  if (offset < 0 || offset + 2 > data.size()) {
    return false;
  }

  quint16 raw = 0;
  std::memcpy(&raw, data.constData() + offset, sizeof(raw));
  outValue = qFromLittleEndian(raw);
  return true;
}

bool readLE16S(const QByteArray& data, qsizetype offset, qint16& outValue)
{
  quint16 raw = 0;
  if (!readLE16(data, offset, raw)) {
    return false;
  }

  outValue = static_cast<qint16>(raw);
  return true;
}

bool readLE32(const QByteArray& data, qsizetype offset, quint32& outValue)
{
  if (offset < 0 || offset + 4 > data.size()) {
    return false;
  }

  quint32 raw = 0;
  std::memcpy(&raw, data.constData() + offset, sizeof(raw));
  outValue = qFromLittleEndian(raw);
  return true;
}

bool readBE32(const QByteArray& data, qsizetype offset, quint32& outValue)
{
  if (offset < 0 || offset + 4 > data.size()) {
    return false;
  }

  quint32 raw = 0;
  std::memcpy(&raw, data.constData() + offset, sizeof(raw));
  outValue = qFromBigEndian(raw);
  return true;
}

QString readCString(const QByteArray& data)
{
  const int nul = data.indexOf('\0');
  if (nul >= 0) {
    return QString::fromLatin1(data.constData(), nul);
  }

  return QString::fromLatin1(data);
}

bool readChunkHeader(const QByteArray& bytes, qsizetype& cursor,
                     RedguardsFntFormat::ChunkHeader& outChunk,
                     QString* errorMessage)
{
  if (cursor < 0 || cursor + 4 > bytes.size()) {
    return setError(errorMessage, "Truncated FNT chunk header");
  }

  outChunk = {};
  outChunk.tag = QString::fromLatin1(bytes.constData() + cursor, 4);
  cursor += 4;

  if (outChunk.tag == "END ") {
    outChunk.payloadOffset = cursor;
    return true;
  }

  if (cursor + 4 > bytes.size()) {
    return setError(errorMessage,
                    QString("Truncated FNT chunk length for '%1'")
                        .arg(outChunk.tag));
  }

  if (!readBE32(bytes, cursor, outChunk.length)) {
    return setError(errorMessage,
                    QString("Failed reading FNT chunk length for '%1'")
                        .arg(outChunk.tag));
  }
  cursor += 4;
  outChunk.payloadOffset = cursor;

  if (cursor + static_cast<qsizetype>(outChunk.length) > bytes.size()) {
    return setError(errorMessage,
                    QString("FNT chunk '%1' overruns file").arg(outChunk.tag));
  }

  return true;
}

bool parsePaletteChunk(const QByteArray& payload,
                       XnginePaletteFormat::Palette& outPalette,
                       QString* errorMessage)
{
  if (payload.size() != 768) {
    return setError(errorMessage,
                    QString("FNT palette block must be 768 bytes, got %1")
                        .arg(payload.size()));
  }

  return XnginePaletteFormat::parseRawRgb256(payload, outPalette, errorMessage);
}

}  // namespace

bool RedguardsFntFormat::loadFile(const QString& filePath, Document& outDocument,
                                  QString* errorMessage)
{
  QByteArray bytes;
  if (!readFileBytes(filePath, bytes, errorMessage)) {
    return false;
  }

  return parseBytes(bytes, outDocument, errorMessage);
}

bool RedguardsFntFormat::parseBytes(const QByteArray& bytes, Document& outDocument,
                                    QString* errorMessage)
{
  outDocument = {};

  qsizetype cursor = 0;
  bool sawFnhd = false;
  bool sawPalette = false;
  bool sawFbmp = false;
  bool sawEnd = false;

  while (!sawEnd) {
    ChunkHeader chunk;
    if (!readChunkHeader(bytes, cursor, chunk, errorMessage)) {
      return false;
    }
    outDocument.chunks.push_back(chunk);

    if (chunk.tag == "END ") {
      sawEnd = true;
      break;
    }

    const QByteArray payload =
        bytes.mid(chunk.payloadOffset, static_cast<qsizetype>(chunk.length));

    if (chunk.tag == "FNHD") {
      if (chunk.length != 56) {
        return setError(errorMessage,
                        QString("FNHD block must be 56 bytes, got %1")
                            .arg(chunk.length));
      }

      outDocument.fnhd.description = readCString(payload.left(32));
      if (!readLE16(payload, 32, outDocument.fnhd.unknown24) ||
          !readLE16(payload, 34, outDocument.fnhd.hasRdat) ||
          !readLE16(payload, 36, outDocument.fnhd.unknown28) ||
          !readLE16(payload, 38, outDocument.fnhd.unknown2A) ||
          !readLE16(payload, 40, outDocument.fnhd.unknown2C) ||
          !readLE16(payload, 42, outDocument.fnhd.maxWidth) ||
          !readLE16(payload, 44, outDocument.fnhd.lineHeight) ||
          !readLE16(payload, 46, outDocument.fnhd.characterStart) ||
          !readLE16(payload, 48, outDocument.fnhd.characterCount) ||
          !readLE16(payload, 50, outDocument.fnhd.unknown36) ||
          !readLE16(payload, 52, outDocument.fnhd.unknown38) ||
          !readLE16(payload, 54, outDocument.fnhd.unknown3A)) {
        return setError(errorMessage, "Failed parsing FNHD fields");
      }

      sawFnhd = true;
    } else if (chunk.tag == "BPAL" || chunk.tag == "FPAL") {
      if (!parsePaletteChunk(payload, outDocument.palette, errorMessage)) {
        return false;
      }
      sawPalette = true;
    } else if (chunk.tag == "FBMP") {
      if (!sawFnhd) {
        return setError(errorMessage, "FBMP encountered before FNHD");
      }

      outDocument.glyphs.clear();
      outDocument.glyphs.reserve(outDocument.fnhd.characterCount);

      qsizetype glyphCursor = 0;
      for (quint16 i = 0; i < outDocument.fnhd.characterCount; ++i) {
        Glyph glyph;
        if (!readLE16(payload, glyphCursor + 0, glyph.isEnabled) ||
            !readLE16S(payload, glyphCursor + 2, glyph.offsetLeft) ||
            !readLE16S(payload, glyphCursor + 4, glyph.offsetTop) ||
            !readLE16(payload, glyphCursor + 6, glyph.width) ||
            !readLE16(payload, glyphCursor + 8, glyph.height)) {
          return setError(errorMessage,
                          QString("Failed reading FNT glyph %1 header").arg(i));
        }

        glyphCursor += 10;
        const qsizetype pixelCount =
            static_cast<qsizetype>(glyph.width) * static_cast<qsizetype>(glyph.height);
        if (pixelCount < 0 || glyphCursor + pixelCount > payload.size()) {
          return setError(errorMessage,
                          QString("FNT glyph %1 pixel data overruns FBMP block")
                              .arg(i));
        }

        glyph.pixelIndices = payload.mid(glyphCursor, pixelCount);
        glyphCursor += pixelCount;
        outDocument.glyphs.push_back(glyph);
      }

      if (glyphCursor != payload.size()) {
        outDocument.warning =
            QString("FBMP block has %1 trailing bytes after glyph data")
                .arg(payload.size() - glyphCursor);
      }

      sawFbmp = true;
    } else if (chunk.tag == "RDAT") {
      outDocument.hasRdat = true;
      outDocument.rdat.rawBytes = payload;

      if (payload.size() == 173) {
        outDocument.rdat.sourceFile = readCString(payload.left(136));
        if (!readLE32(payload, 136, outDocument.rdat.unknown90) ||
            !readLE32(payload, 140, outDocument.rdat.unknown94) ||
            !readLE32(payload, 144, outDocument.rdat.unknown98) ||
            !readLE32(payload, 148, outDocument.rdat.unknown9C) ||
            !readLE32(payload, 152, outDocument.rdat.unknownA0) ||
            !readLE32(payload, 156, outDocument.rdat.unknownA4) ||
            !readLE32(payload, 160, outDocument.rdat.unknownA8) ||
            !readLE32(payload, 164, outDocument.rdat.unknownAC) ||
            !readLE32(payload, 168, outDocument.rdat.unknownB0)) {
          return setError(errorMessage, "Failed parsing RDAT fields");
        }
        outDocument.rdat.unknownB4 =
            static_cast<quint8>(payload.at(172));
        outDocument.rdat.parsedFields = true;
      }
    }

    cursor = chunk.payloadOffset + static_cast<qsizetype>(chunk.length);
  }

  if (!sawFnhd || !sawPalette || !sawFbmp || !sawEnd) {
    return setError(errorMessage,
                    "FNT is missing one or more required blocks");
  }

  if (outDocument.fnhd.hasRdat != 0 && !outDocument.hasRdat) {
    return setError(errorMessage,
                    "FNHD indicates RDAT is present, but no RDAT block was found");
  }

  if (cursor != bytes.size() && outDocument.warning.isEmpty()) {
    outDocument.warning =
        QString("FNT has %1 trailing bytes after END marker")
            .arg(bytes.size() - cursor);
  }

  return true;
}

bool RedguardsFntFormat::decodeGlyphImage(
    const Glyph& glyph, const XnginePaletteFormat::Palette& palette, QImage& outImage,
    QString* errorMessage)
{
  const qsizetype pixelCount =
      static_cast<qsizetype>(glyph.width) * static_cast<qsizetype>(glyph.height);
  if (glyph.pixelIndices.size() != pixelCount) {
    return setError(errorMessage,
                    QString("Glyph pixel data size mismatch: expected %1, got %2")
                        .arg(pixelCount)
                        .arg(glyph.pixelIndices.size()));
  }

  if (palette.colors.size() != 256) {
    return setError(errorMessage, "Palette does not contain 256 colors");
  }

  outImage = QImage(glyph.width, glyph.height, QImage::Format_ARGB32);
  if (outImage.isNull()) {
    return setError(errorMessage, "Failed allocating glyph image");
  }

  for (int y = 0; y < glyph.height; ++y) {
    for (int x = 0; x < glyph.width; ++x) {
      const quint8 index =
          static_cast<quint8>(glyph.pixelIndices.at(y * glyph.width + x));
      if (index == 0) {
        outImage.setPixelColor(x, y, QColor(0, 0, 0, 0));
      } else {
        outImage.setPixelColor(x, y, palette.colors.at(index));
      }
    }
  }

  return true;
}
