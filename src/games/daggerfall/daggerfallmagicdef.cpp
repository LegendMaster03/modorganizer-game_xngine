#include "daggerfallmagicdef.h"
#include "daggerfallformatutils.h"
#include "daggerfallspellsstd.h"

#include <QFile>

namespace {

}  // namespace

bool DaggerfallMagicDef::loadFile(const QString& magicDefPath, File& outFile,
                                  QString* errorMessage)
{
  outFile = {};

  QFile f(magicDefPath);
  if (!f.open(QIODevice::ReadOnly)) {
    return Daggerfall::FormatUtil::setError(errorMessage,
                                            QString("Unable to open MAGIC.DEF: %1").arg(magicDefPath));
  }
  const QByteArray data = f.readAll();
  if (data.size() < 4) {
    return Daggerfall::FormatUtil::setError(errorMessage, "MAGIC.DEF is too small");
  }

  if (!Daggerfall::FormatUtil::readLE32U(data, 0, outFile.recordCount)) {
    return Daggerfall::FormatUtil::setError(errorMessage, "Failed reading MAGIC.DEF record count");
  }

  const qsizetype expectedSize = 4 + static_cast<qsizetype>(outFile.recordCount) * RecordSize;
  if (expectedSize > data.size()) {
    Daggerfall::FormatUtil::appendWarning(
        outFile.warning,
        QString("Header count implies %1 bytes but file has %2 bytes").arg(expectedSize).arg(data.size()));
  } else if (expectedSize < data.size()) {
    Daggerfall::FormatUtil::appendWarning(
        outFile.warning, QString("MAGIC.DEF has %1 trailing bytes").arg(data.size() - expectedSize));
  }

  const int bySizeCount = static_cast<int>((data.size() - 4) / RecordSize);
  const int count = std::min<int>(static_cast<int>(outFile.recordCount), bySizeCount);
  outFile.records.reserve(count);

  qsizetype off = 4;
  for (int i = 0; i < count; ++i, off += RecordSize) {
    Record rec;
    rec.defIndex = i;
    rec.name = Daggerfall::FormatUtil::readFixedString(data, off + 0, 32);
    rec.type = static_cast<quint8>(data.at(off + 32));
    rec.group = static_cast<quint8>(data.at(off + 33));
    rec.subgroup = static_cast<quint8>(data.at(off + 34));

    rec.enchantments.reserve(EnchantmentSlotCount);
    for (int j = 0; j < EnchantmentSlotCount; ++j) {
      quint16 e = 0xFFFF;
      if (!Daggerfall::FormatUtil::readLE16U(data, off + 35 + j * 2, e)) {
        return Daggerfall::FormatUtil::setError(
            errorMessage, QString("Failed reading enchantment %1 of record %2").arg(j).arg(i));
      }
      rec.enchantments.push_back(e);
    }

    if (!Daggerfall::FormatUtil::readLE16U(data, off + 55, rec.uses) ||
        !Daggerfall::FormatUtil::readLE32U(data, off + 57, rec.unknownValue)) {
      return Daggerfall::FormatUtil::setError(
          errorMessage, QString("Failed reading tail fields of MAGIC.DEF record %1").arg(i));
    }
    rec.material = static_cast<quint8>(data.at(off + 61));
    outFile.records.push_back(rec);
  }

  if (outFile.records.isEmpty()) {
    return Daggerfall::FormatUtil::setError(errorMessage,
                                            "MAGIC.DEF parsed but no records were decoded");
  }

  return true;
}

int DaggerfallMagicDef::artifactTextRecordId(int artifactSubgroup)
{
  if (artifactSubgroup < 0 || artifactSubgroup > 255) {
    return -1;
  }
  return 8700 + artifactSubgroup;
}

