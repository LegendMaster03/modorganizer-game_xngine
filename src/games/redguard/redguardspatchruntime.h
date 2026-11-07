#ifndef REDGUARDSPATCHRUNTIME_H
#define REDGUARDSPATCHRUNTIME_H

#include <QString>
#include <QStringList>

namespace MOBase
{
class IOrganizer;
class IModList;
}

// Ordered selection of active patch mods. lastPatchPriority is used to
// position the generated output mod after the last contributor.
struct RedguardsPatchScanResult
{
  int patchModCount = 0;
  int lastPatchPriority = -1;
  QStringList patchModsInOrder;
};

QString readRedguardGlobalXdeltaPath();
void writeRedguardGlobalXdeltaPath(const QString& path);
bool canApplyRedguardExePatchMods(MOBase::IOrganizer* organizer,
                                  const QString& pluginName,
                                  const QString& gameDir);

// Generates a profile-named temporary output mod and applies eligible patch
// inputs in MO2 order. Returns false for failures but does not roll back
// partially staged output; see docs/redguard-patch-lifecycle.md.
bool applyRedguardPatchMods(MOBase::IOrganizer* organizer,
                            const QString& pluginName,
                            const QString& profilePath,
                            const QString& gameDir,
                            bool allowDillon241,
                            bool allowExeMods);

RedguardsPatchScanResult scanRedguardPatchMods(MOBase::IModList* modList,
                                               const QStringList& allMods,
                                               const QString& modsPath,
                                               bool allowDillon241,
                                               bool allowExeMods);

// Removes/recreates the generated output folder and registers finished-run
// cleanup. The current callback is registered only once per process.
bool prepareRedguardTempPatchMod(MOBase::IOrganizer* organizer,
                                 MOBase::IModList* modList,
                                 const QString& tempModName,
                                 const QString& tempModPath,
                                 int lastPatchPriority);

bool applyRedguardPatchModsInOrder(MOBase::IOrganizer* organizer,
                                   const QString& pluginName,
                                   const QStringList& patchModsInOrder,
                                   const QString& modsPath,
                                   const QString& tempModPath,
                                   const QString& gameDir,
                                   bool allowDillon241,
                                   bool allowExeMods);

#endif  // REDGUARDSPATCHRUNTIME_H
