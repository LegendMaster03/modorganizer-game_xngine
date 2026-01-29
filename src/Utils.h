#pragma once

#include <QString>
#include <QByteArray>
#include <QIODevice>
#include <QRegularExpression>
#include <cstdint>

// Utility functions for binary file I/O and encoding (matches Java Utils)
class Utils {
public:
  // Binary reading from stream (little-endian)
  static QByteArray readBytes(QIODevice* input, int numBytes);
  static uint8_t readUnsignedByte(QIODevice* input);
  static int16_t readLittleEndianShort(QIODevice* input);
  static int32_t readLittleEndianInt(QIODevice* input);
  static QString readString(QIODevice* input, int n);
  
  // Convenience: read string from QByteArray at offset
  static QString readString(const QByteArray& array, int offset, int length);

  // Binary writing to stream (little-endian)
  static void writeLittleEndianShort(QIODevice* output, int16_t value);
  static void writeLittleEndianInt(QIODevice* output, int32_t value);
  static void writeString(QIODevice* output, const QString& str);

  // Byte array conversions
  static int32_t byteArrayToInt(const QByteArray& array, bool littleEndian = true);
  static int32_t byteRangeToInt(const QByteArray& array, int start, int length, bool littleEndian = true);
  static QByteArray shortToByteArray(int16_t value, bool littleEndian = true);
  static QByteArray intToByteArray(int32_t value, bool littleEndian = true);

  // String utilities
  static QString validFilename(const QString& str);
  static QString fixUnsupportedCharacters(const QString& str);
};
