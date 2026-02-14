#include "redguardsmapdatabase.h"

#include "redguardsitem.h"
#include "redguardsmapfile.h"
#include "redguardssoupflag.h"
#include "redguardssoupfunction.h"
#include "redguardsrtxdatabase.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

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
  qDeleteAll(mFunctions);
  qDeleteAll(mFlags);
  qDeleteAll(mItems);
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
  mFunctions.append(new RedguardsSoupFunction("function NullFunction params 0"));

  readSoupSection(in, "[functions]");
  readSoupSection(in, "[refs]", [this](const QString& line) {
    mFunctions.append(new RedguardsSoupFunction(line));
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
    mFlags.append(new RedguardsSoupFlag(line));
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
      mItems.append(new RedguardsItem(this, mItems.size(), nameId, descId));
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
