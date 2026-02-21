#ifndef ARENAMODATACONTENT_H
#define ARENAMODATACONTENT_H

#include <xnginemoddatacontent.h>
#include <QString>

class GameArena;

class ArenaModDataContent : public XngineModDataContent
{
public:
  ArenaModDataContent(MOBase::IGameFeatures* gameFeatures)
      : XngineModDataContent(gameFeatures)
  {
  }
};

#endif  // ARENAMODATACONTENT_H
