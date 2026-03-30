#ifndef DAGGERFALLS_MODDATACHECKER_H
#define DAGGERFALLS_MODDATACHECKER_H

#include "gamedaggerfall.h"
#include <xnginemoddatachecker.h>
#include <QDebug>
#include <QSet>
#include <QDir>
#include <QStringList>
#include <QRegularExpression>
#include <limits>

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
    const bool hasLegacyBinaryPatchPayload = containsLegacyBinaryPatchPayload(fileTree);
    const bool hasJsonCatalog = containsJsonCatalog(fileTree);
    const bool hasUnityPayload = containsDaggerfallUnityPayload(fileTree);
    const bool hasClassicPayload = containsKnownClassicPayload(fileTree);
    const bool hasSavePackPayload = containsSavePackPayload(fileTree);
    const bool hasInstallerPayload = containsInstallerPayload(fileTree);
    const bool hasQuestSourcePayload = containsQuestSourcePayload(fileTree);
    const bool hasNestedArchivePayload = containsNestedArchivePayload(fileTree);
    const bool hasDiscImagePayload = containsDiscImagePayload(fileTree);
    const bool hasClassicOrPatchPayload =
        (hasClassicPayload || hasExePayload || hasXdeltaPayload || hasLegacyBinaryPatchPayload ||
         hasJsonCatalog || hasSavePackPayload || hasQuestSourcePayload);
    const bool allowBinaryPatching = isBinaryPatchInstallEnabled();
    const bool allowSavePack = isSavePackInstallAllowed();
    const bool allowUnity = isUnityModFormatAllowed();
    if (hasExePayload && !isAnyPatchInstallAllowed()) {
      qWarning().noquote()
          << "[GameDaggerfall] Rejecting package with EXE payload because executable/binary patch installs are disabled."
          << "Enable 'allow_json_patch_mod_install' and/or 'xdelta_enabled' in plugin settings to allow patch payloads.";
      return CheckReturn::INVALID;
    }
    if (hasJsonCatalog && !isJsonPatchInstallAllowed()) {
      qWarning().noquote()
          << "[GameDaggerfall] Rejecting package with JSON patch catalog because JSON patching is disabled."
          << "Enable 'allow_json_patch_mod_install' in plugin settings.";
      return CheckReturn::INVALID;
    }
    if (hasXdeltaPayload && !isXdeltaPatchInstallAllowed()) {
      qWarning().noquote()
          << "[GameDaggerfall] Rejecting package with xdelta payload (.xdelta/.xdelta3/.vcdiff)."
          << "Enable 'xdelta_enabled' and ensure xdelta is available via 'xdelta_exe_path' or auto-detection.";
      return CheckReturn::INVALID;
    }
    if (hasLegacyBinaryPatchPayload && !allowBinaryPatching) {
      qWarning().noquote()
          << "[GameDaggerfall] Rejecting package with legacy binary patch payload (.ips/.bps/.ups/.ppf) because binary patching is disabled."
          << "Enable 'xdelta_enabled' in plugin settings.";
      return CheckReturn::INVALID;
    }
    if (hasSavePackPayload && !allowSavePack) {
      qWarning().noquote()
          << "[GameDaggerfall] Rejecting package with save-pack payload because save-pack installs are disabled."
          << "Enable 'allow_save_pack_mod_install' in plugin settings.";
      return CheckReturn::INVALID;
    }
    if (hasLegacyBinaryPatchPayload) {
      static bool warnedLegacyPatchCompatibility = false;
      if (!warnedLegacyPatchCompatibility) {
        qWarning().noquote()
            << "[GameDaggerfall] Legacy binary patch format detected (.ips/.bps/.ups/.ppf)."
            << ".ips, .bps, .ups, and .ppf patches are applied automatically.";
        warnedLegacyPatchCompatibility = true;
      }
    }
    if (hasUnityPayload && !allowUnity) {
      if (!hasClassicOrPatchPayload) {
        qWarning().noquote()
            << "[GameDaggerfall] Rejecting package with Daggerfall Unity payload because Unity compatibility mode is disabled."
            << "Enable 'daggerfall_unity_mod_formats' to accept .dfmod/dfmod.json payload markers.";
        return CheckReturn::INVALID;
      }

      static bool warnedMixedUnityClassicPayload = false;
      if (!warnedMixedUnityClassicPayload) {
        qWarning().noquote()
            << "[GameDaggerfall] Package contains both classic and Daggerfall Unity payloads."
            << "Unity payload is ignored while Unity mod format support is disabled.";
        warnedMixedUnityClassicPayload = true;
      }
    }
    if (hasUnityPayload && allowUnity) {
      static bool warnedUnityPayloadCompatibility = false;
      if (!warnedUnityPayloadCompatibility) {
        qWarning().noquote()
            << "[GameDaggerfall] Daggerfall Unity payload detected."
            << "Compatibility mode is limited; pure Unity runtime .dfmod behavior is not available in classic Daggerfall.";
        warnedUnityPayloadCompatibility = true;
      }
    }
    if (hasInstallerPayload) {
      static bool warnedInstallerCompatibility = false;
      if (!warnedInstallerCompatibility) {
        qWarning().noquote()
            << "[GameDaggerfall] Installer-style package detected (setup/install script or SFX-style installer)."
            << "This package is accepted for compatibility, but installer execution is not automatic.";
        warnedInstallerCompatibility = true;
      }
    }
    if (hasQuestSourcePayload) {
      static bool warnedQuestSourceCompatibility = false;
      if (!warnedQuestSourceCompatibility) {
        qWarning().noquote()
            << "[GameDaggerfall] Quest source payload detected (*.src)."
            << "Source scripts are accepted for compatibility, but automatic compilation to QBN/QRC is not supported.";
        warnedQuestSourceCompatibility = true;
      }
    }
    if (hasNestedArchivePayload) {
      static bool warnedNestedArchiveCompatibility = false;
      if (!warnedNestedArchiveCompatibility) {
        qWarning().noquote()
            << "[GameDaggerfall] Nested archive payload detected (compressed/split/recovery archive files)."
            << "Archives inside an installed mod are not auto-extracted by MO2; extract them into the mod to make content active.";
        warnedNestedArchiveCompatibility = true;
      }
    }
    if (hasDiscImagePayload) {
      static bool warnedDiscImageCompatibility = false;
      if (!warnedDiscImageCompatibility) {
        qWarning().noquote()
            << "[GameDaggerfall] Disc-image style payload detected (.iso/.cue/.bin/.mds/.mdf/.nrg)."
            << "These formats are accepted for discovery only; mount or extract contents manually before install.";
        warnedDiscImageCompatibility = true;
      }
    }
    if (hasSavePackPayload) {
      static bool warnedSavePackRisk = false;
      if (!warnedSavePackRisk) {
        qWarning().noquote()
            << "[GameDaggerfall] Save-pack payload detected (SAVE* folders / MAPSAVE.SAV)."
            << "Applying this mod may overwrite existing save-slot data.";
        warnedSavePackRisk = true;
      }
    }

    const auto base = XngineModDataChecker::dataLooksValid(fileTree);
    if (base == CheckReturn::VALID) {
      return CheckReturn::VALID;
    }

    if (hasExePayload || hasXdeltaPayload || hasLegacyBinaryPatchPayload || hasJsonCatalog ||
        hasClassicPayload || hasInstallerPayload || hasQuestSourcePayload ||
        hasNestedArchivePayload || hasDiscImagePayload ||
        (hasUnityPayload && allowUnity)) {
      return CheckReturn::VALID;
    }

    if (selectInstallTreeConst(fileTree)) {
      return CheckReturn::FIXABLE;
    }

    return CheckReturn::INVALID;
  }

  virtual std::shared_ptr<MOBase::IFileTree>
  fix(std::shared_ptr<MOBase::IFileTree> fileTree) const override
  {
    if (!fileTree) {
      return nullptr;
    }
    return selectInstallTree(fileTree);
  }

