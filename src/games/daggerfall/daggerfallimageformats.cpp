#include "daggerfallimageformats.h"
#include "daggerfallformatutils.h"

#include <QFile>
#include <QFileInfo>
#include <QtEndian>

#include <algorithm>
#include <cstring>

namespace Daggerfall
{
namespace Image
{
namespace
{

using Daggerfall::FormatUtil::appendWarning;
using Daggerfall::FormatUtil::readLE16;
using Daggerfall::FormatUtil::readLE16U;
using Daggerfall::FormatUtil::readLE32;
using Daggerfall::FormatUtil::readLE32U;
using Daggerfall::FormatUtil::setError;

bool readLE64U(const QByteArray& data, qsizetype off, quint64& value)
{
  if (off < 0 || off + 8 > data.size()) {
    return false;
  }
  quint64 v = 0;
  std::memcpy(&v, data.constData() + off, sizeof(v));
  value = qFromLittleEndian(v);
  return true;
}

bool decodeRleCompressed(const QByteArray& src, int expectedSize, QByteArray& out,
                         QString* errorMessage)
{
  out.clear();
  out.reserve(expectedSize);

  qsizetype p = 0;
  while (p < src.size() && out.size() < expectedSize) {
    const quint8 count = static_cast<quint8>(src.at(p++));
    if (count == 0xFF) {
      return setError(errorMessage, "Invalid IMG RLE count value 0xFF");
    }
    if (count > 0x7F) {
      if (p >= src.size()) {
        return setError(errorMessage, "Truncated IMG RLE compressed run");
      }
      const char v = src.at(p++);
      const int run = static_cast<int>(count) - 127;
      out.append(QByteArray(run, v));
    } else {
      const int run = static_cast<int>(count) + 1;
      if (p + run > src.size()) {
        return setError(errorMessage, "Truncated IMG RLE literal run");
      }
      out.append(src.mid(p, run));
      p += run;
    }
  }

  if (out.size() < expectedSize) {
    return setError(errorMessage, QString("Decoded %1 bytes, expected %2")
                                      .arg(out.size())
                                      .arg(expectedSize));
  }
  if (out.size() > expectedSize) {
    out.truncate(expectedSize);
  }
  return true;
}

QByteArray readData(const QString& path, QString* errorMessage)
{
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly)) {
    if (errorMessage != nullptr) {
      *errorMessage = QString("Unable to open file: %1").arg(path);
    }
    return {};
  }
  return f.readAll();
}

}  // namespace

QImage toImage(const QByteArray& indexedPixels, int width, int height, const PaletteFile& palette)
{
  QImage image(width, height, QImage::Format_ARGB32);
  if (width <= 0 || height <= 0 || palette.colors.size() < 256) {
    return image;
  }
  const int pxCount = width * height;
  for (int i = 0; i < pxCount && i < indexedPixels.size(); ++i) {
    const int x = i % width;
    const int y = i / width;
    const int idx = static_cast<quint8>(indexedPixels.at(i));
    QColor c = palette.colors.at(idx);
    if (idx == 0) {
      c.setAlpha(0);
    } else {
      c.setAlpha(255);
    }
    image.setPixelColor(x, y, c);
  }
  return image;
}

QByteArray applyColourTranslation(const QByteArray& indexedPixels, const QByteArray& xlat256)
{
  if (xlat256.size() < 256) {
    return indexedPixels;
  }
  QByteArray out = indexedPixels;
  for (qsizetype i = 0; i < out.size(); ++i) {
    const quint8 idx = static_cast<quint8>(out.at(i));
    out[i] = xlat256.at(idx);
  }
  return out;
}

