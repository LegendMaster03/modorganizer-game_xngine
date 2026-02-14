#ifndef REDGUARDSMAPHEADER_H
#define REDGUARDSMAPHEADER_H

#include <QByteArray>
#include <QList>
#include <QString>
#include <cstdint>

class RedguardsMapHeader
{
public:
  explicit RedguardsMapHeader(const QByteArray& data);

  const QByteArray& data() const { return mData; }
  QString name() const { return mName; }
  int32_t instances() const { return mInstances; }
  int32_t scriptLength() const { return mScriptLength; }
  int32_t scriptDataOffset() const { return mScriptDataOffset; }
  int32_t scriptPC() const { return mScriptPC; }

  int32_t numStrings() const { return mNumStrings; }
  int32_t stringOffsetsIndex() const { return mStringOffsetsIndex; }
  int32_t numVariables() const { return mNumVariables; }
  int32_t variableOffset() const { return mVariableOffset; }

  void initStrings(const QString& allStrings, const QByteArray& stringOffsets);
  void initVariables(const QList<int32_t>& allVariables);

  const QList<QString>& strings() const { return mStrings; }
  const QList<int32_t>& variables() const { return mVariables; }

  const QByteArray& scriptBytes() const { return mScriptBytes; }
  void setScriptBytes(const QByteArray& bytes) { mScriptBytes = bytes; }

  const QString& script() const { return mScript; }
  void setScript(const QString& script) { mScript = script; }

  const QByteArray& attributeBytes() const { return mAttributeBytes; }
  void setAttributeBytes(const QByteArray& bytes) { mAttributeBytes = bytes; }

private:
  QByteArray mData;

  QString mName;
  int32_t mInstances = 0;
  int32_t mNumStrings = 0;
  int32_t mStringOffsetsIndex = 0;
  int32_t mNumVariables = 0;
  int32_t mVariableOffset = 0;
  int32_t mScriptLength = 0;
  int32_t mScriptDataOffset = 0;
  int32_t mScriptPC = 0;

  QList<QString> mStrings;
  QList<int32_t> mVariables;
  QByteArray mScriptBytes;
  QByteArray mAttributeBytes;
  QString mScript;
};

#endif  // REDGUARDSMAPHEADER_H
