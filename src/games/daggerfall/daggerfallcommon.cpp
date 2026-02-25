#include "daggerfallcommon.h"

#include <QtMath>

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
QHash<int, QString> regionNames()
{
  return {
      {0, "The Alik'r Desert"},
      {1, "The Dragontail Mountains"},
      {2, "Glenpoint Foothills"},
      {3, "Daggerfall Bluffs"},
      {4, "Yeorth Burrowland"},
      {5, "Dwynnen"},
      {6, "Ravennian Forest"},
      {7, "Devilrock"},
      {8, "Malekna Forest"},
      {9, "The Isle of Balfiera"},
      {10, "Bantha"},
      {11, "Dak'fron"},
      {12, "The Islands in the Western Iliac Bay"},
      {13, "Tamarilyn Point"},
      {14, "Lainlyn Cliffs"},
      {15, "Bjoulsae River"},
      {16, "The Wrothgarian Mountains"},
      {17, "Daggerfall"},
      {18, "Glenpoint"},
      {19, "Betony"},
      {20, "Sentinel"},
      {21, "Anticlere"},
      {22, "Lainlyn"},
      {23, "Wayrest"},
      {24, "Gen Tem High Rock village"},
      {25, "Gen Rai Hammerfell village"},
      {26, "The Orsinium Area"},
      {27, "Skeffington Wood"},
      {28, "Hammerfell bay coast"},
      {29, "Hammerfell sea coast"},
      {30, "High Rock bay coast"},
      {31, "High Rock sea coast"},
      {32, "Northmoor"},
      {33, "Menevia"},
      {34, "Alcaire"},
      {35, "Koegria"},
      {36, "Bhoriane"},
      {37, "Kambria"},
      {38, "Phrygias"},
      {39, "Urvaius"},
      {40, "Ykalon"},
      {41, "Daenia"},
      {42, "Shalgora"},
      {43, "Abibon-Gora"},
      {44, "Kairou"},
      {45, "Pothago"},
      {46, "Myrkwasa"},
      {47, "Ayasofya"},
      {48, "Tigonus"},
      {49, "Kozanset"},
      {50, "Satakalaam"},
      {51, "Totambu"},
      {52, "Mournoth"},
      {53, "Ephesus"},
      {54, "Santaki"},
      {55, "Antiphyllos"},
      {56, "Bergama"},
      {57, "Gavaudon"},
      {58, "Tulune"},
      {59, "Glenumbra Moors"},
      {60, "Ilessan Hills"},
      {61, "Cybiades"},
      {62, "Vraseth"},
      {63, "Haarvenu"},
      {64, "Thrafey"},
      {65, "Lyrezi"},
      {66, "Montalion"},
      {67, "Khulari"},
      {68, "Garlythi"},
      {69, "Anthotis"},
      {70, "Selenu"},
      {105, "UnknownRegion"},
  };
}

QHash<int, QString> locationTypes()
{
  return {
      {0, "Large Town"},
      {1, "Medium Town"},
      {2, "Small Town"},
      {3, "Farmstead"},
      {4, "Large Dungeon"},
      {5, "Temple"},
      {6, "Tavern"},
      {7, "Medium Dungeon"},
      {8, "Manor"},
      {9, "Shrine"},
      {10, "Small Dungeon"},
      {11, "Shack"},
      {12, "Cemetery"},
      {13, "Coven"},
      {14, "Player's Ship"},
  };
}

QHash<int, int> locationTypePaletteIndices()
{
  // FMAP_PAL.COL mapping for world-map location type colors.
  return {
      {0, 33},   // Large Town
      {1, 35},   // Medium Town
      {2, 37},   // Small Town
      {3, 53},   // Farmstead
      {4, 237},  // Large Dungeon
      {5, 96},   // Temple
      {6, 39},   // Tavern
      {7, 240},  // Medium Dungeon
      {8, 51},   // Manor
      {9, 101},  // Shrine
      {10, 243}, // Small Dungeon
      {11, 55},  // Shack
      {12, 246}, // Cemetery
      {13, 0},   // Coven
      {14, -1},  // Ship (player ship; invisible / do not draw)
  };
}

