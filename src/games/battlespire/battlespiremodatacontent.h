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
};

#endif  // BATTLESPIREMODATACONTENT_H
