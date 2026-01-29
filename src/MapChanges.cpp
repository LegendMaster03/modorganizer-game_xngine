#include "MapChanges.h"

#include <QFile>
#include <QTextStream>

MapChanges::MapChanges() = default;

QList<QString>* MapChanges::lineChangesAt(const QString& mapName, int pos)
{
  if (mLineChanges.contains(mapName) && mLineChanges[mapName].contains(pos)) {
    return &mLineChanges[mapName][pos];
  }
  return nullptr;
}

bool MapChanges::hasModifiedMap(const QString& mapName) const
{
  return mLineChanges.contains(mapName);
}

void MapChanges::addChanges(const QString& mapName, const QMap<int, QList<QString>>& mapChanges)
{
  for (auto it = mapChanges.begin(); it != mapChanges.end(); ++it) {
    int pos = it.key();
    const QList<QString>& lines = it.value();
    for (const QString& line : lines) {
      addChange(mapName, pos, line);
    }
  }
}

void MapChanges::addChange(const QString& mapName, int pos, const QString& line)
{
  if (!mLineChanges.contains(mapName)) {
    mLineChanges[mapName] = QMap<int, QList<QString>>();
  }
  if (!mLineChanges[mapName].contains(pos)) {
    mLineChanges[mapName][pos] = QList<QString>();
  }

  QList<QString>& list = mLineChanges[mapName][pos];

  // Handle deletions (represented as empty strings)
  if (line.isEmpty()) {
    if (list.isEmpty() || !list.first().isEmpty()) {
      list.prepend("");  // Add deletion marker at front
    }
  } else {
    list.append(line);  // Add insertion
  }
}

bool MapChanges::readChanges(const QString& changesFilePath)
{
  QFile file(changesFilePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return false;
  }

  QString currentMap;
  QTextStream in(&file);

  while (!in.atEnd()) {
    QString line = in.readLine();
    QString trimmed = line.trimmed();

    if (!trimmed.isEmpty()) {
      // Check if line starts with whitespace (it's a position/change line)
      if (line.at(0) != ' ' && line.at(0) != '\t') {
        // This is a map name
        currentMap = trimmed;
        if (!mLineChanges.contains(currentMap)) {
          mLineChanges[currentMap] = QMap<int, QList<QString>>();
        }
      } else if (!currentMap.isEmpty()) {
        // This is a position and change
        QStringList parts = line.split('\t');
        if (parts.size() >= 1) {
          bool ok = false;
          int pos = parts[0].trimmed().toInt(&ok);
          if (ok) {
            QString change = parts.size() > 1 ? parts[1] : "";
            addChange(currentMap, pos, change);
          }
        }
      }
    }
  }

  file.close();
  return true;
}

bool MapChanges::writeChanges(const QString& filePath) const
{
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    return false;
  }

  QTextStream out(&file);

  for (auto mapIt = mLineChanges.begin(); mapIt != mLineChanges.end(); ++mapIt) {
    out << mapIt.key() << "\n";

    const QMap<int, QList<QString>>& posMap = mapIt.value();
    for (auto posIt = posMap.begin(); posIt != posMap.end(); ++posIt) {
      int pos = posIt.key();
      const QList<QString>& changes = posIt.value();

      for (const QString& change : changes) {
        out << "  " << pos << "\t" << change << "\n";
      }
    }
  }

  file.close();
  return true;
}