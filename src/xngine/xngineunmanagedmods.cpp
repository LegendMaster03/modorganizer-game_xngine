#include "xngineunmanagedmods.h"
#include "gamexngine.h"

#include <QDebug>

XngineUnmanagedMods::XngineUnmanagedMods(const GameXngine* game) : m_Game(game) {}

XngineUnmanagedMods::~XngineUnmanagedMods() {}

QStringList XngineUnmanagedMods::mods(bool) const
{
  qDebug() << "[XnGine] UnmanagedMods: not used";
  return {};
}

QString XngineUnmanagedMods::displayName(const QString& modName) const
{
  return modName;
}

QFileInfo XngineUnmanagedMods::referenceFile(const QString&) const
{
  return QFileInfo();
}

QStringList XngineUnmanagedMods::secondaryFiles(const QString&) const
{
  return {};
}
