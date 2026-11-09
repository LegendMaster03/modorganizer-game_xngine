#include "xngineunmanagedmods.h"
#include "gamexngine.h"

#include <QDebug>

XngineUnmanagedMods::XngineUnmanagedMods(const GameXngine* game) : m_Game(game) {}

XngineUnmanagedMods::~XngineUnmanagedMods() {}

// This adapter does not enumerate unmanaged files. Empty is not evidence
// that the installation contains no externally installed game assets.
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
