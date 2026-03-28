#include "redguardsmapdatabase.h"

#include "redguardsmapfile.h"
#include "redguardssoupflag.h"
#include "redguardssoupfunction.h"
#include "redguardsrtxdatabase.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

#include <array>

namespace
{
QString redguardItemNameOverride(int index)
{
  constexpr std::array<const char*, 87> overrides = [] {
    std::array<const char*, 87> values{};
    values[7] = "GUARD SWORD";
    values[15] = "RUNE (2 LINES AND A DOT)";
    values[16] = "RUNE (2 LINES)";
    values[17] = "RUNE (A LINE AND DOT)";
    values[20] = "ORC'S BLOOD (SUBLIMATED)";
    values[22] = "SPIDER'S MILK (SUBLIMATED)";
    values[24] = "ECTOPLASM (SUBLIMATED)";
    values[26] = "HIST SAP (SUBLIMATED)";
    values[30] = "GLASS VIAL (WITH ELIXIR)";
    values[34] = "RUNE (fist)";
    values[35] = "'ELVEN ARTIFACTS VIII' (COPY)";
    values[53] = "ISZARA'S JOURNAL (OPEN)";
    values[57] = "ISZARA'S JOURNAL (LOCKED)";
    values[59] = "KEY TO KRISANDRA'S STOREROOM";
    values[60] = "KEY TO ISZARA'S LODGE";
    values[61] = "N'GASTA'S NECROMANCY BOOK";
    values[62] = "BAR MUG";
    values[63] = "MARIAH'S WATERING CAN";
    values[69] = "BLOODY BANDAGE";
    values[70] = "SKELETON SWORD";
    values[71] = "KEEP OUT";
    values[72] = "NO TRESPASSING";
    values[73] = "TOBIAS' BAR MUG";
    values[74] = "BONE KEY";
    values[75] = "FLAMING SABRE";
    values[76] = "GOBLIN SWORD";
    values[77] = "OGRE'S AXE";
    values[78] = "DRAM'S SWORD";
    values[79] = "SILVER KEY (PALACE)";
    values[80] = "DRAM'S BOW";
    values[81] = "DRAM'S ARROW";
    values[82] = "SILVER LOCKET (COPY)";
    values[84] = "WANTED POSTER";
    values[85] = "PALACE DIAGRAM";
    values[86] = "LAST";
    return values;
  }();

  if (index < 0 || index >= static_cast<int>(overrides.size())) {
    return {};
  }
  const char* name = overrides[static_cast<std::size_t>(index)];
  return name != nullptr ? QString::fromLatin1(name) : QString();
}
}

RedguardsMapDatabase::RedguardsMapDatabase(const RedguardsRtxDatabase& rtxDatabase)
{
  const auto& entries = rtxDatabase.entries();
  for (auto it = entries.constBegin(); it != entries.constEnd(); ++it) {
    mRtxEntries.insert(it.key(), it.value().subtitle);
  }
}

RedguardsMapDatabase::~RedguardsMapDatabase()
{
  qDeleteAll(mMapFiles);
}

QString RedguardsMapDatabase::rtxEntry(const QString& label) const
{
  return mRtxEntries.value(label, QString());
}

RedguardsMapFile* RedguardsMapDatabase::mapFileFromName(const QString& name) const
{
  return mMapNames.value(name, nullptr);
}

RedguardsMapFile* RedguardsMapDatabase::mapFileFromId(int id) const
{
  return mMapIds.value(id, nullptr);
}

bool RedguardsMapDatabase::readWorldFile(const QString& worldFilePath)
{
  QFile file(worldFilePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return false;
  }

  QTextStream in(&file);
  while (!in.atEnd()) {
    const QString line = in.readLine();
    if (line.startsWith("world_map")) {
      int start = line.indexOf('[');
      int end = line.indexOf(']');
      if (start < 0 || end < 0 || end <= start + 1) {
        continue;
      }
      bool ok = false;
      int id = line.mid(start + 1, end - start - 1).toInt(&ok);
      if (!ok) {
        continue;
      }

      int eqPos = line.indexOf('=');
      if (eqPos < 0) {
        continue;
      }
      QString fullName = line.mid(eqPos + 1).trimmed().toUpper();
      fullName.replace('\\', '/');
      QString name = fullName;
      if (fullName.startsWith("MAPS/")) {
        name = fullName.mid(5);
      }
      int dotPos = name.indexOf('.');
      if (dotPos > 0) {
        name = name.left(dotPos);
      }

      RedguardsMapFile* mapFile = mMapNames.value(name, nullptr);
      if (!mapFile) {
        mapFile = new RedguardsMapFile(this, name);
        mMapFiles.append(mapFile);
      }
      mapFile->addID(id);
      mMapNames.insert(mapFile->name(), mapFile);
      mMapIds.insert(id, mapFile);
    }
  }

  return true;
}

bool RedguardsMapDatabase::readSoupFile(const QString& soupFilePath)
{
  QFile file(soupFilePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return false;
  }

  QTextStream in(&file);
  mFunctions.append(RedguardsSoupFunction("function NullFunction params 0"));

  readSoupSection(in, "[functions]");
  readSoupSection(in, "[refs]", [this](const QString& line) {
    mFunctions.append(RedguardsSoupFunction(line));
  });

  readSoupSection(in, "[equates]", [this](const QString& line) {
    mReferences.append(line);
  });

  readSoupSection(in, "auto");
  readSoupSection(in, "endauto", [this](const QString& line) {
    const QStringList split = line.split(QRegularExpression(" *= *"));
    if (split.size() == 1) {
      mAttributes.append(line.trimmed());
    }
  });

  readSoupSection(in, "[flags]");
  readSoupSection(in, QString(), [this](const QString& line) {
    mFlags.append(RedguardsSoupFlag(line));
  });

  return true;
}

bool RedguardsMapDatabase::readItemsFile(const QString& itemsFilePath)
{
  QFile file(itemsFilePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return false;
  }

  QTextStream in(&file);
  while (!in.atEnd()) {
    QString line = in.readLine();
    if (line.startsWith("name")) {
      int eqPos = line.indexOf('=');
      if (eqPos < 0) {
        continue;
      }
      QString nameId = line.mid(eqPos + 1).trimmed().toLower();
      if (in.atEnd()) {
        break;
      }
      QString descLine = in.readLine();
      int descEq = descLine.indexOf('=');
      if (descEq < 0) {
        continue;
      }
      QString descId = descLine.mid(descEq + 1).trimmed().toLower();
      RedguardsItemData item;
      item.nameId = nameId;
      item.descriptionId = descId;
      item.name = redguardItemNameOverride(mItems.size());
      if (item.name.isEmpty()) {
        item.name = rtxEntry(nameId);
      }
      item.description = rtxEntry(descId);
      mItems.append(item);
    }
  }

  return true;
}

void RedguardsMapDatabase::readSoupSection(QTextStream& in, const QString& stopLine)
{
  while (!in.atEnd()) {
    const QString line = in.readLine();
    if (!stopLine.isNull() && line == stopLine) {
      break;
    }
  }
}

void RedguardsMapDatabase::readSoupSection(QTextStream& in, const QString& stopLine,
                                           const std::function<void(const QString&)>& action)
{
  while (!in.atEnd()) {
    const QString line = in.readLine();
    if (!stopLine.isNull() && line == stopLine) {
      break;
    }
    if (!line.isEmpty() && !line.startsWith(';')) {
      action(line);
    }
  }
}
