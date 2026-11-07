#ifndef REDGUARDS_MODDATACONTENT_H
#define REDGUARDS_MODDATACONTENT_H

#include <xnginemoddatacontent.h>

/**
 * Redguard-specific mod data content categorizer.
 * Categorizes detected mod content into game-specific types.
 */
// Adds the patch-instruction UI label when a recognized Dillon241 marker
// exists. Actual patch permissions are checked by the data checker.
class RedguardsModDataContent : public XngineModDataContent
{
public:
  using XngineModDataContent::XngineModDataContent;

  virtual std::vector<Content> getAllContents() const override;

  virtual std::vector<int>
  getContentsFor(std::shared_ptr<const MOBase::IFileTree> fileTree) const override;
};

#endif  // REDGUARDS_MODDATACONTENT_H
