#include "redguardsmapchanges.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>

RedguardsMapChanges::RedguardsMapChanges() = default;

QList<QString>* RedguardsMapChanges::lineChangesAt(const QString& mapName, int pos)
{
  if (mLineChanges.contains(mapName) && mLineChanges[mapName].contains(pos)) {
    return &mLineChanges[mapName][pos];
  }
  return nullptr;
}

const QList<QString>* RedguardsMapChanges::lineChangesAt(const QString& mapName,
                                                         int pos) const
{
  auto mapIt = mLineChanges.constFind(mapName);
  if (mapIt == mLineChanges.constEnd()) {
    return nullptr;
  }
  auto posIt = mapIt->constFind(pos);
  if (posIt == mapIt->constEnd()) {
    return nullptr;
  }
  return &(*posIt);
}

bool RedguardsMapChanges::hasModifiedMap(const QString& mapName) const
{
  return mLineChanges.contains(mapName);
}

void RedguardsMapChanges::addChanges(const QString& mapName,
                                     const QMap<int, QList<QString>>& mapChanges)
{
  for (auto it = mapChanges.begin(); it != mapChanges.end(); ++it) {
    int pos = it.key();
    const QList<QString>& lines = it.value();
    for (const QString& line : lines) {
      addChange(mapName, pos, line);
    }
  }
}

void RedguardsMapChanges::addChange(const QString& mapName, int pos, const QString& line)
{
  if (!mLineChanges.contains(mapName)) {
    mLineChanges[mapName] = QMap<int, QList<QString>>();
  }
  if (!mLineChanges[mapName].contains(pos)) {
    mLineChanges[mapName][pos] = QList<QString>();
  }

  QList<QString>& list = mLineChanges[mapName][pos];

  if (line.isEmpty() && mapName == "ISLAND" && pos == 8621) {
    qInfo().noquote() << "[GameRedguard] Map" << mapName << "position" << pos
                      << "empty change line preserved as insertion";
  }

  // Handle deletions (represented as literal "null" string)
  if (line == "null") {
    if (list.isEmpty() || list.first() != "null") {
      list.prepend("null");  // Add deletion marker at front
    }
  } else {
    list.append(line);  // Add insertion (including empty strings)
  }
}

bool RedguardsMapChanges::readChanges(const QString& changesFilePath)
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
        currentMap = trimmed.toUpper();
        if (currentMap.startsWith("MAPS/")) {
          currentMap = currentMap.mid(5);
        } else if (currentMap.startsWith("MAPS\\")) {
          currentMap = currentMap.mid(5);
        }
        if (currentMap.endsWith(".RGM")) {
          currentMap = currentMap.left(currentMap.length() - 4);
        }
        if (!mLineChanges.contains(currentMap)) {
          mLineChanges[currentMap] = QMap<int, QList<QString>>();
        }
      } else if (!currentMap.isEmpty()) {
        // This is a position and change
        QStringList parts = line.split('\t');
        if (!parts.isEmpty()) {
          bool ok = false;
          int pos = parts[0].trimmed().toInt(&ok);
          if (ok) {
            QString change = parts.size() > 1 ? parts[1].trimmed() : "";
            addChange(currentMap, pos, change);
          }
        }
      }
    }
  }

  file.close();
  return true;
}

bool RedguardsMapChanges::writeChanges(const QString& filePath) const
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
        // Write literal "null" deletion markers; preserve empty strings
        const QString output = (change == "null") ? "null" : change;
        out << "  " << pos << "\t" << output << "\n";
      }
    }
  }

  file.close();
  return true;
}
