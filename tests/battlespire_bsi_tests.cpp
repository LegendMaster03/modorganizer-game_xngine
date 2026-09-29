#include "battlespirebsi.h"

#include <QtEndian>

#include <iostream>

namespace {

void appendBE32(QByteArray& out, quint32 value)
{
  const quint32 raw = qToBigEndian(value);
  out.append(reinterpret_cast<const char*>(&raw), sizeof(raw));
}

void appendLE16(QByteArray& out, quint16 value)
{
  const quint16 raw = qToLittleEndian(value);
  out.append(reinterpret_cast<const char*>(&raw), sizeof(raw));
}

QByteArray makeChunk(const char tag[5], const QByteArray& payload)
{
  QByteArray out(tag, 4);
  appendBE32(out, static_cast<quint32>(payload.size()));
  out.append(payload);
  return out;
}

QByteArray makeHeader(quint16 width, quint16 height, quint16 frames, quint16 compression)
{
  QByteArray out;
  appendLE16(out, 0);
  appendLE16(out, 0);
  appendLE16(out, width);
  appendLE16(out, height);
  out.append(6, '\0');
  appendLE16(out, frames);
  appendLE16(out, 0);
  appendLE16(out, 0);
  appendLE16(out, 0);
  appendLE16(out, 0);
  appendLE16(out, compression);
  return out;
}

bool check(bool condition, const char* message)
{
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

QByteArray makeCompressedData(bool malformed)
{
  QByteArray data;
  // Row 0: raw at offset 8.
  data.append(static_cast<char>(8));
  data.append(static_cast<char>(0));
  data.append(static_cast<char>(0));
  data.append(static_cast<char>(0));
  // Row 1: RLE at offset 12.
  data.append(static_cast<char>(12));
  data.append(static_cast<char>(0));
  data.append(static_cast<char>(0));
  data.append(static_cast<char>(0x80));
  data.append("\x01\x02\x03\x04", 4);
  data.append(static_cast<char>(malformed ? 0x85 : 0x84));
  data.append(static_cast<char>(9));
  return data;
}

}  // namespace

int main()
{
  bool ok = true;

  QByteArray hicl(256, '\0');
  hicl[0] = static_cast<char>(0x00);
  hicl[1] = static_cast<char>(0x7C);  // 0x7C00: maximum red in 5:5:5.
  QByteArray htbl(256, '\0');
  htbl[0] = static_cast<char>(0xE0);
  htbl[1] = static_cast<char>(0x03);  // 0x03E0: maximum green.
  QByteArray cmap;
  cmap.append(static_cast<char>(1));  // G
  cmap.append(static_cast<char>(2));  // B
  cmap.append(static_cast<char>(3));  // R

  QByteArray uncompressed;
  uncompressed += makeChunk("BHDR", makeHeader(3, 2, 1, 0));
  uncompressed += makeChunk("HICL", hicl);
  uncompressed += makeChunk("HTBL", htbl);
  uncompressed += makeChunk("CMAP", cmap);
  uncompressed += makeChunk("DATA", QByteArray("\x01\x02\x03\x04\x05\x06", 6));
  uncompressed += makeChunk("END ", QByteArray());

  BattlespireBsi::Document doc;
  QString error;
  ok &= check(BattlespireBsi::parseData(uncompressed, doc, &error),
              "parse uncompressed BSI fixture");
  if (!ok) {
    std::cerr << error.toStdString() << '\n';
    return 1;
  }
  ok &= check(doc.hasHeader && doc.header.width == 3 && doc.header.height == 2,
              "decode BHDR dimensions");
  ok &= check(doc.decodedPixels == QByteArray("\x01\x02\x03\x04\x05\x06", 6),
              "decode uncompressed DATA");
  ok &= check(doc.hiclPalette.size() == 128 && doc.hiclPalette.at(0).r == 248,
              "decode HICL 15-bit palette");
  ok &= check(doc.lightingPalettes.size() == 1 &&
                  doc.lightingPalettes.at(0).at(0).g == 248,
              "decode HTBL lighting palette");
  ok &= check(doc.cmapPalette.size() == 1 && doc.cmapPalette.at(0).r == 12 &&
                  doc.cmapPalette.at(0).g == 4 && doc.cmapPalette.at(0).b == 8,
              "decode CMAP channel order and 6-bit scale");

  QByteArray compressed;
  compressed += makeChunk("BHDR", makeHeader(4, 2, 1, 4));
  compressed += makeChunk("DATA", makeCompressedData(false));
  compressed += makeChunk("END ", QByteArray());

  BattlespireBsi::Document compressedDoc;
  error.clear();
  ok &= check(BattlespireBsi::parseData(compressed, compressedDoc, &error),
              "parse compressed BSI fixture");
  if (!ok) {
    std::cerr << error.toStdString() << '\n';
    return 1;
  }
  ok &= check(compressedDoc.decodedPixels ==
                  QByteArray("\x01\x02\x03\x04\x09\x09\x09\x09", 8),
              "decode raw and RLE scanlines");

  QByteArray malformed;
  malformed += makeChunk("BHDR", makeHeader(4, 2, 1, 4));
  malformed += makeChunk("DATA", makeCompressedData(true));
  malformed += makeChunk("END ", QByteArray());
  BattlespireBsi::Document badDoc;
  error.clear();
  ok &= check(!BattlespireBsi::parseData(malformed, badDoc, &error),
              "reject RLE run that exceeds scanline width");
  ok &= check(!error.isEmpty(), "report BSI RLE validation error");

  if (!ok) {
    return 1;
  }
  std::cout << "Battlespire BSI tests passed.\n";
  return 0;
}
