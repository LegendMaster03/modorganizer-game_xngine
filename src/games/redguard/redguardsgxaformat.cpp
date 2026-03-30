#include "redguardsgxaformat.h"

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
                    QString("Unable to open GXA file: %1").arg(filePath));
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
                     RedguardsGxaFormat::ChunkHeader& outChunk,
                     bool& outUsedAbsoluteEndOffset, QString* errorMessage)
{
  if (cursor < 0 || cursor + 4 > bytes.size()) {
    return setError(errorMessage, "Truncated GXA chunk header");
  }

  outChunk = {};
  outUsedAbsoluteEndOffset = false;
  outChunk.tag = QString::fromLatin1(bytes.constData() + cursor, 4);
  cursor += 4;

  if (outChunk.tag == "END ") {
    outChunk.payloadOffset = cursor;
    return true;
  }

  if (cursor + 4 > bytes.size()) {
    return setError(errorMessage,
                    QString("Truncated GXA chunk length for '%1'")
                        .arg(outChunk.tag));
  }

  if (!readBE32(bytes, cursor, outChunk.length)) {
    return setError(errorMessage,
                    QString("Failed reading GXA chunk length for '%1'")
                        .arg(outChunk.tag));
  }
  cursor += 4;
  outChunk.payloadOffset = cursor;

  if (cursor + static_cast<qsizetype>(outChunk.length) > bytes.size()) {
    if (outChunk.tag == "BBMP" &&
        outChunk.length >= static_cast<quint32>(outChunk.payloadOffset) &&
        outChunk.length <= static_cast<quint32>(bytes.size())) {
      outChunk.length -= static_cast<quint32>(outChunk.payloadOffset);
      outUsedAbsoluteEndOffset = true;
      return true;
    }

    return setError(errorMessage,
                    QString("GXA chunk '%1' overruns file").arg(outChunk.tag));
  }

  return true;
}

bool parsePaletteChunk(const QByteArray& payload,
                       XnginePaletteFormat::Palette& outPalette,
                       QString* errorMessage)
{
  if (payload.size() != 768) {
    return setError(errorMessage,
                    QString("GXA palette block must be 768 bytes, got %1")
                        .arg(payload.size()));
  }

  return XnginePaletteFormat::parseRawRgb256(payload, outPalette, errorMessage);
}

bool readImageHeader(const QByteArray& payload, qsizetype offset,
                     RedguardsGxaFormat::Image& outImage)
{
  if (!readLE16S(payload, offset + 0, outImage.unknown0) ||
      !readLE16(payload, offset + 2, outImage.width) ||
      !readLE16(payload, offset + 4, outImage.height)) {
    return false;
  }

  outImage.unknownFields.resize(6);
  for (int i = 0; i < outImage.unknownFields.size(); ++i) {
    if (!readLE16S(payload, offset + 6 + i * 2, outImage.unknownFields[i])) {
      return false;
    }
  }

  return true;
}

bool looksLikeType1Header(const QByteArray& payload, qsizetype offset)
{
  RedguardsGxaFormat::Image image;
  if (!readImageHeader(payload, offset, image)) {
    return false;
  }

  return image.unknown0 == 1 && image.width > 0 && image.height > 0 &&
         image.unknownFields.size() == 6 && image.unknownFields[0] == 0 &&
         image.unknownFields[1] == 0 && image.unknownFields[2] == 1 &&
         image.unknownFields[3] == 0 && image.unknownFields[4] == 0 &&
         image.unknownFields[5] == 0;
}

}  // namespace

bool RedguardsGxaFormat::loadFile(const QString& filePath, Document& outDocument,
                                  QString* errorMessage)
{
  QByteArray bytes;
  if (!readFileBytes(filePath, bytes, errorMessage)) {
    return false;
  }

  return parseBytes(bytes, outDocument, errorMessage);
}

