#include "arenamodatacontent.h"

#include <ifiletree.h>

#include <algorithm>

std::vector<int> ArenaModDataContent::getContentsFor(
    std::shared_ptr<const MOBase::IFileTree> fileTree) const
{
  auto contents = XngineModDataContent::getContentsFor(fileTree);
  if (!fileTree) {
    return contents;
  }

  auto addContent = [&](int contentId) {
    if (std::find(contents.begin(), contents.end(), contentId) == contents.end()) {
      contents.push_back(contentId);
    }
  };

  for (const auto& entry : *fileTree) {
    if (!entry || !entry->isFile()) {
      continue;
    }

    const QString name = entry->name().toLower();
    const QString suffix = entry->suffix().toLower();

    // Arena control/mapping mods often only ship mapper/config files.
    if (name == "mapper.txt" || suffix == "map") {
      addContent(CONTENT_SCRIPTS);
      continue;
    }

    if (name.startsWith("dosbox") && suffix == "conf") {
      addContent(CONTENT_CONFIG);
      continue;
    }

    if (name == "arena.conf" || name == "esarena.conf" ||
        name == "arena_single.conf" || name == "esarena_single.conf") {
      addContent(CONTENT_CONFIG);
      continue;
    }

    // Batch launch helpers and plain text instructions commonly accompany
    // Arena remap/config packs and should still classify as config-oriented.
    if (suffix == "bat") {
      addContent(CONTENT_CONFIG);
      continue;
    }
  }

  return contents;
}
