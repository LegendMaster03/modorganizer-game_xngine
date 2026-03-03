#include "daggerfallsmoddatacontent.h"

#include <algorithm>
#include <QSet>
#include <QRegularExpression>

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
      const QString lowerName = fileName.toLower();
      const bool patchDocByName =
          ((suffix.compare("txt", Qt::CaseInsensitive) == 0 ||
            suffix.compare("nfo", Qt::CaseInsensitive) == 0 ||
            suffix.compare("diz", Qt::CaseInsensitive) == 0) &&
           (lowerName.contains("patch") || lowerName.contains("fix") ||
            lowerName.contains("hotfix") || lowerName.contains("update")));

      if (fileName.compare("fall_exe_patches.json", Qt::CaseInsensitive) == 0 ||
          fileName.compare("exe_patches.json", Qt::CaseInsensitive) == 0 ||
          fileName.compare("FALL.EXE", Qt::CaseInsensitive) == 0 ||
          fileName.compare("DAGGER.EXE", Qt::CaseInsensitive) == 0 ||
          fileName.compare("PATCHED.TXT", Qt::CaseInsensitive) == 0 ||
          fileName.compare("README.TXT", Qt::CaseInsensitive) == 0 ||
          suffix.compare("ips", Qt::CaseInsensitive) == 0 ||
          suffix.compare("bps", Qt::CaseInsensitive) == 0 ||
          suffix.compare("ups", Qt::CaseInsensitive) == 0 ||
          suffix.compare("ppf", Qt::CaseInsensitive) == 0 ||
          suffix.compare("xdelta3", Qt::CaseInsensitive) == 0 ||
          suffix.compare("vcdiff", Qt::CaseInsensitive) == 0 ||
          suffix.compare("xdelta", Qt::CaseInsensitive) == 0 ||
          patchDocByName) {
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

bool containsUnityPayload(const std::shared_ptr<const MOBase::IFileTree>& fileTree)
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
      if (entry->suffix().compare("dfmod", Qt::CaseInsensitive) == 0 ||
          fileName.compare("dfmod.json", Qt::CaseInsensitive) == 0) {
        return true;
      }
    } else if (entry->isDir()) {
      const auto subtree = fileTree->findDirectory(entry->name());
      if (subtree && containsUnityPayload(subtree)) {
        return true;
      }
    }
  }

  return false;
}

bool containsInstallerExecutablePayload(const std::shared_ptr<const MOBase::IFileTree>& fileTree)
{
  if (!fileTree) {
    return false;
  }

  static const QRegularExpression installKeywordPattern(
      R"((^|[^a-z0-9])(setup|install|installme|patch|update|upgrade|extract|unpack|runme)($|[^a-z0-9]))",
      QRegularExpression::CaseInsensitiveOption);

  for (const auto& entry : *fileTree) {
    if (!entry) {
      continue;
    }

    if (entry->isFile()) {
      const QString fileName = entry->name().toLower();
      const QString suffix = entry->suffix().toLower();
      const bool installerScript =
          ((suffix == "bat" || suffix == "btm" || suffix == "cmd" || suffix == "ps1") &&
           installKeywordPattern.match(fileName).hasMatch());
      const bool installerExecutable =
          ((suffix == "exe" || suffix == "com" || suffix == "pif") &&
           installKeywordPattern.match(fileName).hasMatch());
      const bool selfExtractingExecutable =
          (suffix == "exe" &&
           (fileName.contains("sfx") || fileName.contains("selfextract") ||
            fileName.contains("extractor") || fileName.contains("unpack")));

      if (installerScript || installerExecutable || selfExtractingExecutable) {
        return true;
      }
    } else if (entry->isDir()) {
      const QString dirName = entry->name().toLower();
      if (dirName.contains("install") || dirName.contains("installer") ||
          dirName.contains("setup") || dirName.contains("patch")) {
        return true;
      }
      const auto subtree = fileTree->findDirectory(entry->name());
      if (subtree && containsInstallerExecutablePayload(subtree)) {
        return true;
      }
    }
  }

  return false;
}

