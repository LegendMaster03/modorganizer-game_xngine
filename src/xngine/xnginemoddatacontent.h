#ifndef XNGINE_MODDATACONTENT_H
#define XNGINE_MODDATACONTENT_H

#include <ifiletree.h>
#include <moddatacontent.h>

namespace MOBase
{
class IGameFeatures;
}

/**
 * @brief ModDataContent for XnGine games.
 */
class XngineModDataContent : public MOBase::ModDataContent
{
protected:
  /**
   * Note: These are used to index m_Enabled so should have standard
   * enum values, not custom ones.
   */
  enum EContent
  {
    CONTENT_PATCH_INSTRUCTIONS,
    CONTENT_FILE_OVERRIDES,
    CONTENT_AUDIO,
    CONTENT_TEXTURES,
    CONTENT_CONFIG,
    CONTENT_SCRIPTS,
    CONTENT_TEXT
  };

  /**
   * This is the first value that can be used for game-specific contents.
   */
  constexpr static auto CONTENT_NEXT_VALUE = CONTENT_TEXT + 1;

public:
  /**
   *
   */
  XngineModDataContent(const MOBase::IGameFeatures* gameFeatures);

  /**
   * @return the list of all possible contents for the corresponding game.
   */
  virtual std::vector<Content> getAllContents() const override;

  /**
   * @brief Retrieve the list of contents in the given tree.
   *
   * @param fileTree The tree corresponding to the mod to retrieve contents for.
   *
   * @return the IDs of the content in the given tree.
   */
  virtual std::vector<int>
  getContentsFor(std::shared_ptr<const MOBase::IFileTree> fileTree) const override;

protected:
  MOBase::IGameFeatures const* const m_GameFeatures;

  // List of enabled contents:
  std::vector<bool> m_Enabled;
};

#endif  // XNGINE_MODDATACONTENT_H
