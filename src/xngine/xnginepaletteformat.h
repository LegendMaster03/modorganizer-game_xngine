#ifndef XNGINEPALETTEFORMAT_H
#define XNGINEPALETTEFORMAT_H

#include <QColor>
#include <QByteArray>
#include <QString>
#include <QVector>
#include <QtGlobal>

// Interprets fixed-size RGB palette layouts. Six-bit vs eight-bit color
// scaling is inferred from the entire byte range, not each pixel channel.
class XnginePaletteFormat
{
public:
  enum class Variant
  {
    Auto,
    RawRgb256,
    HeaderedRgb256
  };

  // Trailing-data compatibility can select the final 768 bytes of a
  // larger raw palette file; it is not a lossless passthrough of prefixes.
  struct Traits
  {
    Variant variant = Variant::Auto;
    bool strictValidation = true;
    bool allowTrailingPaletteData = true;
  };

  struct Palette
  {
    QByteArray rgbBytes;      // exactly 768 bytes (raw stored values)
    bool sixBitRange = false; // true if all components are <= 63
    QVector<QColor> colors;   // 256 colors, scaled to 8-bit
  };

  struct Header
  {
    quint32 length = 0;
    quint16 major = 0;
    quint16 minor = 0;
  };

  struct Document
  {
    Variant variant = Variant::Auto;
    Palette palette;
    Header headered;
    // Records when extra input bytes were discarded to parse a raw palette.
    bool usedTrailingPaletteData = false;
    QString warning;
  };

public:
  static bool readFile(const QString& filePath, Document& outDocument,
                       QString* errorMessage = nullptr,
                       const Traits& traits = Traits{});
  static bool writeFile(const QString& filePath, const Document& document,
                        QString* errorMessage = nullptr,
                        const Traits& traits = Traits{});

  static bool parseBytes(const QByteArray& bytes, Document& outDocument,
                         QString* errorMessage = nullptr,
                         const Traits& traits = Traits{});
  static bool writeBytes(const Document& document, QByteArray& outBytes,
                        QString* errorMessage = nullptr,
                        const Traits& traits = Traits{});

  static bool parseRawRgb256(const QByteArray& bytes, Palette& outPalette,
                             QString* errorMessage = nullptr);
  static bool parseHeaderedRgb256(const QByteArray& bytes, Header& outHeader,
                                  Palette& outPalette,
                                  QString* errorMessage = nullptr);

  static bool writeRawRgb256(const Palette& palette, QByteArray& outBytes,
                             QString* errorMessage = nullptr);
  static bool writeHeaderedRgb256(const Header& header, const Palette& palette,
                                  QByteArray& outBytes,
                                  QString* errorMessage = nullptr);
};

#endif  // XNGINEPALETTEFORMAT_H
