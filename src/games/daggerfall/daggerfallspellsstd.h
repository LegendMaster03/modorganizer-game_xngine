#ifndef DAGGERFALL_SPELLSSTD_H
#define DAGGERFALL_SPELLSSTD_H

#include <QString>
#include <QVector>

class DaggerfallSpellsStd
{
public:
  struct EffectType
  {
    int type = -1;
    int subtype = -1;
    QString name;
    bool removed = false;
  };

  struct SpellEntry
  {
    int stdIndex = -1;
    QString name;
    int icon = -1;
    bool usedByMagicDef = false;  // '*' in catalogue notes
    bool starterSpell = false;    // '~' in catalogue notes
    bool duplicateIndex = false;  // '?' in catalogue notes
  };

  static QVector<EffectType> effectTypes();
  static QVector<SpellEntry> spellCatalogue();

  static QString effectTypeName(int type, int subtype);
  static bool isEffectTypeRemoved(int type, int subtype);

  static QVector<SpellEntry> findSpellsByStdIndex(int stdIndex);
  static SpellEntry findFirstSpellByName(const QString& name);
};

#endif  // DAGGERFALL_SPELLSSTD_H
