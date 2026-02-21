#include "daggerfallspellsstd.h"

namespace {

QVector<DaggerfallSpellsStd::EffectType> buildEffectTypes()
{
  return {
      {-1, -1, "(nothing)", false},
      {0, -1, "Paralysis", false},
      {1, 0, "Continuous Damage -- Health", false},
      {1, 1, "Continuous Damage -- Stamina", false},
      {1, 2, "Continuous Damage -- Spell Points", false},
      {2, -1, "Create Item", false},
      {3, 0, "Cure -- Disease", false},
      {3, 1, "Cure -- Poison", false},
      {3, 2, "Cure -- Paralysis", false},
      {3, 3, "Cure -- Magic", false},
      {4, 0, "Damage -- Health", false},
      {4, 1, "Damage -- Stamina", false},
      {4, 2, "Damage -- Spell Points", false},
      {5, -1, "Disintegrate", false},
      {6, 0, "Dispell -- Magic", false},
      {6, 1, "Dispell -- Undead", false},
      {6, 2, "Dispell -- Daedra", false},
      {7, 0, "Drain -- Strength", false},
      {7, 1, "Drain -- Intelligence", false},
      {7, 2, "Drain -- Willpower", false},
      {7, 3, "Drain -- Agility", false},
      {7, 4, "Drain -- Endurance", false},
      {7, 5, "Drain -- Personality", false},
      {7, 6, "Drain -- Speed", false},
      {7, 7, "Drain -- Luck", false},
      {8, 0, "Resist -- Fire", false},
      {8, 1, "Resist -- Cold", false},
      {8, 2, "Resist -- Poison", false},
      {8, 3, "Resist -- Shock", false},
      {8, 4, "Resist -- Magicka", false},
      {9, 0, "Increase -- Strength", false},
      {9, 1, "Increase -- Intelligence", false},
      {9, 2, "Increase -- Willpower", false},
      {9, 3, "Increase -- Agility", false},
      {9, 4, "Increase -- Endurance", false},
      {9, 5, "Increase -- Personality", false},
      {9, 6, "Increase -- Speed", false},
      {9, 7, "Increase -- Luck", false},
      {10, 0, "Heal -- Strength", false},
      {10, 1, "Heal -- Intelligence", false},
      {10, 2, "Heal -- Willpower", false},
      {10, 3, "Heal -- Agility", false},
      {10, 4, "Heal -- Endurance", false},
      {10, 5, "Heal -- Personality", false},
      {10, 6, "Heal -- Speed", false},
      {10, 7, "Heal -- Luck", false},
      {10, 8, "Heal -- Health", false},
      {10, 9, "Heal -- Stamina", false},
      {11, 0, "Leech -- Strength", false},
      {11, 1, "Leech -- Intelligence", false},
      {11, 2, "Leech -- Willpower", false},
      {11, 3, "Leech -- Agility", false},
      {11, 4, "Leech -- Endurance", false},
      {11, 5, "Leech -- Personality", false},
      {11, 6, "Leech -- Speed", false},
      {11, 7, "Leech -- Luck", false},
      {11, 8, "Leech -- Health", false},
      {11, 9, "Leech -- Stamina", false},
      {12, -1, "Soul Trap", false},
      {13, 0, "Invisibility -- Normal", false},
      {13, 1, "Invisibility -- True", false},
      {14, -1, "Levitate", false},
      {15, -1, "Light", false},
      {16, -1, "Lock", false},
      {17, -1, "Open", false},
      {18, -1, "Regenerate Health", false},
      {19, -1, "Silence", false},
      {20, -1, "Spell Absorption", false},
      {21, -1, "Spell Reflection", false},
      {22, -1, "Spell Resistance", false},
      {23, 0, "Chameleon -- Normal", false},
      {23, 1, "Chameleon -- True", false},
      {24, 0, "Shadow Form -- Normal", false},
      {24, 1, "Shadow Form -- True", false},
      {25, -1, "Slowfall", false},
      {26, -1, "Climbing", false},
      {27, -1, "Jumping", false},
      {28, -1, "Free Action", false},
      {29, -1, "Lycanthropy", false},
      {30, -1, "Water Breathing", false},
      {31, -1, "Water Walking", false},
      {32, -1, "Dimunition", true},
      {33, 0, "Calm -- Animal", false},
      {33, 1, "Calm -- Undead", false},
      {33, 2, "Calm -- Humanoid", false},
      {34, -1, "Charm Mortal", false},
      {35, -1, "Shield", false},
      {36, -1, "Telekinesis", true},
      {37, -1, "Astral Travel", true},
      {38, -1, "Etherealness", true},
      {39, 0, "Detect -- Magic", false},
      {39, 1, "Detect -- Enemy", false},
      {39, 2, "Detect -- Treasure", false},
      {40, -1, "Identify", false},
      {41, -1, "Wizard Sight", true},
      {42, -1, "Darkness", true},
      {43, -1, "Recall", false},
      {44, -1, "Tongues", false},
      {45, -1, "Intensify Fire", true},
      {46, -1, "Diminish Fire", true},
  };
}

QVector<DaggerfallSpellsStd::SpellEntry> buildSpellCatalogue()
{
  using S = DaggerfallSpellsStd::SpellEntry;
  return {
      {1, "Fenrik's Door Jam", 1, false, true, false},
      {2, "Buoyancy", 2, false, true, false},
      {3, "Frostbite", 2, false, false, false},
      {4, "Levitate", 2, true, false, false},
      {5, "Light", 2, true, false, false},
      {6, "Invisibility", 2, false, false, false},
      {7, "Wizard's Fire", 2, true, false, false},
      {8, "Shock", 2, true, true, false},
      {9, "Strength Leech", 2, false, false, false},
      {10, "Free Action", 2, true, false, false},
      {11, "Resist Cold", 2, false, false, false},
      {12, "Resist Fire", 2, false, false, false},
      {13, "Resist Shock", 2, false, false, false},
      {14, "Fireball", 2, true, false, false},
      {15, "Cure Poison", 2, true, false, false},
      {16, "Ice Bolt", 2, false, false, false},
      {17, "Shield", 2, true, false, false},
      {18, "Open", 2, true, false, false},
      {19, "Wizard Lock", 1, true, false, false},
      {20, "Ice Storm", 2, true, false, false},
      {22, "Spell Shield", 1, false, false, false},
      {23, "Silence", 2, true, false, false},
      {24, "Troll's Blood", 1, false, false, false},
      {25, "Fire Storm", 2, true, false, false},
      {26, "Resist Poison", 1, false, false, false},
      {27, "Spell Drain", 1, false, false, false},
      {28, "Far Silence", 2, true, false, false},
      {29, "Toxic Cloud", 2, true, false, false},
      {30, "Shalidor's Mirror", 47, false, false, false},
      {31, "Lightning", 3, true, false, false},
      {32, "Gods' Fire", 2, false, false, false},
      {33, "Wildfire", 2, true, false, false},
      {34, "Wizard Rend", 3, false, false, false},
      {35, "Medusa's Gaze", 3, true, false, false},
      {36, "Force Bolt", 1, true, false, false},
      {37, "Slowfalling", 13, true, true, false},
      {38, "Jumping", 1, false, false, false},
      {39, "Spell Resistance", 11, false, false, false},
      {40, "Stamina", 3, true, false, false},
      {41, "Water Walking", 2, true, false, false},
      {42, "Water Breathing", 6, false, false, false},
      {44, "Chameleon", 2, false, true, false},
      {45, "Shadow Form", 15, false, false, false},
      {46, "Spell Reflection", 9, false, false, false},
      {47, "Spell Absorbtion", 25, false, false, false},
      {49, "Tongues", 44, false, false, false},
      {50, "Paralysis", 2, false, false, false},
      {51, "Sleep", 2, false, false, false},
      {52, "Vampiric Touch", 2, false, false, false},
      {53, "Hand of Sleep", 1, false, false, false},
      {54, "Magicka Leech", 47, false, false, false},
      {55, "Sphere of Negation", 2, true, false, false},
      {56, "Hand of Decay", 39, false, false, false},
      {57, "Banish Daedra", 2, false, false, false},
      {58, "Holy Word", 12, false, false, true},
      {58, "Holy Touch", 12, false, false, true},
      {59, "Null Magicka", 31, false, false, false},
      {60, "Balyna's Antidote", 49, false, false, false},
      {61, "Cure Disease", 2, false, false, false},
      {62, "Soul Trap", 36, false, false, false},
      {64, "Heal", 3, true, false, false},
      {66, "Spider Touch", 2, false, false, false},
      {67, "Energy Leech", 33, false, false, false},
      {71, "!Nux Vomica", 2, false, false, false},
      {72, "!Arsenic", 5, false, false, false},
      {73, "!Moonseed", 1, false, false, false},
      {74, "!Drothweed", 3, false, false, false},
      {75, "!Somnalius", 2, false, false, false},
      {76, "!Phyrric Acid", 1, false, false, false},
      {77, "!Magebane", 3, false, false, false},
      {78, "!Thyrwort", 16, false, false, false},
      {79, "!Indulcet", 3, false, false, false},
      {80, "!Sursum", 12, false, false, false},
      {81, "!Questro Vil", 3, false, false, false},
      {82, "Orc Strength", 1, true, false, false},
      {83, "Wisdom", 5, true, false, false},
      {84, "Iron Will", 45, true, false, false},
      {85, "Nimbleness", 1, true, false, false},
      {86, "Feet of Notorgo", 25, true, false, false},
      {87, "Fortitude", 47, true, false, false},
      {88, "Charisma", 44, true, false, false},
      {89, "Jack of Trades", 4, true, false, false},
      {90, "Charm Mortal", 6, false, false, false},
      {91, "Calm Humanoid", 2, false, false, false},
      {92, "!Lycanthropy", 10, false, false, false},
      {94, "Recall", 1, false, false, false},
      {97, "Balyna's Balm", 3, false, true, false},
      {98, "Quiet Undead", 2, false, false, false},
      {99, "Tame", 2, false, false, false},
  };
}

}  // namespace

