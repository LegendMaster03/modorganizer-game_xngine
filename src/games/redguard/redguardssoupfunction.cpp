#include "redguardssoupfunction.h"

#include <QRegularExpression>

// Read type, name, and the fourth token's parameter count from a SOUP
// function declaration. Incomplete declarations retain default values.

RedguardsSoupFunction::RedguardsSoupFunction(const QString& line)
{
  const QStringList split = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
  if (split.size() >= 4) {
    mType = split[0];
    mName = split[1];
    mParamCount = split[3].toInt();
  }
}
