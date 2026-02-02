#ifndef ARENASMODDATACONTENT_H
#define ARENASMODDATACONTENT_H

#include <xnginemoddatacontent.h>

#include <QString>

class GameArena;

class ArenasModDataContent : public XngineModDataContent
{
public:
  ArenasModDataContent(MOBase::IGameFeatures* gameFeatures)
      : XngineModDataContent(gameFeatures)
  {
  }
};

#endif  // ARENASMODDATACONTENT_H
