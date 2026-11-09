#ifndef XNGINE_MODATACHECKER_H
#define XNGINE_MODATACHECKER_H

#include <ifiletree.h>
#include <moddatachecker.h>

class GameXngine;

/**
 * @brief ModDataChecker for XnGine games.
 *
 * The base only checks immediate names and extensions for common
 * XnGine-era indicators. These are broad heuristics and may accept a
 * nonfunctional payload; game-specific checkers apply additional policy.
 */
class XngineModDataChecker : public MOBase::ModDataChecker
{
public:
  /**
   * @brief Construct a new mod-data checker for XnGine games.
   */
  XngineModDataChecker(const GameXngine* game);

  virtual CheckReturn
  dataLooksValid(std::shared_ptr<const MOBase::IFileTree> fileTree) const override;

protected:
  GameXngine const* const m_Game;

  using FileNameSet = std::set<QString, MOBase::FileNameComparator>;

  const GameXngine* game() const { return m_Game; }

  /**
   * @return the list of possible folder names in data.
   */
  virtual const FileNameSet& possibleFolderNames() const;

  /**
   * @return the extensions of possible files in data.
   */
  virtual const FileNameSet& possibleFileExtensions() const;
};

#endif  // XNGINE_MODATACHECKER_H
