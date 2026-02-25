#include "xnginewldformat.h"

#include <QFile>
#include <QtEndian>

#include <array>
#include <cstring>

namespace {

constexpr qsizetype kDfHeaderSize = 0x90;
constexpr qsizetype kDfPixelDataSize = 47;
constexpr qsizetype kRgHeaderBytes = 1184;
constexpr qsizetype kRgHeaderDwords = 296;
constexpr qsizetype kRgSectionCount = 4;
constexpr qsizetype kRgSectionHeaderWords = 11;
constexpr qsizetype kRgSectionHeaderBytes = 22;
constexpr qsizetype kRgMapBytes = 128 * 128;
constexpr qsizetype kRgSectionBytes = kRgSectionHeaderBytes + (kRgMapBytes * 4);
constexpr qsizetype kRgFooterDwords = 4;
constexpr qsizetype kRgFooterBytes = 16;
constexpr qsizetype kRgTotalBytes = kRgHeaderBytes + (kRgSectionCount * kRgSectionBytes) + kRgFooterBytes;

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
    warning.append('\n');
  }
  warning.append(part.trimmed());
}

template <typename T>
bool readLittle(const QByteArray& data, qsizetype offset, T& value)
{
  if (offset < 0 || offset + static_cast<qsizetype>(sizeof(T)) > data.size()) {
    return false;
  }
  T tmp{};
  std::memcpy(&tmp, data.constData() + offset, sizeof(T));
  value = qFromLittleEndian(tmp);
  return true;
}

template <typename T>
bool readBig(const QByteArray& data, qsizetype offset, T& value)
{
  if (offset < 0 || offset + static_cast<qsizetype>(sizeof(T)) > data.size()) {
    return false;
  }
  T tmp{};
  std::memcpy(&tmp, data.constData() + offset, sizeof(T));
  value = qFromBigEndian(tmp);
  return true;
}

template <typename T>
void appendLittle(QByteArray& out, T value)
{
  T tmp = qToLittleEndian(value);
  out.append(reinterpret_cast<const char*>(&tmp), static_cast<qsizetype>(sizeof(T)));
}

template <typename T>
void appendBig(QByteArray& out, T value)
{
  T tmp = qToBigEndian(value);
  out.append(reinterpret_cast<const char*>(&tmp), static_cast<qsizetype>(sizeof(T)));
}

XngineWldFormat::Variant detectVariant(const QByteArray& bytes)
{
  if (bytes.size() == kRgTotalBytes) {
    quint32 h0 = 0, h6 = 0;
    if (readBig(bytes, 0, h0) && readBig(bytes, 24, h6) && h0 == 16U && h6 == 22U) {
      return XngineWldFormat::Variant::Redguard;
    }
  }

  qint32 width = 0, height = 0, offsetSize = 0;
  if (bytes.size() >= kDfHeaderSize &&
      readLittle(bytes, 0x00, offsetSize) &&
      readLittle(bytes, 0x04, width) &&
      readLittle(bytes, 0x08, height) &&
      width > 0 && height > 0 && offsetSize > 0 && (offsetSize % 4) == 0) {
    return XngineWldFormat::Variant::DaggerfallWoods;
  }

  return XngineWldFormat::Variant::Auto;
}

