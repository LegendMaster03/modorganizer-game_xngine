#ifndef DAGGERFALLS_MODDATACHECKER_H
#define DAGGERFALLS_MODDATACHECKER_H

#include "gamedaggerfall.h"
#include <xnginemoddatachecker.h>
#include <QDebug>

/**
 * Daggerfall-specific mod data checker.
 * Daggerfall mod detection based on Format 0 (file replacement) only.
 */
class DaggerfallsModDataChecker : public XngineModDataChecker
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
    const bool hasXdeltaPayload = containsXdeltaPayload(fileTree);
    const bool hasJsonCatalog = containsJsonCatalog(fileTree);
    if (hasExePayload && !isAnyPatchInstallAllowed()) {
      qWarning().noquote()
          << "[GameDaggerfall] Rejecting package with EXE payload because binary patching is disabled by plugin settings.";
      return CheckReturn::INVALID;
    }
    if (hasJsonCatalog && !isJsonPatchInstallAllowed()) {
      qWarning().noquote()
          << "[GameDaggerfall] Rejecting package with JSON patch catalog because JSON patching is disabled by plugin settings.";
      return CheckReturn::INVALID;
    }
    if (hasXdeltaPayload && !isXdeltaPatchInstallAllowed()) {
      qWarning().noquote()
          << "[GameDaggerfall] Rejecting package with .xdelta payload because xdelta patching is disabled by plugin settings.";
      return CheckReturn::INVALID;
    }

    const auto base = XngineModDataChecker::dataLooksValid(fileTree);
    if (base == CheckReturn::VALID) {
      return CheckReturn::VALID;
    }

    if (hasExePayload || hasXdeltaPayload || hasJsonCatalog) {
      return CheckReturn::VALID;
    }

    return CheckReturn::INVALID;
  }

protected:
  virtual const FileNameSet& possibleFolderNames() const override
  {
    static FileNameSet result{
        "data",      "dagger",    "df",     "df/dagger",
        "textures",  "sounds",    "music",  "maps",
        "resources", "graphics",  "audio",  "video",
        "images",    "sprites",   "mif",    "pals"};
    return result;
  }

  virtual const FileNameSet& possibleFileExtensions() const override
  {
    // Daggerfall-specific extensions (Format 0 only at this time)
    static FileNameSet result{
        "dat",     // Daggerfall data files
        "mif",     // Map interchange format
        "img",     // Image/sprite resources
        "pal",     // Palette files
        "mus",     // Music files
        "xmid",    // Extended MIDI
        "fnt",     // Font files
        "wld",     // WOODS.WLD world map data
        "txt",     // Text files
        "cfg",     // Configuration files
        "ini",     // INI files
        "xdelta",  // Binary patch payloads (typically EXE patches)
        "bsa",     // Archive files
        "zip",     // Compressed archives
        "7z",      // Compressed archives
        "rar"      // Compressed archives
    };
    return result;
  }

private:
  bool isJsonPatchInstallAllowed() const
  {
    const auto* g = dynamic_cast<const GameDaggerfall*>(game());
    return (g != nullptr) ? g->allowJsonPatchInstall() : false;
  }

  bool isAnyPatchInstallAllowed() const
  {
    const auto* g = dynamic_cast<const GameDaggerfall*>(game());
    return (g != nullptr) ? g->allowExeModInstall() : false;
  }

  bool isXdeltaPatchInstallAllowed() const
  {
    const auto* g = dynamic_cast<const GameDaggerfall*>(game());
    return (g != nullptr) ? g->allowXdeltaPatchInstall() : false;
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
        if (name.compare("FALL.EXE", Qt::CaseInsensitive) == 0 ||
            name.compare("DAGGER.EXE", Qt::CaseInsensitive) == 0) {
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

  bool containsXdeltaPayload(std::shared_ptr<const MOBase::IFileTree> fileTree) const
  {
    if (!fileTree) {
      return false;
    }
    for (const auto& entry : *fileTree) {
      if (!entry) {
        continue;
      }
      if (entry->isFile()) {
        if (entry->suffix().compare("xdelta", Qt::CaseInsensitive) == 0) {
          return true;
        }
      } else if (entry->isDir()) {
        auto subtree = fileTree->findDirectory(entry->name());
        if (subtree && containsXdeltaPayload(subtree)) {
          return true;
        }
      }
    }
    return false;
  }

  bool containsJsonCatalog(std::shared_ptr<const MOBase::IFileTree> fileTree) const
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
        if (name.compare("fall_exe_patches.json", Qt::CaseInsensitive) == 0 ||
            name.compare("exe_patches.json", Qt::CaseInsensitive) == 0) {
          return true;
        }
      } else if (entry->isDir()) {
        auto subtree = fileTree->findDirectory(entry->name());
        if (subtree && containsJsonCatalog(subtree)) {
          return true;
        }
      }
    }
    return false;
  }
};

#endif  // DAGGERFALLS_MODDATACHECKER_H
