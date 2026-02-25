#include "xnginepaletteformat.h"

#include <QFile>
#include <QFileInfo>
#include <QtEndian>

#include <cstring>

namespace {

bool setError(QString* errorMessage, const QString& error)
{
  if (errorMessage != nullptr) {
    *errorMessage = error;
  }
  return false;
}

bool readLE32U(const QByteArray& data, qsizetype off, quint32& value)
{
  if (off < 0 || off + 4 > data.size()) {
    return false;
  }
  quint32 v = 0;
  std::memcpy(&v, data.constData() + off, sizeof(v));
  value = qFromLittleEndian(v);
  return true;
}

bool readLE16U(const QByteArray& data, qsizetype off, quint16& value)
{
  if (off < 0 || off + 2 > data.size()) {
    return false;
  }
  quint16 v = 0;
  std::memcpy(&v, data.constData() + off, sizeof(v));
  value = qFromLittleEndian(v);
  return true;
}

void writeLE32U(QByteArray& out, quint32 value)
{
  const quint32 le = qToLittleEndian(value);
  out.append(reinterpret_cast<const char*>(&le), sizeof(le));
}

void writeLE16U(QByteArray& out, quint16 value)
{
  const quint16 le = qToLittleEndian(value);
  out.append(reinterpret_cast<const char*>(&le), sizeof(le));
}

void finalizePalette(const QByteArray& rgbBytes, XnginePaletteFormat::Palette& outPalette)
{
  outPalette = {};
  outPalette.rgbBytes = rgbBytes;
  outPalette.sixBitRange = true;
  for (qsizetype i = 0; i < rgbBytes.size(); ++i) {
    if (static_cast<quint8>(rgbBytes.at(i)) > 63) {
      outPalette.sixBitRange = false;
      break;
    }
  }

  outPalette.colors.reserve(256);
  for (int i = 0; i < 256; ++i) {
    int r = static_cast<quint8>(rgbBytes.at(i * 3 + 0));
    int g = static_cast<quint8>(rgbBytes.at(i * 3 + 1));
    int b = static_cast<quint8>(rgbBytes.at(i * 3 + 2));
    if (outPalette.sixBitRange) {
      r = (r * 255) / 63;
      g = (g * 255) / 63;
      b = (b * 255) / 63;
    }
    outPalette.colors.push_back(QColor(r, g, b));
  }
}

XnginePaletteFormat::Variant detectVariant(const QByteArray& bytes,
                                           const XnginePaletteFormat::Traits& traits,
                                           bool* usedTrailing)
{
  if (usedTrailing != nullptr) {
    *usedTrailing = false;
  }

  if (bytes.size() == 768) {
    return XnginePaletteFormat::Variant::RawRgb256;
  }
  if (bytes.size() == 776) {
    return XnginePaletteFormat::Variant::HeaderedRgb256;
  }
  if (traits.allowTrailingPaletteData && bytes.size() > 768) {
    if (usedTrailing != nullptr) {
      *usedTrailing = true;
    }
    return XnginePaletteFormat::Variant::RawRgb256;
  }
  return XnginePaletteFormat::Variant::Auto;
}

}  // namespace

bool XnginePaletteFormat::parseRawRgb256(const QByteArray& bytes, Palette& outPalette,
                                         QString* errorMessage)
{
  if (bytes.size() != 768) {
    return setError(errorMessage, QString("RawRgb256 requires exactly 768 bytes, got %1")
                                      .arg(bytes.size()));
  }

  finalizePalette(bytes, outPalette);
  return true;
}

bool XnginePaletteFormat::parseHeaderedRgb256(const QByteArray& bytes, Header& outHeader,
                                              Palette& outPalette, QString* errorMessage)
{
  outHeader = {};
  if (bytes.size() != 776) {
    return setError(errorMessage, QString("HeaderedRgb256 requires exactly 776 bytes, got %1")
                                      .arg(bytes.size()));
  }

  if (!readLE32U(bytes, 0, outHeader.length) || !readLE16U(bytes, 4, outHeader.major) ||
      !readLE16U(bytes, 6, outHeader.minor)) {
    return setError(errorMessage, "Invalid palette header");
  }

  finalizePalette(bytes.mid(8, 768), outPalette);
  return true;
}

