#ifndef BATTLESPIREDATACHECKER_H
#define BATTLESPIREDATACHECKER_H

#include <xnginemoddatachecker.h>
#include <gamexngine.h>
#include "gamebattlespire.h"

#include <QDir>
#include <QStringList>
#include <QString>
#include <memory>
#include <QDebug>

class BattlespiresModDataChecker : public XngineModDataChecker
{
public:
  BattlespiresModDataChecker(GameBattlespire const* game) : XngineModDataChecker(game) {}

  virtual CheckReturn
  dataLooksValid(std::shared_ptr<const MOBase::IFileTree> fileTree) const override
  {
    if (!fileTree) {
      return CheckReturn::INVALID;
    }

    if (containsExecutablePayload(fileTree) && !isExeInstallAllowed()) {
      qWarning().noquote()
          << "[GameBattlespire] Rejecting package with executable patch payload (GAME.EXE/.xdelta/.xdelta3/.vcdiff) because patching is disabled."
          << "Enable 'xdelta_enabled' in plugin settings and set 'xdelta_exe_path' (or rely on auto-detection) to allow these mods.";
      return CheckReturn::INVALID;
    }

    const auto base = XngineModDataChecker::dataLooksValid(fileTree);
    if (base == CheckReturn::VALID) {
      return CheckReturn::VALID;
    }
    if (containsExecutablePayload(fileTree)) {
      return CheckReturn::VALID;
    }
    if (containsBattlespireLooseFiles(fileTree)) {
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
        "gamedata", "data",  "textures", "sound",  "audio", "maps",
        "fonts",    "video", "dosbox",   "dosbox-0.73", "dosbox-0.74", "mss"};
    return result;
  }

  virtual const FileNameSet& possibleFileExtensions() const override
  {
    static FileNameSet result{
        "bat", "bsa", "bs6", "bsi", "cfg", "conf", "dat", "exe", "flc", "ini",
        "pal", "snd", "txt", "wav", "rtx", "dig", "xdelta", "xdelta3", "vcdiff"};
    return result;
  }

private:
  bool isExeInstallAllowed() const
  {
    const auto* g = dynamic_cast<const GameBattlespire*>(game());
    return (g != nullptr) ? g->allowExeModInstall() : false;
  }

  bool containsExecutablePayload(
      std::shared_ptr<const MOBase::IFileTree> fileTree) const
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
        if (name.compare("GAME.EXE", Qt::CaseInsensitive) == 0 ||
            suffix == "xdelta" || suffix == "xdelta3" || suffix == "vcdiff") {
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

  bool containsBattlespireLooseFiles(
      std::shared_ptr<const MOBase::IFileTree> fileTree) const
  {
    if (!fileTree) {
      return false;
    }

    static const QStringList kKnownRootFiles = {
        "GAME.EXE", "SPIRE.BAT", "SPIRE.CFG", "PATCH.TXT", "README.TXT", "3D.BSA",
        "BS6.BSA",  "BSI.BSA",   "TXT.BSA",   "FLC.BSA",   "SPIRE.SND", "WAVES.BSA",
        "DIG.INI",  "SB16.DIG"};

    for (const auto& entry : *fileTree) {
      if (!entry || entry->isDir()) {
        continue;
      }

      const QString upper = entry->name().toUpper();
      if (kKnownRootFiles.contains(upper)) {
        return true;
      }

      const QString suffix = entry->suffix().toLower();
      if (suffix == "bsa" || suffix == "bsi" || suffix == "bs6" || suffix == "snd" ||
          suffix == "flc" || suffix == "xdelta" || suffix == "xdelta3" ||
          suffix == "vcdiff") {
        return true;
      }
    }
    return false;
  }

  QString joinPath(const QString& lhs, const QString& rhs) const
  {
    if (lhs.isEmpty()) {
      return rhs;
    }
    return QString("%1/%2").arg(lhs, rhs);
  }

  QStringList installVariantNames() const
  {
    return {
        "GAMEDATA",
        "gamedata",
        "DATA",
        "data",
        "UESP INSTALL",
        "STEAM INSTALL",
        "GOG INSTALL",
        "DOSBOX STAGING",
        "MAIN FILES",
        "MOD FILES",
        "OPTIONAL FILES",
    };
  }

  QStringList wrapperRoots() const
  {
    return {
        "",
        "battlespire",
        "Battlespire",
        "BATTLESPIRE",
        "An Elder Scrolls Legend Battlespire",
        "common",
        "game",
        "files",
        "install",
        "installation",
        "mods",
    };
  }

  QStringList discoverWrapperRoots(std::shared_ptr<const MOBase::IFileTree> fileTree) const
  {
    QStringList roots = wrapperRoots();
    if (!fileTree) {
      return roots;
    }

    for (const auto& entry : *fileTree) {
      if (!entry || !entry->isDir()) {
        continue;
      }
      const QString level1 = entry->name();
      if (!roots.contains(level1)) {
        roots.push_back(level1);
      }

      const auto dir1 = fileTree->findDirectory(level1);
      if (!dir1) {
        continue;
      }
      for (const auto& child : *dir1) {
        if (!child || !child->isDir()) {
          continue;
        }
        const QString level2 = joinPath(level1, child->name());
        if (!roots.contains(level2)) {
          roots.push_back(level2);
        }
      }
    }
    return roots;
  }

  QStringList preferredVariantOrder() const
  {
    const auto* g = game();
    if (!g) {
      return {"STEAM INSTALL", "GOG INSTALL", "UESP INSTALL", "DOSBOX STAGING", "GAMEDATA"};
    }

    const QDir root = g->gameDirectory();
    const bool steamLayout = QDir(root.filePath("DOSBox-0.73")).exists() ||
                             QDir(root.filePath("DOSBox-0.74")).exists();
    const bool gogLayout = QDir(root.filePath("DOSBOX")).exists();

    if (steamLayout) {
      return {"STEAM INSTALL", "UESP INSTALL", "GOG INSTALL", "DOSBOX STAGING", "GAMEDATA"};
    }
    if (gogLayout) {
      return {"GOG INSTALL", "UESP INSTALL", "STEAM INSTALL", "DOSBOX STAGING", "GAMEDATA"};
    }
    return {"UESP INSTALL", "STEAM INSTALL", "GOG INSTALL", "DOSBOX STAGING", "GAMEDATA"};
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

    for (const auto& path : paths) {
      auto tree = fileTree->findDirectory(path);
      if (!tree) {
        continue;
      }
      if (XngineModDataChecker::dataLooksValid(tree) == CheckReturn::VALID ||
          containsBattlespireLooseFiles(tree)) {
        return tree;
      }
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

    const QString selectedPath = selectedConst->path();
    if (selectedPath.isEmpty()) {
      return fileTree;
    }
    auto selected = fileTree->findDirectory(selectedPath);
    return selected ? selected : fileTree;
  }
};

#endif  // BATTLESPIREDATACHECKER_H
