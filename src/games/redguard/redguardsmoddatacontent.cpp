#include "redguardsmoddatacontent.h"

#include <algorithm>

std::vector<RedguardsModDataContent::Content> RedguardsModDataContent::getAllContents() const
{
	auto contents = XngineModDataContent::getAllContents();
	if (m_Enabled[CONTENT_PATCH_INSTRUCTIONS]) {
		contents.insert(contents.begin(),
										{CONTENT_PATCH_INSTRUCTIONS, QT_TR_NOOP("Patch Instructions"),
										 ":/MO/gui/content/script"});
	}
	return contents;
}

std::vector<int>
RedguardsModDataContent::getContentsFor(std::shared_ptr<const MOBase::IFileTree> fileTree) const
{
	auto contents = XngineModDataContent::getContentsFor(fileTree);
	if (!fileTree) {
		return contents;
	}

	const bool isPatch =
			fileTree->find("About.txt", MOBase::IFileTree::FILE) ||
			fileTree->find("INI Changes.txt", MOBase::IFileTree::FILE) ||
			fileTree->find("Map Changes.txt", MOBase::IFileTree::FILE) ||
			fileTree->find("RTX Changes.txt", MOBase::IFileTree::FILE);

	if (isPatch && m_Enabled[CONTENT_PATCH_INSTRUCTIONS] &&
			std::find(contents.begin(), contents.end(), CONTENT_PATCH_INSTRUCTIONS) == contents.end()) {
		contents.push_back(CONTENT_PATCH_INSTRUCTIONS);
	}

	return contents;
}