bool XnginePaletteFormat::writeRawRgb256(const Palette& palette, QByteArray& outBytes,
                                         QString* errorMessage)
{
  if (palette.rgbBytes.size() != 768) {
    return setError(errorMessage, QString("RawRgb256 write requires 768 rgb bytes, got %1")
                                      .arg(palette.rgbBytes.size()));
  }
  outBytes = palette.rgbBytes;
  return true;
}

bool XnginePaletteFormat::writeHeaderedRgb256(const Header& header, const Palette& palette,
                                              QByteArray& outBytes, QString* errorMessage)
{
  if (palette.rgbBytes.size() != 768) {
    return setError(errorMessage, QString("HeaderedRgb256 write requires 768 rgb bytes, got %1")
                                      .arg(palette.rgbBytes.size()));
  }

  outBytes.clear();
  outBytes.reserve(776);
  writeLE32U(outBytes, header.length);
  writeLE16U(outBytes, header.major);
  writeLE16U(outBytes, header.minor);
  outBytes.append(palette.rgbBytes);
  return true;
}

bool XnginePaletteFormat::parseBytes(const QByteArray& bytes, Document& outDocument,
                                     QString* errorMessage, const Traits& traits)
{
  outDocument = {};

  bool usedTrailing = false;
  Variant variant = traits.variant;
  if (variant == Variant::Auto) {
    variant = detectVariant(bytes, traits, &usedTrailing);
    if (variant == Variant::Auto) {
      return setError(errorMessage, QString("Unable to detect palette format for %1 bytes")
                                        .arg(bytes.size()));
    }
  }

  QByteArray parseBytes = bytes;
  if (variant == Variant::RawRgb256 && parseBytes.size() != 768) {
    if (!(traits.allowTrailingPaletteData && parseBytes.size() >= 768)) {
      return setError(errorMessage, QString("RawRgb256 requires 768 bytes (or trailing palette), got %1")
                                        .arg(parseBytes.size()));
    }
    parseBytes = parseBytes.right(768);
    usedTrailing = true;
  }

  outDocument.variant = variant;
  outDocument.usedTrailingPaletteData = usedTrailing;
  if (usedTrailing) {
    outDocument.warning = "Used trailing 768 bytes as palette data.";
  }

  switch (variant) {
    case Variant::RawRgb256:
      return parseRawRgb256(parseBytes, outDocument.palette, errorMessage);
    case Variant::HeaderedRgb256:
      return parseHeaderedRgb256(parseBytes, outDocument.headered, outDocument.palette,
                                 errorMessage);
    case Variant::Auto:
    default:
      return setError(errorMessage, "Unsupported/unknown palette variant");
  }
}

bool XnginePaletteFormat::writeBytes(const Document& document, QByteArray& outBytes,
                                     QString* errorMessage, const Traits& traits)
{
  Variant variant = (traits.variant == Variant::Auto) ? document.variant : traits.variant;

  switch (variant) {
    case Variant::RawRgb256:
      return writeRawRgb256(document.palette, outBytes, errorMessage);
    case Variant::HeaderedRgb256:
      return writeHeaderedRgb256(document.headered, document.palette, outBytes, errorMessage);
    case Variant::Auto:
    default:
      return setError(errorMessage, "Palette write: unspecified variant");
  }
}

bool XnginePaletteFormat::readFile(const QString& filePath, Document& outDocument,
                                   QString* errorMessage, const Traits& traits)
{
  QFile f(filePath);
  if (!f.open(QIODevice::ReadOnly)) {
    return setError(errorMessage, QString("Unable to open palette file: %1").arg(filePath));
  }
  return parseBytes(f.readAll(), outDocument, errorMessage, traits);
}

bool XnginePaletteFormat::writeFile(const QString& filePath, const Document& document,
                                    QString* errorMessage, const Traits& traits)
{
  QByteArray outBytes;
  if (!writeBytes(document, outBytes, errorMessage, traits)) {
    return false;
  }

  QFile f(filePath);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return setError(errorMessage, QString("Unable to write palette file: %1").arg(filePath));
  }
  if (f.write(outBytes) != outBytes.size()) {
    return setError(errorMessage, "Failed writing palette bytes");
  }
  return true;
}
