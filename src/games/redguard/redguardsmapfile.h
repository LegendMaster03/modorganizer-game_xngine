#ifndef REDGUARDSMAPFILE_H
#define REDGUARDSMAPFILE_H

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QString>

class RedguardsMapDatabase;
class RedguardsMapChanges;
class RedguardsMapHeader;

class RedguardsMapFile
{
public:
  RedguardsMapFile(RedguardsMapDatabase* mapDatabase, const QString& name);
  ~RedguardsMapFile();

  QString fullName() const { return mFullName; }
  QString name() const { return mName; }
  const QList<int>& ids() const { return mIds; }
  void addID(int id) { mIds.append(id); }

  bool readMap(const QString& filePath);
  bool writeMap(const QString& filePath, const QString& script);

  bool isEmpty() const { return mMapHeaders.isEmpty(); }
  QString getScript() const;
  QString getModifiedScript(const RedguardsMapChanges& mapChanges) const;

  int scriptDataOffset() const { return mScriptDataOffset; }

private:
  RedguardsMapDatabase* mMapDatabase;
  QString mName;
  QString mFullName;
  QList<int> mIds;
  QMap<QString, QByteArray> mRecords;
  QList<RedguardsMapHeader*> mMapHeaders;
  int mScriptDataOffset = 0;

  void parseMapHeaders();
};

#endif  // REDGUARDSMAPFILE_H
