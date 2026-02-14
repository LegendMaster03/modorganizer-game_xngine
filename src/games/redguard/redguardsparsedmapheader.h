#ifndef REDGUARDSPARSEDMAPHEADER_H
#define REDGUARDSPARSEDMAPHEADER_H

#include <QByteArray>
#include <QString>
#include <QList>

class RedguardsParsedMapHeader
{
public:
  explicit RedguardsParsedMapHeader(const QString& name) : mName(name) {}

  const QString& name() const { return mName; }

  int scriptDataOffset() const { return mScriptDataOffset; }
  void setScriptDataOffset(int offset) { mScriptDataOffset = offset; }

  int scriptPC() const { return mScriptPC; }
  void setScriptPC(int pc) { mScriptPC = pc; }

  const QList<QString>& strings() const { return mStrings; }

  int addString(const QString& str)
  {
    int idx = mStrings.indexOf(str);
    if (idx >= 0) {
      return idx;
    }
    mStrings.append(str);
    return mStrings.size() - 1;
  }

  const QByteArray& scriptBytes() const { return mScriptBytes; }
  void setScriptBytes(const QByteArray& bytes) { mScriptBytes = bytes; }

  const QByteArray& attributeBytes() const { return mAttributeBytes; }
  void setAttributeBytes(const QByteArray& bytes) { mAttributeBytes = bytes; }

private:
  QString mName;
  int mScriptDataOffset = 0;
  int mScriptPC = 0;
  QList<QString> mStrings;
  QByteArray mScriptBytes;
  QByteArray mAttributeBytes;
};

#endif  // REDGUARDSPARSEDMAPHEADER_H