protected:
  virtual const FileNameSet& possibleFolderNames() const override
  {
    static FileNameSet result{
        "arena2",    "data",      "dagger", "df",
        "arena 2",   "arena-2",   "arena_2",
        "dfdagger",  "df-dagger", "df dagger",
        "textures",  "sounds",    "music",  "maps",
        "resources", "graphics",  "audio",  "video",
        "images",    "sprites",   "mif",    "pals",
        "tes2",      "tes2daggerfall", "tes2 daggerfall",
        "classic",   "dos",       "dos version",
        "game data", "game_data", "gamedata",
        "patch",     "patches",   "update", "updates",
        "hotfix",    "hotfixes",  "fix",    "fixes",
        "mods",      "mod files", "main files", "optional files", "old files",
        "arena2 files", "arena2files", "arena2 mods",
        "dosbox",    "dosbox-0.73", "dosbox-0.74",
        "save0",     "save1",     "save2",  "save3",
        "save4",     "save5",     "save6",  "save7",
        "save8",     "save9",     "save", "saves", "savegame", "savegames",
        "save game", "save games"};
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
        "xmi",     // MIDI sequence files
        "mid",     // MIDI sequence files
        "xmf",     // Extended music files
        "fnt",     // Font files
        "exe",     // Executables (game/installer payloads)
        "com",     // DOS executables (installer/patcher tools)
        "pif",     // Legacy executable shortcut/program info files
        "wld",     // WOODS.WLD world map data
        "vid",     // Video resources
        "flc",     // Legacy animation/video resources
        "wav",     // Sound effects / voice files
        "hmi",     // HMI MIDI test/resource files declared by SETUP.INI
        "bnk",     // MIDI bank files declared by SETUP.INI
        "ico",     // Icon resources
        "cps",     // Compressed image resources
        "lip",     // Lip/portrait resources
        "txt",     // Text files
        "json",    // Patch catalogs / metadata
        "nfo",     // Readme / release notes
        "diz",     // Description-in-zip legacy text files
        "md",      // Markdown readmes
        "rtf",     // Rich text readmes
        "conf",    // DOSBox / launcher config files
        "cnf",     // Legacy config files
        "cfg",     // Configuration files
        "ini",     // INI files
        "bat",     // DOS/Windows script helpers
        "btm",     // 4DOS/NDOS batch scripts
        "cmd",     // DOS/Windows script helpers
        "ps1",     // PowerShell helpers/wrappers
        "src",     // Legacy quest source scripts
        "sav",     // Save data files
        "raw",     // Raw image/palette files
        "col",     // Palette collection files
        "rsc",     // Text/resource container
        "pak",     // Packed data files (e.g. CLIMATE/POLITIC)
        "def",     // Definition data files (e.g. MAGIC.DEF)
        "std",     // Standard spell/resource data (e.g. SPELLS.STD)
        "cif",     // Texture archives
        "rci",     // Record/bitmap containers
        "rmb",     // Exterior block files
        "rdb",     // Dungeon block files
        "rdi",     // Dungeon metadata records
        "qbn",     // Quest binary scripts
        "qrc",     // Quest resources
        "set",     // Settings tables
        "tbl",     // Table data
        "snd",     // Sound archives (DAGGER.SND)
        "voc",     // Legacy sound data
        "anm",     // Animation resources
        "cfa",     // Cinematic/animation resources
        "ips",     // Legacy binary patch format
        "bps",     // Legacy binary patch format
        "ups",     // Legacy binary patch format
        "ppf",     // Legacy binary patch format
        "xdelta",  // Binary patch payloads (typically EXE patches)
        "xdelta3", // Binary patch payloads
        "vcdiff",  // Binary patch payloads
        "bsa",     // Archive files
        "tar",     // Archive files
        "tgz",     // Tar+gzip archive shorthand
        "tbz",     // Tar+bzip archive shorthand
        "tbz2",    // Tar+bzip archive shorthand
        "txz",     // Tar+xz archive shorthand
        "tlz",     // Tar+lzma archive shorthand
        "tzst",    // Tar+zstd archive shorthand
        "gz",      // Compressed archives
        "lz",      // Compressed archives
        "lz4",     // Compressed archives
        "lzo",     // Compressed archives
        "lrz",     // Compressed archives
        "bz2",     // Compressed archives
        "xz",      // Compressed archives
        "zst",     // Compressed archives
        "lzma",    // Compressed archives
        "001",     // Split archive part (e.g. .zip.001)
        "cab",     // Legacy archive files
        "ace",     // Legacy archive files
        "arc",     // Legacy archive files
        "arj",     // Legacy archive files
        "ha",      // Legacy archive files
        "hpk",     // Legacy archive files
        "hyp",     // Legacy archive files
        "lzh",     // Legacy archive files
        "lha",     // Legacy archive files
        "uc2",     // Legacy archive files
        "z",       // Legacy compressed archive files
        "zoo",     // Legacy archive files
        "uha",     // Legacy archive files
        "jar",     // Java archive containers sometimes used as wrappers
        "pak0",    // Legacy packed container variant
        "r00",     // Split archive part
        "rev",     // RAR recovery volume / split archive companion
        "z01",     // Split archive part
        "zip",     // Compressed archives
        "zipx",    // Extended ZIP archives
        "7z",      // Compressed archives
        "rar",     // Compressed archives
        "iso",     // Disc image containers (manual extraction required)
        "cue",     // Disc image cue sheets
        "mds",     // Disc image descriptor
        "mdf",     // Disc image data
        "nrg"      // Nero disc image
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

  bool isBinaryPatchInstallEnabled() const
  {
    const auto* g = dynamic_cast<const GameDaggerfall*>(game());
    return (g != nullptr) ? g->allowBinaryPatchInstall() : false;
  }

  bool isSavePackInstallAllowed() const
  {
    const auto* g = dynamic_cast<const GameDaggerfall*>(game());
    return (g != nullptr) ? g->allowSavePackModInstall() : true;
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
        const QString ext = entry->suffix().toLower();
        if (ext == "xdelta" || ext == "xdelta3" || ext == "vcdiff") {
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

  bool containsLegacyBinaryPatchPayload(std::shared_ptr<const MOBase::IFileTree> fileTree) const
  {
    if (!fileTree) {
      return false;
    }
    for (const auto& entry : *fileTree) {
      if (!entry) {
        continue;
      }
      if (entry->isFile()) {
        const QString ext = entry->suffix().toLower();
        if (ext == "ips" || ext == "bps" || ext == "ups" || ext == "ppf") {
          return true;
        }
      } else if (entry->isDir()) {
        const auto subtree = fileTree->findDirectory(entry->name());
        if (subtree && containsLegacyBinaryPatchPayload(subtree)) {
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

  bool containsDaggerfallUnityPayload(std::shared_ptr<const MOBase::IFileTree> fileTree) const
  {
    if (!fileTree) {
      return false;
    }
    for (const auto& entry : *fileTree) {
      if (!entry) {
        continue;
      }
      if (entry->isFile()) {
        const QString fileName = entry->name();
        if (entry->suffix().compare("dfmod", Qt::CaseInsensitive) == 0 ||
            fileName.compare("dfmod.json", Qt::CaseInsensitive) == 0) {
          return true;
        }
      } else if (entry->isDir()) {
        const auto subtree = fileTree->findDirectory(entry->name());
        if (subtree && containsDaggerfallUnityPayload(subtree)) {
          return true;
        }
      }
    }
    return false;
  }

  bool containsKnownClassicPayload(std::shared_ptr<const MOBase::IFileTree> fileTree) const
  {
    if (!fileTree) {
      return false;
    }

    static const QSet<QString> classicFileNames = {
        "ARCH3D.BSA", "BLOCKS.BSA", "MAPS.BSA",   "MONSTER.BSA", "DAGGER.SND", "TEXT.RSC",
        "MIDI.BSA",   "SPELLS.STD", "MAGIC.DEF",  "WOODS.WLD",   "POLITIC.PAK", "CLIMATE.PAK",
        "FLATS.CFG",
        "FALL.EXE",   "DAGGER.EXE", "DAGGER.ICO", "SETUP.INI",  "Z.CFG", "HMISET.CFG",
        "CASTER.CFG", "TEST.WAV", "TEST.HMI", "MELODIC.BNK", "DRUM.BNK",
        "MAPSAVE.SAV", "PAL.RAW", "PAL.PAL", "OLDPAL.PAL", "MAP.PAL", "ART_PAL.COL",
        "FMAP_PAL.COL", "SAVENAME.TXT", "SAVETREE.DAT", "SAVEVARS.DAT", "IMAGE.RAW",
        "FACES.CIF", "FRAM00I0.IMG", "TALK00I0.IMG"};

    static const QSet<QString> classicExtensions = {
        "bsa", "snd", "wld", "rsc", "pak", "def", "std", "cfg", "conf", "cnf", "ini", "txt",
        "bat", "btm", "cmd", "src",
        "dat", "mif", "img", "pal", "raw", "col", "sav", "cif", "rci", "rmb", "rdb", "rdi",
        "qbn", "qrc", "set", "tbl", "voc", "xmi", "xmid", "xmf", "mid", "wav", "cps", "lip",
        "vid", "flc", "anm", "cfa"};

    for (const auto& entry : *fileTree) {
      if (!entry) {
        continue;
      }

      if (entry->isFile()) {
        const QString name = entry->name();
        const QString upperName = name.toUpper();
        static const QRegularExpression mapSaveRowRe("(?i)^MAPSAVE\\.\\d{3}$");
        static const QRegularExpression textureTripleRe("(?i)^TEXTURE\\.\\d{3}$");
        static const QRegularExpression mapsLooseRecordRe(
            "(?i)^(MAPNAMES|MAPTABLE|MAPPITEM|MAPDITEM)\\.\\d+$");
        if (classicFileNames.contains(upperName)) {
          return true;
        }
        if (mapSaveRowRe.match(name).hasMatch()) {
          return true;
        }
        if (textureTripleRe.match(name).hasMatch()) {
          return true;
        }
        if (mapsLooseRecordRe.match(name).hasMatch()) {
          return true;
        }

        const QString suffix = entry->suffix().toLower();
        if (!suffix.isEmpty() && classicExtensions.contains(suffix)) {
          return true;
        }
      } else if (entry->isDir()) {
        const QString dirName = entry->name().toLower();
        if (dirName == "arena2" || dirName == "arena 2" || dirName == "arena-2" ||
            dirName == "arena_2" || dirName == "df" || dirName == "dagger" ||
            dirName == "tes2" || dirName == "tes2daggerfall" ||
            dirName == "tes2 daggerfall" ||
            dirName == "dosbox" || dirName.startsWith("dosbox-")) {
          return true;
        }
        if (dirName == "save" || dirName == "saves" || dirName == "savegame" ||
            dirName == "savegames" || dirName == "save game" || dirName == "save games" ||
            (dirName.startsWith("save") && dirName.size() > 4 &&
             std::all_of(dirName.begin() + 4, dirName.end(),
                         [](const QChar& c) { return c.isDigit(); })) ||
            (dirName.startsWith("slot") && dirName.size() > 4 &&
             std::all_of(dirName.begin() + 4, dirName.end(),
                         [](const QChar& c) { return c.isDigit(); }))) {
          return true;
        }

        const auto subtree = fileTree->findDirectory(entry->name());
        if (subtree && containsKnownClassicPayload(subtree)) {
          return true;
        }
      }
    }

    return false;
  }

  bool containsSavePackPayload(std::shared_ptr<const MOBase::IFileTree> fileTree) const
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
        static const QSet<QString> savePackFiles = {
            "MAPSAVE.SAV", "SAVENAME.TXT", "SAVETREE.DAT", "SAVEVARS.DAT", "IMAGE.RAW"};
        static const QRegularExpression mapSaveRowRe("(?i)^MAPSAVE\\.\\d{3}$");
        if (savePackFiles.contains(name.toUpper())) {
          return true;
        }
        if (mapSaveRowRe.match(name).hasMatch()) {
          return true;
        }
      } else if (entry->isDir()) {
        const QString dirName = entry->name().toLower();
        if (dirName == "save" || dirName == "saves" || dirName == "savegame" ||
            dirName == "savegames" || dirName == "save game" || dirName == "save games" ||
            (dirName.startsWith("save") && dirName.size() > 4 &&
             std::all_of(dirName.begin() + 4, dirName.end(),
                         [](const QChar& c) { return c.isDigit(); })) ||
            (dirName.startsWith("slot") && dirName.size() > 4 &&
             std::all_of(dirName.begin() + 4, dirName.end(),
                         [](const QChar& c) { return c.isDigit(); }))) {
          return true;
        }

        const auto subtree = fileTree->findDirectory(entry->name());
        if (subtree && containsSavePackPayload(subtree)) {
          return true;
        }
      }
    }
    return false;
  }

  bool containsInstallerPayload(std::shared_ptr<const MOBase::IFileTree> fileTree) const
  {
    if (!fileTree) {
      return false;
    }
    for (const auto& entry : *fileTree) {
      if (!entry) {
        continue;
      }
      if (entry->isFile()) {
        const QString name = entry->name().toLower();
        const QString ext = entry->suffix().toLower();
        static const QRegularExpression installKeywordPattern(
            R"((^|[^a-z0-9])(setup|install|installme|patch|update|upgrade|extract|unpack|runme)($|[^a-z0-9]))",
            QRegularExpression::CaseInsensitiveOption);
        const bool installScriptByName =
            ((ext == "bat" || ext == "btm" || ext == "cmd" || ext == "ps1") &&
             installKeywordPattern.match(name).hasMatch());
        const bool selfExtractingArchiveExe =
            (ext == "exe" &&
             (name.contains("sfx") || name.contains("selfextract") ||
              name.contains("extractor") || name.contains("unpack")));
        if (name == "setup.exe" || name == "install.exe" || name == "install.bat" ||
            name == "setup.bat" || name == "setup.cmd" || name == "install.cmd" ||
            name == "setup.com" || name == "install.com" || name == "patch.com" ||
            name == "go.bat" || name == "runme.bat" || name == "runme.cmd" ||
            name == "installme.bat" || name == "installme.cmd" ||
            (ext == "exe" && installKeywordPattern.match(name).hasMatch()) ||
            (ext == "com" && installKeywordPattern.match(name).hasMatch()) ||
            (ext == "pif" && installKeywordPattern.match(name).hasMatch()) ||
            selfExtractingArchiveExe ||
            installScriptByName) {
          return true;
        }
      } else if (entry->isDir()) {
        const QString dirName = entry->name().toLower();
        if (dirName.contains("install") || dirName.contains("installer") ||
            dirName.contains("setup") || dirName.contains("patch")) {
          return true;
        }
        const auto subtree = fileTree->findDirectory(entry->name());
        if (subtree && containsInstallerPayload(subtree)) {
          return true;
        }
      }
    }
    return false;
  }

  bool containsQuestSourcePayload(std::shared_ptr<const MOBase::IFileTree> fileTree) const
  {
    if (!fileTree) {
      return false;
    }
    for (const auto& entry : *fileTree) {
      if (!entry) {
        continue;
      }
      if (entry->isFile()) {
        if (entry->suffix().compare("src", Qt::CaseInsensitive) == 0) {
          return true;
        }
      } else if (entry->isDir()) {
        const auto subtree = fileTree->findDirectory(entry->name());
        if (subtree && containsQuestSourcePayload(subtree)) {
          return true;
        }
      }
    }
    return false;
  }

  bool containsNestedArchivePayload(std::shared_ptr<const MOBase::IFileTree> fileTree) const
  {
    if (!fileTree) {
      return false;
    }
    for (const auto& entry : *fileTree) {
      if (!entry) {
        continue;
      }
      if (entry->isFile()) {
        const QString ext = entry->suffix().toLower();
        const QString fileName = entry->name().toLower();
        const bool isSfxExe =
            (ext == "exe" &&
             (fileName.contains("sfx") || fileName.contains("selfextract") ||
              fileName.contains("extractor") || fileName.contains("unpack")));
        static const QRegularExpression splitArchivePattern(
            R"((\.((zip|7z|rar)\.\d{3}|r\d{2}|z\d{2}|part\d+\.(rar|7z|zip)))$)",
            QRegularExpression::CaseInsensitiveOption);
        static const QRegularExpression tarCompoundPattern(
            R"((\.(tar\.(gz|bz2|xz|zst|lzma)))$)",
            QRegularExpression::CaseInsensitiveOption);
        const bool isSplitArchivePart = splitArchivePattern.match(fileName).hasMatch();
        const bool isTarCompound = tarCompoundPattern.match(fileName).hasMatch();
        if (ext == "zip" || ext == "zipx" || ext == "7z" || ext == "rar" || ext == "tar" || ext == "tgz" ||
            ext == "tbz" || ext == "tbz2" || ext == "txz" || ext == "tlz" ||
            ext == "tzst" || ext == "gz" || ext == "lz" || ext == "lz4" || ext == "lzo" ||
            ext == "lrz" || ext == "bz2" || ext == "xz" ||
            ext == "zst" || ext == "lzma" ||
            ext == "ace" || ext == "arc" || ext == "cab" || ext == "arj" ||
            ext == "ha" || ext == "hpk" || ext == "hyp" || ext == "lzh" ||
            ext == "lha" || ext == "uc2" || ext == "z" || ext == "zoo" ||
            ext == "uha" || ext == "jar" || ext == "pak0" ||
            isSplitArchivePart || isTarCompound || isSfxExe) {
          return true;
        }
      } else if (entry->isDir()) {
        const auto subtree = fileTree->findDirectory(entry->name());
        if (subtree && containsNestedArchivePayload(subtree)) {
          return true;
        }
      }
    }
    return false;
  }

  bool containsDiscImagePayload(std::shared_ptr<const MOBase::IFileTree> fileTree) const
  {
    if (!fileTree) {
      return false;
    }
    static const QSet<QString> discExt = {"iso", "cue", "mds", "mdf", "nrg"};
    static const QRegularExpression binCueTrackPattern(
        R"((^|[^a-z0-9])track\d+\.bin$)", QRegularExpression::CaseInsensitiveOption);

    for (const auto& entry : *fileTree) {
      if (!entry) {
        continue;
      }
      if (entry->isFile()) {
        const QString ext = entry->suffix().toLower();
        const QString name = entry->name().toLower();
        if (discExt.contains(ext) || binCueTrackPattern.match(name).hasMatch()) {
          return true;
        }
      } else if (entry->isDir()) {
        const auto subtree = fileTree->findDirectory(entry->name());
        if (subtree && containsDiscImagePayload(subtree)) {
          return true;
        }
      }
    }

    return false;
  }

  bool isUnityModFormatAllowed() const
  {
    const auto* g = dynamic_cast<const GameDaggerfall*>(game());
    return (g != nullptr) ? g->allowUnityModFormats() : false;
  }

  QString joinPath(const QString& lhs, const QString& rhs) const
  {
    if (lhs.isEmpty()) {
      return rhs;
    }
    return QString("%1/%2").arg(lhs, rhs);
  }

  QString normalizeSelectedInstallRootPath(const QString& selectedPath) const
  {
    if (selectedPath.isEmpty()) {
      return selectedPath;
    }

    QStringList parts = selectedPath.split('/', Qt::SkipEmptyParts);
    if (parts.size() < 2) {
      return selectedPath;
    }

    const QString last = parts.back().toLower();
    const bool lastIsArena2 =
        (last == "arena2" || last == "arena 2" || last == "arena-2" || last == "arena_2");
    if (!lastIsArena2) {
      return selectedPath;
    }

    const QString parent = parts.at(parts.size() - 2).toLower();
    if (parent == "dagger" || parent == "df") {
      parts.removeLast();
      return parts.join('/');
    }

    if (parts.size() >= 3) {
      const QString grandParent = parts.at(parts.size() - 3).toLower();
      if (parent == "dagger" && grandParent == "df") {
        parts.removeLast();
        return parts.join('/');
      }
    }

    return selectedPath;
  }

  QStringList installVariantNames() const
  {
    return {
        "ARENA2",
        "arena2",
        "ARENA2 FILES",
        "Arena2 Files",
        "arena2 files",
        "ARENA2FILES",
        "arena2files",
        "ARENA2 MODS",
        "Arena2 Mods",
        "arena2 mods",
        "ARENA 2",
        "Arena 2",
        "arena 2",
        "ARENA-2",
        "arena-2",
        "ARENA_2",
        "arena_2",
        "DF/DAGGER",
        "df/dagger",
        "DFDAGGER",
        "Dfdagger",
        "dfdagger",
        "DF-DAGGER",
        "Df-Dagger",
        "df-dagger",
        "DF DAGGER",
        "Df Dagger",
        "df dagger",
        "TES2",
        "tes2",
        "TES2-DAGGERFALL",
        "Tes2-Daggerfall",
        "tes2-daggerfall",
        "TES2 DAGGERFALL",
        "Tes2 Daggerfall",
        "tes2 daggerfall",
        "TES2DAGGERFALL",
        "tes2daggerfall",
        "CLASSIC",
        "Classic",
        "classic",
        "DOS VERSION",
        "Dos Version",
        "dos version",
        "DAGGER",
        "dagger",
        "DAGGERFALL",
        "Daggerfall",
        "daggerfall",
        "DATA",
        "data",
        "GAMEDATA",
        "gamedata",
        "GAME DATA",
        "Game Data",
        "game data",
        "GAME_DATA",
        "game_data",
        "GAMEDATA FILES",
        "Gamedata Files",
        "gamedata files",
        "GAME FILES",
        "Game Files",
        "game files",
        "INSTALL FILES",
        "Install Files",
        "install files",
        "PATCH",
        "Patch",
        "patch",
        "PATCHES",
        "Patches",
        "patches",
        "UPDATE",
        "Update",
        "update",
        "UPDATES",
        "Updates",
        "updates",
        "HOTFIX",
        "Hotfix",
        "hotfix",
        "HOTFIXES",
        "Hotfixes",
        "hotfixes",
        "FIX",
        "Fix",
        "fix",
        "FIXES",
        "Fixes",
        "fixes",
        "UESP INSTALL",
        "STEAM INSTALL",
        "GOG INSTALL",
        "MANUAL INSTALL",
        "Manual Install",
        "manual install",
        "MAIN FILES",
        "Main Files",
        "main files",
        "OPTIONAL FILES",
        "Optional Files",
        "optional files",
        "OLD FILES",
        "Old Files",
        "old files",
        "DOSBOX STAGING",
        "MOD FILES",
        "Mod Files",
        "mod files",
    };
  }

  QStringList wrapperRoots() const
  {
    return {
        "",
        "daggerfall",
        "Daggerfall",
        "DAGGERFALL",
        "dfdagger",
        "DFDAGGER",
        "TES2-Daggerfall",
        "The Elder Scrolls II - Daggerfall",
        "The Elder Scrolls II Daggerfall",
        "TES2 Daggerfall",
        "common",
        "Common",
        "game",
        "Game",
        "files",
        "Files",
        "install",
        "Install",
        "installation",
        "Installation",
        "mods",
        "Mods",
    };
  }

  QStringList discoverWrapperRoots(std::shared_ptr<const MOBase::IFileTree> fileTree) const
  {
    QStringList roots = wrapperRoots();
    if (!fileTree) {
      return roots;
    }
    discoverWrapperRootsRecursive(fileTree, "", 6, roots, 512);
    return roots;
  }

  void discoverWrapperRootsRecursive(std::shared_ptr<const MOBase::IFileTree> tree,
                                     const QString& currentPath, int remainingDepth,
                                     QStringList& roots, int maxRoots) const
  {
    if (!tree || remainingDepth <= 0 || roots.size() >= maxRoots) {
      return;
    }

    for (const auto& entry : *tree) {
      if (!entry || !entry->isDir()) {
        continue;
      }

      const QString childPath = joinPath(currentPath, entry->name());
      if (!roots.contains(childPath)) {
        roots.push_back(childPath);
        if (roots.size() >= maxRoots) {
          return;
        }
      }

      const auto childTree = tree->findDirectory(entry->name());
      if (childTree) {
        discoverWrapperRootsRecursive(childTree, childPath, remainingDepth - 1, roots, maxRoots);
        if (roots.size() >= maxRoots) {
          return;
        }
      }
    }
  }

  QStringList preferredVariantOrder() const
  {
    const auto* g = game();
    if (!g) {
      return {"GOG INSTALL", "STEAM INSTALL", "UESP INSTALL", "DOSBOX STAGING", "ARENA2"};
    }

    const QDir root = g->gameDirectory();
    const bool steamLayout = QDir(root.filePath("DOSBox-0.73")).exists() ||
                             QDir(root.filePath("DOSBox-0.74")).exists();
    const bool gogLayout = QDir(root.filePath("DOSBOX")).exists();

    if (steamLayout) {
      return {"STEAM INSTALL", "UESP INSTALL", "GOG INSTALL", "DOSBOX STAGING", "ARENA2"};
    }
    if (gogLayout) {
      return {"GOG INSTALL", "UESP INSTALL", "STEAM INSTALL", "DOSBOX STAGING", "ARENA2"};
    }
    return {"UESP INSTALL", "STEAM INSTALL", "GOG INSTALL", "DOSBOX STAGING", "ARENA2"};
  }

  QStringList candidatePathsInOrder(std::shared_ptr<const MOBase::IFileTree> fileTree) const
  {
    QStringList out;
    const auto variants = installVariantNames();
    const auto preferred = preferredVariantOrder();
    const auto roots = discoverWrapperRoots(fileTree);

    for (const auto& root : roots) {
      for (const auto& variant : preferred) {
        const QString path = joinPath(root, variant);
        if (!out.contains(path)) {
          out.push_back(path);
        }
      }
      for (const auto& variant : variants) {
        const QString path = joinPath(root, variant);
        if (!out.contains(path)) {
          out.push_back(path);
        }
      }
    }

    for (const auto& root : roots) {
      if (!root.isEmpty() && !out.contains(root)) {
        out.push_back(root);
      }
    }
    return out;
  }

  std::shared_ptr<const MOBase::IFileTree> selectInstallTreeConstByPath(
      std::shared_ptr<const MOBase::IFileTree> fileTree, const QStringList& paths) const
  {
    if (!fileTree) {
      return nullptr;
    }

    auto scoreInstallTree = [this](const std::shared_ptr<const MOBase::IFileTree>& tree) -> int {
      if (!tree) {
        return -1;
      }

      const bool hasBaseValid = (XngineModDataChecker::dataLooksValid(tree) == CheckReturn::VALID);
      const bool hasClassicPayload = containsKnownClassicPayload(tree);
      const bool hasExePayload = containsExecutablePayload(tree);
      const bool hasJsonPayload = containsJsonCatalog(tree);
      const bool hasBinaryPatchPayload =
          (containsXdeltaPayload(tree) || containsLegacyBinaryPatchPayload(tree));
      const bool hasQuestSourcePayload = containsQuestSourcePayload(tree);
      const bool hasNestedArchivePayload = containsNestedArchivePayload(tree);
      const bool hasInstallerPayload = containsInstallerPayload(tree);

      int score = 0;
      if (hasBaseValid) {
        score += 120;
      }
      if (hasClassicPayload) {
        score += 80;
      }
      if (hasExePayload) {
        score += 50;
      }
      if (hasJsonPayload) {
        score += 45;
      }
      if (hasBinaryPatchPayload) {
        score += 35;
      }
      if (hasQuestSourcePayload) {
        score += 20;
      }
      if (hasNestedArchivePayload) {
        score += 12;
      }

      const bool hasStrongPayload =
          (hasBaseValid || hasClassicPayload || hasExePayload || hasJsonPayload ||
           hasBinaryPatchPayload || hasQuestSourcePayload || hasNestedArchivePayload);
      if (hasInstallerPayload) {
        score += hasStrongPayload ? 4 : -20;
      }

      const QString lowerPath = tree->path().toLower();
      if (lowerPath.endsWith("/arena2") || lowerPath.endsWith("/arena 2") ||
          lowerPath.endsWith("/arena-2") || lowerPath.endsWith("/arena_2") ||
          lowerPath.endsWith("/df") || lowerPath.endsWith("/dagger")) {
        score += 35;
      } else if (lowerPath.endsWith("/data") || lowerPath.endsWith("/gamedata") ||
                 lowerPath.endsWith("/game data") || lowerPath.endsWith("/game_data")) {
        score += 18;
      }

      static const QStringList docPathTokens = {
          "readme", "docs", "documentation", "screenshots", "images", "video", "videos", "manual"};
      for (const auto& token : docPathTokens) {
        if (lowerPath.contains(token)) {
          score += hasStrongPayload ? -5 : -25;
          break;
        }
      }

      if (lowerPath.contains("optional")) {
        score -= hasStrongPayload ? 2 : 10;
      }
      if (lowerPath.contains("main files")) {
        score += 6;
      }
      if (lowerPath.contains("old files") || lowerPath.contains("legacy")) {
        score -= hasStrongPayload ? 3 : 12;
      }

      // Prefer shallower payload roots when scores are close.
      if (!lowerPath.isEmpty()) {
        const int depth = lowerPath.count('/');
        score -= (depth / 3);
      }

      // Strong hint for classic install layout: DF/DAGGER subtree.
      for (const auto& entry : *tree) {
        if (!entry || !entry->isDir()) {
          continue;
        }
        if (entry->name().compare("df", Qt::CaseInsensitive) != 0) {
          continue;
        }
        const auto dfTree = tree->findDirectory(entry->name());
        if (!dfTree) {
          continue;
        }
        for (const auto& dfEntry : *dfTree) {
          if (!dfEntry || !dfEntry->isDir()) {
            continue;
          }
          if (dfEntry->name().compare("dagger", Qt::CaseInsensitive) == 0) {
            score += 30;
            break;
          }
        }
        break;
      }

      // Bonus for canonical classic roots/files at this tree level.
      for (const auto& entry : *tree) {
        if (!entry) {
          continue;
        }
        if (entry->isDir()) {
          const QString dirName = entry->name().toLower();
          if (dirName == "arena2" || dirName == "arena 2" || dirName == "arena-2" ||
              dirName == "arena_2" || dirName == "df" || dirName == "dagger" ||
              dirName == "tes2" || dirName == "tes2daggerfall" || dirName == "tes2 daggerfall") {
            score += 25;
          }
        } else if (entry->isFile()) {
          const QString fileName = entry->name().toUpper();
          if (fileName == "FALL.EXE" || fileName == "DAGGER.EXE" || fileName == "ARCH3D.BSA" ||
              fileName == "BLOCKS.BSA" || fileName == "MAPS.BSA" || fileName == "MONSTER.BSA" ||
              fileName == "FACES.CIF" || fileName == "FRAM00I0.IMG" ||
              fileName == "TALK00I0.IMG") {
            score += 25;
          }
        }
      }

      return score;
    };

    std::shared_ptr<const MOBase::IFileTree> bestTree = nullptr;
    int bestScore = -1;
    int bestPathLen = (std::numeric_limits<int>::max)();

    for (const auto& path : paths) {
      auto tree = fileTree->findDirectory(path);
      if (!tree) {
        continue;
      }

      const int score = scoreInstallTree(tree);
      const int pathLen = tree->path().size();
      if (score > bestScore || (score == bestScore && pathLen < bestPathLen)) {
        bestScore = score;
        bestPathLen = pathLen;
        bestTree = tree;
      }
    }

    if (bestTree != nullptr && bestScore > 0) {
      const QString normalized = normalizeSelectedInstallRootPath(bestTree->path());
      if (normalized != bestTree->path()) {
        auto normalizedTree = fileTree->findDirectory(normalized);
        if (normalizedTree) {
          return normalizedTree;
        }
      }
      return bestTree;
    }

    // If no candidate path scored, keep root if it still looks like payload.
    const int rootScore = scoreInstallTree(fileTree);
    if (rootScore > 0) {
      return fileTree;
    }

    for (const auto& path : paths) {
      auto tree = fileTree->findDirectory(path);
      if (tree) {
        return tree;
      }
    }
    return nullptr;
  }

  std::shared_ptr<const MOBase::IFileTree>
  selectInstallTreeConst(std::shared_ptr<const MOBase::IFileTree> fileTree) const
  {
    return selectInstallTreeConstByPath(fileTree, candidatePathsInOrder(fileTree));
  }

  std::shared_ptr<MOBase::IFileTree>
  selectInstallTree(std::shared_ptr<MOBase::IFileTree> fileTree) const
  {
    if (!fileTree) {
      return nullptr;
    }

    const auto selectedConst =
        selectInstallTreeConstByPath(fileTree, candidatePathsInOrder(fileTree));
    if (!selectedConst) {
      return nullptr;
    }

    const QString selectedPath = normalizeSelectedInstallRootPath(selectedConst->path());
    if (selectedPath.isEmpty()) {
      return fileTree;
    }
    auto selected = fileTree->findDirectory(selectedPath);
    return selected ? selected : fileTree;
  }
};

#endif  // DAGGERFALLS_MODDATACHECKER_H
