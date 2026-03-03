#ifndef BATTLESPIREMODATACONTENT_H
#define BATTLESPIREMODATACONTENT_H

#include <xnginemoddatacontent.h>

#include <QString>

class GameBattlespire;

class BattlespireModDataContent : public XngineModDataContent
{
public:
  BattlespireModDataContent(MOBase::IGameFeatures* gameFeatures)
      : XngineModDataContent(gameFeatures)
  {
  }

  virtual std::vector<int>
  getContentsFor(std::shared_ptr<const MOBase::IFileTree> fileTree) const override;
};

#endif  // BATTLESPIREMODATACONTENT_H
