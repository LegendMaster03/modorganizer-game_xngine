#ifndef XGINESAVEGAMEINFOWIDGET_H
#define XGINESAVEGAMEINFOWIDGET_H

#include "isavegameinfowidget.h"

#include <QObject>

class XngineSaveGameInfo;

namespace Ui
{
class XngineSaveGameInfoWidget;
}

// MO2 save-details presentation. setSave() rebuilds display labels and
// optional game-specific detail widgets for each selected save.
class XngineSaveGameInfoWidget : public MOBase::ISaveGameInfoWidget
{
  Q_OBJECT

public:
  XngineSaveGameInfoWidget(XngineSaveGameInfo const* info, QWidget* parent);
  ~XngineSaveGameInfoWidget();

  virtual void setSave(MOBase::ISaveGame const&) override;

private:
  Ui::XngineSaveGameInfoWidget* ui;
  XngineSaveGameInfo const* m_Info;
};

#endif  // XGINESAVEGAMEINFOWIDGET_H