bool readDaggerfallWoods(const QByteArray& bytes, XngineWldFormat::Document& outDocument,
                         QString* errorMessage, bool strictValidation)
{
  using Doc = XngineWldFormat::Document;
  using Pixel = XngineWldFormat::DaggerfallPixelData;
  using Terrain = XngineWldFormat::DaggerfallTerrainParameter;

  if (bytes.size() < kDfHeaderSize) {
    return setError(errorMessage, "WOODS.WLD is too small");
  }

  outDocument = {};
  outDocument.variant = XngineWldFormat::Variant::DaggerfallWoods;
  auto& d = outDocument.daggerfallWoods;

  if (!readLittle(bytes, 0x00, d.offsetSize) ||
      !readLittle(bytes, 0x04, d.width) ||
      !readLittle(bytes, 0x08, d.height) ||
      !readLittle(bytes, 0x0C, d.nullValue1) ||
      !readLittle(bytes, 0x10, d.terrainTypesOffset) ||
      !readLittle(bytes, 0x14, d.constant1) ||
      !readLittle(bytes, 0x18, d.constant2) ||
      !readLittle(bytes, 0x1C, d.elevationMapOffset)) {
    return setError(errorMessage, "Failed parsing Daggerfall WLD header");
  }

  d.nullValue2.reserve(28);
  for (int i = 0; i < 28; ++i) {
    qint32 v = 0;
    if (!readLittle(bytes, 0x20 + i * 4, v)) {
      return setError(errorMessage, "Failed reading Daggerfall WLD header null array");
    }
    d.nullValue2.push_back(v);
  }

  if (d.width <= 0 || d.height <= 0) {
    return setError(errorMessage, "Daggerfall WLD header width/height invalid");
  }

  const qsizetype pixelCount = static_cast<qsizetype>(d.width) * static_cast<qsizetype>(d.height);
  const qsizetype offsetBytes = static_cast<qsizetype>(d.offsetSize);
  if (offsetBytes <= 0 || (offsetBytes % 4) != 0) {
    return setError(errorMessage, "Daggerfall WLD offset size invalid");
  }
  if (kDfHeaderSize + offsetBytes > bytes.size()) {
    return setError(errorMessage, "Daggerfall WLD offset list exceeds file size");
  }
  if ((offsetBytes / 4) != pixelCount) {
    appendWarning(outDocument.warning,
                  QString("Header offset-size implies %1 pixels, but width*height is %2")
                      .arg(offsetBytes / 4).arg(pixelCount));
    if (strictValidation) {
      // keep warning-only to preserve current behavior
    }
  }

  d.pixelOffsets.reserve(offsetBytes / 4);
  for (qsizetype i = 0; i < (offsetBytes / 4); ++i) {
    quint32 off = 0;
    if (!readLittle(bytes, kDfHeaderSize + i * 4, off)) {
      return setError(errorMessage, "Failed parsing Daggerfall WLD pixel offset table");
    }
    d.pixelOffsets.push_back(off);
  }

  if (d.terrainTypesOffset < 0 ||
      static_cast<qsizetype>(d.terrainTypesOffset) + (256 * 4) > bytes.size()) {
    return setError(errorMessage, "Daggerfall WLD terrain type section is out of range");
  }
  d.terrainTypes.reserve(256);
  for (int i = 0; i < 256; ++i) {
    const qsizetype off = static_cast<qsizetype>(d.terrainTypesOffset) + i * 4;
    Terrain t;
    t.clampHeight = static_cast<quint8>(bytes.at(off + 0));
    t.clampSensitivity = static_cast<quint8>(bytes.at(off + 1));
    t.unknown2 = static_cast<quint8>(bytes.at(off + 2));
    t.unknown3 = static_cast<quint8>(bytes.at(off + 3));
    d.terrainTypes.push_back(t);
  }

  if (d.elevationMapOffset < 0 ||
      static_cast<qsizetype>(d.elevationMapOffset) + pixelCount > bytes.size()) {
    return setError(errorMessage, "Daggerfall WLD elevation map is out of range");
  }
  d.elevationMap.reserve(pixelCount);
  for (qsizetype i = 0; i < pixelCount; ++i) {
    d.elevationMap.push_back(static_cast<qint8>(
        bytes.at(static_cast<qsizetype>(d.elevationMapOffset) + i)));
  }

  d.pixelData.reserve(d.pixelOffsets.size());
  for (qsizetype i = 0; i < d.pixelOffsets.size(); ++i) {
    const quint32 recordOff = d.pixelOffsets.at(i);
    if (recordOff == 0 || static_cast<qsizetype>(recordOff) + kDfPixelDataSize > bytes.size()) {
      d.pixelData.push_back(Pixel{});
      appendWarning(outDocument.warning, QString("PixelData[%1] offset out of range").arg(i));
      continue;
    }

    const qsizetype off = static_cast<qsizetype>(recordOff);
    Pixel p{};
    if (!readLittle(bytes, off + 0, p.unknown1) ||
        !readLittle(bytes, off + 2, p.nullValue1) ||
        !readLittle(bytes, off + 6, p.fileIndex) ||
        !readLittle(bytes, off + 10, p.nullValue2a) ||
        !readLittle(bytes, off + 14, p.nullValue2b) ||
        !readLittle(bytes, off + 18, p.nullValue2c)) {
      return setError(errorMessage, QString("Failed parsing Daggerfall PixelData[%1]").arg(i));
    }
    p.terrainType = static_cast<quint8>(bytes.at(off + 8));
    p.terrainNoise = static_cast<quint8>(bytes.at(off + 9));
    for (int n = 0; n < 25; ++n) {
      p.elevationNoise[n] = static_cast<qint8>(bytes.at(off + 22 + n));
    }
    d.pixelData.push_back(p);
  }

  return true;
}