bool containsClassicArchivePayload(const std::shared_ptr<const MOBase::IFileTree>& fileTree)
{
  if (!fileTree) {
    return false;
  }

  static const QSet<QString> classicContainerNames = {
      "DAGGER.SND", "ARCH3D.BSA", "BLOCKS.BSA", "MAPS.BSA", "MONSTER.BSA",
      "MIDI.BSA",   "TEXT.RSC",   "CLIMATE.PAK", "POLITIC.PAK", "MAGIC.DEF",
      "SPELLS.STD", "WOODS.WLD"};
  static const QSet<QString> classicContainerExt = {"bsa", "snd", "rsc", "pak", "def", "std",
                                                     "wld"};

  for (const auto& entry : *fileTree) {
    if (!entry) {
      continue;
    }

    if (entry->isFile()) {
      const QString fileName = entry->name();
      const QString suffix = entry->suffix();
      const QString upperName = fileName.toUpper();
      const QString lowerSuffix = suffix.toLower();
      if (classicContainerNames.contains(upperName) ||
          (!lowerSuffix.isEmpty() && classicContainerExt.contains(lowerSuffix))) {
        return true;
      }
    } else if (entry->isDir()) {
      const auto subtree = fileTree->findDirectory(entry->name());
      if (subtree && containsClassicArchivePayload(subtree)) {
        return true;
      }
    }
  }

  return false;
}

bool containsNestedArchivePayload(const std::shared_ptr<const MOBase::IFileTree>& fileTree)
{
  if (!fileTree) {
    return false;
  }

  for (const auto& entry : *fileTree) {
    if (!entry) {
      continue;
    }

    if (entry->isFile()) {
      const QString ext = entry->suffix().toLower();
      const QString fileName = entry->name().toLower();
      const bool isSfxExe =
          (ext == "exe" &&
           (fileName.contains("sfx") || fileName.contains("selfextract") ||
            fileName.contains("extractor") || fileName.contains("unpack")));
      static const QRegularExpression splitArchivePattern(
          R"((\.((zip|7z|rar)\.\d{3}|r\d{2}|z\d{2}|part\d+\.(rar|7z|zip)))$)",
          QRegularExpression::CaseInsensitiveOption);
      static const QRegularExpression tarCompoundPattern(
          R"((\.(tar\.(gz|bz2|xz|zst|lzma)))$)",
          QRegularExpression::CaseInsensitiveOption);
      const bool isSplitArchivePart = splitArchivePattern.match(fileName).hasMatch();
      const bool isTarCompound = tarCompoundPattern.match(fileName).hasMatch();
      if (ext == "zip" || ext == "zipx" || ext == "7z" || ext == "rar" || ext == "tar" || ext == "tgz" ||
          ext == "tbz" || ext == "tbz2" || ext == "txz" || ext == "tlz" ||
          ext == "tzst" || ext == "gz" || ext == "lz" || ext == "lz4" || ext == "lzo" ||
          ext == "lrz" || ext == "bz2" || ext == "xz" ||
          ext == "zst" || ext == "lzma" ||
          ext == "ace" || ext == "arc" || ext == "cab" || ext == "arj" ||
          ext == "ha" || ext == "hpk" || ext == "hyp" || ext == "lzh" ||
          ext == "lha" || ext == "uc2" || ext == "z" || ext == "zoo" ||
          ext == "uha" || ext == "jar" || ext == "pak0" ||
          ext == "iso" || ext == "cue" || ext == "mds" || ext == "mdf" || ext == "nrg" ||
          isSplitArchivePart || isTarCompound || isSfxExe) {
        return true;
      }
    } else if (entry->isDir()) {
      const auto subtree = fileTree->findDirectory(entry->name());
      if (subtree && containsNestedArchivePayload(subtree)) {
        return true;
      }
    }
  }

  return false;
}

bool containsDosboxConfigPayload(const std::shared_ptr<const MOBase::IFileTree>& fileTree)
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
      const QString lowerName = fileName.toLower();
      if ((lowerName.startsWith("dosbox") && suffix.compare("conf", Qt::CaseInsensitive) == 0) ||
          lowerName == "daggerfall.conf" || lowerName == "daggerfall_single.conf" ||
          suffix.compare("conf", Qt::CaseInsensitive) == 0 ||
          suffix.compare("bat", Qt::CaseInsensitive) == 0 ||
          suffix.compare("btm", Qt::CaseInsensitive) == 0 ||
          suffix.compare("cmd", Qt::CaseInsensitive) == 0 ||
          suffix.compare("ps1", Qt::CaseInsensitive) == 0) {
        return true;
      }
    } else if (entry->isDir()) {
      const auto subtree = fileTree->findDirectory(entry->name());
      if (subtree && containsDosboxConfigPayload(subtree)) {
        return true;
      }
    }
  }

  return false;
}

