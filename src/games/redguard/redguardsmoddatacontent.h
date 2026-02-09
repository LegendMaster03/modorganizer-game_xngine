#ifndef REDGUARDS_MODDATACONTENT_H
#define REDGUARDS_MODDATACONTENT_H

#include <xnginemoddatacontent.h>

/**
 * Redguard-specific mod data content categorizer.
 * Categorizes detected mod content into game-specific types.
 */
class RedguardsModDataContent : public XngineModDataContent
{
public:
  using XngineModDataContent::XngineModDataContent;

  virtual std::vector<Content> getAllContents() const override;

  virtual std::vector<int>
  getContentsFor(std::shared_ptr<const MOBase::IFileTree> fileTree) const override;
};

#endif  // REDGUARDS_MODDATACONTENT_H
