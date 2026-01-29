#ifndef SCRIPTINSTRUCTION_H
#define SCRIPTINSTRUCTION_H

#include <QString>

/**
 * Represents a single instruction in a compiled script.
 * Handles text formatting, indentation, and labels.
 */
class ScriptInstruction
{
public:
  static const QString INDENT;

  ScriptInstruction(int address, int indentLevel);

  int getAddress() const { return mAddress; }
  int getIndentLevel() const { return mIndentLevel; }
  QString getText() const { return mText; }

  ScriptInstruction* appendText(const QString& str)
  {
    mText.append(str);
    return this;
  }

  QString getComment() const { return mComment; }
  void setComment(const QString& comment) { mComment = comment; }

  void build(QString& output, int previousIndentLevel, bool addLabel) const;

  static QString makeLabel(int address);

  QString toString() const { return mText; }

private:
  int mAddress;
  int mIndentLevel;
  QString mText;
  QString mComment;
};

#endif // SCRIPTINSTRUCTION_H
