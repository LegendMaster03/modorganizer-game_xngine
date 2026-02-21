#include "daggerfallpak.h"
#include "daggerfallformatutils.h"

#include <QtEndian>

namespace {

using Daggerfall::FormatUtil::readLE16U;
using Daggerfall::FormatUtil::setError;

}  // namespace

bool DaggerfallPak::decompress(const QByteArray& packed, QByteArray& unpacked,
                               QString* errorMessage)
{
  unpacked.clear();
  if ((packed.size() % 3) != 0) {
    return setError(errorMessage, "PAK data length must be a multiple of 3");
  }

  qsizetype pos = 0;
  while (pos + 3 <= packed.size()) {
    quint16 count = 0;
    if (!readLE16U(packed, pos, count)) {
      return setError(errorMessage, "Failed to read PAK run count");
    }
    const quint8 value = static_cast<quint8>(packed.at(pos + 2));
    if (count == 0) {
      return setError(errorMessage, "PAK run count of 0 is invalid");
    }

    const QByteArray run(static_cast<int>(count), static_cast<char>(value));
    unpacked.append(run);
    pos += 3;
  }

  return true;
}

QByteArray DaggerfallPak::compress(const QByteArray& unpacked)
{
  QByteArray out;
  if (unpacked.isEmpty()) {
    return out;
  }

  int i = 0;
  while (i < unpacked.size()) {
    const quint8 value = static_cast<quint8>(unpacked.at(i));
    quint16 count = 1;
    while (i + count < unpacked.size() &&
           static_cast<quint8>(unpacked.at(i + count)) == value &&
           count < 0xFFFF) {
      ++count;
    }

    const quint16 le = qToLittleEndian(count);
    out.append(reinterpret_cast<const char*>(&le), sizeof(le));
    out.append(static_cast<char>(value));
    i += count;
  }

  return out;
}