bool loadPaletteFile(const QString& path, PaletteFile& out, QString* errorMessage)
{
  out = {};
  const QByteArray data = readData(path, errorMessage);
  if (data.isEmpty()) {
    return false;
  }

  qsizetype base = -1;
  if (data.size() == 768) {
    out.isColFile = false;
    base = 0;
  } else if (data.size() == 776) {
    out.isColFile = true;
    if (!readLE32U(data, 0, out.colLength) || !readLE16U(data, 4, out.colMajor) ||
        !readLE16U(data, 6, out.colMinor)) {
      return setError(errorMessage, "Invalid COL header");
    }
    base = 8;
  } else if (data.size() >= 768) {
    // Be permissive, some files have extra bytes; use trailing 768.
    base = data.size() - 768;
  } else {
    return setError(errorMessage, "Palette file is too small");
  }

  bool sixBit = true;
  for (qsizetype i = 0; i < 768; ++i) {
    if (static_cast<quint8>(data.at(base + i)) > 63) {
      sixBit = false;
      break;
    }
  }

  out.colors.reserve(256);
  for (int i = 0; i < 256; ++i) {
    int r = static_cast<quint8>(data.at(base + i * 3 + 0));
    int g = static_cast<quint8>(data.at(base + i * 3 + 1));
    int b = static_cast<quint8>(data.at(base + i * 3 + 2));
    if (sixBit) {
      r = (r * 255) / 63;
      g = (g * 255) / 63;
      b = (b * 255) / 63;
    }
    out.colors.push_back(QColor(r, g, b, i == 0 ? 0 : 255));
  }
  return true;
}

bool loadColourTranslationFile(const QString& path, ColourTranslationFile& out,
                               QString* errorMessage)
{
  out = {};
  const QByteArray data = readData(path, errorMessage);
  if (data.isEmpty()) {
    return false;
  }
  constexpr qsizetype kExpected = 64 * 256;
  if (data.size() < kExpected) {
    return setError(errorMessage, QString("Colour translation file too small: %1").arg(data.size()));
  }
  out.tables.reserve(64);
  for (int i = 0; i < 64; ++i) {
    out.tables.push_back(data.mid(static_cast<qsizetype>(i) * 256, 256));
  }
  return true;
}

bool loadImgFile(const QString& path, ImgFile& out, QString* errorMessage)
{
  out = {};
  const QByteArray data = readData(path, errorMessage);
  if (data.size() < 12) {
    return setError(errorMessage, "IMG file too small");
  }

  if (!readLE16(data, 0, out.header.xOffset) || !readLE16(data, 2, out.header.yOffset) ||
      !readLE16(data, 4, out.header.width) || !readLE16(data, 6, out.header.height) ||
      !readLE16U(data, 8, out.header.compression) || !readLE16(data, 10, out.header.pixelDataLength)) {
    return setError(errorMessage, "Failed reading IMG header");
  }

  const int expectedUncompressed = std::max(0, static_cast<int>(out.header.width) *
                                                   static_cast<int>(out.header.height));
  const int declaredLen = std::max(0, static_cast<int>(out.header.pixelDataLength));
  const qsizetype available = std::max<qsizetype>(0, data.size() - 12);
  out.pixelDataRaw = data.mid(12, std::min<qsizetype>(declaredLen, available));

  const QString fn = QFileInfo(path).fileName().toUpper();
  const bool treatAsUncompressedUnknown0800 = (out.header.compression == 0x0800 &&
                                               (fn == "FRAM00I0.IMG" || fn == "TALK00I0.IMG"));

  if (out.header.compression == static_cast<quint16>(ImgCompression::Uncompressed) ||
      treatAsUncompressedUnknown0800) {
    out.pixelDataDecoded = out.pixelDataRaw;
  } else if (out.header.compression == static_cast<quint16>(ImgCompression::RleCompressed)) {
    QString rleErr;
    if (!decodeRleCompressed(out.pixelDataRaw, expectedUncompressed, out.pixelDataDecoded, &rleErr)) {
      return setError(errorMessage, QString("IMG RLE decode failed: %1").arg(rleErr));
    }
  } else {
    // Texture-specific compression values can appear in non-IMG contexts.
    out.pixelDataDecoded = out.pixelDataRaw;
    appendWarning(out.warning,
                  QString("Unsupported IMG compression 0x%1, data left undecoded")
                      .arg(out.header.compression, 4, 16, QChar('0')));
  }

  if (out.pixelDataDecoded.size() < expectedUncompressed) {
    appendWarning(out.warning,
                  QString("Decoded pixel data is %1 bytes, expected %2")
                      .arg(out.pixelDataDecoded.size())
                      .arg(expectedUncompressed));
  } else if (expectedUncompressed > 0 && out.pixelDataDecoded.size() > expectedUncompressed) {
    out.pixelDataDecoded.truncate(expectedUncompressed);
  }

  return true;
}

