#ifndef REDGUARDSMAPDATABASE_H
#define REDGUARDSMAPDATABASE_H

#include <QList>
#include <QMap>
#include <QString>
#include <QTextStream>
#include <functional>

class RedguardsMapFile;
class RedguardsSoupFunction;
class RedguardsSoupFlag;
class RedguardsRtxDatabase;

struct RedguardsItemData
{
  QString nameId;
  QString name;
  QString descriptionId;
  QString description;
  int playerMax = 0;
};

// Context for compiled RGM scripts: map, SOUP function/flag, item,
// reference, attribute, and RTX labels are loaded from game data.
class RedguardsMapDatabase
{
public:
  explicit RedguardsMapDatabase(const RedguardsRtxDatabase& rtxDatabase);
  ~RedguardsMapDatabase();

  const QMap<QString, QString>& rtxEntries() const { return mRtxEntries; }
  QString rtxEntry(const QString& label) const;

  const QList<RedguardsMapFile*>& mapFiles() const { return mMapFiles; }
  RedguardsMapFile* mapFileFromName(const QString& name) const;
  RedguardsMapFile* mapFileFromId(int id) const;

  // List indices are script-visible function IDs. Catalog ordering
  // matters when compiling a script against this database.
  const QList<RedguardsSoupFunction>& functions() const { return mFunctions; }
  const QList<RedguardsSoupFlag>& flags() const { return mFlags; }
  const QList<RedguardsItemData>& items() const { return mItems; }
  const QList<QString>& references() const { return mReferences; }
  const QList<QString>& attributes() const { return mAttributes; }

  bool readWorldFile(const QString& worldFilePath);
  bool readSoupFile(const QString& soupFilePath);
  bool readItemsFile(const QString& itemsFilePath);

private:
  QList<RedguardsMapFile*> mMapFiles;
  QList<RedguardsSoupFunction> mFunctions;
  QList<RedguardsSoupFlag> mFlags;
  QList<RedguardsItemData> mItems;
  QList<QString> mReferences;
  QList<QString> mAttributes;

  QMap<QString, QString> mRtxEntries;
  QMap<QString, RedguardsMapFile*> mMapNames;
  QMap<int, RedguardsMapFile*> mMapIds;

  void readSoupSection(QTextStream& in, const QString& stopLine);
  void readSoupSection(QTextStream& in, const QString& stopLine,
                       const std::function<void(const QString&)>& action);
};

#endif  // REDGUARDSMAPDATABASE_H