bool writeDaggerfallWoods(const QString& filePath, const XngineWldFormat::Document& document,
                          QString* errorMessage)
{
  const auto& d = document.daggerfallWoods;
  const qsizetype pixelCount = static_cast<qsizetype>(d.width) * static_cast<qsizetype>(d.height);
  if (d.width <= 0 || d.height <= 0) {
    return setError(errorMessage, "Daggerfall WLD write: width/height invalid");
  }
  if (d.nullValue2.size() != 28) {
    return setError(errorMessage, "Daggerfall WLD write: header null array must contain 28 values");
  }
  if (d.pixelOffsets.size() != (d.offsetSize / 4)) {
    return setError(errorMessage, "Daggerfall WLD write: pixel offset count does not match offsetSize");
  }
  if (d.terrainTypes.size() != 256) {
    return setError(errorMessage, "Daggerfall WLD write: terrain type count must be 256");
  }
  if (d.elevationMap.size() != pixelCount) {
    return setError(errorMessage, "Daggerfall WLD write: elevation map size mismatch");
  }
  if (d.pixelData.size() != d.pixelOffsets.size()) {
    return setError(errorMessage, "Daggerfall WLD write: pixel data count mismatch");
  }

  QByteArray out;
  out.reserve(kDfHeaderSize + static_cast<qsizetype>(d.offsetSize) + d.elevationMap.size() + d.pixelData.size() * kDfPixelDataSize);

  appendLittle(out, d.offsetSize);
  appendLittle(out, d.width);
  appendLittle(out, d.height);
  appendLittle(out, d.nullValue1);
  appendLittle(out, d.terrainTypesOffset);
  appendLittle(out, d.constant1);
  appendLittle(out, d.constant2);
  appendLittle(out, d.elevationMapOffset);
  for (const qint32 v : d.nullValue2) {
    appendLittle(out, v);
  }
  if (out.size() != kDfHeaderSize) {
    return setError(errorMessage, "Daggerfall WLD write: internal header size mismatch");
  }

  for (const quint32 off : d.pixelOffsets) {
    appendLittle(out, off);
  }

  auto ensureSize = [&](qsizetype target) {
    if (out.size() < target) {
      out.append(QByteArray(target - out.size(), '\0'));
    }
  };

  ensureSize(d.terrainTypesOffset);
  for (const auto& t : d.terrainTypes) {
    out.append(static_cast<char>(t.clampHeight));
    out.append(static_cast<char>(t.clampSensitivity));
    out.append(static_cast<char>(t.unknown2));
    out.append(static_cast<char>(t.unknown3));
  }

  ensureSize(d.elevationMapOffset);
  for (const qint8 e : d.elevationMap) {
    out.append(static_cast<char>(e));
  }

  for (int i = 0; i < d.pixelData.size(); ++i) {
    const quint32 recordOff = d.pixelOffsets.at(i);
    if (recordOff == 0) {
      continue;
    }
    const qsizetype off = static_cast<qsizetype>(recordOff);
    ensureSize(off);
    QByteArray rec;
    rec.reserve(kDfPixelDataSize);
    const auto& p = d.pixelData.at(i);
    appendLittle(rec, p.unknown1);
    appendLittle(rec, p.nullValue1);
    appendLittle(rec, p.fileIndex);
    rec.append(static_cast<char>(p.terrainType));
    rec.append(static_cast<char>(p.terrainNoise));
    appendLittle(rec, p.nullValue2a);
    appendLittle(rec, p.nullValue2b);
    appendLittle(rec, p.nullValue2c);
    for (int n = 0; n < 25; ++n) {
      rec.append(static_cast<char>(p.elevationNoise[n]));
    }
    if (rec.size() != kDfPixelDataSize) {
      return setError(errorMessage, QString("Daggerfall WLD write: PixelData[%1] size mismatch").arg(i));
    }
    if (out.size() == off) {
      out.append(rec);
    } else {
      ensureSize(off + rec.size());
      for (int n = 0; n < rec.size(); ++n) {
        out[off + n] = rec.at(n);
      }
    }
  }

  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return setError(errorMessage, QString("Unable to write WLD file: %1").arg(filePath));
  }
  if (file.write(out) != out.size()) {
    return setError(errorMessage, "Failed writing Daggerfall WLD bytes");
  }
  return true;
}