bool loadRciFile(const QString& path, qint16 width, qint16 height, RciFile& out,
                 QString* errorMessage)
{
  out = {};
  out.width = width;
  out.height = height;
  if (width <= 0 || height <= 0) {
    return setError(errorMessage, "RCI width/height must be positive");
  }
  const QByteArray data = readData(path, errorMessage);
  if (data.isEmpty()) {
    return false;
  }

  const qsizetype frameBytes = static_cast<qsizetype>(width) * static_cast<qsizetype>(height);
  if (frameBytes <= 0) {
    return setError(errorMessage, "Invalid RCI frame size");
  }

  qsizetype payloadSize = data.size();
  if ((data.size() >= frameBytes) && ((data.size() - 768) >= frameBytes) &&
      ((data.size() - 768) % frameBytes == 0)) {
    // Palettized RCI special case: trailing palette.
    out.hasEmbeddedPalette = true;
    payloadSize = data.size() - 768;
    QString palErr;
    if (!loadPaletteFile(path, out.embeddedPalette, &palErr)) {
      out.hasEmbeddedPalette = false;
    }
  }

  const int frameCount = static_cast<int>(payloadSize / frameBytes);
  if (frameCount <= 0) {
    return setError(errorMessage, "RCI has no complete frames");
  }
  out.frames.reserve(frameCount);
  for (int i = 0; i < frameCount; ++i) {
    out.frames.push_back(data.mid(static_cast<qsizetype>(i) * frameBytes, frameBytes));
  }
  return true;
}

bool loadSkyFile(const QString& path, SkyFile& out, QString* errorMessage)
{
  out = {};
  const QByteArray data = readData(path, errorMessage);
  if (data.isEmpty()) {
    return false;
  }

  constexpr qsizetype kPalCount = 32;
  constexpr qsizetype kPalBytes = 776;
  constexpr qsizetype kXlatCount = 32;
  constexpr qsizetype kXlatBytes = 64 * 256;
  constexpr qsizetype kImgCount = 64;
  constexpr qsizetype kImgBytes = 512 * 220;

  const qsizetype need = (kPalCount * kPalBytes) + (kXlatCount * kXlatBytes) +
                         (kImgCount * kImgBytes);
  if (data.size() < need) {
    return setError(errorMessage, QString("SKY file too small: %1 < %2")
                                      .arg(data.size())
                                      .arg(need));
  }

  qsizetype p = 0;
  out.palettes.reserve(kPalCount);
  for (int i = 0; i < kPalCount; ++i) {
    PaletteFile pal;
    QByteArray palBytes = data.mid(p, kPalBytes);
    p += kPalBytes;
    QString perr;
    const QString tmpPath = QString();  // not used
    Q_UNUSED(tmpPath);
    // Parse palette bytes directly.
    pal.isColFile = true;
    readLE32U(palBytes, 0, pal.colLength);
    readLE16U(palBytes, 4, pal.colMajor);
    readLE16U(palBytes, 6, pal.colMinor);
    pal.colors.reserve(256);
    bool sixBit = true;
    for (int j = 8; j < 8 + 768; ++j) {
      if (static_cast<quint8>(palBytes.at(j)) > 63) {
        sixBit = false;
        break;
      }
    }
    for (int c = 0; c < 256; ++c) {
      int r = static_cast<quint8>(palBytes.at(8 + c * 3 + 0));
      int g = static_cast<quint8>(palBytes.at(8 + c * 3 + 1));
      int b = static_cast<quint8>(palBytes.at(8 + c * 3 + 2));
      if (sixBit) {
        r = (r * 255) / 63;
        g = (g * 255) / 63;
        b = (b * 255) / 63;
      }
      pal.colors.push_back(QColor(r, g, b, c == 0 ? 0 : 255));
    }
    out.palettes.push_back(pal);
    Q_UNUSED(perr);
  }

  out.translations.reserve(kXlatCount);
  for (int i = 0; i < kXlatCount; ++i) {
    ColourTranslationFile tf;
    tf.tables.reserve(64);
    for (int t = 0; t < 64; ++t) {
      tf.tables.push_back(data.mid(p, 256));
      p += 256;
    }
    out.translations.push_back(tf);
  }

  out.images.reserve(kImgCount);
  for (int i = 0; i < kImgCount; ++i) {
    out.images.push_back(data.mid(p, kImgBytes));
    p += kImgBytes;
  }

  if (p < data.size()) {
    appendWarning(out.warning, QString("SKY file has %1 trailing bytes").arg(data.size() - p));
  }
  return true;
}

