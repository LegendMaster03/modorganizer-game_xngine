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
  enum EDaggerfallContent
  {
    CONTENT_DAGGERFALL_UNITY = CONTENT_NEXT_VALUE,
    CONTENT_DAGGERFALL_LIMITED_UNITY_RUNTIME,
    CONTENT_DAGGERFALL_MANUAL_INSTALLER,
    CONTENT_DAGGERFALL_ARCHIVES,
    CONTENT_DAGGERFALL_DOSBOX_CONFIG,
    CONTENT_DAGGERFALL_QUESTS,
    CONTENT_DAGGERFALL_WORLD_BLOCKS,
    CONTENT_DAGGERFALL_TEXTURES,
    CONTENT_DAGGERFALL_TEXT_RESOURCES,
    CONTENT_DAGGERFALL_SAVE_PACKS
  };

  DaggerfallsModDataContent(const MOBase::IGameFeatures* gameFeatures)
      : XngineModDataContent(gameFeatures)
  {
    if (m_Enabled.size() <= CONTENT_DAGGERFALL_SAVE_PACKS) {
      m_Enabled.resize(CONTENT_DAGGERFALL_SAVE_PACKS + 1, true);
    }
  }

  virtual std::vector<Content> getAllContents() const override;

  virtual std::vector<int>
  getContentsFor(std::shared_ptr<const MOBase::IFileTree> fileTree) const override;
};

#endif  // DAGGERFALLS_MODDATACONTENT_H
