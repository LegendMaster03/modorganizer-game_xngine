#pragma once

#include <QString>
#include <QList>
#include <QByteArray>
#include <cstdint>

// Represents a single map header from the RAHD section
// Binary format: 165 bytes per header, parsed according to Java MapHeader
class MapHeader {
public:
  // Constructor takes binary header data (165 bytes from RAHD section)
  MapHeader(const QByteArray& data);

  // Basic accessors (parsed from binary header)
  QString name() const { return mName; }
  int32_t instances() const { return mInstances; }
  int32_t scriptLength() const { return mScriptLength; }
  int32_t scriptDataOffset() const { return mScriptDataOffset; }
  int32_t scriptPC() const { return mScriptPC; }

  // Counts and offsets for strings and variables
  int32_t numStrings() const { return mNumStrings; }
  int32_t stringOffsetsIndex() const { return mStringOffsetsIndex; }
  int32_t numVariables() const { return mNumVariables; }
  int32_t variableOffset() const { return mVariableOffset; }

  // String and variable initialization from parent map data
  void initStrings(const QString& allStrings, const QByteArray& stringOffsets);
  void initVariables(const QList<int32_t>& allVariables);

  // Dynamic content
  QList<QString> strings() const { return mStrings; }
  QList<int32_t> variables() const { return mVariables; }

  QByteArray scriptBytes() const { return mScriptBytes; }
  void setScriptBytes(const QByteArray& bytes) { mScriptBytes = bytes; }

  QString script() const { return mScript; }
  void setScript(const QString& script) { mScript = script; }

private:
  // Raw binary data (165 bytes)
  QByteArray mData;

  // Parsed binary fields (all offsets and lengths match Java MapHeader)
  QString mName;
  int32_t mInstances;
  int32_t mNumStrings;
  int32_t mStringOffsetsIndex;
  int32_t mNumVariables;
  int32_t mVariableOffset;
  int32_t mScriptLength;
  int32_t mScriptDataOffset;
  int32_t mScriptPC;

  // Dynamic content loaded at runtime
  QList<QString> mStrings;
  QList<int32_t> mVariables;
  QByteArray mScriptBytes;
  QString mScript;
};
