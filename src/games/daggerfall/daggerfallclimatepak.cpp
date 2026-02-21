#include "daggerfallclimatepak.h"
#include "daggerfallformatutils.h"

#include "daggerfallpak.h"

#include <QFile>

#include <algorithm>

namespace {

constexpr int kRows = 500;
constexpr int kRowWidth = 1001;

}  // namespace

bool DaggerfallClimatePak::load(const QString& filePath, Data& outData, QString* errorMessage)
{
  outData = {};
  outData.width = kRowWidth;
  outData.height = kRows;

  QFile f(filePath);
  if (!f.open(QIODevice::ReadOnly)) {
    return Daggerfall::FormatUtil::setError(errorMessage,
                                            QString("Unable to open CLIMATE.PAK: %1").arg(filePath));
  }
  const QByteArray bytes = f.readAll();
  if (bytes.size() < kRows * 4) {
    return Daggerfall::FormatUtil::setError(errorMessage, "CLIMATE.PAK too small for offset list");
  }

  outData.offsets.reserve(kRows);
  for (int i = 0; i < kRows; ++i) {
    quint32 off = 0;
    if (!Daggerfall::FormatUtil::readLE32U(bytes, static_cast<qsizetype>(i) * 4, off)) {
      return Daggerfall::FormatUtil::setError(errorMessage, "Failed reading CLIMATE.PAK offset list");
    }
    outData.offsets.push_back(off);
  }

  const qsizetype dataStart = kRows * 4;
  QVector<quint32> sorted = outData.offsets;
  std::sort(sorted.begin(), sorted.end());

  outData.rows.resize(kRows);
  outData.bitmap.reserve(kRows * kRowWidth);
  for (int row = 0; row < kRows; ++row) {
    const quint32 relStart = outData.offsets.at(row);
    const qsizetype start = dataStart + static_cast<qsizetype>(relStart);
    if (start < dataStart || start >= bytes.size()) {
      return Daggerfall::FormatUtil::setError(errorMessage,
                                              QString("CLIMATE.PAK row %1 has invalid offset").arg(row));
    }

    qsizetype end = bytes.size();
    for (quint32 candidate : sorted) {
      if (candidate > relStart) {
        end = dataStart + static_cast<qsizetype>(candidate);
        break;
      }
    }
    if (end <= start || end > bytes.size()) {
      return Daggerfall::FormatUtil::setError(
          errorMessage, QString("CLIMATE.PAK row %1 has invalid run bounds").arg(row));
    }

    const QByteArray packedRow = bytes.mid(start, end - start);
    QByteArray unpacked;
    QString pakError;
    if (!DaggerfallPak::decompress(packedRow, unpacked, &pakError)) {
      return Daggerfall::FormatUtil::setError(errorMessage,
                                              QString("CLIMATE.PAK row %1: %2").arg(row).arg(pakError));
    }

    if (unpacked.size() != kRowWidth) {
      Daggerfall::FormatUtil::appendWarning(
          outData.warning, QString("row %1 decompresses to %2 bytes (expected %3)")
                               .arg(row).arg(unpacked.size()).arg(kRowWidth));
      if (unpacked.size() < kRowWidth) {
        unpacked.append(QByteArray(kRowWidth - unpacked.size(), static_cast<char>(223)));
      } else if (unpacked.size() > kRowWidth) {
        unpacked.truncate(kRowWidth);
      }
    }

    outData.rows[row] = unpacked;
    outData.bitmap.append(unpacked);
  }

  return true;
}

int DaggerfallClimatePak::valueAt(const Data& data, int x, int y)
{
  if (x < 0 || y < 0 || y >= data.rows.size()) {
    return -1;
  }
  const QByteArray& row = data.rows.at(y);
  if (x >= row.size()) {
    return -1;
  }
  return static_cast<quint8>(row.at(x));
}

DaggerfallClimatePak::TextureMapping DaggerfallClimatePak::mappingForValue(int value)
{
  switch (value) {
    case 223: return {223, 402, 502, 29};
    case 224: return {224, 2, 503, 9};
    case 225: return {225, 2, 503, 9};
    case 226: return {226, 102, 510, 1};
    case 227: return {227, 402, 500, 29};
    case 228: return {228, 402, 502, 29};
    case 229: return {229, 2, 503, 25};
    case 230: return {230, 102, 504, 17};
    case 231: return {231, 302, 508, 17};
    case 232: return {232, 302, 508, 17};
    default: return {value, 0, 0, 0};
  }
}
