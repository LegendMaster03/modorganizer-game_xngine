#ifndef XNGINEUNMANAGEDMODS_H
#define XNGINEUNMANAGEDMODS_H

#include <unmanagedmods.h>

class GameXngine;

// No unmanaged-mod discovery is implemented in this shared adapter.
// The overrides currently return empty lists and file references.
class XngineUnmanagedMods : public MOBase::UnmanagedMods
{
public:
  XngineUnmanagedMods(const GameXngine* game);
  ~XngineUnmanagedMods();

  virtual QStringList mods(bool onlyOfficial) const override;
  virtual QString displayName(const QString& modName) const override;
  virtual QFileInfo referenceFile(const QString& modName) const override;
  virtual QStringList secondaryFiles(const QString& modName) const override;

protected:
  const GameXngine* game() const { return m_Game; }

private:
  const GameXngine* m_Game;
};

#endif  // XNGINEUNMANAGEDMODS_H
