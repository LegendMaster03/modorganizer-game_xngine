#ifndef REDGUARDSSCRIPTINSTRUCTION_H
#define REDGUARDSSCRIPTINSTRUCTION_H

#include <QString>

class RedguardsScriptInstruction
{
public:
  static const QString INDENT;

  RedguardsScriptInstruction(int address, int indentLevel);

  int address() const { return mAddress; }
  int indentLevel() const { return mIndentLevel; }
  QString text() const { return mText; }

  RedguardsScriptInstruction* appendText(const QString& str)
  {
    mText.append(str);
    return this;
  }

  const QString& comment() const { return mComment; }
  void setComment(const QString& comment) { mComment = comment; }

  void build(QString& output, int previousIndentLevel, bool addLabel) const;

  static QString makeLabel(int address);

private:
  int mAddress = 0;
  int mIndentLevel = 0;
  QString mText;
  QString mComment;
};

#endif  // REDGUARDSSCRIPTINSTRUCTION_H
