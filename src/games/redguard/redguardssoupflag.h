#ifndef REDGUARDSSOUPFLAG_H
#define REDGUARDSSOUPFLAG_H

#include <QString>

class RedguardsSoupFlag
{
public:
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
