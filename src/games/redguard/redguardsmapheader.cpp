#include "redguardsmapheader.h"

#include "redguardsutils.h"

RedguardsMapHeader::RedguardsMapHeader(const QByteArray& data)
    : mData(data)
{
  mName = RedguardsUtils::readString(mData, 4, 9).trimmed();
  mInstances = RedguardsUtils::byteRangeToInt(mData, 13, 2, true);
  mNumStrings = RedguardsUtils::byteRangeToInt(mData, 65, 4, true);
  mStringOffsetsIndex = RedguardsUtils::byteRangeToInt(mData, 73, 4, true);
  mScriptLength = RedguardsUtils::byteRangeToInt(mData, 77, 4, true);
  mScriptDataOffset = RedguardsUtils::byteRangeToInt(mData, 81, 4, true);
  mScriptPC = RedguardsUtils::byteRangeToInt(mData, 85, 4, true);
  mNumVariables = RedguardsUtils::byteRangeToInt(mData, 117, 4, true);
  mVariableOffset = RedguardsUtils::byteRangeToInt(mData, 125, 4, true);
}

void RedguardsMapHeader::initStrings(const QString& allStrings,
                                     const QByteArray& stringOffsets)
{
  mStrings.clear();
  if (mNumStrings <= 0) {
    return;
  }

  for (int i = 0; i < mNumStrings; ++i) {
    int offsetInTable = mStringOffsetsIndex + i * 4;
    int stringOffset = RedguardsUtils::byteRangeToInt(stringOffsets, offsetInTable, 4, true);
    int stringEnd = allStrings.indexOf(QChar(0), stringOffset);
    if (stringEnd == -1) {
      stringEnd = allStrings.length();
    }
    mStrings.append(allStrings.mid(stringOffset, stringEnd - stringOffset));
  }
}

void RedguardsMapHeader::initVariables(const QList<int32_t>& allVariables)
{
  mVariables.clear();
  if (mNumVariables <= 0) {
    return;
  }

  int startIndex = mVariableOffset / 4;
  for (int i = 0; i < mNumVariables && startIndex + i < allVariables.size(); ++i) {
    mVariables.append(allVariables[startIndex + i]);
  }
}
