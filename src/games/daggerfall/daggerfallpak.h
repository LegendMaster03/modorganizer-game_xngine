#ifndef DAGGERFALL_PAK_H
#define DAGGERFALL_PAK_H

#include <QByteArray>
#include <QString>

class DaggerfallPak
{
public:
  struct Run
  {
    quint16 count = 0;
    quint8 value = 0;
  };

  // Decompresses a whole PAK stream (list of 3-byte runs).
  static bool decompress(const QByteArray& packed, QByteArray& unpacked,
                         QString* errorMessage = nullptr);

  // Encodes bytes into simple run-length PAK runs.
  static QByteArray compress(const QByteArray& unpacked);
};

#endif  // DAGGERFALL_PAK_H
