#ifndef REDGUARDS_MODDATACHECKER_H
#define REDGUARDS_MODDATACHECKER_H

#include <xnginemoddatachecker.h>

/**
 * Redguard-specific mod data checker.
 * Redguard mod detection follows these indicators:
 * - Format 1 (Patch-based): About.txt + at least one *Changes.txt file (INI/Map/RTX changes)
 * - Format 2 (File replacement): .RGM (Redguard module), .RTX (texture), .SAV (save) files
 *   or mod contains Redguard game data folders (data, maps, textures, etc.)
 */
class RedguardsModDataChecker : public XngineModDataChecker
{
public:
  using XngineModDataChecker::XngineModDataChecker;

protected:
  virtual const FileNameSet& possibleFolderNames() const override
  {
    static FileNameSet result{"data",
                              "maps",
                              "textures",
                              "textures/sky",
                              "textures/ui",
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
        "rgm",     // Redguard module files
        "rtx",     // Redguard texture format
        "sav",     // Savegame files
        "dat",     // Data files
        "mif",     // Map interchange format
        "img",     // Image resources
        "pal",     // Palette files
        "fnt",     // Font files
        "ini",     // Configuration files (for About.txt mods)
        "txt",     // Text files (including Changes.txt)
        "cfg",     // Config files
        "bsa",     // BSA archive (less common for Redguard)
        "zip",     // Compressed archives
        "7z",      // Compressed archives
        "rar"      // Compressed archives
    };
    return result;
  }
};

#endif  // REDGUARDS_MODDATACHECKER_H
