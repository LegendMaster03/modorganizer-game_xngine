#ifndef ARENAMODATACONTENT_H
#define ARENAMODATACONTENT_H

#include <xnginemoddatacontent.h>
#include <QString>

class GameArena;

// Adds Arena-specific mapper and DOSBox configuration labels to the shared
// XnGine mod-content classification. It does not validate or execute files.

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
