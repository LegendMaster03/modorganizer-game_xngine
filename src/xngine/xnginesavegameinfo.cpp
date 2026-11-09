#include "xnginesavegameinfo.h"

#include "xnginesavegameinfowidget.h"

XngineSaveGameInfo::XngineSaveGameInfo(GameXngine const* game) : m_Game(game) {}

XngineSaveGameInfo::~XngineSaveGameInfo() {}

// No referenced-resource verification exists in this shared adapter.
// Returning empty does not prove a save's dependencies are present.
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
