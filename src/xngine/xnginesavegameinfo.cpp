#include "xnginesavegameinfo.h"

#include "xnginesavegameinfowidget.h"

XngineSaveGameInfo::XngineSaveGameInfo(GameXngine const* game) : m_Game(game) {}

XngineSaveGameInfo::~XngineSaveGameInfo() {}

XngineSaveGameInfo::MissingAssets
XngineSaveGameInfo::getMissingAssets(MOBase::ISaveGame const&) const
{
  return {};
}

MOBase::ISaveGameInfoWidget*
XngineSaveGameInfo::getSaveGameWidget(QWidget* parent) const
{
  return new XngineSaveGameInfoWidget(this, parent);
}
