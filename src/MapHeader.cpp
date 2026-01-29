#include "MapHeader.h"

#include "Utils.h"

// Constructor: Parse 165-byte binary header data
// Layout matches Java MapHeader exactly
MapHeader::MapHeader(const QByteArray& data)
    : mData(data), mInstances(0), mNumStrings(0), mStringOffsetsIndex(0),
      mNumVariables(0), mVariableOffset(0), mScriptLength(0),
      mScriptDataOffset(0), mScriptPC(0)
{
  // Parse fixed offsets from 165-byte header
  // Offset 4-13: name (9 bytes)
  mName = Utils::readString(mData, 4, 9).trimmed();

  // Offset 13-15: instances (2 bytes, little-endian)
  mInstances = Utils::byteRangeToInt(mData, 13, 2, true);

  // Offset 65-69: numStrings (4 bytes, little-endian)
  mNumStrings = Utils::byteRangeToInt(mData, 65, 4, true);

  // Offset 73-77: stringOffsetsIndex (4 bytes, little-endian)
  mStringOffsetsIndex = Utils::byteRangeToInt(mData, 73, 4, true);

  // Offset 77-81: scriptLength (4 bytes, little-endian)
  mScriptLength = Utils::byteRangeToInt(mData, 77, 4, true);

  // Offset 81-85: scriptDataOffset (4 bytes, little-endian)
  mScriptDataOffset = Utils::byteRangeToInt(mData, 81, 4, true);

  // Offset 85-89: scriptPC (4 bytes, little-endian)
  mScriptPC = Utils::byteRangeToInt(mData, 85, 4, true);

  // Offset 117-121: numVariables (4 bytes, little-endian)
  mNumVariables = Utils::byteRangeToInt(mData, 117, 4, true);

  // Offset 125-129: variableOffset (4 bytes, little-endian)
  mVariableOffset = Utils::byteRangeToInt(mData, 125, 4, true);
}

void MapHeader::initStrings(const QString& allStrings, const QByteArray& stringOffsets)
{
  mStrings.clear();
  if (mNumStrings <= 0) {
    return;
  }

  // For each string index, read offset from stringOffsets array
  for (int i = 0; i < mNumStrings; ++i) {
    // String offset stored at index (mStringOffsetsIndex + i * 4) in stringOffsets
    int offsetInTable = mStringOffsetsIndex + i * 4;
    int stringOffset = Utils::byteRangeToInt(stringOffsets, offsetInTable, 4, true);

    // Find null terminator in allStrings
    int stringEnd = allStrings.indexOf(QChar(0), stringOffset);
    if (stringEnd == -1) {
      stringEnd = allStrings.length();
    }

    QString str = allStrings.mid(stringOffset, stringEnd - stringOffset);
    mStrings.append(str);
  }
}

void MapHeader::initVariables(const QList<int32_t>& allVariables)
{
  mVariables.clear();
  if (mNumVariables <= 0) {
    return;
  }

  // Variable offset is in bytes; divide by 4 to get index
  int startIndex = mVariableOffset / 4;
  for (int i = 0; i < mNumVariables && startIndex + i < allVariables.size(); ++i) {
    mVariables.append(allVariables[startIndex + i]);
  }
}
