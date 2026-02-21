#include "daggerfallwoodswld.h"
#include "daggerfallformatutils.h"

#include <QFile>

namespace {

constexpr qsizetype kHeaderSize = 0x90;
constexpr qsizetype kPixelDataSize = 47;

using Daggerfall::FormatUtil::appendWarning;
using Daggerfall::FormatUtil::readLE16U;
using Daggerfall::FormatUtil::readLE32;
using Daggerfall::FormatUtil::readLE32U;
using Daggerfall::FormatUtil::setError;

}  // namespace

bool DaggerfallWoodsWld::load(const QString& filePath, Data& outData,
                              QString* errorMessage)
{
  outData = {};

  QFile f(filePath);
  if (!f.open(QIODevice::ReadOnly)) {
    return setError(errorMessage, QString("Unable to open WOODS.WLD: %1").arg(filePath));
  }
  const QByteArray bytes = f.readAll();
  if (bytes.size() < kHeaderSize) {
    return setError(errorMessage, "WOODS.WLD is too small");
  }

  auto& h = outData.header;
  if (!readLE32(bytes, 0x00, h.offsetSize) ||
      !readLE32(bytes, 0x04, h.width) ||
      !readLE32(bytes, 0x08, h.height) ||
      !readLE32(bytes, 0x0C, h.nullValue1) ||
      !readLE32(bytes, 0x10, h.terrainTypesOffset) ||
      !readLE32U(bytes, 0x14, h.constant1) ||
      !readLE32U(bytes, 0x18, h.constant2) ||
      !readLE32(bytes, 0x1C, h.elevationMapOffset)) {
    return setError(errorMessage, "Failed parsing WOODS.WLD header");
  }

  h.nullValue2.reserve(28);
  for (int i = 0; i < 28; ++i) {
    qint32 v = 0;
    if (!readLE32(bytes, 0x20 + i * 4, v)) {
      return setError(errorMessage, "Failed reading WOODS.WLD header null array");
    }
    h.nullValue2.push_back(v);
  }

  if (h.width <= 0 || h.height <= 0) {
    return setError(errorMessage, "WOODS.WLD header width/height invalid");
  }
  const qsizetype pixelCount = static_cast<qsizetype>(h.width) * static_cast<qsizetype>(h.height);
  const qsizetype offsetBytes = static_cast<qsizetype>(h.offsetSize);
  if (offsetBytes <= 0 || (offsetBytes % 4) != 0) {
    return setError(errorMessage, "WOODS.WLD offset size invalid");
  }
  if (kHeaderSize + offsetBytes > bytes.size()) {
    return setError(errorMessage, "WOODS.WLD offset list exceeds file size");
  }
  if ((offsetBytes / 4) != pixelCount) {
    appendWarning(outData.warning,
                  QString("Header offset-size implies %1 pixels, but width*height is %2")
                      .arg(offsetBytes / 4).arg(pixelCount));
  }

  // Data offsets
  outData.pixelOffsets.reserve(offsetBytes / 4);
  for (qsizetype i = 0; i < (offsetBytes / 4); ++i) {
    quint32 off = 0;
    if (!readLE32U(bytes, kHeaderSize + i * 4, off)) {
      return setError(errorMessage, "Failed parsing WOODS.WLD pixel offset table");
    }
    outData.pixelOffsets.push_back(off);
  }

  // Terrain types
  if (h.terrainTypesOffset < 0 ||
      static_cast<qsizetype>(h.terrainTypesOffset) + 256 * 4 > bytes.size()) {
    return setError(errorMessage, "WOODS.WLD terrain type section is out of range");
  }
  outData.terrainTypes.reserve(256);
  for (int i = 0; i < 256; ++i) {
    const qsizetype off = static_cast<qsizetype>(h.terrainTypesOffset) + i * 4;
    TerrainParameter t;
    t.clampHeight = static_cast<quint8>(bytes.at(off + 0));
    t.clampSensitivity = static_cast<quint8>(bytes.at(off + 1));
    t.unknown2 = static_cast<quint8>(bytes.at(off + 2));
    t.unknown3 = static_cast<quint8>(bytes.at(off + 3));
    outData.terrainTypes.push_back(t);
  }

  // Elevation map
  if (h.elevationMapOffset < 0 ||
      static_cast<qsizetype>(h.elevationMapOffset) + pixelCount > bytes.size()) {
    return setError(errorMessage, "WOODS.WLD elevation map is out of range");
  }
  outData.elevationMap.reserve(pixelCount);
  for (qsizetype i = 0; i < pixelCount; ++i) {
    outData.elevationMap.push_back(static_cast<qint8>(
        bytes.at(static_cast<qsizetype>(h.elevationMapOffset) + i)));
  }

  // PixelData records
  outData.pixelData.reserve(outData.pixelOffsets.size());
  for (qsizetype i = 0; i < outData.pixelOffsets.size(); ++i) {
    const quint32 recordOff = outData.pixelOffsets.at(i);
    if (recordOff == 0 || static_cast<qsizetype>(recordOff) + kPixelDataSize > bytes.size()) {
      PixelData empty{};
      outData.pixelData.push_back(empty);
      appendWarning(outData.warning,
                    QString("PixelData[%1] offset out of range").arg(i));
      continue;
    }

    const qsizetype off = static_cast<qsizetype>(recordOff);
    PixelData p{};
    if (!readLE16U(bytes, off + 0, p.unknown1) ||
        !readLE32U(bytes, off + 2, p.nullValue1) ||
        !readLE16U(bytes, off + 6, p.fileIndex)) {
      return setError(errorMessage, QString("Failed parsing PixelData[%1] header").arg(i));
    }
    p.terrainType = static_cast<quint8>(bytes.at(off + 8));
    p.terrainNoise = static_cast<quint8>(bytes.at(off + 9));
    if (!readLE32U(bytes, off + 10, p.nullValue2a) ||
        !readLE32U(bytes, off + 14, p.nullValue2b) ||
        !readLE32U(bytes, off + 18, p.nullValue2c)) {
      return setError(errorMessage, QString("Failed parsing PixelData[%1] null values").arg(i));
    }
    for (int n = 0; n < 25; ++n) {
      p.elevationNoise[n] = static_cast<qint8>(bytes.at(off + 22 + n));
    }
    outData.pixelData.push_back(p);
  }

  return true;
}

int DaggerfallWoodsWld::mapIndex(const Data& data, int x, int y)
{
  if (x < 0 || y < 0 || x >= data.header.width || y >= data.header.height) {
    return -1;
  }
  return y * data.header.width + x;
}
