#ifndef REDGUARDSPATCHRUNTIME_H
#define REDGUARDSPATCHRUNTIME_H

#include <QString>
#include <QStringList>

namespace MOBase
{
class IOrganizer;
class IModList;
}

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
