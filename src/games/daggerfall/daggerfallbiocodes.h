#ifndef DAGGERFALL_BIOCODES_H
#define DAGGERFALL_BIOCODES_H

#include <QHash>
#include <QString>
#include <QVector>

class DaggerfallBioCodes
{
public:
  struct PredefinedClass
  {
    int gamma = -1;
    QString name;
  };

  static QVector<PredefinedClass> predefinedClasses();
  static QString predefinedClassName(int gamma);
  static QString biographyFileNameForGamma(int gamma);  // BIOG??T0.TXT

  static QHash<int, QString> skillCodeNames();
  static QString skillName(int code);

  static QHash<int, QString> demographicNames();
  static QString demographicName(int index);

  static QHash<int, QString> knownFactionNames();
  static QString factionName(int factionId);
};

#endif  // DAGGERFALL_BIOCODES_H
