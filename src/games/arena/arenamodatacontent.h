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

  virtual std::vector<int>
  getContentsFor(std::shared_ptr<const MOBase::IFileTree> fileTree) const override;
};

#endif  // ARENAMODATACONTENT_H
