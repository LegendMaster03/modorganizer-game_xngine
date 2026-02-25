#ifndef DAGGERFALLS_MODDATACHECKER_H
#define DAGGERFALLS_MODDATACHECKER_H

#include <xnginemoddatachecker.h>

/**
 * Daggerfall-specific mod data checker.
 * Daggerfall mod detection based on Format 0 (file replacement) only.
 */
class DaggerfallsModDataChecker : public XngineModDataChecker
{
public:
  using XngineModDataChecker::XngineModDataChecker;

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
        "bsa",     // Archive files
        "zip",     // Compressed archives
        "7z",      // Compressed archives
        "rar"      // Compressed archives
    };
    return result;
  }
};

#endif  // DAGGERFALLS_MODDATACHECKER_H
