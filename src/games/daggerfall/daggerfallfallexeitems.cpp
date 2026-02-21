#include "daggerfallfallexeitems.h"
#include "daggerfallformatutils.h"

#include <QFile>

namespace {

QString decodeItemName(const QByteArray& data, qsizetype off)
{
  if (off < 0 || off + DaggerfallFallExeItems::NameSize > data.size()) {
    return {};
  }

  QByteArray raw = data.mid(off, DaggerfallFallExeItems::NameSize);
  const int zero = raw.indexOf('\0');
  if (zero >= 0) {
    raw.truncate(zero);
  }

  for (char& c : raw) {
    const quint8 b = static_cast<quint8>(c);
    if ((b < 0x20 || b > 0x7E) && b != 0x09) {
      return {};
    }
  }
  return QString::fromLatin1(raw).trimmed();
}

bool probablyEndOfTable(const DaggerfallFallExeItems::ItemRecord& record)
{
  if (record.name.isEmpty()) {
    return true;
  }

  // Conservative upper bounds for non-wearable item table continuity.
  if (record.name.size() > 24 || record.value < -1 || record.value > 2000000000) {
    return true;
  }

  return false;
}

}  // namespace

bool DaggerfallFallExeItems::loadFromExe(const QString& exePath, ItemTable& outTable,
                                         QString* errorMessage)
{
  outTable = {};

  QFile f(exePath);
  if (!f.open(QIODevice::ReadOnly)) {
    return Daggerfall::FormatUtil::setError(errorMessage,
                                            QString("Unable to open FALL.EXE: %1").arg(exePath));
  }
  const QByteArray data = f.readAll();
  return decodeFromBytes(data, outTable, errorMessage);
}

bool DaggerfallFallExeItems::decodeFromBytes(const QByteArray& exeData, ItemTable& outTable,
                                             QString* errorMessage)
{
  outTable = {};
  if (exeData.size() < RecordSize) {
    return Daggerfall::FormatUtil::setError(errorMessage, "FALL.EXE data is too small");
  }

  // Documentation anchor: first item name in this table is "Ruby".
  const QByteArray anchor("Ruby");
  const qsizetype anchorPos = exeData.indexOf(anchor);
  if (anchorPos < 0) {
    return Daggerfall::FormatUtil::setError(errorMessage,
                                            "Unable to locate item table anchor string 'Ruby'");
  }

  // Align to presumed record boundary by scanning backward a short distance.
  qsizetype start = anchorPos;
  const qsizetype scanStart = qMax<qsizetype>(0, anchorPos - RecordSize);
  for (qsizetype off = scanStart; off <= anchorPos; ++off) {
    if (((anchorPos - off) >= 0) && ((anchorPos - off) < NameSize)) {
      const QString testName = decodeItemName(exeData, off);
      if (testName.startsWith("Ruby")) {
        start = off;
        break;
      }
    }
  }
  outTable.startOffset = start;

  qsizetype off = start;
  const qsizetype maxRecords = (exeData.size() - start) / RecordSize;
  outTable.records.reserve(static_cast<int>(qMin<qsizetype>(maxRecords, 4096)));

  for (qsizetype i = 0; i < maxRecords; ++i, off += RecordSize) {
    ItemRecord rec;
    rec.offset = off;
    rec.name = decodeItemName(exeData, off + 0x00);

    if (!Daggerfall::FormatUtil::readLE32(exeData, off + 0x18, rec.weightQuarterKg) ||
        !Daggerfall::FormatUtil::readLE16(exeData, off + 0x1C, rec.hitPoints) ||
        !Daggerfall::FormatUtil::readLE32(exeData, off + 0x1E, rec.unknown1) ||
        !Daggerfall::FormatUtil::readLE32(exeData, off + 0x22, rec.value) ||
        !Daggerfall::FormatUtil::readLE16(exeData, off + 0x26, rec.enchantmentPoints)) {
      return Daggerfall::FormatUtil::setError(
          errorMessage, QString("Truncated record at 0x%1").arg(static_cast<qulonglong>(off), 0, 16));
    }

    rec.unknown28 = static_cast<quint8>(exeData.at(off + 0x28));
    rec.unknown29 = static_cast<quint8>(exeData.at(off + 0x29));
    rec.unknown2A = static_cast<quint8>(exeData.at(off + 0x2A));
    rec.unknown2B = static_cast<quint8>(exeData.at(off + 0x2B));
    rec.pictureLo = static_cast<quint8>(exeData.at(off + 0x2C));
    rec.pictureHi = static_cast<quint8>(exeData.at(off + 0x2D));
    rec.unknown2E = static_cast<quint8>(exeData.at(off + 0x2E));
    rec.unknown2F = static_cast<quint8>(exeData.at(off + 0x2F));

    if (i > 0 && probablyEndOfTable(rec)) {
      Daggerfall::FormatUtil::appendWarning(
          outTable.warning,
          QString("Stopped at record %1 (offset 0x%2), likely end of contiguous table")
              .arg(i)
              .arg(static_cast<qulonglong>(off), 0, 16));
      break;
    }

    outTable.records.push_back(rec);
  }

  if (outTable.records.isEmpty()) {
    return Daggerfall::FormatUtil::setError(errorMessage, "No item records decoded from FALL.EXE");
  }

  if (outTable.startOffset != 0x1B682A) {
    Daggerfall::FormatUtil::appendWarning(
        outTable.warning,
        QString("Item table start differs from known v1.07.213 offset (found 0x%1)")
            .arg(static_cast<qulonglong>(outTable.startOffset), 0, 16));
  }

  return true;
}