bool containsQuestPayload(const std::shared_ptr<const MOBase::IFileTree>& fileTree)
{
  if (!fileTree) {
    return false;
  }

  for (const auto& entry : *fileTree) {
    if (!entry) {
      continue;
    }

    if (entry->isFile()) {
      const QString suffix = entry->suffix();
      if (suffix.compare("qbn", Qt::CaseInsensitive) == 0 ||
          suffix.compare("qrc", Qt::CaseInsensitive) == 0 ||
          suffix.compare("src", Qt::CaseInsensitive) == 0) {
        return true;
      }
    } else if (entry->isDir()) {
      const auto subtree = fileTree->findDirectory(entry->name());
      if (subtree && containsQuestPayload(subtree)) {
        return true;
      }
    }
  }

  return false;
}

bool containsWorldBlockPayload(const std::shared_ptr<const MOBase::IFileTree>& fileTree)
{
  if (!fileTree) {
    return false;
  }

  for (const auto& entry : *fileTree) {
    if (!entry) {
      continue;
    }

    if (entry->isFile()) {
      const QString suffix = entry->suffix();
      if (suffix.compare("rmb", Qt::CaseInsensitive) == 0 ||
          suffix.compare("rdb", Qt::CaseInsensitive) == 0 ||
          suffix.compare("rdi", Qt::CaseInsensitive) == 0 ||
          suffix.compare("wld", Qt::CaseInsensitive) == 0) {
        return true;
      }
    } else if (entry->isDir()) {
      const auto subtree = fileTree->findDirectory(entry->name());
      if (subtree && containsWorldBlockPayload(subtree)) {
        return true;
      }
    }
  }

  return false;
}

bool containsMapRecordPayload(const std::shared_ptr<const MOBase::IFileTree>& fileTree)
{
  if (!fileTree) {
    return false;
  }

  static const QRegularExpression mapsLooseRecordRe(
      "(?i)^(MAPNAMES|MAPTABLE|MAPPITEM|MAPDITEM)\\.\\d+$");

  for (const auto& entry : *fileTree) {
    if (!entry) {
      continue;
    }

    if (entry->isFile()) {
      const QString fileName = entry->name();
      if (mapsLooseRecordRe.match(fileName).hasMatch()) {
        return true;
      }
    } else if (entry->isDir()) {
      const auto subtree = fileTree->findDirectory(entry->name());
      if (subtree && containsMapRecordPayload(subtree)) {
        return true;
      }
    }
  }

  return false;
}

bool containsClassicConfigPayload(const std::shared_ptr<const MOBase::IFileTree>& fileTree)
{
  if (!fileTree) {
    return false;
  }

  static const QSet<QString> cfgExt = {"cfg", "cnf", "conf", "ini"};
  static const QSet<QString> cfgNames = {"SETUP.INI", "Z.CFG", "HMISET.CFG", "CASTER.CFG"};

  for (const auto& entry : *fileTree) {
    if (!entry) {
      continue;
    }

    if (entry->isFile()) {
      const QString fileNameUpper = entry->name().toUpper();
      if (cfgNames.contains(fileNameUpper)) {
        return true;
      }

      const QString suffix = entry->suffix().toLower();
      if (!suffix.isEmpty() && cfgExt.contains(suffix)) {
        return true;
      }
    } else if (entry->isDir()) {
      const auto subtree = fileTree->findDirectory(entry->name());
      if (subtree && containsClassicConfigPayload(subtree)) {
        return true;
      }
    }
  }

  return false;
}

