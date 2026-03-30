#ifndef REDGUARDSGXAFORMAT_H
#define REDGUARDSGXAFORMAT_H

#include <xnginepaletteformat.h>

#include <QByteArray>
#include <QImage>
#include <QString>
#include <QVector>

class RedguardsGxaFormat
{
public:
  enum class ImageEncoding
  {
    RawIndexed,
    EncodedType1,
    EncodedType2
  };

  struct ChunkHeader
  {
    QString tag;
    quint32 length = 0;
    qsizetype payloadOffset = 0;
  };

  struct Bmhd
  {
    QString title;
    QByteArray unknownBytes;
    quint16 imageCount = 0;
  };

  struct Image
  {
    qint16 unknown0 = 0;
    quint16 width = 0;
    quint16 height = 0;
    QVector<qint16> unknownFields;
    ImageEncoding encoding = ImageEncoding::RawIndexed;
    QByteArray imageData;
  };

  struct Document
  {
    QVector<ChunkHeader> chunks;
    Bmhd bmhd;
    XnginePaletteFormat::Palette palette;
    QVector<Image> images;
    QString warning;
  };

public:
  static bool loadFile(const QString& filePath, Document& outDocument,
                       QString* errorMessage = nullptr);
  static bool parseBytes(const QByteArray& bytes, Document& outDocument,
                         QString* errorMessage = nullptr);

  static bool decodeImage(const Image& image,
                          const XnginePaletteFormat::Palette& palette,
                          QImage& outImage,
                          QString* errorMessage = nullptr);
};

#endif  // REDGUARDSGXAFORMAT_H
