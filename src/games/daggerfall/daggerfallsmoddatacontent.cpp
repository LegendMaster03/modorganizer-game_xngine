#include "daggerfallsmoddatacontent.h"

#include <algorithm>

namespace
{
bool containsPatchPayload(const std::shared_ptr<const MOBase::IFileTree>& fileTree)
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
      const QString suffix = entry->suffix();

      if (fileName.compare("fall_exe_patches.json", Qt::CaseInsensitive) == 0 ||
          fileName.compare("exe_patches.json", Qt::CaseInsensitive) == 0 ||
          fileName.compare("FALL.EXE", Qt::CaseInsensitive) == 0 ||
          fileName.compare("DAGGER.EXE", Qt::CaseInsensitive) == 0 ||
          suffix.compare("xdelta", Qt::CaseInsensitive) == 0) {
        return true;
      }
    } else if (entry->isDir()) {
      const auto subtree = fileTree->findDirectory(entry->name());
      if (subtree && containsPatchPayload(subtree)) {
        return true;
      }
    }
  }

  return false;
}
}  // namespace

std::vector<DaggerfallsModDataContent::Content> DaggerfallsModDataContent::getAllContents() const
{
  auto contents = XngineModDataContent::getAllContents();
  if (m_Enabled[CONTENT_PATCH_INSTRUCTIONS]) {
    contents.insert(contents.begin(),
                    {CONTENT_PATCH_INSTRUCTIONS, QT_TR_NOOP("Patch Instructions"),
                     ":/MO/gui/content/script"});
  }
  return contents;
}

std::vector<int> DaggerfallsModDataContent::getContentsFor(
    std::shared_ptr<const MOBase::IFileTree> fileTree) const
{
  auto contents = XngineModDataContent::getContentsFor(fileTree);
  if (!fileTree) {
    return contents;
  }

  const bool isPatch = containsPatchPayload(fileTree);
  if (isPatch && m_Enabled[CONTENT_PATCH_INSTRUCTIONS] &&
      std::find(contents.begin(), contents.end(), CONTENT_PATCH_INSTRUCTIONS) == contents.end()) {
    contents.push_back(CONTENT_PATCH_INSTRUCTIONS);
  }

  return contents;
}
