#ifndef ARENADATACHECKER_H
#define ARENADATACHECKER_H

#include <xnginemoddatachecker.h>
#include <gamexngine.h>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QRegularExpression>
#include <memory>

class GameArena;

class ArenaModDataChecker : public XngineModDataChecker
{
public:
  ArenaModDataChecker(GameArena const* game) : XngineModDataChecker(game) {}

  virtual CheckReturn
  dataLooksValid(std::shared_ptr<const MOBase::IFileTree> fileTree) const override
  {
    if (!fileTree) {
      return CheckReturn::INVALID;
    }

    // Standard Arena/XnGine layout at archive root.
    const auto base = XngineModDataChecker::dataLooksValid(fileTree);
    if (base == CheckReturn::VALID) {
      return CheckReturn::VALID;
    }
    if (containsArenaLooseFiles(fileTree)) {
      return CheckReturn::VALID;
    }

    // Common Arena Nexus package wrappers:
    //   arena/STEAM INSTALL
    //   arena/GOG INSTALL
    //   arena/UESP INSTALL
    //   arena/DOSBOX STAGING
    // and variants directly at archive root.
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
        "arena",
        "arena2",
        "data",
        "dosbox",
        "dosbox-0.74",
        "dosbox modifications",
        "fonts",
        "maps",
        "sound",
        "speech",
        "textures",
        "video",
    };
    return result;
  }

  virtual const FileNameSet& possibleFileExtensions() const override
  {
    static FileNameSet result{
        "bat", "bsa", "cfg", "cif", "col", "conf", "dat", "exe", "img", "inf",
        "ini", "lst", "map", "mif", "pal", "raw", "txt", "voc", "xmi",
    };
    return result;
  }

private:
  bool containsArenaLooseFiles(std::shared_ptr<const MOBase::IFileTree> fileTree) const
  {
    if (!fileTree) {
      return false;
    }

    static const QRegularExpression kSaveSlotPattern(
        R"(^(SAVEENGN|SAVEGAME|SPELLS|LOG)\.\d{1,2}$)",
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression kCityDataPattern(
        R"(^CITYDATA\.\d{1,3}$)", QRegularExpression::CaseInsensitiveOption);

    for (const auto& entry : *fileTree) {
      if (!entry || entry->isDir()) {
        continue;
      }

      const QString name = entry->name();
      const QString upper = name.toUpper();
      if (upper == "GLOBAL.BSA" || upper == "NAMES.DAT" || upper == "ARENA.BAT" ||
          upper == "ARENA.CONF" || upper == "MAPPER.TXT" ||
          upper == "DOSBOX_ARENA.CONF" || upper == "DOSBOX_ARENA_SINGLE.CONF") {
        return true;
      }
      if (kSaveSlotPattern.match(name).hasMatch() || kCityDataPattern.match(name).hasMatch()) {
        return true;
      }
    }
    return false;
  }

  QStringList installVariantNames() const
  {
    return {
        "UESP INSTALL",
        "STEAM INSTALL",
        "GOG INSTALL",
        "DOSBOX STAGING",
        "DOSBox Modifications",
        "uesp install",
        "steam install",
        "gog install",
        "dosbox staging",
        "dosbox modifications",
    };
  }

  QStringList wrapperRoots() const
  {
    return {
        "",
        "arena",
        "Arena",
        "ARENA",
        "common",
        "Common",
        "COMMON",
        "The Elder Scrolls Arena",
        "TES Arena",
        "GAME",
        "game",
        "mods",
        "Mods",
        "MODS",
    };
  }

  QStringList discoverWrapperRoots(std::shared_ptr<const MOBase::IFileTree> fileTree) const
  {
    QStringList roots = wrapperRoots();
    if (!fileTree) {
      return roots;
    }

    // Auto-discover one and two-level wrapper paths from archive root.
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

  QString joinPath(const QString& lhs, const QString& rhs) const
  {
    if (lhs.isEmpty()) {
      return rhs;
    }
    return QString("%1/%2").arg(lhs, rhs);
  }

  QStringList candidatePathsInOrder(std::shared_ptr<const MOBase::IFileTree> fileTree) const
  {
    QStringList out;
    const auto variants = installVariantNames();
    const auto roots = discoverWrapperRoots(fileTree);
    const auto preferred = preferredVariantOrder();

    for (const auto& root : roots) {
      for (const auto& variant : preferred) {
        out.push_back(joinPath(root, variant));
      }
      for (const auto& variant : variants) {
        if (!out.contains(joinPath(root, variant))) {
          out.push_back(joinPath(root, variant));
        }
      }
    }

    // Common "content root" folders used in archives.
    for (const auto& root : roots) {
      for (const auto& child : {"ARENA", "Arena", "arena", "DATA", "data"}) {
        const QString candidate = joinPath(root, child);
        if (!out.contains(candidate)) {
          out.push_back(candidate);
        }
      }
    }

    // Finally try wrapper roots themselves.
    for (const auto& root : roots) {
      if (!root.isEmpty() && !out.contains(root)) {
        out.push_back(root);
      }
    }
    return out;
  }

  std::shared_ptr<const MOBase::IFileTree>
  selectInstallTreeConstByPath(std::shared_ptr<const MOBase::IFileTree> fileTree,
                               const QStringList& paths) const
  {
    if (!fileTree) {
      return nullptr;
    }

    auto findDir = [&](const QString& path) -> std::shared_ptr<const MOBase::IFileTree> {
      auto tree = fileTree->findDirectory(path);
      if (tree) {
        return tree;
      }
      return nullptr;
    };

    for (const auto& path : paths) {
      if (auto t = findDir(path)) {
        if (XngineModDataChecker::dataLooksValid(t) == CheckReturn::VALID ||
            containsArenaLooseFiles(t)) {
          return t;
        }
      }
    }

    for (const auto& path : paths) {
      if (auto t = findDir(path)) {
        return t;
      }
    }

    return nullptr;
  }

  QStringList preferredVariantOrder() const
  {
    const auto* g = game();
    if (!g) {
      return {"UESP INSTALL", "STEAM INSTALL", "GOG INSTALL", "DOSBOX STAGING"};
    }

    const QDir root = g->gameDirectory();
    const bool steamLayout = QDir(root.filePath("DOSBox-0.73")).exists() ||
                             QDir(root.filePath("DOSBox-0.74")).exists();
    const bool gogLayout = QDir(root.filePath("DOSBOX")).exists();

    if (steamLayout) {
      return {"STEAM INSTALL", "UESP INSTALL", "GOG INSTALL", "DOSBOX STAGING"};
    }
    if (gogLayout) {
      return {"GOG INSTALL", "UESP INSTALL", "STEAM INSTALL", "DOSBOX STAGING"};
    }

    return {"UESP INSTALL", "STEAM INSTALL", "GOG INSTALL", "DOSBOX STAGING"};
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

    auto findDir = [&](const QString& path) -> std::shared_ptr<MOBase::IFileTree> {
      auto tree = fileTree->findDirectory(path);
      if (tree) {
        return tree;
      }
      return nullptr;
    };

    return findDir(selectedPath);
  }
};

#endif  // ARENADATACHECKER_H
