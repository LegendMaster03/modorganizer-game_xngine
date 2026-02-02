#ifndef XGINEDATAARCHIVES_H
#define XGINEDATAARCHIVES_H

#include <QDir>

#include "dataarchives.h"

class GameXngine;

class XngineDataArchives : public MOBase::DataArchives
{

public:
  XngineDataArchives(const GameXngine* game);

  virtual void addArchive(MOBase::IProfile* profile, int index,
                          const QString& archiveName) override;
  virtual void removeArchive(MOBase::IProfile* profile,
                             const QString& archiveName) override;

protected:
  QDir gameDirectory() const;
  QDir localGameDirectory() const;

  QStringList getArchivesFromKey(const QString& iniFile, const QString& key,
                                 int size = 256) const;
  void setArchivesToKey(const QString& iniFile, const QString& key,
                        const QString& value);

private:
  const GameXngine* m_Game;

  virtual void writeArchiveList(MOBase::IProfile* profile,
                                const QStringList& before) = 0;
};

#endif  // XGINEDATAARCHIVES_H
