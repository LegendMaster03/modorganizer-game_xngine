#ifndef REDGUARDSSOUPFUNCTION_H
#define REDGUARDSSOUPFUNCTION_H

#include <QString>

// Parsed SOUP function signature used for script ID/parameter decoding;
// function declarations are metadata, not executable implementations.
class RedguardsSoupFunction
{
public:
  RedguardsSoupFunction() = default;
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