bool readRedguardWld(const QByteArray& bytes, XngineWldFormat::Document& outDocument,
                     QString* errorMessage, bool strictValidation)
{
  Q_UNUSED(strictValidation);
  outDocument = {};
  outDocument.variant = XngineWldFormat::Variant::Redguard;

  if (bytes.size() < kRgTotalBytes) {
    return setError(errorMessage, "Redguard WLD is too small");
  }
  if (bytes.size() != kRgTotalBytes) {
    appendWarning(outDocument.warning,
                  QString("Unexpected Redguard WLD size %1 (expected %2)")
                      .arg(bytes.size()).arg(kRgTotalBytes));
  }
  auto& d = outDocument.redguard;

  d.headerDwords.reserve(kRgHeaderDwords);
  for (qsizetype i = 0; i < kRgHeaderDwords; ++i) {
    quint32 v = 0;
    if (!readBig(bytes, i * 4, v)) {
      return setError(errorMessage, "Failed parsing Redguard WLD header");
    }
    d.headerDwords.push_back(v);
  }

  d.sections.reserve(kRgSectionCount);
  qsizetype sectionBase = kRgHeaderBytes;
  for (qsizetype s = 0; s < kRgSectionCount; ++s) {
    if (sectionBase + kRgSectionBytes > bytes.size()) {
      return setError(errorMessage, QString("Redguard WLD section %1 out of range").arg(s));
    }

    XngineWldFormat::RedguardSection sec;
    sec.headerWords.reserve(kRgSectionHeaderWords);
    for (qsizetype i = 0; i < kRgSectionHeaderWords; ++i) {
      quint16 w = 0;
      if (!readBig(bytes, sectionBase + i * 2, w)) {
        return setError(errorMessage, QString("Failed parsing Redguard section %1 header").arg(s));
      }
      sec.headerWords.push_back(w);
    }

    qsizetype mapBase = sectionBase + kRgSectionHeaderBytes;
    sec.map1 = QVector<quint8>(kRgMapBytes);
    sec.map2 = QVector<quint8>(kRgMapBytes);
    sec.map3 = QVector<quint8>(kRgMapBytes);
    sec.map4 = QVector<quint8>(kRgMapBytes);
    for (qsizetype i = 0; i < kRgMapBytes; ++i) {
      sec.map1[i] = static_cast<quint8>(bytes.at(mapBase + i));
      sec.map2[i] = static_cast<quint8>(bytes.at(mapBase + kRgMapBytes + i));
      sec.map3[i] = static_cast<quint8>(bytes.at(mapBase + (kRgMapBytes * 2) + i));
      sec.map4[i] = static_cast<quint8>(bytes.at(mapBase + (kRgMapBytes * 3) + i));
    }
    d.sections.push_back(sec);
    sectionBase += kRgSectionBytes;
  }

  d.footerDwords.reserve(kRgFooterDwords);
  const qsizetype footerBase = bytes.size() - kRgFooterBytes;
  for (qsizetype i = 0; i < kRgFooterDwords; ++i) {
    quint32 v = 0;
    if (!readBig(bytes, footerBase + i * 4, v)) {
      return setError(errorMessage, "Failed parsing Redguard WLD footer");
    }
    d.footerDwords.push_back(v);
  }

  return true;
}

