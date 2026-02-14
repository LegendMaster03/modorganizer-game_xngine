#ifndef REDGUARDSITEM_H
#define REDGUARDSITEM_H

#include <QString>

class RedguardsMapDatabase;

class RedguardsItem
{
public:
  RedguardsItem(RedguardsMapDatabase* mapDatabase, int id, const QString& nameId,
                const QString& descriptionId);

  const QString& nameId() const { return mNameId; }
  const QString& name() const { return mName; }
  const QString& descriptionId() const { return mDescriptionId; }
  const QString& description() const { return mDescription; }

private:
  QString mNameId;
  QString mName;
  QString mDescriptionId;
  QString mDescription;
};

#endif  // REDGUARDSITEM_H
