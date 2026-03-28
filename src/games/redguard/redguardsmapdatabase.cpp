#include "redguardsmapdatabase.h"

#include "redguardsmapfile.h"
#include "redguardssoupflag.h"
#include "redguardssoupfunction.h"
#include "redguardsrtxdatabase.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

namespace
{
QString redguardItemNameOverride(int index)
{
  static const QMap<int, QString> overrides = {
      {7, "GUARD SWORD"},
      {15, "RUNE (2 LINES AND A DOT)"},
      {16, "RUNE (2 LINES)"},
      {17, "RUNE (A LINE AND DOT)"},
      {20, "ORC'S BLOOD (SUBLIMATED)"},
      {22, "SPIDER'S MILK (SUBLIMATED)"},
      {24, "ECTOPLASM (SUBLIMATED)"},
      {26, "HIST SAP (SUBLIMATED)"},
      {30, "GLASS VIAL (WITH ELIXIR)"},
      {34, "RUNE (fist)"},
      {35, "'ELVEN ARTIFACTS VIII' (COPY)"},
      {53, "ISZARA'S JOURNAL (OPEN)"},
      {57, "ISZARA'S JOURNAL (LOCKED)"},
      {59, "KEY TO KRISANDRA'S STOREROOM"},
      {60, "KEY TO ISZARA'S LODGE"},
      {61, "N'GASTA'S NECROMANCY BOOK"},
      {62, "BAR MUG"},
      {63, "MARIAH'S WATERING CAN"},
      {69, "BLOODY BANDAGE"},
      {70, "SKELETON SWORD"},
      {71, "KEEP OUT"},
      {72, "NO TRESPASSING"},
      {73, "TOBIAS' BAR MUG"},
      {74, "BONE KEY"},
      {75, "FLAMING SABRE"},
      {76, "GOBLIN SWORD"},
      {77, "OGRE'S AXE"},
      {78, "DRAM'S SWORD"},
      {79, "SILVER KEY (PALACE)"},
      {80, "DRAM'S BOW"},
      {81, "DRAM'S ARROW"},
      {82, "SILVER LOCKET (COPY)"},
      {84, "WANTED POSTER"},
      {85, "PALACE DIAGRAM"},
      {86, "LAST"},
  };

  return overrides.value(index);
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
