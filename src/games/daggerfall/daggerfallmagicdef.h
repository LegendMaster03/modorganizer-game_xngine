#ifndef DAGGERFALL_MAGICDEF_H
#define DAGGERFALL_MAGICDEF_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

class DaggerfallMagicDef
{
public:
  static constexpr qsizetype RecordSize = 62;
  static constexpr int EnchantmentSlotCount = 10;

  enum class MagicType : quint8
  {
    Regular = 0,
    Artifact = 1,
    ArtifactAlt = 2
  };

  struct Record
  {
    int defIndex = -1;                // zero-based index in MAGIC.DEF
    QString name;
    quint8 type = 0;
    quint8 group = 0;
    quint8 subgroup = 0;
    QVector<quint16> enchantments;  // 10 entries (0xFFFF means empty)
    quint16 uses = 0;
    quint32 unknownValue = 0;       // likely value/cost
    quint8 material = 0;

    bool isArtifact() const
    {
      return type == static_cast<quint8>(MagicType::Artifact) ||
             type == static_cast<quint8>(MagicType::ArtifactAlt);
    }
  };

  enum class ItemMakerTrigger : quint8
  {
    CastWhenUsed = 0,
    CastWhenHeld = 1,
    CastWhenStrikes = 2
  };

  struct SpellTypeCode
  {
    int code = -1;  // catalogue ordinal
    QString name;
  };

  struct SpellReference
  {
    int typeCode = -1;
    int stdIndex = -1;
    QString name;
  };

  struct File
  {
    quint32 recordCount = 0;
    QVector<Record> records;
    QString warning;
  };

  static bool loadFile(const QString& magicDefPath, File& outFile,
                       QString* errorMessage = nullptr);

  // Artifact descriptions in TEXT.RSC are at 8700 + artifactSubgroup.
  static int artifactTextRecordId(int artifactSubgroup);

  // Item-maker spell catalogues.
  static QStringList castWhenUsedSpells();
  static QStringList castWhenHeldSpells();
  static QStringList castWhenStrikesSpells();
  static QVector<SpellTypeCode> spellTypeCodes(ItemMakerTrigger trigger);
  static QString spellNameFromTypeCode(ItemMakerTrigger trigger, int code);
  static int typeCodeFromSpellName(ItemMakerTrigger trigger, const QString& spellName);
  static QVector<SpellReference> spellReferences(ItemMakerTrigger trigger);
};

#endif  // DAGGERFALL_MAGICDEF_H