bool loadCfaFile(const QString& path, CfaFile& out, QString* errorMessage)
{
  out = {};
  const QByteArray data = readData(path, errorMessage);
  if (data.size() < 14) {
    return setError(errorMessage, "CFA file too small");
  }

  if (!readLE16U(data, 0, out.widthUncompressed) || !readLE16U(data, 2, out.height) ||
      !readLE16U(data, 4, out.widthCompressed) || !readLE16U(data, 6, out.unknown1) ||
      !readLE16U(data, 8, out.unknown2)) {
    return setError(errorMessage, "Failed reading CFA header");
  }
  out.bitsPerPixel = static_cast<quint8>(data.at(10));
  out.frameCount = static_cast<quint8>(data.at(11));
  if (!readLE16U(data, 12, out.headerSize)) {
    return setError(errorMessage, "Failed reading CFA header size");
  }
  if (out.headerSize >= data.size()) {
    return setError(errorMessage, "CFA header size exceeds file size");
  }

  const QByteArray compressed = data.mid(out.headerSize);
  const int framePixels = static_cast<int>(out.widthUncompressed) * static_cast<int>(out.height);
  const int expectedTotal = framePixels * static_cast<int>(out.frameCount);
  QByteArray decoded;
  QString decErr;
  if (!decodeRleCompressed(compressed, expectedTotal, decoded, &decErr)) {
    return setError(errorMessage, QString("CFA RLE decode failed: %1").arg(decErr));
  }

  out.frames.reserve(out.frameCount);
  for (int i = 0; i < out.frameCount; ++i) {
    out.frames.push_back(decoded.mid(static_cast<qsizetype>(i) * framePixels, framePixels));
  }
  return true;
}

bool loadCifFile(const QString& path, CifFile& out, QString* errorMessage)
{
  out = {};
  const QFileInfo fi(path);
  const QString name = fi.fileName().toUpper();
  const QByteArray data = readData(path, errorMessage);
  if (data.isEmpty()) {
    return false;
  }

  if (name == "FACES.CIF") {
    RciFile rci;
    if (!loadRciFile(path, 64, 64, rci, errorMessage)) {
      return false;
    }
    out.isFacesRci = true;
    out.images.reserve(rci.frames.size());
    for (const auto& f : rci.frames) {
      ImgFile img;
      img.header.width = 64;
      img.header.height = 64;
      img.pixelDataDecoded = f;
      out.images.push_back(img);
    }
    return true;
  }

  auto readImgAt = [&](qsizetype off, ImgFile& img, qsizetype& nextOff) -> bool {
    if (off + 12 > data.size()) {
      return false;
    }
    QByteArray slice = data.mid(off);
    QString err;
    // parse from bytes directly (adapt minimal logic from loadImgFile)
    if (!readLE16(slice, 0, img.header.xOffset) || !readLE16(slice, 2, img.header.yOffset) ||
        !readLE16(slice, 4, img.header.width) || !readLE16(slice, 6, img.header.height) ||
        !readLE16U(slice, 8, img.header.compression) || !readLE16(slice, 10, img.header.pixelDataLength)) {
      return false;
    }
    const int len = std::max(0, static_cast<int>(img.header.pixelDataLength));
    if (12 + len > slice.size()) {
      return false;
    }
    img.pixelDataRaw = slice.mid(12, len);
    const int expected = std::max(0, static_cast<int>(img.header.width) *
                                         static_cast<int>(img.header.height));
    if (img.header.compression == static_cast<quint16>(ImgCompression::RleCompressed)) {
      if (!decodeRleCompressed(img.pixelDataRaw, expected, img.pixelDataDecoded, &err)) {
        return false;
      }
    } else {
      img.pixelDataDecoded = img.pixelDataRaw;
    }
    if (img.pixelDataDecoded.size() > expected && expected > 0) {
      img.pixelDataDecoded.truncate(expected);
    }
    nextOff = off + 12 + len;
    return true;
  };

  if (name.startsWith("WEAPON09")) {
    // Bow special case: no root wielded image; currently leave to animation parser extension.
    appendWarning(out.warning, "WEAPON09.CIF animation-only parsing is not yet implemented");
    return true;
  }

  // Default CIF path: contiguous list of ImgFile frames.
  qsizetype p = 0;
  while (p + 12 <= data.size()) {
    ImgFile img;
    qsizetype next = p;
    if (!readImgAt(p, img, next) || next <= p) {
      break;
    }
    out.images.push_back(img);
    p = next;
  }
  if (out.images.isEmpty()) {
    appendWarning(out.warning, "No IMG frames decoded from CIF");
  }
  if (p < data.size()) {
    appendWarning(out.warning, QString("CIF has %1 trailing bytes").arg(data.size() - p));
  }
  return true;
}