bool writeRedguardWld(const QString& filePath, const XngineWldFormat::Document& document,
                      QString* errorMessage)
{
  const auto& d = document.redguard;
  if (d.headerDwords.size() != kRgHeaderDwords) {
    return setError(errorMessage, "Redguard WLD write: header must contain 296 dwords");
  }
  if (d.sections.size() != kRgSectionCount) {
    return setError(errorMessage, "Redguard WLD write: must contain 4 sections");
  }
  if (d.footerDwords.size() != kRgFooterDwords) {
    return setError(errorMessage, "Redguard WLD write: footer must contain 4 dwords");
  }

  QByteArray out;
  out.reserve(kRgTotalBytes);

  for (quint32 v : d.headerDwords) {
    appendBig(out, v);
  }
  for (int s = 0; s < d.sections.size(); ++s) {
    const auto& sec = d.sections.at(s);
    if (sec.headerWords.size() != kRgSectionHeaderWords) {
      return setError(errorMessage, QString("Redguard WLD write: section %1 header must contain 11 words").arg(s));
    }
    if (sec.map1.size() != kRgMapBytes || sec.map2.size() != kRgMapBytes ||
        sec.map3.size() != kRgMapBytes || sec.map4.size() != kRgMapBytes) {
      return setError(errorMessage, QString("Redguard WLD write: section %1 maps must be 16384 bytes each").arg(s));
    }
    for (quint16 w : sec.headerWords) {
      appendBig(out, w);
    }
    for (quint8 b : sec.map1) out.append(static_cast<char>(b));
    for (quint8 b : sec.map2) out.append(static_cast<char>(b));
    for (quint8 b : sec.map3) out.append(static_cast<char>(b));
    for (quint8 b : sec.map4) out.append(static_cast<char>(b));
  }
  for (quint32 v : d.footerDwords) {
    appendBig(out, v);
  }

  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return setError(errorMessage, QString("Unable to write WLD file: %1").arg(filePath));
  }
  if (file.write(out) != out.size()) {
    return setError(errorMessage, "Failed writing Redguard WLD bytes");
  }
  return true;
}

}  // namespace

bool XngineWldFormat::readFile(const QString& filePath, Document& outDocument,
                               QString* errorMessage, const Traits& traits)
{
  outDocument = {};

  QFile f(filePath);
  if (!f.open(QIODevice::ReadOnly)) {
    return setError(errorMessage, QString("Unable to open WLD file: %1").arg(filePath));
  }
  const QByteArray bytes = f.readAll();

  Variant variant = traits.variant;
  if (variant == Variant::Auto) {
    variant = detectVariant(bytes);
    if (variant == Variant::Auto) {
      return setError(errorMessage, "Unable to detect WLD variant");
    }
  }

  switch (variant) {
    case Variant::DaggerfallWoods:
      return readDaggerfallWoods(bytes, outDocument, errorMessage, traits.strictValidation);
    case Variant::Redguard:
      return readRedguardWld(bytes, outDocument, errorMessage, traits.strictValidation);
    case Variant::Auto:
    default:
      return setError(errorMessage, "Unsupported/unknown WLD variant");
  }
}

bool XngineWldFormat::writeFile(const QString& filePath, const Document& document,
                                QString* errorMessage, const Traits& traits)
{
  Variant variant = (traits.variant == Variant::Auto) ? document.variant : traits.variant;
  switch (variant) {
    case Variant::DaggerfallWoods:
      return writeDaggerfallWoods(filePath, document, errorMessage);
    case Variant::Redguard:
      return writeRedguardWld(filePath, document, errorMessage);
    case Variant::Auto:
    default:
      return setError(errorMessage, "WLD write: unspecified variant");
  }
}

int XngineWldFormat::daggerfallMapIndex(const DaggerfallWoodsData& data, int x, int y)
{
  if (x < 0 || y < 0 || x >= data.width || y >= data.height) {
    return -1;
  }
  return y * data.width + x;
}

QVector<quint8> XngineWldFormat::redguardCombinedMap(const RedguardData& data, int planeIndex)
{
  if (planeIndex < 0 || planeIndex > 3 || data.sections.size() < 4) {
    return {};
  }

  QVector<quint8> out(256 * 256);
  auto sectionPlane = [&](const RedguardSection& sec) -> const QVector<quint8>* {
    switch (planeIndex) {
      case 0: return &sec.map1;
      case 1: return &sec.map2;
      case 2: return &sec.map3;
      case 3: return &sec.map4;
      default: return nullptr;
    }
  };

  for (int sy = 0; sy < 2; ++sy) {
    for (int sx = 0; sx < 2; ++sx) {
      const int sectionIndex = sy * 2 + sx;  // 0,1 / 2,3
      if (sectionIndex >= data.sections.size()) {
        continue;
      }
      const QVector<quint8>* plane = sectionPlane(data.sections.at(sectionIndex));
      if (plane == nullptr || plane->size() != kRgMapBytes) {
        continue;
      }
      for (int y = 0; y < 128; ++y) {
        for (int x = 0; x < 128; ++x) {
          out[(sy * 128 + y) * 256 + (sx * 128 + x)] = plane->at(y * 128 + x);
        }
      }
    }
  }
  return out;
}
