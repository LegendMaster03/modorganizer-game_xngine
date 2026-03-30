#ifndef REDGUARDS_MODDATACHECKER_H
#define REDGUARDS_MODDATACHECKER_H

#include "gameredguard.h"
#include <xnginemoddatachecker.h>
#include <QDebug>
#include <QFileInfo>
#include <QSet>

/**
 * Redguard-specific mod data checker.
 * Redguard mod detection follows these indicators:
 * - Format 0 (File replacement): .RGM (Redguard module), .RTX (texture), .SAV (save) files
 * - Format 1 (Patch-based): About.txt + at least one *Changes.txt file (INI/Map/RTX changes)
 *   or mod contains Redguard game data folders (data, maps, textures, etc.)
 */
class RedguardsModDataChecker : public XngineModDataChecker
{
public:
  using XngineModDataChecker::XngineModDataChecker;

  virtual CheckReturn
  dataLooksValid(std::shared_ptr<const MOBase::IFileTree> fileTree) const override
  {
    if (!fileTree) {
      return CheckReturn::INVALID;
    }
    const bool hasExePayload = containsExecutablePayload(fileTree);
    if (hasExePayload && !isExeInstallAllowed()) {
      qWarning().noquote()
          << "[GameRedguard] Rejecting package with REDGUARD.EXE/.xdelta payload because executable patching is disabled by plugin settings.";
      return CheckReturn::INVALID;
    }

    // Redguard-only patch-instruction indicators
    if (fileTree->find("About.txt", MOBase::IFileTree::FILE) ||
        fileTree->find("INI Changes.txt", MOBase::IFileTree::FILE) ||
        fileTree->find("Map Changes.txt", MOBase::IFileTree::FILE) ||
        fileTree->find("RTX Changes.txt", MOBase::IFileTree::FILE)) {
      if (!isDillon241PatchInstallAllowed()) {
        qWarning().noquote()
            << "[GameRedguard] Rejecting Dillon241 patch package because Dillon241 patching is disabled by plugin settings.";
        return CheckReturn::INVALID;
      }
      return CheckReturn::VALID;
    }
    if (hasExePayload) {
      return CheckReturn::VALID;
    }

    if (containsConfiguredDeclaredPayload(fileTree)) {
      return CheckReturn::VALID;
    }

    return XngineModDataChecker::dataLooksValid(fileTree);
  }

protected:
  virtual const FileNameSet& possibleFolderNames() const override
  {
    static FileNameSet result{"data",
                              "3dart",
                              "fxart",
                              "maps",
                              "textures",
                              "textures/sky",
                              "textures/ui",
                              "fonts",
                              "system",
                              "sounds",
                              "sound",
                              "audio",
                              "music",
                              "video",
                              "redguard",
                              "RG",
                              "saves",
                              "savegame"};
    return result;
  }

  virtual const FileNameSet& possibleFileExtensions() const override
  {
    // Redguard-specific extensions
    static FileNameSet result{
        "3d",      // Redguard 3D mesh files
        "3dc",     // Redguard compressed/variant 3D mesh files
        "rob",     // Redguard object container files
        "rgm",     // Redguard module files
        "rtx",     // Redguard texture format
        "gxa",     // Redguard graphics archive
        "col",     // Redguard palette files
        "bsi",     // Redguard texture sky data
        "wld",     // Redguard world height/texture maps
        "sav",     // Savegame files
        "dat",     // Data files
        "mif",     // Map interchange format
        "img",     // Image resources
        "pal",     // Palette files
        "bmp",     // Bitmap resources declared by SYSTEM.INI
        "fnt",     // Font files
        "ini",     // Configuration files (for About.txt mods)
        "smk",     // Smacker videos declared by SYSTEM/ MENU config
        "noo",     // World/scene files declared by WORLD.INI
        "txt",     // Text files (including Changes.txt)
        "cfg",     // Config files
        "bsa",     // BSA archive (less common for Redguard)
        "xdelta",  // Binary patch payloads (typically EXE patches)
        "zip",     // Compressed archives
        "7z",      // Compressed archives
        "rar"      // Compressed archives
    };
    return result;
  }

private:
  bool isExeInstallAllowed() const
  {
    const auto* g = dynamic_cast<const GameRedguard*>(game());
    return (g != nullptr) ? g->allowExeModInstall() : false;
  }

  bool isDillon241PatchInstallAllowed() const
  {
    const auto* g = dynamic_cast<const GameRedguard*>(game());
    return (g != nullptr) ? g->allowDillon241PatchInstall() : false;
  }

  bool containsExecutablePayload(std::shared_ptr<const MOBase::IFileTree> fileTree) const
  {
    if (!fileTree) {
      return false;
    }

    for (const auto& entry : *fileTree) {
      if (!entry) {
        continue;
      }
      if (entry->isFile()) {
        const QString name = entry->name();
        const QString suffix = entry->suffix().toLower();
        if (name.compare("REDGUARD.EXE", Qt::CaseInsensitive) == 0 || suffix == "xdelta") {
          return true;
        }
      } else if (entry->isDir()) {
        auto subtree = fileTree->findDirectory(entry->name());
        if (subtree && containsExecutablePayload(subtree)) {
          return true;
        }
      }
    }
    return false;
  }

  bool containsConfiguredDeclaredPayload(std::shared_ptr<const MOBase::IFileTree> fileTree) const
  {
    if (!fileTree) {
      return false;
    }

    QSet<QString> declaredNames;
    declaredNames.insert(QStringLiteral("SYSTEM.INI"));
    declaredNames.insert(QStringLiteral("MENU.INI"));
    declaredNames.insert(QStringLiteral("COMBAT.INI"));
    declaredNames.insert(QStringLiteral("KEYS.INI"));
    declaredNames.insert(QStringLiteral("REGISTRY.INI"));

    const auto* g = dynamic_cast<const GameRedguard*>(game());
    if (g != nullptr) {
      declaredNames.insert(QFileInfo(g->configuredRtxFilename()).fileName());
      declaredNames.insert(QFileInfo(g->configuredWorldIniFilename()).fileName());
      declaredNames.insert(QFileInfo(g->configuredItemIniFilename()).fileName());
    }

    for (const auto& entry : *fileTree) {
      if (!entry) {
        continue;
      }
      if (entry->isFile()) {
        const QString entryName = entry->name();
        for (const QString& declaredName : declaredNames) {
          if (declaredName.compare(entryName, Qt::CaseInsensitive) == 0) {
            return true;
          }
        }
      } else if (entry->isDir()) {
        auto subtree = fileTree->findDirectory(entry->name());
        if (subtree && containsConfiguredDeclaredPayload(subtree)) {
          return true;
        }
      }
    }
    return false;
  }
};

#endif  // REDGUARDS_MODDATACHECKER_H