QHash<int, QString> buildingTypes()
{
  // BLOCKS.BSA block-prefix categories (index-based).
  return {
      {0, "House For Sale"},
      {1, "Tavern"},
      {2, "Residence"},
      {3, "Weaponsmith"},
      {4, "Armorer"},
      {5, "Alchemist"},
      {6, "Bank"},
      {7, "Bookstore"},
      {8, "Clothing Store"},
      {9, "Furniture Store"},
      {10, "Gem Store"},
      {11, "Library"},
      {12, "Pawn Shop"},
      {13, "Temple"},
      {14, "Guildhall"},
      {15, "Palace"},
      {16, "Farm"},
      {17, "Dungeon"},
      {18, "Castle"},
      {19, "Manor"},
      {20, "Shrine"},
      {21, "Ruins"},
      {22, "Shack"},
      {23, "Cemetery"},
      {24, "Generic"},
      {25, "Knightly Order"},
      {26, "Knightly Order"},
      {27, "Knightly Order"},
      {28, "Knightly Order"},
      {29, "Knightly Order"},
      {30, "Knightly Order"},
      {31, "Knightly Order"},
      {32, "Knightly Order"},
      {33, "Knightly Order"},
      {34, "Knightly Order"},
      {35, "Knightly Order"},
      {36, "Mages Guild"},
      {37, "Thieves Guild"},
      {38, "Dark Brotherhood"},
      {39, "Fighters Guild"},
      {40, "Custom"},
      {41, "City Wall"},
      {42, "Market"},
      {43, "Ship"},
      {44, "Coven"},
  };
}

QHash<int, QString> townBuildingTypes()
{
  // MAP/BLOCK town building type codes used by automap and quest systems.
  return {
      {0x00, "Alchemist"},
      {0x01, "House For Sale"},
      {0x02, "Armorer"},
      {0x03, "Bank"},
      {0x04, "Town4"},
      {0x05, "Bookseller"},
      {0x06, "Clothing Store"},
      {0x07, "Furniture Store"},
      {0x08, "Gem Store"},
      {0x09, "General Store"},
      {0x0A, "Library"},
      {0x0B, "Guildhall"},
      {0x0C, "Pawn Shop"},
      {0x0D, "Weapon Smith"},
      {0x0E, "Temple"},
      {0x0F, "Tavern"},
      {0x10, "Palace"},
      {0x11, "House1"},
      {0x12, "House2"},
      {0x13, "House3"},
      {0x14, "House4"},
      {0x15, "House5"},
      {0x16, "House6"},
      {0x17, "City Wall"},
      {0x18, "Ship"},
      {0x74, "Special1"},
      {0xDF, "Special2"},
      {0xF9, "Special3"},
      {0xFA, "Special4"},
  };
}

QHash<int, QString> rulerTitles()
{
  // FACTION.TXT ruler enumeration used by %rt in text records.
  return {
      {1, "King"},
      {2, "Queen"},
      {3, "Duke"},
      {4, "Duchess"},
      {5, "Marquis"},
      {6, "Marquise"},
      {7, "Count"},
      {8, "Countess"},
      {9, "Baron"},
      {10, "Baroness"},
      {11, "Lord"},
      {12, "Lady"},
  };
}

QVector<QString> rmbPrefixes()
{
  return {
      "TVRN", "GENR", "RESI", "WEAP", "ARMR", "ALCH", "BANK", "BOOK", "CLOT",
      "FURN", "GEMS", "LIBR", "PAWN", "TEMP", "TEMP", "PALA", "FARM", "DUNG",
      "CAST", "MANR", "SHRI", "RUIN", "SHCK", "GRVE", "FILL", "KRAV", "KDRA",
      "KOWL", "KMOO", "KCAN", "KFLA", "KHOR", "KROS", "KWHE", "KSCA", "KHAW",
      "MAGE", "THIE", "DARK", "FIGH", "CUST", "WALL", "MARK", "SHIP", "WITC",
  };
}

QVector<QString> rdbPrefixes()
{
  return {"N", "W", "L", "S", "B", "M"};
}

QString regionName(int regionIndex)
{
  return regionNames().value(regionIndex);
}

QString locationTypeName(int locationType)
{
  return locationTypes().value(locationType);
}

int locationTypePaletteIndex(int locationType)
{
  return locationTypePaletteIndices().value(locationType, -1);
}

QString buildingTypeName(int buildingType)
{
  return buildingTypes().value(buildingType);
}

QString townBuildingTypeName(int buildingTypeCode)
{
  return townBuildingTypes().value(buildingTypeCode);
}

QString rulerTitleName(int rulerCode)
{
  return rulerTitles().value(rulerCode);
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

QString rmbPrefixForBlockIndex(quint8 blockIndex)
{
  const auto values = rmbPrefixes();
  if (blockIndex >= static_cast<quint8>(values.size())) {
    return {};
  }
  return values.at(blockIndex);
}

QString rdbPrefixForBlockIndex(quint8 blockIndex)
{
  const auto values = rdbPrefixes();
  if (blockIndex >= static_cast<quint8>(values.size())) {
    return {};
  }
  return values.at(blockIndex);
}
}  // namespace Data
}  // namespace Daggerfall