bool containsTextResourcePayload(const std::shared_ptr<const MOBase::IFileTree>& fileTree)
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
      const QString lowerName = fileName.toLower();
      const QString suffix = entry->suffix();

      if (fileName.compare("TEXT.RSC", Qt::CaseInsensitive) == 0 ||
          suffix.compare("rsc", Qt::CaseInsensitive) == 0 ||
          (lowerName.startsWith("book") && suffix.compare("txt", Qt::CaseInsensitive) == 0)) {
        return true;
      }
    } else if (entry->isDir()) {
      const auto subtree = fileTree->findDirectory(entry->name());
      if (subtree && containsTextResourcePayload(subtree)) {
        return true;
      }
    }
  }

  return false;
}

bool containsTexturePayload(const std::shared_ptr<const MOBase::IFileTree>& fileTree)
{
  if (!fileTree) {
    return false;
  }

  static const QSet<QString> textureExt = {"img", "cif", "rci", "pal", "col", "raw", "cps",
                                           "lip"};
  static const QRegularExpression textureTripleRe("(?i)^TEXTURE\\.\\d{3}$");

  for (const auto& entry : *fileTree) {
    if (!entry) {
      continue;
    }

    if (entry->isFile()) {
      const QString fileName = entry->name();
      if (textureTripleRe.match(fileName).hasMatch()) {
        return true;
      }

      const QString suffix = entry->suffix().toLower();
      if (!suffix.isEmpty() && textureExt.contains(suffix)) {
        return true;
      }
    } else if (entry->isDir()) {
      const auto subtree = fileTree->findDirectory(entry->name());
      if (subtree && containsTexturePayload(subtree)) {
        return true;
      }
    }
  }

  return false;
}

bool containsClassicAudioPayload(const std::shared_ptr<const MOBase::IFileTree>& fileTree)
{
  if (!fileTree) {
    return false;
  }

  static const QSet<QString> audioExt = {"xmi", "xmid", "xmf", "mid", "mus", "voc", "wav"};
  static const QSet<QString> audioNames = {"DAGGER.SND", "MIDI.BSA"};

  for (const auto& entry : *fileTree) {
    if (!entry) {
      continue;
    }

    if (entry->isFile()) {
      const QString fileNameUpper = entry->name().toUpper();
      if (audioNames.contains(fileNameUpper)) {
        return true;
      }

      const QString suffix = entry->suffix().toLower();
      if (!suffix.isEmpty() && audioExt.contains(suffix)) {
        return true;
      }
    } else if (entry->isDir()) {
      const QString dirName = entry->name().toLower();
      if (dirName == "sound" || dirName == "sounds" || dirName == "music" || dirName == "audio") {
        return true;
      }
      const auto subtree = fileTree->findDirectory(entry->name());
      if (subtree && containsClassicAudioPayload(subtree)) {
        return true;
      }
    }
  }

  return false;
}

