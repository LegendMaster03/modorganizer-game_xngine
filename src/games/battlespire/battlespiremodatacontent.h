#ifndef BATTLESPIREMODATACONTENT_H
#define BATTLESPIREMODATACONTENT_H

#include <xnginemoddatacontent.h>

#include <QString>

class GameBattlespire;

// Adds Battlespire archive, patch, and launch/config labels to the shared
// XnGine content categories. Classification is not payload validation.

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
