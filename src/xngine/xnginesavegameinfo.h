#ifndef XGINESAVEGAMEINFO_H
#define XGINESAVEGAMEINFO_H

#include "savegameinfo.h"

class GameXngine;

class XngineSaveGameInfo : public MOBase::SaveGameInfo
{
public:
  XngineSaveGameInfo(GameXngine const* game);
  ~XngineSaveGameInfo();

  virtual MissingAssets getMissingAssets(MOBase::ISaveGame const& save) const override;

  virtual MOBase::ISaveGameInfoWidget* getSaveGameWidget(QWidget*) const override;

protected:
  friend class XngineSaveGameInfoWidget;
  GameXngine const* m_Game;
};

#endif  // XGINESAVEGAMEINFO_H
