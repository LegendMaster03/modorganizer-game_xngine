#include "redguardspatchruntime.h"

#include "redguardsmapchanges.h"
#include "redguardspatchutils.h"
#include "xngineexepatch.h"

#include <imodlist.h>
#include <imoinfo.h>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QDebug>
#include <QSettings>
#include <QTextStream>

#include "utility.h"

using namespace MOBase;

namespace
{
constexpr const char* kPatchTempModPrefix = "__redguard_patch_output_";
constexpr const char* kTempModDescription = "Temporary patched mod output";
constexpr const char* kGlobalXdeltaPathKey = "xngine/global_xdelta_exe_path";

QString profileSuffix(const QString& profilePath)
{
  if (profilePath.isEmpty()) {
    return "Default";
  }

  const QString name = QDir(profilePath).dirName();
  return name.isEmpty() ? QString("Default") : name;
}
}

QString readRedguardGlobalXdeltaPath()
{
  QSettings s;
  return s.value(kGlobalXdeltaPathKey).toString().trimmed();
}

void writeRedguardGlobalXdeltaPath(const QString& path)
{
  const QString trimmed = path.trimmed();
  if (trimmed.isEmpty()) {
    return;
  }

  QSettings s;
  if (s.value(kGlobalXdeltaPathKey).toString().trimmed() == trimmed) {
    return;
  }

  s.setValue(kGlobalXdeltaPathKey, trimmed);
  s.sync();
}

bool canApplyRedguardExePatchMods(IOrganizer* organizer,
                                  const QString& pluginName,
                                  const QString& gameDir)
{
  if (organizer == nullptr) {
    return false;
  }

  if (!organizer->pluginSetting(pluginName, "xdelta_enabled").toBool()) {
    return false;
  }

  QString configuredTool =
      organizer->pluginSetting(pluginName, "xdelta_exe_path").toString().trimmed();
  if (!configuredTool.isEmpty()) {
    writeRedguardGlobalXdeltaPath(configuredTool);
  } else {
    configuredTool = readRedguardGlobalXdeltaPath();
  }

  const QString resolved =
      XngineExePatch::findXdeltaTool({}, gameDir, configuredTool);
  if (!resolved.isEmpty()) {
    return true;
  }

  static bool warnedMissingXdelta = false;
  if (!warnedMissingXdelta) {
    qWarning().noquote()
        << "[GameRedguard] .xdelta patch setting is enabled but no xdelta tool was found."
        << "Set plugin setting 'xdelta_exe_path' or install xdelta.exe in an auto-detected location.";
    warnedMissingXdelta = true;
  }
  return false;
}

bool applyRedguardPatchMods(IOrganizer* organizer,
                            const QString& pluginName,
                            const QString& profilePath,
                            const QString& gameDir,
                            bool allowDillon241,
                            bool allowExeMods)
{
  if (organizer == nullptr) {
    qWarning().noquote() << "[GameRedguard] Organizer is null";
    return false;
  }

  auto* modList = organizer->modList();
  if (modList == nullptr) {
    qWarning().noquote() << "[GameRedguard] modList is NULL";
    return false;
  }

  const QStringList allMods = modList->allModsByProfilePriority();
  if (allMods.isEmpty()) {
    qInfo().noquote() << "[GameRedguard] No mods in profile, nothing to apply";
    return true;
  }

  const QString modsPath = organizer->modsPath();
  const QString tempModName = QString(kPatchTempModPrefix) + profileSuffix(profilePath);
  const QString tempModPath = QDir(modsPath).filePath(tempModName);

  qInfo().noquote() << "[GameRedguard] Scanning" << allMods.size() << "mods for patch files";
  qInfo().noquote() << "[GameRedguard] Mods path:" << modsPath;
  qInfo().noquote() << "[GameRedguard] Game directory:" << gameDir;

  const RedguardsPatchScanResult scanResult =
      scanRedguardPatchMods(modList, allMods, modsPath, allowDillon241, allowExeMods);

  if (scanResult.patchModCount == 0) {
    qInfo().noquote() << "[GameRedguard] No patch-based mods found";
    return true;
  }

  qWarning().noquote() << "[GameRedguard] ========================================";
  qWarning().noquote() << "[GameRedguard] PATCH MODS DETECTED:" << scanResult.patchModCount
                       << "mod(s)";
  qWarning().noquote() << "[GameRedguard] ========================================";
  qWarning().noquote() << "[GameRedguard] Building temporary patch output mod:"
                       << tempModName;
  qInfo().noquote() << "[GameRedguard] Temp mod path:" << tempModPath;
  qWarning().noquote() << "[GameRedguard] ========================================";

  if (!prepareRedguardTempPatchMod(organizer, modList, tempModName, tempModPath,
                                   scanResult.lastPatchPriority)) {
    return false;
  }

  return applyRedguardPatchModsInOrder(organizer, pluginName, scanResult.patchModsInOrder,
                                       modsPath, tempModPath, gameDir, allowDillon241,
                                       allowExeMods);
}

