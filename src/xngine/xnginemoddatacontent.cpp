#include "xnginemoddatacontent.h"

#include <igamefeatures.h>

#include <QDebug>

XngineModDataContent::XngineModDataContent(
  MOBase::IGameFeatures const* gameFeatures)
  : m_GameFeatures(gameFeatures), m_Enabled(CONTENT_TEXT + 1, true)
{
  OutputDebugStringA("[XngineModDataContent] Constructor ENTRY\n");
  if (!gameFeatures) {
    OutputDebugStringA("[XngineModDataContent] WARNING: gameFeatures pointer is NULL\n");
  }
  OutputDebugStringA("[XngineModDataContent] Constructor EXIT\n");
}

std::vector<XngineModDataContent::Content>
XngineModDataContent::getAllContents() const
{
  qInfo().noquote() << "[XngineModDataContent] getAllContents() ENTRY";
  static std::vector<Content> XNGINE_CONTENTS{
    {CONTENT_FILE_OVERRIDES, QT_TR_NOOP("File Overrides"),
     ":/MO/gui/content/mesh"},
    {CONTENT_AUDIO, QT_TR_NOOP("Audio"), ":/MO/gui/content/sound"},
    {CONTENT_TEXTURES, QT_TR_NOOP("Textures"), ":/MO/gui/content/texture"},
    {CONTENT_CONFIG, QT_TR_NOOP("Config / INI"), ":/MO/gui/content/inifile"},
    {CONTENT_SCRIPTS, QT_TR_NOOP("Scripts / Maps"), ":/MO/gui/content/script"},
    {CONTENT_TEXT, QT_TR_NOOP("Text / RTX"), ":/MO/gui/content/menu"}};

  // Copy the list of enabled contents:
  std::vector<Content> contents;
  std::copy_if(std::begin(XNGINE_CONTENTS), std::end(XNGINE_CONTENTS),
               std::back_inserter(contents), [this](auto e) {
                 return m_Enabled[e.id()];
               });
  return contents;
}

std::vector<int> XngineModDataContent::getContentsFor(
    std::shared_ptr<const MOBase::IFileTree> fileTree) const
{
  qInfo().noquote() << "[XngineModDataContent] getContentsFor() ENTRY";
  std::vector<int> contents;

  for (auto e : *fileTree) {
    if (e->isFile()) {
      auto suffix = e->suffix().toLower();
      if (m_Enabled[CONTENT_CONFIG] && suffix == "ini" &&
          e->compare("meta.ini") != 0) {
        contents.push_back(CONTENT_CONFIG);
      } else if (m_Enabled[CONTENT_TEXT] && suffix == "rtx") {
        contents.push_back(CONTENT_TEXT);
      } else if (m_Enabled[CONTENT_SCRIPTS] && suffix == "rgm") {
        contents.push_back(CONTENT_SCRIPTS);
      } else if (m_Enabled[CONTENT_FILE_OVERRIDES] &&
                 (suffix == "dat" || suffix == "mif" || suffix == "img" ||
                  suffix == "bsa" || suffix == "cfg")) {
        contents.push_back(CONTENT_FILE_OVERRIDES);
      }
    } else {
      if (m_Enabled[CONTENT_TEXTURES] && e->compare("textures") == 0) {
        contents.push_back(CONTENT_TEXTURES);
      } else if (m_Enabled[CONTENT_AUDIO] &&
                 (e->compare("audio") == 0 || e->compare("sound") == 0 ||
                  e->compare("music") == 0)) {
        contents.push_back(CONTENT_AUDIO);
      } else if (m_Enabled[CONTENT_SCRIPTS] && e->compare("maps") == 0) {
        contents.push_back(CONTENT_SCRIPTS);
      } else if (m_Enabled[CONTENT_FILE_OVERRIDES] &&
                 (e->compare("system") == 0 || e->compare("fonts") == 0 ||
                  e->compare("arena2") == 0)) {
        contents.push_back(CONTENT_FILE_OVERRIDES);
      }
    }
  }

  return contents;
}
