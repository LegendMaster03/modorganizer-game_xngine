#ifndef MAPFILE_H
#define MAPFILE_H

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QString>

class MapDatabase;

/**
 * Represents a single RGM (map) file from the game.
 * Handles reading and writing map data including scripts, headers, and resources.
 */
class MapFile
{
public:
  MapFile(MapDatabase* mapDatabase, const QString& name);

  QString getFullName() const { return mFullName; }
  QString getName() const { return mName; }
  const QList<int>& getIDs() const { return mIds; }
  void addID(int id) { mIds.append(id); }

  bool readMap(const QString& filePath);
  bool writeMap(const QString& filePath, const QString& script);

  bool isEmpty() const { return mMapHeaders.isEmpty(); }
  QString getScript() const;
  QString getModifiedScript(const class MapChanges& mapChanges) const;

private:
  MapDatabase* mMapDatabase;
  QString mName;
  QString mFullName;
  QList<int> mIds;
  QMap<QString, QByteArray> mRecords;   // RGM file sections (RAHD, RASC, etc.)
  QList<class MapHeader*> mMapHeaders;   // Parsed map headers
  int mScriptDataOffset;

  void parseMapHeaders();
};

#endif // MAPFILE_H
