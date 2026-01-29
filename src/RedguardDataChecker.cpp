#include "RedguardDataChecker.h"
#include <ifiletree.h>
#include <QDebug>
#include <cstdio>
#include <ctime>

using namespace MOBase;

ModDataChecker::CheckReturn RedguardDataChecker::dataLooksValid(std::shared_ptr<const IFileTree> fileTree) const
{
  qDebug() << "================== RedguardDataChecker::dataLooksValid() CALLED ==================";
  qDebug() << "fileTree is" << (fileTree ? "valid" : "NULL");
  
  // Temporary: just log and return VALID to test if this is being called
  return CheckReturn::VALID;
}

std::shared_ptr<IFileTree> RedguardDataChecker::fix(std::shared_ptr<IFileTree> fileTree) const
{
  qDebug() << "RedguardDataChecker::fix() called";
  return fileTree;  // No changes for now
}

bool RedguardDataChecker::isFormat1Mod(std::shared_ptr<const IFileTree> tree) const
{
  try {
    if (!tree) {
      return false;
    }
    
    // Format 1 must have About.txt
    auto aboutFile = tree->find("About.txt", FileTreeEntry::FILE);
    if (!aboutFile) {
      return false;
    }
    
    // Should also have at least one change file (but About.txt alone is acceptable)
    return true;
  } catch (const std::exception& e) {
    qWarning() << "isFormat1Mod exception:" << e.what();
    return false;
  } catch (...) {
    qWarning() << "isFormat1Mod unknown exception";
    return false;
  }
}

bool RedguardDataChecker::isFormat2Mod(std::shared_ptr<const IFileTree> tree) const
{
  try {
    if (!tree) {
      return false;
    }
    
    // Format 2 should NOT have About.txt
    if (tree->find("About.txt", FileTreeEntry::FILE)) {
      return false;
    }
    
    // Check for game file indicators
    QStringList gameFiles = {"ENGLISH.RTX", "COMBAT.INI", "KEYS.INI", "MENU.INI", 
                             "REGISTRY.INI", "SYSTEM.INI", "WORLD.INI"};
    
    for (const QString& filename : gameFiles) {
      try {
        if (tree->find(filename, FileTreeEntry::FILE)) {
          return true;
        }
      } catch (...) {
        // Skip files that can't be checked
        continue;
      }
    }
    
    // Check for game folders
    QStringList gameFolders = {"fonts", "system", "Audio", "Textures"};
    for (const QString& folder : gameFolders) {
      try {
        if (tree->find(folder, FileTreeEntry::DIRECTORY)) {
          return true;
        }
      } catch (...) {
        // Skip folders that can't be checked
        continue;
      }
    }
    
    return false;
  } catch (const std::exception& e) {
    qWarning() << "isFormat2Mod exception:" << e.what();
    return false;
  } catch (...) {
    qWarning() << "isFormat2Mod unknown exception";
    return false;
  }
}

std::shared_ptr<const IFileTree> RedguardDataChecker::findModRoot(std::shared_ptr<const IFileTree> tree) const
{
  try {
    if (!tree) return nullptr;
    
    // Check if current level is valid
    if (isFormat1Mod(tree) || isFormat2Mod(tree)) {
      return tree;
    }
    
    // Check for single nested directory
    std::vector<std::shared_ptr<const IFileTree>> dirs;
    for (auto entry : *tree) {
      if (entry && entry->isDir()) {
        auto dir = entry->astree();
        if (dir) {
          dirs.push_back(dir);
        }
      }
    }
    
    // If exactly one directory, recursively check it
    if (dirs.size() == 1) {
      return findModRoot(dirs[0]);
    }
    
    return tree;
  } catch (const std::exception& e) {
    qWarning() << "findModRoot exception:" << e.what();
    return tree;
  } catch (...) {
    qWarning() << "findModRoot unknown exception";
    return tree;
  }
}