QVector<DaggerfallSpellsStd::EffectType> DaggerfallSpellsStd::effectTypes()
{
  return buildEffectTypes();
}

QVector<DaggerfallSpellsStd::SpellEntry> DaggerfallSpellsStd::spellCatalogue()
{
  return buildSpellCatalogue();
}

QString DaggerfallSpellsStd::effectTypeName(int type, int subtype)
{
  const QVector<EffectType> values = buildEffectTypes();
  for (const EffectType& e : values) {
    if (e.type == type && e.subtype == subtype) {
      return e.name;
    }
  }
  return {};
}

bool DaggerfallSpellsStd::isEffectTypeRemoved(int type, int subtype)
{
  const QVector<EffectType> values = buildEffectTypes();
  for (const EffectType& e : values) {
    if (e.type == type && e.subtype == subtype) {
      return e.removed;
    }
  }
  return false;
}

QVector<DaggerfallSpellsStd::SpellEntry> DaggerfallSpellsStd::findSpellsByStdIndex(int stdIndex)
{
  QVector<SpellEntry> out;
  if (stdIndex < 0) {
    return out;
  }
  const QVector<SpellEntry> values = buildSpellCatalogue();
  for (const SpellEntry& e : values) {
    if (e.stdIndex == stdIndex) {
      out.push_back(e);
    }
  }
  return out;
}

DaggerfallSpellsStd::SpellEntry DaggerfallSpellsStd::findFirstSpellByName(const QString& name)
{
  const QString needle = name.trimmed();
  if (needle.isEmpty()) {
    return {};
  }

  const QVector<SpellEntry> values = buildSpellCatalogue();
  for (const SpellEntry& e : values) {
    if (QString::compare(e.name, needle, Qt::CaseInsensitive) == 0) {
      return e;
    }
  }

  // Alias seen in docs and tooling output.
  if (QString::compare(needle, "Spell Absorption", Qt::CaseInsensitive) == 0) {
    for (const SpellEntry& e : values) {
      if (QString::compare(e.name, "Spell Absorbtion", Qt::CaseInsensitive) == 0) {
        return e;
      }
    }
  }

  return {};
}
