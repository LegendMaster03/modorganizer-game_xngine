#include "daggerfallbooktxt.h"
#include "daggerfallformatutils.h"

#include <QFile>

#include <algorithm>

namespace {

QString decodePageText(const QByteArray& raw)
{
  QString out;
  out.reserve(raw.size());
  for (int i = 0; i < raw.size(); ++i) {
    const quint8 b = static_cast<quint8>(raw.at(i));

    if (b == 0xF6) {
      break;
    }

    if (b == 0xFC || b == 0xFD) {
      // EndOfLine is FC 00 or FD 00.
      if (i + 1 < raw.size() && static_cast<quint8>(raw.at(i + 1)) == 0x00) {
        out.append('\n');
        ++i;
        continue;
      }
    }

    if (b == 0x00) {
      out.append('\n');
      continue;
    }

    // Skip formatting tokens but preserve readable output.
    if (b == 0xF9 || b == 0xFA) {
      if (i + 1 < raw.size()) {
        ++i;
      }
      continue;
    }
    if (b == 0xFB) {
      // Position code payload: X + Y.
      if (i + 2 < raw.size()) {
        i += 2;
      }
      continue;
    }
    if (b == 0xF7) {
      // Book image token followed by zero-terminated name.
      while (i + 1 < raw.size() && raw.at(i + 1) != '\0') {
        ++i;
      }
      if (i + 1 < raw.size()) {
        ++i;
      }
      continue;
    }

    out.append(QChar::fromLatin1(static_cast<char>(b)));
  }
  return out;
}

}  // namespace

bool DaggerfallBookTxt::loadBook(const QString& filePath, Book& outBook,
                                 QString* errorMessage)
{
  outBook = {};

  QFile f(filePath);
  if (!f.open(QIODevice::ReadOnly)) {
    return Daggerfall::FormatUtil::setError(errorMessage,
                                            QString("Unable to open book file: %1").arg(filePath));
  }
  const QByteArray data = f.readAll();
  if (data.size() < 0xEC) {
    return Daggerfall::FormatUtil::setError(errorMessage, "Book file is too small");
  }

  outBook.title = Daggerfall::FormatUtil::readFixedString(data, 0x00, 64);
  outBook.author = Daggerfall::FormatUtil::readFixedString(data, 0x40, 64);

  QByteArray naughtyRaw = data.mid(0x80, 8);
  outBook.isNaughty = (naughtyRaw == QByteArray("naughty "));

  if (!Daggerfall::FormatUtil::readLE32U(data, 0xE0, outBook.price) ||
      !Daggerfall::FormatUtil::readLE16U(data, 0xE4, outBook.rarity) ||
      !Daggerfall::FormatUtil::readLE16U(data, 0xE6, outBook.unknown2) ||
      !Daggerfall::FormatUtil::readLE16U(data, 0xE8, outBook.unknown3) ||
      !Daggerfall::FormatUtil::readLE16U(data, 0xEA, outBook.pageCount)) {
    return Daggerfall::FormatUtil::setError(errorMessage, "Book header is truncated");
  }

  const qsizetype expectedHeaderSize = 0xEC + static_cast<qsizetype>(outBook.pageCount) * 4;
  if (expectedHeaderSize > data.size()) {
    return Daggerfall::FormatUtil::setError(errorMessage, "Book page offset table exceeds file size");
  }

  outBook.pageOffsets.reserve(outBook.pageCount);
  for (int i = 0; i < outBook.pageCount; ++i) {
    quint32 off = 0;
    if (!Daggerfall::FormatUtil::readLE32U(data, 0xEC + static_cast<qsizetype>(i) * 4, off)) {
      return Daggerfall::FormatUtil::setError(errorMessage, "Book page offset table is truncated");
    }
    outBook.pageOffsets.push_back(off);
  }

  QVector<quint32> sortedOffsets = outBook.pageOffsets;
  std::sort(sortedOffsets.begin(), sortedOffsets.end());
  sortedOffsets.erase(std::unique(sortedOffsets.begin(), sortedOffsets.end()),
                      sortedOffsets.end());

  outBook.pages.reserve(outBook.pageCount);
  for (int i = 0; i < outBook.pageOffsets.size(); ++i) {
    const quint32 pageOffset = outBook.pageOffsets.at(i);
    if (static_cast<qsizetype>(pageOffset) >= data.size()) {
      Daggerfall::FormatUtil::appendWarning(
          outBook.warning, QString("Page %1 offset %2 is outside file").arg(i).arg(pageOffset));
      continue;
    }

    qsizetype pageEnd = data.size();
    for (quint32 off : sortedOffsets) {
      if (off > pageOffset) {
        pageEnd = static_cast<qsizetype>(off);
        break;
      }
    }
    if (pageEnd <= static_cast<qsizetype>(pageOffset)) {
      pageEnd = data.size();
    }

    QByteArray raw = data.mid(static_cast<qsizetype>(pageOffset),
                              pageEnd - static_cast<qsizetype>(pageOffset));
    const int eop = raw.indexOf(static_cast<char>(0xF6));
    if (eop >= 0) {
      raw.truncate(eop + 1);
    }

    Page page;
    page.offset = pageOffset;
    page.rawBytes = raw;
    page.text = decodePageText(raw);
    if (eop < 0) {
      page.warning = "Page does not contain 0xF6 end-of-page token";
      Daggerfall::FormatUtil::appendWarning(outBook.warning,
                                            QString("Page %1 missing 0xF6 terminator").arg(i));
    }
    outBook.pages.push_back(page);
  }

  return true;
}
