#ifndef DAGGERFALLS_MODDATACONTENT_H
#define DAGGERFALLS_MODDATACONTENT_H

#include <xnginemoddatacontent.h>

/**
 * Daggerfall-specific mod data content categorizer.
 * Categorizes detected mod content into game-specific types.
 */
class DaggerfallsModDataContent : public XngineModDataContent
{
public:
  using XngineModDataContent::XngineModDataContent;
};

#endif  // DAGGERFALLS_MODDATACONTENT_H
