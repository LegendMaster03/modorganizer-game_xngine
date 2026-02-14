#include "redguardssoupfunction.h"

#include <QRegularExpression>

RedguardsSoupFunction::RedguardsSoupFunction(const QString& line)
{
  const QStringList split = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
  if (split.size() >= 4) {
    mType = split[0];
    mName = split[1];
    mParamCount = split[3].toInt();
  }
}