QHash<quint16, QString> DaggerfallFallExeItems::knownPictureCodes()
{
  // Stored as little-endian code: low byte first, high byte second.
  return {
      {0xBE00, "Spurting blood"},
      {0xBE01, "Dead Body"},
      {0xBE02, "Magic Explosion"},
      {0xBE03, "Magic Explosion #2"},
      {0xBE04, "Pile Of Bones"},
      {0xBE05, "Magic Item Swirl"},
      {0xBE06, "For Sale Shop Coins"},
      {0xBE07, "Gray Pattern (End)"},

      {0x6A00, "Well"},
      {0x6A01, "Haystack"},
      {0x6A02, "Fountain #1"},
      {0x6A03, "Fountain #2"},
      {0x6A04, "Town Picture Sign"},
      {0x6A05, "Town Blank Sign"},
      {0x6A06, "Town Rooster Sign"},
      {0x6A07, "Coal Pile"},
      {0x6A08, "Water pump #1"},
      {0x6A09, "Water pump #2"},
      {0x6A0A, "Sundial"},
      {0x6A0B, "Wood Pile #1"},
      {0x6A0C, "Wood Pile #2"},
      {0x6A0D, "Wooden Post #1"},
      {0x6A0E, "Wooden Post #2"},
      {0x6A0F, "Large Trellis"},
      {0x6A10, "Small Trellis"},
      {0x6A11, "Rock/Tombstone #1"},
      {0x6A12, "Rock/Tombstone #2"},
      {0x6A13, "Gray Pattern (End)"},
      {0x6A8F, "Shrubbery"},

      {0x6680, "Barrel"},
      {0x6681, "Filled Regular Jug"},
      {0x6682, "Jug W/Pickle"},
      {0x6684, "Orange Jug"},
      {0x6685, "Small Orange Jug"},
      {0x6686, "Purple Jug"},
      {0x6688, "Small Basket"},
      {0x6689, "Large Basket"},
      {0x668A, "Large Filled Basket"},
      {0x668B, "Regular Potion Bottle"},
      {0x668C, "Half Full Orange Potion Bottle"},
      {0x668D, "Full Orange Potion Bottle"},
      {0x668E, "Empty Green Potion Bottle"},
      {0x668F, "Half Full Orange Potion Bottle (Green Bottle)"},
      {0x6690, "Orange Potion"},
      {0x6691, "Small Bag #1"},
      {0x6692, "Mini Bag"},
      {0x6693, "Large Bag"},
      {0x6694, "Small Bag #2"},
      {0x6695, "Chest #1"},
      {0x6696, "Chest #2"},

      {0x671E, "Arm Bone"},
      {0x671F, "Misc. Bones"},
      {0x6720, "Cow Skull"},
      {0x6723, "Dead Animal"},
      {0x670A, "Skeleton"},

      {0x6800, "Globe"},
      {0x6803, "Scales"},
      {0x6804, "Telescope"},
      {0x6905, "Candelabra"},
      {0x69B5, "Altar"},
      {0x69B7, "Mantella"},
      {0x6D00, "Clay Jar"},
      {0x7F28, "Quaesto Vil"},
  };
}

QString DaggerfallFallExeItems::pictureDescription(quint16 code)
{
  return knownPictureCodes().value(code);
}
