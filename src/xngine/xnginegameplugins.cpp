#include "xnginegameplugins.h"
#include <imodinterface.h>
#include <iplugingame.h>
#include <ipluginlist.h>
#include <report.h>
#include <safewritefile.h>
#include <scopeguard.h>
#include <utility.h>

#include <QDateTime>
#include <QDir>
#include <QHash>
#include <QDebug>
#include <QString>
#include <QStringEncoder>
#include <QStringList>

using MOBase::IOrganizer;
using MOBase::IPluginList;
using MOBase::reportError;
using MOBase::SafeWriteFile;

XngineGamePlugins::XngineGamePlugins(IOrganizer* organizer) : m_Organizer(organizer)
{}

void XngineGamePlugins::writePluginLists(const IPluginList*)
{
  qDebug() << "[XnGine] GamePlugins: plugin lists are not used for XnGine games";
}

void XngineGamePlugins::readPluginLists(MOBase::IPluginList*)
{
  qDebug() << "[XnGine] GamePlugins: plugin lists are not used for XnGine games";
}

QStringList XngineGamePlugins::getLoadOrder()
{
  qDebug() << "[XnGine] GamePlugins: no load order for XnGine games";
  return {};
}

void XngineGamePlugins::writePluginList(const MOBase::IPluginList*,
                                        const QString&)
{
  qDebug() << "[XnGine] GamePlugins: writePluginList ignored";
}

void XngineGamePlugins::writeLoadOrderList(const MOBase::IPluginList*,
                                           const QString&)
{
  qDebug() << "[XnGine] GamePlugins: writeLoadOrderList ignored";
}

void XngineGamePlugins::writeList(const IPluginList*, const QString&, bool)
{
  qDebug() << "[XnGine] GamePlugins: writeList ignored";
}

QStringList XngineGamePlugins::readLoadOrderList(MOBase::IPluginList*,
                                                 const QString&)
{
  qDebug() << "[XnGine] GamePlugins: readLoadOrderList ignored";
  return {};
}

QStringList XngineGamePlugins::readPluginList(MOBase::IPluginList*)
{
  qDebug() << "[XnGine] GamePlugins: readPluginList ignored";
  return {};
}