RedguardsPatchScanResult scanRedguardPatchMods(IModList* modList,
                                               const QStringList& allMods,
                                               const QString& modsPath,
                                               bool allowDillon241,
                                               bool allowExeMods)
{
  RedguardsPatchScanResult result;
  const QStringList patchFileTypes = {"INI Changes.txt", "Map Changes.txt", "RTX Changes.txt"};

  for (const QString& modName : allMods) {
    if (!(modList->state(modName) & IModList::STATE_ACTIVE)) {
      continue;
    }

    const QString modPath = QDir(modsPath).filePath(modName);
    QDir modDir(modPath);
    if (!modDir.exists()) {
      continue;
    }

    bool hasPatchFiles = false;
    for (const QString& patchFile : patchFileTypes) {
      if (QFile::exists(modDir.absoluteFilePath(patchFile))) {
        if (!hasPatchFiles) {
          qInfo().noquote() << "[GameRedguard] Found patch mod:" << modName;
          hasPatchFiles = true;
        }
        qInfo().noquote() << "[GameRedguard]   - Has" << patchFile;
      }
    }

    bool hasXdelta = false;
    QDirIterator xdeltaIt(modPath, QStringList() << "*.xdelta", QDir::Files,
                          QDirIterator::Subdirectories);
    while (xdeltaIt.hasNext()) {
      hasXdelta = true;
      if (!hasPatchFiles) {
        qInfo().noquote() << "[GameRedguard] Found xdelta patch mod:" << modName;
      }
      qInfo().noquote() << "[GameRedguard]   - Has xdelta:" << xdeltaIt.next();
    }

    if ((hasPatchFiles && allowDillon241) || (hasXdelta && allowExeMods)) {
      result.lastPatchPriority = modList->priority(modName);
      result.patchModCount++;
      result.patchModsInOrder.append(modName);
    }
  }

  return result;
}

bool prepareRedguardTempPatchMod(IOrganizer* organizer, IModList* modList,
                                 const QString& tempModName,
                                 const QString& tempModPath,
                                 int lastPatchPriority)
{
  if (!removeDirRecursive(tempModPath)) {
    qWarning().noquote() << "[GameRedguard] Failed to clean existing temp mod:" << tempModPath;
  }

  if (!modList->getMod(tempModName)) {
    GuessedValue<QString> guessedName(tempModName);
    auto* created = organizer->createMod(guessedName);
    if (!created) {
      qWarning().noquote() << "[GameRedguard] Failed to create temp mod entry:" << tempModName;
      qWarning().noquote() << "[GameRedguard] Patches will be generated but may not be loaded by MO2";
      if (!ensureDir(tempModPath)) {
        qWarning().noquote() << "[GameRedguard] Failed to create temp mod path:" << tempModPath;
        return false;
      }
    }
  }

  if (!ensureDir(tempModPath)) {
    qWarning().noquote() << "[GameRedguard] Failed to create temp mod path:" << tempModPath;
    return false;
  }

  const QString metaIniPath = QDir(tempModPath).filePath("meta.ini");
  QFile metaFile(metaIniPath);
  if (!metaFile.exists()) {
    if (metaFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
      QTextStream out(&metaFile);
      out << "[General]\n";
      out << "name=" << tempModName << "\n";
      out << "version=1.0\n";
      out << "author=Mod Organizer\n";
      out << "description=" << kTempModDescription << "\n";
      metaFile.close();
      qInfo().noquote() << "[GameRedguard] Created meta.ini for temp mod";
    }
  }

  if (modList->getMod(tempModName)) {
    modList->setActive(tempModName, true);
    qInfo().noquote() << "[GameRedguard] Set temp mod active:" << tempModName;

    if (lastPatchPriority >= 0) {
      modList->setPriority(tempModName, lastPatchPriority + 1);
      qInfo().noquote() << "[GameRedguard] Set temp mod priority:" << (lastPatchPriority + 1);
    }
  } else {
    qWarning().noquote() << "[GameRedguard] Temp mod not found in mod list - cannot activate";
  }

  static bool cleanupRegistered = false;
  if (!cleanupRegistered) {
    cleanupRegistered = true;
    organizer->onFinishedRun([tempModPath, tempModName, modList](const QString&, unsigned int) {
      if (modList) {
        modList->setActive(tempModName, false);
      }
      removeDirRecursive(tempModPath);
      qInfo().noquote() << "[GameRedguard] Cleanup complete - temp mod deleted";
    });
  }

  return true;
}