bool loadTextureFile(const QString& path, TextureFile& out, QString* errorMessage)
{
  out = {};
  const QByteArray data = readData(path, errorMessage);
  if (data.isEmpty()) {
    return false;
  }
  if (data.size() < 26) {
    return setError(errorMessage, "Texture file too small");
  }

  if (!readLE16(data, 0, out.header.recordCount)) {
    return setError(errorMessage, "Failed reading texture record count");
  }
  out.header.name = QString::fromLatin1(data.mid(2, 24)).trimmed();

  const int recordCount = std::max(0, static_cast<int>(out.header.recordCount));
  const qsizetype headersSize = 26 + static_cast<qsizetype>(recordCount) * 20;
  if (headersSize > data.size()) {
    return setError(errorMessage, "Texture header list exceeds file size");
  }

  out.recordHeaders.reserve(recordCount);
  for (int i = 0; i < recordCount; ++i) {
    const qsizetype off = 26 + static_cast<qsizetype>(i) * 20;
    TextureRecordHeader h;
    if (!readLE16U(data, off + 0, h.unknown1) || !readLE32(data, off + 2, h.recordPosition) ||
        !readLE16U(data, off + 6, h.unknown2) || !readLE32U(data, off + 8, h.unknown3) ||
        !readLE64U(data, off + 12, h.nullValue)) {
      return setError(errorMessage, QString("Failed reading texture record header %1").arg(i));
    }
    out.recordHeaders.push_back(h);
  }

  out.records.reserve(out.recordHeaders.size());
  for (int i = 0; i < out.recordHeaders.size(); ++i) {
    const auto& rh = out.recordHeaders.at(i);
    if (rh.recordPosition <= 0 || rh.recordPosition + 28 > data.size()) {
      TextureRecord rec;
      appendWarning(rec.warning, "Invalid record position");
      out.records.push_back(rec);
      continue;
    }
    const qsizetype base = rh.recordPosition;
    TextureRecord rec;
    if (!readLE16(data, base + 0, rec.offsetX) || !readLE16(data, base + 2, rec.offsetY) ||
        !readLE16(data, base + 4, rec.width) || !readLE16(data, base + 6, rec.height) ||
        !readLE16U(data, base + 8, rec.compression) || !readLE32U(data, base + 10, rec.recordSize) ||
        !readLE32U(data, base + 14, rec.dataOffset) || !readLE16U(data, base + 18, rec.isNormal) ||
        !readLE16U(data, base + 20, rec.frameCount) || !readLE16U(data, base + 22, rec.unknown1) ||
        !readLE16(data, base + 24, rec.xScale) || !readLE16(data, base + 26, rec.yScale)) {
      return setError(errorMessage, QString("Failed reading texture record %1").arg(i));
    }

    // Decode common case: uncompressed single-frame.
    const int w = std::max(0, static_cast<int>(rec.width));
    const int h = std::max(0, static_cast<int>(rec.height));
    const qsizetype dataStart = base + rec.dataOffset;
    if (w > 0 && h > 0 && dataStart < data.size()) {
      if ((rec.compression == 0 || rec.compression > 0x2000) && rec.frameCount == 1) {
        QByteArray frame;
        frame.reserve(w * h);
        qsizetype p = dataStart;
        for (int row = 0; row < h; ++row) {
          if (p + w > data.size()) {
            appendWarning(rec.warning, "Texture row exceeds file size");
            break;
          }
          frame.append(data.mid(p, w));
          if (256 - w < 0) {
            appendWarning(rec.warning, "Texture width exceeds 256 for uncompressed-single-frame");
            break;
          }
          p += 256;
        }
        rec.frames.push_back(frame);
      } else {
        appendWarning(rec.warning, "Texture compression/frame mode currently parsed as header-only");
      }
    }

    out.records.push_back(rec);
  }
  return true;
}

}  // namespace Image
}  // namespace Daggerfall
