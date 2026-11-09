#ifndef XGINESAVEGAMEINFO_H
#define XGINESAVEGAMEINFO_H

#include "savegameinfo.h"

class GameXngine;

// Shared save-info feature supplies the details widget, but currently
// reports no missing assets; an empty result does not prove dependencies
// are present in the MO2 profile.
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
