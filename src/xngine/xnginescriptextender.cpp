#include "xnginescriptextender.h"

#include "gamexngine.h"

#include <QDebug>

XngineScriptExtender::XngineScriptExtender(const GameXngine* game) : m_Game(game) {}

XngineScriptExtender::~XngineScriptExtender() {}

QString XngineScriptExtender::loaderName() const
{
  return {};
}

QString XngineScriptExtender::loaderPath() const
{
  return {};
}

QString XngineScriptExtender::savegameExtension() const
{
  return {};
}

bool XngineScriptExtender::isInstalled() const
{
  qDebug() << "[XnGine] ScriptExtender: not applicable";
  return false;
}

QString XngineScriptExtender::getExtenderVersion() const
{
  return {};
}

WORD XngineScriptExtender::getArch() const
{
  return 0;
}
