#include "redguardsutils.h"

#include <QIODevice>
#include <algorithm>

QByteArray RedguardsUtils::readBytes(QIODevice* input, int numBytes)
{
  if (!input) {
    return QByteArray();
  }
  return input->read(numBytes);
}

uint8_t RedguardsUtils::readUnsignedByte(QIODevice* input)
{
  if (!input) {
    return 0;
  }
  QByteArray byte = input->read(1);
  return byte.isEmpty() ? 0 : static_cast<uint8_t>(byte[0]) & 0xff;
}

int16_t RedguardsUtils::readLittleEndianShort(QIODevice* input)
{
  QByteArray bytes = readBytes(input, 2);
  return static_cast<int16_t>(byteArrayToInt(bytes, true));
}

int32_t RedguardsUtils::readLittleEndianInt(QIODevice* input)
{
  QByteArray bytes = readBytes(input, 4);
  return byteArrayToInt(bytes, true);
}

QString RedguardsUtils::readString(QIODevice* input, int n)
{
  QByteArray bytes = readBytes(input, n);
  // Use Latin1 encoding (matches Java implementation)
  return QString::fromLatin1(bytes);
}

void RedguardsUtils::writeLittleEndianShort(QIODevice* output, int16_t value)
{
  if (!output) {
    return;
  }
  output->write(shortToByteArray(value, true));
}

void RedguardsUtils::writeLittleEndianInt(QIODevice* output, int32_t value)
{
  if (!output) {
    return;
  }
  output->write(intToByteArray(value, true));
}

void RedguardsUtils::writeString(QIODevice* output, const QString& str)
{
  if (!output) {
    return;
  }
  QString fixed = fixUnsupportedCharacters(str);
  // Use Latin1 encoding
  QByteArray bytes = fixed.toLatin1();
  output->write(bytes);
}

int32_t RedguardsUtils::byteArrayToInt(const QByteArray& array, bool littleEndian)
{
  return byteRangeToInt(array, 0, array.size(), littleEndian);
}

int32_t RedguardsUtils::byteRangeToInt(const QByteArray& array, int start, int length,
                                       bool littleEndian)
{
  int32_t value = 0;
  for (int i = 0; i < length && start + i < array.size(); ++i) {
    uint8_t byte = static_cast<uint8_t>(array[start + i]) & 0xff;
    if (littleEndian) {
      value += byte << (8 * i);
    } else {
      value = (value << 8) + byte;
    }
  }
  return value;
}

QByteArray RedguardsUtils::shortToByteArray(int16_t value, bool littleEndian)
{
  QByteArray bytes(2, 0);
  if (littleEndian) {
    bytes[0] = value & 0xff;
    bytes[1] = (value >> 8) & 0xff;
  } else {
    bytes[0] = (value >> 8) & 0xff;
    bytes[1] = value & 0xff;
  }
  return bytes;
}

QByteArray RedguardsUtils::intToByteArray(int32_t value, bool littleEndian)
{
  QByteArray bytes(4, 0);
  if (littleEndian) {
    bytes[0] = value & 0xff;
    bytes[1] = (value >> 8) & 0xff;
    bytes[2] = (value >> 16) & 0xff;
    bytes[3] = (value >> 24) & 0xff;
  } else {
    bytes[0] = (value >> 24) & 0xff;
    bytes[1] = (value >> 16) & 0xff;
    bytes[2] = (value >> 8) & 0xff;
    bytes[3] = value & 0xff;
  }
  return bytes;
}

QString RedguardsUtils::validFilename(const QString& str)
{
  QString result = str;
  result.replace(QRegularExpression("[\\\\/:*\"<>|]"), "_");
  return result;
}

QString RedguardsUtils::fixUnsupportedCharacters(const QString& str)
{
  QString result = str;
  // Convert uppercase accented characters to lowercase equivalents
  result.replace(QChar(0xC1), QChar(0xE1));  // Á -> á
  result.replace(QChar(0xC9), QChar(0xE9));  // É -> é
  result.replace(QChar(0xCD), QChar(0xED));  // Í -> í
  result.replace(QChar(0xD3), QChar(0xF3));  // Ó -> ó
  result.replace(QChar(0xDA), QChar(0xFA));  // Ú -> ú
  return result;
}

QString RedguardsUtils::readString(const QByteArray& array, int offset, int length)
{
  if (offset + length > array.size() || offset < 0 || length < 0) {
    return QString();
  }
  QByteArray bytes = array.mid(offset, length);
  // Use Latin1 encoding
  return QString::fromLatin1(bytes);
}
