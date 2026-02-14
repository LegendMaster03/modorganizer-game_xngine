#include "redguardsitem.h"

#include "redguardsmapdatabase.h"

RedguardsItem::RedguardsItem(RedguardsMapDatabase* mapDatabase, int id,
                             const QString& nameId, const QString& descriptionId)
    : mNameId(nameId), mDescriptionId(descriptionId)
{
  switch (id) {
  case 7:
    mName = "GUARD SWORD";
    break;
  case 15:
    mName = "RUNE (2 LINES AND A DOT)";
    break;
  case 16:
    mName = "RUNE (2 LINES)";
    break;
  case 17:
    mName = "RUNE (A LINE AND DOT)";
    break;
  case 20:
    mName = "ORC'S BLOOD (SUBLIMATED)";
    break;
  case 22:
    mName = "SPIDER'S MILK (SUBLIMATED)";
    break;
  case 24:
    mName = "ECTOPLASM (SUBLIMATED)";
    break;
  case 26:
    mName = "HIST SAP (SUBLIMATED)";
    break;
  case 30:
    mName = "GLASS VIAL (WITH ELIXIR)";
    break;
  case 34:
    mName = "RUNE (fist)";
    break;
  case 35:
    mName = "'ELVEN ARTIFACTS VIII' (COPY)";
    break;
  case 53:
    mName = "ISZARA'S JOURNAL (OPEN)";
    break;
  case 57:
    mName = "ISZARA'S JOURNAL (LOCKED)";
    break;
  case 61:
    mName = "N'GASTA'S NECROMANCY BOOK";
    break;
  case 62:
    mName = "BAR MUG";
    break;
  case 63:
    mName = "MARIAH'S WATERING CAN";
    break;
  case 70:
    mName = "SKELETON SWORD";
    break;
  case 71:
    mName = "KEEP OUT";
    break;
  case 72:
    mName = "NO TRESPASSING";
    break;
  case 73:
    mName = "TOBIAS' BAR MUG";
    break;
  case 75:
    mName = "FLAMING SABRE";
    break;
  case 76:
    mName = "GOBLIN SWORD";
    break;
  case 77:
    mName = "OGRE'S AXE";
    break;
  case 78:
    mName = "DRAM'S SWORD";
    break;
  case 79:
    mName = "SILVER KEY (PALACE)";
    break;
  case 80:
    mName = "DRAM'S BOW";
    break;
  case 81:
    mName = "DRAM'S ARROW";
    break;
  case 82:
    mName = "SILVER LOCKET (COPY)";
    break;
  case 84:
    mName = "WANTED POSTER";
    break;
  case 85:
    mName = "PALACE DIAGRAM";
    break;
  case 86:
    mName = "LAST";
    break;
  default:
    mName = mapDatabase->rtxEntry(nameId);
    break;
  }

  mDescription = mapDatabase->rtxEntry(descriptionId);
}