QStringList DaggerfallMagicDef::castWhenUsedSpells()
{
  return {
      "Levitate",         "Light",             "Invisibility",      "Wizard's Fire",
      "Shock",            "Strength Leech",    "Free Action",       "Open",
      "Resist Cold",      "Resist Fire",       "Resist Shock",      "Wizard Lock",
      "Fire Ball",        "Cure Poison",       "Ice Bolt",          "Shield",
      "Spell Shield",     "Silence",           "Troll's Blood",     "Ice Storm",
      "Fire Storm",       "Resist Poison",     "Wildfire",          "Spell Drain",
      "Far Silence",      "Toxic Cloud",       "Wizard Rend",       "Shalidor's Mirror",
      "Lightning",        "Medusa's Gaze",     "Force Bolt",        "Gods' Fire",
      "Stamina",          "Heal",              "Balyna's Antidote", "Recall",
  };
}

QStringList DaggerfallMagicDef::castWhenHeldSpells()
{
  return {
      "Slowfalling",    "Spell Resistance", "Water-walking", "Free Action",
      "Water Breathing","Resist Cold",      "Resist Fire",   "Resist Poison",
      "Resist Shock",   "Invisibility",     "Chameleon",     "Shadow Form",
      "Spell Reflection","Troll's Blood",   "Spell Absorption","Levitate",
      "Tongues",        "Orc Strength",     "Wisdom",        "Iron Will",
      "Nimbleness",     "Feet of Notorgo",  "Fortitude",     "Charisma",
      "Jack of Trades",
  };
}

QStringList DaggerfallMagicDef::castWhenStrikesSpells()
{
  return {
      "Paralysis",      "Hand of Sleep",   "Vampiric Touch", "Magicka Leech",
      "Hand of Decay",  "Wildfire",        "Ice Storm",      "Fire Storm",
      "Ice Bolt",       "Wizard's Fire",   "Sphere of Negation", "Energy Leech",
  };
}

QVector<DaggerfallMagicDef::SpellTypeCode> DaggerfallMagicDef::spellTypeCodes(
    ItemMakerTrigger trigger)
{
  QStringList names;
  switch (trigger) {
    case ItemMakerTrigger::CastWhenUsed:
      names = castWhenUsedSpells();
      break;
    case ItemMakerTrigger::CastWhenHeld:
      names = castWhenHeldSpells();
      break;
    case ItemMakerTrigger::CastWhenStrikes:
      names = castWhenStrikesSpells();
      break;
  }

  QVector<SpellTypeCode> out;
  out.reserve(names.size());
  for (int i = 0; i < names.size(); ++i) {
    out.push_back({i, names.at(i)});
  }
  return out;
}

QString DaggerfallMagicDef::spellNameFromTypeCode(ItemMakerTrigger trigger, int code)
{
  QStringList names;
  switch (trigger) {
    case ItemMakerTrigger::CastWhenUsed:
      names = castWhenUsedSpells();
      break;
    case ItemMakerTrigger::CastWhenHeld:
      names = castWhenHeldSpells();
      break;
    case ItemMakerTrigger::CastWhenStrikes:
      names = castWhenStrikesSpells();
      break;
  }
  if (code < 0 || code >= names.size()) {
    return {};
  }
  return names.at(code);
}

int DaggerfallMagicDef::typeCodeFromSpellName(ItemMakerTrigger trigger, const QString& spellName)
{
  if (spellName.isEmpty()) {
    return -1;
  }

  const QString needle = spellName.trimmed();
  QStringList names;
  switch (trigger) {
    case ItemMakerTrigger::CastWhenUsed:
      names = castWhenUsedSpells();
      break;
    case ItemMakerTrigger::CastWhenHeld:
      names = castWhenHeldSpells();
      break;
    case ItemMakerTrigger::CastWhenStrikes:
      names = castWhenStrikesSpells();
      break;
  }

  for (int i = 0; i < names.size(); ++i) {
    if (QString::compare(names.at(i), needle, Qt::CaseInsensitive) == 0) {
      return i;
    }
  }
  return -1;
}

QVector<DaggerfallMagicDef::SpellReference> DaggerfallMagicDef::spellReferences(
    ItemMakerTrigger trigger)
{
  const QVector<SpellTypeCode> types = spellTypeCodes(trigger);
  QVector<SpellReference> out;
  out.reserve(types.size());

  for (const SpellTypeCode& t : types) {
    const DaggerfallSpellsStd::SpellEntry spell = DaggerfallSpellsStd::findFirstSpellByName(t.name);
    out.push_back({t.code, spell.stdIndex, t.name});
  }

  return out;
}
