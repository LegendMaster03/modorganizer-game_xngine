#include "battlespiremodatacontent.h"

#include <ifiletree.h>

#include <algorithm>

std::vector<int> BattlespireModDataContent::getContentsFor(
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

    // Archive and executable replacement packs are core Battlespire mod payloads.
    if (name == "game.exe" || suffix == "bsa" || suffix == "bsi" || suffix == "bs6" ||
        suffix == "snd" || suffix == "flc" || suffix == "xdelta" ||
        suffix == "xdelta3" || suffix == "vcdiff") {
      addContent(CONTENT_FILE_OVERRIDES);
      continue;
    }

    // DOSBox + runtime config/launcher wrappers.
    if (suffix == "conf" || suffix == "cfg" || suffix == "ini" || suffix == "bat" ||
        name.startsWith("dosbox") || name == "spire.cfg") {
      addContent(CONTENT_CONFIG);
      continue;
    }

    if (suffix == "txt" || suffix == "rtx") {
      addContent(CONTENT_TEXT);
      continue;
    }
  }

  return contents;
}
