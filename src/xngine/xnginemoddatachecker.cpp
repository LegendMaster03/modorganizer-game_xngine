#include <ifiletree.h>

#include "xnginemoddatachecker.h"

#include <QDebug>

/**
 * @return the list of possible folder names in data.
 */
auto XngineModDataChecker::possibleFolderNames() const -> const FileNameSet&
{
  static FileNameSet result{"audio",
                            "sound",
                            "textures",
                            "fonts",
                            "system",
                            "maps",
                            "arena2",
                            "data",
                            "dosbox",
                            "video"};
  return result;
}

/**
 * @return the extensions of possible files in data.
 */
auto XngineModDataChecker::possibleFileExtensions() const -> const FileNameSet&
{
  static FileNameSet result{"ini", "rtx", "rgm", "wld", "bsa", "dat", "mif", "img", "cfg"};
  return result;
}

XngineModDataChecker::XngineModDataChecker(const GameXngine* game) : m_Game(game)
{
  OutputDebugStringA("[XngineModDataChecker] Constructor ENTRY\n");
  if (!game) {
    OutputDebugStringA("[XngineModDataChecker] WARNING: game pointer is NULL\n");
  }
  OutputDebugStringA("[XngineModDataChecker] Constructor EXIT\n");
}

XngineModDataChecker::CheckReturn XngineModDataChecker::dataLooksValid(
    std::shared_ptr<const MOBase::IFileTree> fileTree) const
{
  qInfo().noquote() << "[XngineModDataChecker] dataLooksValid() ENTRY";
  if (!fileTree) {
    qDebug() << "[XnGine] ModDataChecker: fileTree is null";
    return CheckReturn::INVALID;
  }

  auto& folders  = possibleFolderNames();
  auto& suffixes = possibleFileExtensions();
  for (auto entry : *fileTree) {
    if (entry->isDir()) {
      if (folders.count(entry->name()) > 0) {
        qDebug() << "[XnGine] ModDataChecker: matched folder" << entry->name();
        return CheckReturn::VALID;
      }
    } else {
      if (suffixes.count(entry->suffix()) > 0) {
        qDebug() << "[XnGine] ModDataChecker: matched extension" << entry->suffix();
        return CheckReturn::VALID;
      }
    }
  }
  qDebug() << "[XnGine] ModDataChecker: no valid indicators found";
  return CheckReturn::INVALID;
}