bool applyRedguardPatchModsInOrder(IOrganizer* organizer,
                                   const QString& pluginName,
                                   const QStringList& patchModsInOrder,
                                   const QString& modsPath,
                                   const QString& tempModPath,
                                   const QString& gameDir,
                                   bool allowDillon241,
                                   bool allowExeMods)
{
  bool success = true;
  RedguardsMapChanges combinedMapChanges;
  bool hasMapChanges = false;

  for (const QString& modName : patchModsInOrder) {
    const QString modPath = QDir(modsPath).filePath(modName);
    qInfo().noquote() << "[GameRedguard] Applying patch mod:" << modName
                      << "from" << modPath;

    if (allowDillon241 && QFile::exists(QDir(modPath).filePath("INI Changes.txt"))) {
      if (!applyIniChanges(modPath, tempModPath, gameDir)) {
        success = false;
      }
    }

    if (allowDillon241 && QFile::exists(QDir(modPath).filePath("Map Changes.txt"))) {
      const QString changesPath = QDir(modPath).filePath("Map Changes.txt");
      qInfo().noquote() << "[GameRedguard] Parsing Map Changes from" << changesPath;
      if (!combinedMapChanges.readChanges(changesPath)) {
        qWarning().noquote() << "[GameRedguard] Failed to read Map Changes:" << changesPath;
        success = false;
      } else {
        hasMapChanges = true;
      }
    }

    if (allowDillon241 && QFile::exists(QDir(modPath).filePath("RTX Changes.txt"))) {
      if (!applyRtxChanges(modPath, tempModPath, gameDir)) {
        success = false;
      }
    }

    const QString audioSource = QDir(modPath).filePath("Audio");
    if (QDir(audioSource).exists()) {
      const QString audioDest = QDir(tempModPath).filePath("Audio");
      qInfo().noquote() << "[GameRedguard] Staging Audio ->" << audioDest;
      if (!copyDirectoryContents(audioSource, audioDest)) {
        qWarning().noquote() << "[GameRedguard] Failed to stage Audio for mod:" << modName;
        success = false;
      }
    }

    const QString texturesSource = QDir(modPath).filePath("Textures");
    if (QDir(texturesSource).exists()) {
      const QString texturesDest = QDir(tempModPath).filePath("Textures");
      qInfo().noquote() << "[GameRedguard] Staging Textures ->" << texturesDest;
      if (!copyDirectoryContents(texturesSource, texturesDest)) {
        qWarning().noquote() << "[GameRedguard] Failed to stage Textures for mod:" << modName;
        success = false;
      }
    }

    if (allowExeMods) {
      QDirIterator xdeltaProbe(modPath, QStringList() << "*.xdelta", QDir::Files,
                               QDirIterator::Subdirectories);
      if (xdeltaProbe.hasNext()) {
        const QString configuredTool =
            organizer->pluginSetting(pluginName, "xdelta_exe_path").toString().trimmed();
        QString resolvedConfiguredTool = configuredTool;
        if (!resolvedConfiguredTool.isEmpty()) {
          writeRedguardGlobalXdeltaPath(resolvedConfiguredTool);
        } else {
          resolvedConfiguredTool = readRedguardGlobalXdeltaPath();
        }
        const QString xdeltaTool =
            XngineExePatch::findXdeltaTool(modPath, gameDir, resolvedConfiguredTool);
        if (xdeltaTool.isEmpty()) {
          qWarning().noquote()
              << "[GameRedguard] xdelta tool not found for mod:" << modName
              << "- checked mod/game folders, MO2 folder/tools, XDELTA_EXE, and PATH.";
          success = false;
        } else {
          QDirIterator xdeltaIt(modPath, QStringList() << "*.xdelta", QDir::Files,
                                QDirIterator::Subdirectories);
          while (xdeltaIt.hasNext()) {
            const QString patchFile = xdeltaIt.next();
            QString matchedRel;
            QString err;
            if (!XngineExePatch::applyXdeltaPatchToAnyFileInTree(
                    xdeltaTool, patchFile, gameDir, tempModPath, &matchedRel, &err)) {
              qWarning().noquote() << "[GameRedguard] Failed applying .xdelta patch" << patchFile
                                   << "from mod" << modName << ":" << err;
              success = false;
              break;
            }
            qInfo().noquote() << "[GameRedguard] Applied .xdelta patch to" << matchedRel
                              << "from mod" << modName;
          }
        }
      }
    }
  }

  if (hasMapChanges) {
    if (!applyMapChanges(combinedMapChanges, tempModPath, gameDir)) {
      success = false;
    }
  }

  return success;
}
