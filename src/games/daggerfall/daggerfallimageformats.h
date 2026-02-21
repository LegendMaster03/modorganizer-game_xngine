#ifndef DAGGERFALL_IMAGEFORMATS_H
#define DAGGERFALL_IMAGEFORMATS_H

#include <QColor>
#include <QImage>
#include <QVector>
#include <QString>
#include <QtGlobal>

namespace Daggerfall
{
namespace Image
{

struct PaletteFile
{
  bool isColFile = false;
  quint32 colLength = 0;
  quint16 colMajor = 0;
  quint16 colMinor = 0;
  QVector<QColor> colors;  // 256
};

struct ColourTranslationFile
{
  QVector<QByteArray> tables;  // 64 entries, each 256 bytes
};

struct ImgHeader
{
  qint16 xOffset = 0;
  qint16 yOffset = 0;
  qint16 width = 0;
  qint16 height = 0;
  quint16 compression = 0;
  qint16 pixelDataLength = 0;
};

enum class ImgCompression : quint16
{
  Uncompressed = 0x0000,
  RleCompressed = 0x0002,
  ImageRle = 0x0108,
  RecordRle = 0x1108
};

struct ImgFile
{
  ImgHeader header;
  QByteArray pixelDataRaw;
  QByteArray pixelDataDecoded;  // width*height indexed
  QString warning;
};

struct RciFile
{
  qint16 width = 0;
  qint16 height = 0;
  QVector<QByteArray> frames;  // indexed, uncompressed
  bool hasEmbeddedPalette = false;
  PaletteFile embeddedPalette;
};

struct SkyFile
{
  QVector<PaletteFile> palettes;                 // 32
  QVector<ColourTranslationFile> translations;   // 32
  QVector<QByteArray> images;                    // 64, 512*220 each indexed
  QString warning;
};

struct CfaFile
{
  quint16 widthUncompressed = 0;
  quint16 height = 0;
  quint16 widthCompressed = 0;
  quint16 unknown1 = 0;
  quint16 unknown2 = 0;
  quint8 bitsPerPixel = 0;
  quint8 frameCount = 0;
  quint16 headerSize = 0;
  QVector<QByteArray> frames;  // decoded indexed
  QString warning;
};

struct CifAnimationHeader
{
  quint16 width = 0;
  quint16 height = 0;
  quint16 lastFrameWidth = 0;
  qint16 xOffset = 0;
  qint16 lastFrameYOffset = 0;
  qint16 dataLength = 0;
  QVector<quint16> frameDataOffsets;  // 31 entries
  quint16 totalSize = 0;
};

struct CifAnimation
{
  CifAnimationHeader header;
  QVector<QByteArray> frames;  // decoded indexed
  QString warning;
};

struct CifFile
{
  bool isFacesRci = false;
  QVector<ImgFile> images;       // used by single/magic CIF
  QVector<CifAnimation> anims;   // used by weapon CIF
  QString warning;
};

struct TextureFileHeader
{
  qint16 recordCount = 0;
  QString name;
};

struct TextureRecordHeader
{
  quint16 unknown1 = 0;
  qint32 recordPosition = 0;
  quint16 unknown2 = 0;
  quint32 unknown3 = 0;
  quint64 nullValue = 0;
};

struct TextureRecord
{
  qint16 offsetX = 0;
  qint16 offsetY = 0;
  qint16 width = 0;
  qint16 height = 0;
  quint16 compression = 0;
  quint32 recordSize = 0;
  quint32 dataOffset = 0;
  quint16 isNormal = 0;
  quint16 frameCount = 0;
  quint16 unknown1 = 0;
  qint16 xScale = 0;
  qint16 yScale = 0;
  QVector<QByteArray> frames;  // decoded indexed when supported
  QString warning;
};

struct TextureFile
{
  TextureFileHeader header;
  QVector<TextureRecordHeader> recordHeaders;
  QVector<TextureRecord> records;
  QString warning;
};

bool loadPaletteFile(const QString& path, PaletteFile& out, QString* errorMessage = nullptr);
bool loadColourTranslationFile(const QString& path, ColourTranslationFile& out,
                               QString* errorMessage = nullptr);
bool loadImgFile(const QString& path, ImgFile& out, QString* errorMessage = nullptr);
bool loadRciFile(const QString& path, qint16 width, qint16 height, RciFile& out,
                 QString* errorMessage = nullptr);
bool loadSkyFile(const QString& path, SkyFile& out, QString* errorMessage = nullptr);
bool loadCfaFile(const QString& path, CfaFile& out, QString* errorMessage = nullptr);
bool loadCifFile(const QString& path, CifFile& out, QString* errorMessage = nullptr);
bool loadTextureFile(const QString& path, TextureFile& out, QString* errorMessage = nullptr);

QImage toImage(const QByteArray& indexedPixels, int width, int height, const PaletteFile& palette);
QByteArray applyColourTranslation(const QByteArray& indexedPixels, const QByteArray& xlat256);

}  // namespace Image
}  // namespace Daggerfall

#endif  // DAGGERFALL_IMAGEFORMATS_H
