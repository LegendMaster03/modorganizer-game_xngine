#include "daggerfallcommon.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QtMath>

#include <array>
#include <cstddef>

namespace Daggerfall
{
namespace Angle
{
double daToRadians(qint16 da)
{
  return static_cast<double>(da) * RadianPerDa;
}

double daToDegrees(qint16 da)
{
  return static_cast<double>(da) / DaPerDegree;
}

qint16 radiansToDa(double radians)
{
  return static_cast<qint16>(qRound64(radians * DaPerRadian));
}

qint16 degreesToDa(double degrees)
{
  return static_cast<qint16>(qRound64(degrees * DaPerDegree));
}
}  // namespace Angle

namespace Data
{
namespace
{
template <std::size_t N>
QString lookupString(const std::array<const char*, N>& entries, int key)
{
  if (key < 0 || key >= static_cast<int>(N)) {
    return {};
  }
  const char* value = entries[static_cast<std::size_t>(key)];
  return value != nullptr ? QString::fromLatin1(value) : QString();
}

template <std::size_t N>
int lookupInt(const std::array<int, N>& entries, int key, int fallback = -1)
{
  if (key < 0 || key >= static_cast<int>(N)) {
    return fallback;
  }
  return entries[static_cast<std::size_t>(key)];
}

constexpr std::array<const char*, 106> kRegionNames = {
    "The Alik'r Desert",
    "The Dragontail Mountains",
    "Glenpoint Foothills",
    "Daggerfall Bluffs",
    "Yeorth Burrowland",
    "Dwynnen",
    "Ravennian Forest",
    "Devilrock",
    "Malekna Forest",
    "The Isle of Balfiera",
    "Bantha",
    "Dak'fron",
    "The Islands in the Western Iliac Bay",
    "Tamarilyn Point",
    "Lainlyn Cliffs",
    "Bjoulsae River",
    "The Wrothgarian Mountains",
    "Daggerfall",
    "Glenpoint",
    "Betony",
    "Sentinel",
    "Anticlere",
    "Lainlyn",
    "Wayrest",
    "Gen Tem High Rock village",
    "Gen Rai Hammerfell village",
    "The Orsinium Area",
    "Skeffington Wood",
    "Hammerfell bay coast",
    "Hammerfell sea coast",
    "High Rock bay coast",
    "High Rock sea coast",
    "Northmoor",
    "Menevia",
    "Alcaire",
    "Koegria",
    "Bhoriane",
    "Kambria",
    "Phrygias",
    "Urvaius",
    "Ykalon",
    "Daenia",
    "Shalgora",
    "Abibon-Gora",
    "Kairou",
    "Pothago",
    "Myrkwasa",
    "Ayasofya",
    "Tigonus",
    "Kozanset",
    "Satakalaam",
    "Totambu",
    "Mournoth",
    "Ephesus",
    "Santaki",
    "Antiphyllos",
    "Bergama",
    "Gavaudon",
    "Tulune",
    "Glenumbra Moors",
    "Ilessan Hills",
    "Cybiades",
    "Vraseth",
    "Haarvenu",
    "Thrafey",
    "Lyrezi",
    "Montalion",
    "Khulari",
    "Garlythi",
    "Anthotis",
    "Selenu",
};

constexpr std::array<const char*, 15> kLocationTypes = {
    "Large Town",
    "Medium Town",
    "Small Town",
    "Farmstead",
    "Large Dungeon",
    "Temple",
    "Tavern",
    "Medium Dungeon",
    "Manor",
    "Shrine",
    "Small Dungeon",
    "Shack",
    "Cemetery",
    "Coven",
    "Player's Ship",
};

constexpr std::array<int, 15> kLocationTypePaletteIndices = {
    33,   // Large Town
    35,   // Medium Town
    37,   // Small Town
    53,   // Farmstead
    237,  // Large Dungeon
    96,   // Temple
    39,   // Tavern
    240,  // Medium Dungeon
    51,   // Manor
    101,  // Shrine
    243,  // Small Dungeon
    55,   // Shack
    246,  // Cemetery
    0,    // Coven
    -1,   // Player's Ship
};

constexpr std::array<const char*, 45> kBuildingTypes = {
    "House For Sale",
    "Tavern",
    "Residence",
    "Weaponsmith",
    "Armorer",
    "Alchemist",
    "Bank",
    "Bookstore",
    "Clothing Store",
    "Furniture Store",
    "Gem Store",
    "Library",
    "Pawn Shop",
    "Temple",
    "Guildhall",
    "Palace",
    "Farm",
    "Dungeon",
    "Castle",
    "Manor",
    "Shrine",
    "Ruins",
    "Shack",
    "Cemetery",
    "Generic",
    "Knightly Order",
    "Knightly Order",
    "Knightly Order",
    "Knightly Order",
    "Knightly Order",
    "Knightly Order",
    "Knightly Order",
    "Knightly Order",
    "Knightly Order",
    "Knightly Order",
    "Knightly Order",
    "Mages Guild",
    "Thieves Guild",
    "Dark Brotherhood",
    "Fighters Guild",
    "Custom",
    "City Wall",
    "Market",
    "Ship",
    "Coven",
};

constexpr std::array<const char*, 251> kTownBuildingTypes = [] {
  std::array<const char*, 251> values{};
  values[0x00] = "Alchemist";
  values[0x01] = "House For Sale";
  values[0x02] = "Armorer";
  values[0x03] = "Bank";
  values[0x04] = "Town4";
  values[0x05] = "Bookseller";
  values[0x06] = "Clothing Store";
  values[0x07] = "Furniture Store";
  values[0x08] = "Gem Store";
  values[0x09] = "General Store";
  values[0x0A] = "Library";
  values[0x0B] = "Guildhall";
  values[0x0C] = "Pawn Shop";
  values[0x0D] = "Weapon Smith";
  values[0x0E] = "Temple";
  values[0x0F] = "Tavern";
  values[0x10] = "Palace";
  values[0x11] = "House1";
  values[0x12] = "House2";
  values[0x13] = "House3";
  values[0x14] = "House4";
  values[0x15] = "House5";
  values[0x16] = "House6";
  values[0x17] = "City Wall";
  values[0x18] = "Ship";
  values[0x74] = "Special1";
  values[0xDF] = "Special2";
  values[0xF9] = "Special3";
  values[0xFA] = "Special4";
  return values;
}();

constexpr std::array<const char*, 13> kRulerTitles = {
    nullptr,
    "King",
    "Queen",
    "Duke",
    "Duchess",
    "Marquis",
    "Marquise",
    "Count",
    "Countess",
    "Baron",
    "Baroness",
    "Lord",
    "Lady",
};

constexpr const char* kRmbPrefixes[] = {
    "TVRN", "GENR", "RESI", "WEAP", "ARMR", "ALCH", "BANK", "BOOK", "CLOT",
    "FURN", "GEMS", "LIBR", "PAWN", "TEMP", "TEMP", "PALA", "FARM", "DUNG",
    "CAST", "MANR", "SHRI", "RUIN", "SHCK", "GRVE", "FILL", "KRAV", "KDRA",
    "KOWL", "KMOO", "KCAN", "KFLA", "KHOR", "KROS", "KWHE", "KSCA", "KHAW",
    "MAGE", "THIE", "DARK", "FIGH", "CUST", "WALL", "MARK", "SHIP", "WITC",
};

constexpr const char* kRdbPrefixes[] = {"N", "W", "L", "S", "B", "M"};
}

QString regionName(int regionIndex)
{
  if (regionIndex == 105) {
    return QStringLiteral("UnknownRegion");
  }
  return lookupString(kRegionNames, regionIndex);
}

QString locationTypeName(int locationType)
{
  return lookupString(kLocationTypes, locationType);
}

int locationTypePaletteIndex(int locationType)
{
  return lookupInt(kLocationTypePaletteIndices, locationType, -1);
}

QString buildingTypeName(int buildingType)
{
  return lookupString(kBuildingTypes, buildingType);
}

QString townBuildingTypeName(int buildingTypeCode)
{
  return lookupString(kTownBuildingTypes, buildingTypeCode);
}

QString rulerTitleName(int rulerCode)
{
  return lookupString(kRulerTitles, rulerCode);
}

bool isTownBuildingTypeShownOnAutomap(int buildingTypeCode)
{
  switch (buildingTypeCode) {
    case 0x18:  // Ship
    case 0x74:  // Special1
    case 0xDF:  // Special2
    case 0xF9:  // Special3
    case 0xFA:  // Special4
      return false;
    default:
      return true;
  }
}

bool isLocationTypeKnownOnWorldMap(int locationType)
{
  return locationType == (0xAF & locationType);
}

bool isLocationTypeUnknownOnWorldMap(int locationType)
{
  return locationType == (0x8F & locationType);
}

bool isMapsaveArchiveName(const QString& archiveName)
{
  return QFileInfo(archiveName).fileName().compare("MAPSAVE.SAV", Qt::CaseInsensitive) == 0;
}

bool isMapsaveRecordName(const QString& recordName)
{
  static const QRegularExpression rowRe("(?i)^MAPSAVE\\.0\\d\\d$");
  return rowRe.match(recordName).hasMatch();
}

QString rmbPrefixForBlockIndex(quint8 blockIndex)
{
  if (blockIndex >= std::size(kRmbPrefixes)) {
    return {};
  }
  return QString::fromLatin1(kRmbPrefixes[blockIndex]);
}

QString rdbPrefixForBlockIndex(quint8 blockIndex)
{
  if (blockIndex >= std::size(kRdbPrefixes)) {
    return {};
  }
  return QString::fromLatin1(kRdbPrefixes[blockIndex]);
}
}  // namespace Data
}  // namespace Daggerfall
