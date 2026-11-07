#ifndef REDGUARDSFNTFORMAT_H
#define REDGUARDSFNTFORMAT_H

#include <xnginepaletteformat.h>

#include <QByteArray>
#include <QImage>
#include <QString>
#include <QVector>

// Chunked font file with indexed glyphs and optional raw RDAT metadata.
// Unknown RDAT layouts remain available without guessed field mappings.
class RedguardsFntFormat
{
public:
  struct ChunkHeader
  {
    QString tag;
    quint32 length = 0;
    qsizetype payloadOffset = 0;
  };

  struct Fnhd
  {
    QString description;
    quint16 unknown24 = 0;
    quint16 hasRdat = 0;
    quint16 unknown28 = 0;
    quint16 unknown2A = 0;
    quint16 unknown2C = 0;
    quint16 maxWidth = 0;
    quint16 lineHeight = 0;
    quint16 characterStart = 0;
    quint16 characterCount = 0;
    quint16 unknown36 = 0;
    quint16 unknown38 = 0;
    quint16 unknown3A = 0;
  };

  struct Glyph
  {
    quint16 isEnabled = 0;
    qint16 offsetLeft = 0;
    qint16 offsetTop = 0;
    quint16 width = 0;
    quint16 height = 0;
    QByteArray pixelIndices;
  };

  struct Rdat
  {
    QByteArray rawBytes;
    QString sourceFile;
    quint32 unknown90 = 0;
    quint32 unknown94 = 0;
    quint32 unknown98 = 0;
    quint32 unknown9C = 0;
    quint32 unknownA0 = 0;
    quint32 unknownA4 = 0;
    quint32 unknownA8 = 0;
    quint32 unknownAC = 0;
    quint32 unknownB0 = 0;
    quint8 unknownB4 = 0;
    // True only for the recognized 173-byte RDAT payload shape.
    bool parsedFields = false;
  };

  struct Document
  {
    QVector<ChunkHeader> chunks;
    Fnhd fnhd;
    XnginePaletteFormat::Palette palette;
    QVector<Glyph> glyphs;
    bool hasRdat = false;
    Rdat rdat;
    QString warning;
  };

public:
  static bool loadFile(const QString& filePath, Document& outDocument,
                       QString* errorMessage = nullptr);
  static bool parseBytes(const QByteArray& bytes, Document& outDocument,
                         QString* errorMessage = nullptr);

  // Converts parsed glyph indices with the supplied palette; it does not
  // independently validate the original file's chunk structure.
  static bool decodeGlyphImage(const Glyph& glyph,
                               const XnginePaletteFormat::Palette& palette,
                               QImage& outImage,
                               QString* errorMessage = nullptr);
};

#endif  // REDGUARDSFNTFORMAT_H
