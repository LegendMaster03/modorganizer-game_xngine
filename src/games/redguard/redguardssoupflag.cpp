#include "redguardssoupflag.h"

#include <QRegularExpression>

// Read the first three declaration fields and, when there is exactly one
// semicolon delimiter, store its trailing text as a descriptive comment.
// Incomplete declarations leave the corresponding fields empty.

RedguardsSoupFlag::RedguardsSoupFlag(const QString& line)
{
  const QStringList split = line.split(';');
  if (split.size() == 2) {
    mComment = split[1].trimmed();
  }

  const QStringList leftSplit = split[0].split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
  if (leftSplit.size() >= 3) {
    mType = leftSplit[0];
    mName = leftSplit[1];
    mValue = leftSplit[2];
  }
}
