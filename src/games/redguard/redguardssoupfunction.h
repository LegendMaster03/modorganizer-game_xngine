#ifndef REDGUARDSSOUPFUNCTION_H
#define REDGUARDSSOUPFUNCTION_H

#include <QString>

class RedguardsSoupFunction
{
public:
  explicit RedguardsSoupFunction(const QString& line);

  const QString& type() const { return mType; }
  const QString& name() const { return mName; }
  int paramCount() const { return mParamCount; }

private:
  QString mType;
  QString mName;
  int mParamCount = 0;
};

#endif  // REDGUARDSSOUPFUNCTION_H