bool containsSavePackPayload(const std::shared_ptr<const MOBase::IFileTree>& fileTree)
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
      static const QSet<QString> savePackFiles = {
          "MAPSAVE.SAV", "SAVENAME.TXT", "SAVETREE.DAT", "SAVEVARS.DAT", "IMAGE.RAW"};
      static const QRegularExpression mapSaveRowRe("(?i)^MAPSAVE\\.\\d{3}$");
      if (savePackFiles.contains(fileName.toUpper())) {
        return true;
      }
      if (mapSaveRowRe.match(fileName).hasMatch()) {
        return true;
      }
    } else if (entry->isDir()) {
      const QString lowerName = entry->name().toLower();
      if (lowerName == "save" || lowerName == "saves" || lowerName == "savegame" ||
          lowerName == "savegames" || lowerName == "save game" || lowerName == "save games" ||
          (lowerName.startsWith("save") && lowerName.size() > 4 &&
           std::all_of(lowerName.begin() + 4, lowerName.end(),
                       [](const QChar& c) { return c.isDigit(); })) ||
          (lowerName.startsWith("slot") && lowerName.size() > 4 &&
           std::all_of(lowerName.begin() + 4, lowerName.end(),
                       [](const QChar& c) { return c.isDigit(); }))) {
        return true;
      }
      const auto subtree = fileTree->findDirectory(entry->name());
      if (subtree && containsSavePackPayload(subtree)) {
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
  if (m_Enabled[CONTENT_DAGGERFALL_UNITY]) {
    contents.insert(contents.begin(), {CONTENT_DAGGERFALL_UNITY, QT_TR_NOOP("Daggerfall Unity"),
                                       ":/MO/gui/content/plugin"});
  }
  if (m_Enabled[CONTENT_DAGGERFALL_LIMITED_UNITY_RUNTIME]) {
    contents.insert(
        contents.begin(),
        {CONTENT_DAGGERFALL_LIMITED_UNITY_RUNTIME, QT_TR_NOOP("Unity Runtime Mod (Limited)"),
         ":/MO/gui/content/plugin"});
  }
  if (m_Enabled[CONTENT_DAGGERFALL_MANUAL_INSTALLER]) {
    contents.insert(contents.begin(),
                    {CONTENT_DAGGERFALL_MANUAL_INSTALLER, QT_TR_NOOP("Manual Installer Required"),
                     ":/MO/gui/content/script"});
  }
  if (m_Enabled[CONTENT_DAGGERFALL_ARCHIVES]) {
    contents.insert(contents.begin(),
                    {CONTENT_DAGGERFALL_ARCHIVES, QT_TR_NOOP("Classic Archives"),
                     ":/MO/gui/content/archive"});
  }
  if (m_Enabled[CONTENT_DAGGERFALL_DOSBOX_CONFIG]) {
    contents.insert(contents.begin(),
                    {CONTENT_DAGGERFALL_DOSBOX_CONFIG, QT_TR_NOOP("DOSBox / Launcher Config"),
                     ":/MO/gui/content/inifile"});
  }
  if (m_Enabled[CONTENT_DAGGERFALL_QUESTS]) {
    contents.insert(contents.begin(),
                    {CONTENT_DAGGERFALL_QUESTS, QT_TR_NOOP("Quest Scripts"),
                     ":/MO/gui/content/script"});
  }
  if (m_Enabled[CONTENT_DAGGERFALL_WORLD_BLOCKS]) {
    contents.insert(contents.begin(),
                    {CONTENT_DAGGERFALL_WORLD_BLOCKS, QT_TR_NOOP("World Blocks"),
                     ":/MO/gui/content/landscape"});
  }
  if (m_Enabled[CONTENT_DAGGERFALL_TEXTURES]) {
    contents.insert(contents.begin(),
                    {CONTENT_DAGGERFALL_TEXTURES, QT_TR_NOOP("Classic Textures"),
                     ":/MO/gui/content/texture"});
  }
  if (m_Enabled[CONTENT_DAGGERFALL_TEXT_RESOURCES]) {
    contents.insert(contents.begin(),
                    {CONTENT_DAGGERFALL_TEXT_RESOURCES, QT_TR_NOOP("Text Resources"),
                     ":/MO/gui/content/menu"});
  }
  if (m_Enabled[CONTENT_DAGGERFALL_SAVE_PACKS]) {
    contents.insert(contents.begin(),
                    {CONTENT_DAGGERFALL_SAVE_PACKS, QT_TR_NOOP("Save Pack"),
                     ":/MO/gui/content/savegame"});
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
  const bool isUnity = containsUnityPayload(fileTree);
  const bool isInstallerExecutable = containsInstallerExecutablePayload(fileTree);
  const bool isClassicArchive = containsClassicArchivePayload(fileTree);
  const bool isNestedArchive = containsNestedArchivePayload(fileTree);
  const bool isDosboxConfig = containsDosboxConfigPayload(fileTree);
  const bool isQuest = containsQuestPayload(fileTree);
  const bool isWorldBlocks = containsWorldBlockPayload(fileTree);
  const bool isMapRecords = containsMapRecordPayload(fileTree);
  const bool isClassicConfig = containsClassicConfigPayload(fileTree);
  const bool isTextures = containsTexturePayload(fileTree);
  const bool isClassicAudio = containsClassicAudioPayload(fileTree);
  const bool isTextResources = containsTextResourcePayload(fileTree);
  const bool isSavePack = containsSavePackPayload(fileTree);
  if (isPatch && m_Enabled[CONTENT_PATCH_INSTRUCTIONS] &&
      std::find(contents.begin(), contents.end(), CONTENT_PATCH_INSTRUCTIONS) == contents.end()) {
    contents.push_back(CONTENT_PATCH_INSTRUCTIONS);
  }
  if (isUnity && m_Enabled[CONTENT_DAGGERFALL_UNITY] &&
      std::find(contents.begin(), contents.end(), CONTENT_DAGGERFALL_UNITY) == contents.end()) {
    contents.push_back(CONTENT_DAGGERFALL_UNITY);
  }
  if (isUnity && m_Enabled[CONTENT_DAGGERFALL_LIMITED_UNITY_RUNTIME] &&
      std::find(contents.begin(), contents.end(), CONTENT_DAGGERFALL_LIMITED_UNITY_RUNTIME) ==
          contents.end()) {
    contents.push_back(CONTENT_DAGGERFALL_LIMITED_UNITY_RUNTIME);
  }
  if (isInstallerExecutable && m_Enabled[CONTENT_DAGGERFALL_MANUAL_INSTALLER] &&
      std::find(contents.begin(), contents.end(), CONTENT_DAGGERFALL_MANUAL_INSTALLER) ==
          contents.end()) {
    contents.push_back(CONTENT_DAGGERFALL_MANUAL_INSTALLER);
  }
  if ((isClassicArchive || isNestedArchive) && m_Enabled[CONTENT_DAGGERFALL_ARCHIVES] &&
      std::find(contents.begin(), contents.end(), CONTENT_DAGGERFALL_ARCHIVES) ==
          contents.end()) {
    contents.push_back(CONTENT_DAGGERFALL_ARCHIVES);
  }
  if (isDosboxConfig && m_Enabled[CONTENT_DAGGERFALL_DOSBOX_CONFIG] &&
      std::find(contents.begin(), contents.end(), CONTENT_DAGGERFALL_DOSBOX_CONFIG) ==
          contents.end()) {
    contents.push_back(CONTENT_DAGGERFALL_DOSBOX_CONFIG);
  }
  if (isQuest && m_Enabled[CONTENT_DAGGERFALL_QUESTS] &&
      std::find(contents.begin(), contents.end(), CONTENT_DAGGERFALL_QUESTS) ==
          contents.end()) {
    contents.push_back(CONTENT_DAGGERFALL_QUESTS);
  }
  if (isWorldBlocks && m_Enabled[CONTENT_DAGGERFALL_WORLD_BLOCKS] &&
      std::find(contents.begin(), contents.end(), CONTENT_DAGGERFALL_WORLD_BLOCKS) ==
          contents.end()) {
    contents.push_back(CONTENT_DAGGERFALL_WORLD_BLOCKS);
  }
  if (isMapRecords && m_Enabled[CONTENT_SCRIPTS] &&
      std::find(contents.begin(), contents.end(), CONTENT_SCRIPTS) == contents.end()) {
    contents.push_back(CONTENT_SCRIPTS);
  }
  if (isClassicConfig && m_Enabled[CONTENT_CONFIG] &&
      std::find(contents.begin(), contents.end(), CONTENT_CONFIG) == contents.end()) {
    contents.push_back(CONTENT_CONFIG);
  }
  if (isTextures && m_Enabled[CONTENT_DAGGERFALL_TEXTURES] &&
      std::find(contents.begin(), contents.end(), CONTENT_DAGGERFALL_TEXTURES) ==
          contents.end()) {
    contents.push_back(CONTENT_DAGGERFALL_TEXTURES);
  }
  if (isClassicAudio && m_Enabled[CONTENT_AUDIO] &&
      std::find(contents.begin(), contents.end(), CONTENT_AUDIO) == contents.end()) {
    contents.push_back(CONTENT_AUDIO);
  }
  if (isTextResources && m_Enabled[CONTENT_DAGGERFALL_TEXT_RESOURCES] &&
      std::find(contents.begin(), contents.end(), CONTENT_DAGGERFALL_TEXT_RESOURCES) ==
          contents.end()) {
    contents.push_back(CONTENT_DAGGERFALL_TEXT_RESOURCES);
  }
  if (isSavePack && m_Enabled[CONTENT_DAGGERFALL_SAVE_PACKS] &&
      std::find(contents.begin(), contents.end(), CONTENT_DAGGERFALL_SAVE_PACKS) ==
          contents.end()) {
    contents.push_back(CONTENT_DAGGERFALL_SAVE_PACKS);
  }

  return contents;
}
