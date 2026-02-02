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
};

#endif  // REDGUARDS_MODDATACONTENT_H
