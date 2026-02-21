#include "daggerfallpoliticpak.h"
#include "daggerfallformatutils.h"

#include "daggerfallpak.h"

#include <QFile>

#include <algorithm>

namespace {

constexpr int kRows = 500;
constexpr int kRowWidth = 1001;

}  // namespace

bool DaggerfallPoliticPak::load(const QString& filePath, Data& outData, QString* errorMessage)
{
  outData = {};
  outData.width = kRowWidth;
  outData.height = kRows;

  QFile f(filePath);
  if (!f.open(QIODevice::ReadOnly)) {
    return Daggerfall::FormatUtil::setError(errorMessage,
                                            QString("Unable to open POLITIC.PAK: %1").arg(filePath));
  }
  const QByteArray bytes = f.readAll();
  if (bytes.size() < kRows * 4) {
    return Daggerfall::FormatUtil::setError(errorMessage, "POLITIC.PAK too small for offset list");
  }

  outData.offsets.reserve(kRows);
  for (int i = 0; i < kRows; ++i) {
    quint32 off = 0;
    if (!Daggerfall::FormatUtil::readLE32U(bytes, static_cast<qsizetype>(i) * 4, off)) {
      return Daggerfall::FormatUtil::setError(errorMessage, "Failed reading POLITIC.PAK offset list");
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
                                              QString("POLITIC.PAK row %1 has invalid offset").arg(row));
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
          errorMessage, QString("POLITIC.PAK row %1 has invalid run bounds").arg(row));
    }

    const QByteArray packedRow = bytes.mid(start, end - start);
    QByteArray unpacked;
    QString pakError;
    if (!DaggerfallPak::decompress(packedRow, unpacked, &pakError)) {
      return Daggerfall::FormatUtil::setError(errorMessage,
                                              QString("POLITIC.PAK row %1: %2").arg(row).arg(pakError));
    }

    if (unpacked.size() != kRowWidth) {
      Daggerfall::FormatUtil::appendWarning(
          outData.warning, QString("row %1 decompresses to %2 bytes (expected %3)")
                               .arg(row).arg(unpacked.size()).arg(kRowWidth));
      if (unpacked.size() < kRowWidth) {
        unpacked.append(QByteArray(kRowWidth - unpacked.size(), static_cast<char>(64)));
      } else if (unpacked.size() > kRowWidth) {
        unpacked.truncate(kRowWidth);
      }
    }

    outData.rows[row] = unpacked;
    outData.bitmap.append(unpacked);
  }

  return true;
}

int DaggerfallPoliticPak::valueAt(const Data& data, int x, int y)
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

int DaggerfallPoliticPak::regionFromValue(int value)
{
  if (value == 64) {
    return -1;  // water
  }
  if (value >= 128 && value <= 233) {
    return value - 128;
  }
  return -2;  // unknown/special
}
