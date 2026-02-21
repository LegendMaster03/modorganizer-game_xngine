#ifndef DAGGERFALL_TEXTRSCINDICES_H
#define DAGGERFALL_TEXTRSCINDICES_H

#include <QString>
#include <QVector>
#include <QtGlobal>

class DaggerfallTextRscIndices
{
public:
  struct Range
  {
    quint16 first = 0;
    quint16 last = 0;
    QString label;

    bool contains(quint16 id) const { return id >= first && id <= last; }
  };

  static QVector<Range> knownRanges();
  static QString labelForRecordId(quint16 id);
};

#endif  // DAGGERFALL_TEXTRSCINDICES_H