bool RedguardsGxaFormat::parseBytes(const QByteArray& bytes, Document& outDocument,
                                    QString* errorMessage)
{
  outDocument = {};

  qsizetype cursor = 0;
  bool sawBmhd = false;
  bool sawPalette = false;
  bool sawBbmp = false;
  bool sawEnd = false;

  while (!sawEnd) {
    ChunkHeader chunk;
    bool usedAbsoluteEndOffset = false;
    if (!readChunkHeader(bytes, cursor, chunk, usedAbsoluteEndOffset,
                         errorMessage)) {
      return false;
    }
    outDocument.chunks.push_back(chunk);

    if (chunk.tag == "END ") {
      sawEnd = true;
      break;
    }

    const QByteArray payload =
        bytes.mid(chunk.payloadOffset, static_cast<qsizetype>(chunk.length));

    if (chunk.tag == "BMHD") {
      if (chunk.length != 34) {
        return setError(errorMessage,
                        QString("BMHD block must be 34 bytes, got %1")
                            .arg(chunk.length));
      }

      outDocument.bmhd.title = readCString(payload.left(22));
      outDocument.bmhd.unknownBytes = payload.mid(22, 10);
      if (!readLE16(payload, 32, outDocument.bmhd.imageCount)) {
        return setError(errorMessage, "Failed parsing BMHD image count");
      }

      sawBmhd = true;
    } else if (chunk.tag == "BPAL") {
      if (!parsePaletteChunk(payload, outDocument.palette, errorMessage)) {
        return false;
      }
      sawPalette = true;
    } else if (chunk.tag == "BBMP") {
      if (!sawBmhd) {
        return setError(errorMessage, "BBMP encountered before BMHD");
      }

      outDocument.images.clear();
      outDocument.images.reserve(outDocument.bmhd.imageCount);

      Image firstImage;
      if (!readImageHeader(payload, 0, firstImage)) {
        return setError(errorMessage, "Failed reading first GXA image header");
      }

      const qint16 subtype =
          firstImage.unknownFields.size() >= 3 ? firstImage.unknownFields[2] : -1;

      if (subtype == 0) {
        qsizetype imageCursor = 0;
        for (quint16 i = 0; i < outDocument.bmhd.imageCount; ++i) {
          Image image;
          if (!readImageHeader(payload, imageCursor, image)) {
            return setError(errorMessage,
                            QString("Failed reading GXA image %1 header").arg(i));
          }

          image.encoding = ImageEncoding::RawIndexed;
          imageCursor += 18;

          const qsizetype pixelCount = static_cast<qsizetype>(image.width) *
                                       static_cast<qsizetype>(image.height);
          if (pixelCount < 0 || imageCursor + pixelCount > payload.size()) {
            return setError(errorMessage,
                            QString("GXA image %1 pixel data overruns BBMP block")
                                .arg(i));
          }

          image.imageData = payload.mid(imageCursor, pixelCount);
          imageCursor += pixelCount;
          outDocument.images.push_back(image);
        }

        if (imageCursor < payload.size()) {
          outDocument.warning =
              QString("BBMP block has %1 trailing bytes beyond parsed image data")
                  .arg(payload.size() - imageCursor);
        }
      } else if (subtype == 1) {
        QVector<qsizetype> imageOffsets;
        imageOffsets.reserve(outDocument.bmhd.imageCount);
        for (qsizetype off = 0; off + 18 <= payload.size(); ++off) {
          if (looksLikeType1Header(payload, off)) {
            imageOffsets.push_back(off);
          }
        }

        if (imageOffsets.size() != outDocument.bmhd.imageCount) {
          return setError(
              errorMessage,
              QString("Type-1 BBMP header scan found %1 images, expected %2")
                  .arg(imageOffsets.size())
                  .arg(outDocument.bmhd.imageCount));
        }

        for (qsizetype i = 0; i < imageOffsets.size(); ++i) {
          Image image;
          const qsizetype imageOffset = imageOffsets.at(i);
          if (!readImageHeader(payload, imageOffset, image)) {
            return setError(errorMessage,
                            QString("Failed reading type-1 GXA image %1 header")
                                .arg(i));
          }

          const qsizetype dataStart = imageOffset + 18;
          const qsizetype dataEnd =
              (i + 1 < imageOffsets.size()) ? imageOffsets.at(i + 1) : payload.size();
          if (dataEnd < dataStart) {
            return setError(errorMessage,
                            QString("Invalid type-1 GXA image %1 data range").arg(i));
          }

          image.encoding = ImageEncoding::EncodedType1;
          image.imageData = payload.mid(dataStart, dataEnd - dataStart);
          outDocument.images.push_back(image);
        }
      } else if (subtype == 2) {
        if (outDocument.bmhd.imageCount != 1) {
          return setError(errorMessage,
                          "Type-2 BBMP currently only supports single-image files");
        }

        firstImage.encoding = ImageEncoding::EncodedType2;
        firstImage.imageData = payload.mid(18);
        outDocument.images.push_back(firstImage);
      } else {
        return setError(errorMessage,
                        QString("Unsupported GXA BBMP subtype marker: %1")
                            .arg(subtype));
      }

      if (usedAbsoluteEndOffset) {
        outDocument.warning =
            "Used BBMP absolute-END-offset length workaround";
      }

      sawBbmp = true;
    }

    cursor = chunk.payloadOffset + static_cast<qsizetype>(chunk.length);
  }

  if (!sawBmhd || !sawPalette || !sawBbmp || !sawEnd) {
    return setError(errorMessage,
                    "GXA is missing one or more required blocks");
  }

  if (cursor != bytes.size() && outDocument.warning.isEmpty()) {
    outDocument.warning =
        QString("GXA has %1 trailing bytes after END marker")
            .arg(bytes.size() - cursor);
  }

  return true;
}

bool RedguardsGxaFormat::decodeImage(const Image& image,
                                     const XnginePaletteFormat::Palette& palette,
                                     QImage& outImage, QString* errorMessage)
{
  if (image.encoding != ImageEncoding::RawIndexed) {
    return setError(errorMessage,
                    "Only raw indexed GXA images can currently be raster-decoded");
  }

  const qsizetype pixelCount =
      static_cast<qsizetype>(image.width) * static_cast<qsizetype>(image.height);
  if (image.imageData.size() != pixelCount) {
    return setError(errorMessage,
                    QString("Image pixel data size mismatch: expected %1, got %2")
                        .arg(pixelCount)
                        .arg(image.imageData.size()));
  }

  if (palette.colors.size() != 256) {
    return setError(errorMessage, "Palette does not contain 256 colors");
  }

  outImage = QImage(image.width, image.height, QImage::Format_ARGB32);
  if (outImage.isNull()) {
    return setError(errorMessage, "Failed allocating GXA image");
  }

  for (int y = 0; y < image.height; ++y) {
    for (int x = 0; x < image.width; ++x) {
      const quint8 index =
          static_cast<quint8>(image.imageData.at(y * image.width + x));
      if (index == 0) {
        outImage.setPixelColor(x, y, QColor(0, 0, 0, 0));
      } else {
        outImage.setPixelColor(x, y, palette.colors.at(index));
      }
    }
  }

  return true;
}
