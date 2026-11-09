#ifndef REDGUARDSSOUPFLAG_H
#define REDGUARDSSOUPFLAG_H

#include <QString>

// Parsed SOUP flag declaration: leading fields and optional semicolon
// comment. It supplies names/IDs to script tools without evaluating flags.
class RedguardsSoupFlag
{
public:
  RedguardsSoupFlag() = default;
  explicit RedguardsSoupFlag(const QString& line);

  const QString& type() const { return mType; }
  const QString& name() const { return mName; }
  const QString& value() const { return mValue; }
  const QString& comment() const { return mComment; }

private:
  QString mType;
  QString mName;
  QString mValue;
  QString mComment;
};

#endif  // REDGUARDSSOUPFLAG_H
