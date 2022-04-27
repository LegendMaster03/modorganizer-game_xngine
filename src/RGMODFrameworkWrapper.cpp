#include "RGMODFrameworkWrapper.h"

#include <imodlist.h>
#include <iplugingame.h>
#include <ipluginlist.h>

#include "RtxDatabase.cpp"
#include "MenuFile.cpp"

 // Standard mod changes file names
 public static final String RTX_CHANGES = "RTX Changes.txt";
 public static final String MENU_CHANGES = "Menu Changes.txt";
 public static final String MAP_CHANGES = "Map Changes.txt";

using namespace MOBase;

   /**
     * Collects all of the changes from enabled mods in their load order, and patches the game files.
     */
    private static void applyChanges() {
        // Check if the user really wants to apply changes
        int shouldApply = JOptionPane.showConfirmDialog(window, "Are you sure you want to apply changes from the enabled mods?",
                "Confirm apply changes", JOptionPane.YES_NO_OPTION);
        if (shouldApply != JOptionPane.YES_OPTION) {
            return;
        }

        RtxDatabase modifiedDatabase = null;
        MenuFile modifiedMenu = null;
        MapChanges mapChanges = new MapChanges();
        try {
            // Go through all enabled mods and collect the sum of their changes, based on load order
            for (Mod mod : modTable.getModList()) {
                if (mod.isEnabled()) {
                    Path path = getModPath(mod);

                    // Get RTX changes
                    File rtxChangesFile = path.resolve(RTX_CHANGES).toFile();
                    if (rtxChangesFile.exists()) {
                        if (modifiedDatabase == null) {
                            modifiedDatabase = new RtxDatabase(rtxDatabase);
                        }
                        modifiedDatabase.applyChanges(rtxChangesFile);
                    }

                    // Get menu changes
                    File menuChangesFile = path.resolve(MENU_CHANGES).toFile();
                    if (menuChangesFile.exists()) {
                        modifiedMenu = new MenuFile(menuFile);
                        modifiedMenu.readChanges(menuChangesFile);
                    }

                    // Get map changes
                    File mapChangesFile = path.resolve(MAP_CHANGES).toFile();
                    if (mapChangesFile.exists()) {
                        mapChanges.readChanges(mapChangesFile);
                    }
                }
            }

            // Replace ENGLISH.RTX, either with a new version or the original copy
            if (modifiedDatabase == null) {
                Files.copy(dataPath.resolve("ENGLISH.RTX"), gamePath.resolve("ENGLISH.RTX"), StandardCopyOption.REPLACE_EXISTING);
            } else {
                modifiedDatabase.writeFile(gamePath.resolve("ENGLISH.RTX").toFile());
            }

            // Replace MENU.INI, again with a new version or the original copy
            if (modifiedMenu == null) {
                Files.copy(dataPath.resolve("MENU.INI"), gamePath.resolve("MENU.INI"), StandardCopyOption.REPLACE_EXISTING);
            } else {
                modifiedMenu.writeFile(gamePath.resolve("MENU.INI").toFile());
            }

            // Replace map files that have changes and copy the rest
            for (MapFile mapFile : mapDatabase.getMapFiles()) {
                if (mapChanges.hasModifiedMap(mapFile.getName())) {
                    if (mapFile.isEmpty()) {
                        mapFile.readMap(dataPath.resolve(mapFile.getFullName()).toFile());
                    }
                    String modifiedScript = mapFile.getModifiedScript(mapChanges);
                    mapFile.writeMap(gamePath.resolve(mapFile.getFullName()).toFile(), modifiedScript);
                } else {
                    Files.copy(dataPath.resolve(mapFile.getFullName()), gamePath.resolve(mapFile.getFullName()), StandardCopyOption.REPLACE_EXISTING);
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        JOptionPane.showMessageDialog(window, "Changes were applied successfully.", "Redguard Mod Manager",
                JOptionPane.INFORMATION_MESSAGE);
    }

    /**
     * Get the path to a mod's directory.
     *
     * @param mod The mod whose path is returned
     * @return The path to the mod
     */
    public static Path getModPath(Mod mod) {
        return modsPath.resolve(mod.getName());
    }
