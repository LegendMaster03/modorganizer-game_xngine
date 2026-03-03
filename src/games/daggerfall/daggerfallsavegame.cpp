#include "daggerfallsavegame.h"

#include "daggerfallcommon.h"
#include "gamedaggerfall.h"
#include "daggerfallmapsbsa.h"
#include "xnginepaletteformat.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QColor>
#include <QImage>
#include <QRegularExpression>
#include <QStringList>
#include <QtEndian>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <unordered_map>
#include <unordered_set>

namespace {
constexpr quint8 kRecordTypeCharacterPosition = 0x01;
constexpr quint8 kRecordTypeItem = 0x02;
constexpr quint8 kRecordTypeCharacter = 0x03;
constexpr quint8 kRecordTypeSpell = 0x09;
constexpr quint8 kRecordTypeGuildMembership = 0x0A;
constexpr quint8 kRecordTypeBankAccount = 0x19;
constexpr quint8 kRecordTypeContainer = 0x34;
constexpr quint8 kRecordTypeDungeonInformation = 0x07;
constexpr qsizetype kRecordBaseSize = 71;
constexpr qsizetype kHeaderSize = 0x13;
constexpr qsizetype kCharacterRecordNameOffset = 0x00;
constexpr qsizetype kCharacterRecordNameLength = 0x20;
constexpr qsizetype kCharacterRecordClassOffset = 0x24c;
constexpr qsizetype kCharacterRecordClassLength = 0x12;
constexpr qsizetype kCharacterRecordLevelOffset = 0x81;
constexpr qsizetype kCharacterRecordRaceOffset = 0x43;
constexpr qsizetype kCharacterRecordHealthOffset = 0x7c;
constexpr qsizetype kCharacterRecordManaOffset = 0x8d;
constexpr qsizetype kCharacterRecordReflexOffset = 0x83;
constexpr qsizetype kCharacterRecordGoldOffset = 0x85;
constexpr qsizetype kCharacterRecordTimestampOffset = 0x201;
constexpr qsizetype kSaveNameLength = 32;
constexpr qsizetype kImageWidth = 80;
constexpr qsizetype kImageHeight = 50;
constexpr qsizetype kImageBytesPerPixel8 = 1;
constexpr qsizetype kImageBytesPerPixel = 2;
constexpr qsizetype kImageRawSize8 = kImageWidth * kImageHeight * kImageBytesPerPixel8;
constexpr qsizetype kImageRawSize15 = kImageWidth * kImageHeight * kImageBytesPerPixel;

bool loadPaletteFromFile(const QString& path, std::array<QColor, 256>& palette)
{
  XnginePaletteFormat::Document doc;
  XnginePaletteFormat::Traits traits;
  traits.variant = XnginePaletteFormat::Variant::Auto;
  traits.allowTrailingPaletteData = true;
  traits.strictValidation = false;
  if (!XnginePaletteFormat::readFile(path, doc, nullptr, traits)) {
    return false;
  }
  if (doc.palette.colors.size() < 256) {
    return false;
  }
  for (int i = 0; i < 256; ++i) {
    palette[static_cast<size_t>(i)] = doc.palette.colors.at(i);
  }
  palette[0].setAlpha(255); // save screenshot uses color 0 as a normal pixel, not transparency

  return true;
}

bool loadDaggerfallPalette(const GameDaggerfall* game, std::array<QColor, 256>& palette)
{
  if (game == nullptr) {
    return false;
  }

  const QDir arena2(game->gameDirectory().filePath("arena2"));
  const QStringList candidates = {
      "PAL.RAW",
      "PAL.PAL",
      "OLDPAL.PAL",
      "MAP.PAL",
      "ART_PAL.COL",
  };

  for (const auto& name : candidates) {
    const QString filePath = arena2.filePath(name);
    if (loadPaletteFromFile(filePath, palette)) {
      return true;
    }
  }
  return false;
}

bool loadDaggerfallFmapPalette(const GameDaggerfall* game, std::array<QColor, 256>& palette)
{
  if (game == nullptr) {
    return false;
  }

  const QDir arena2(game->gameDirectory().filePath("arena2"));
  return loadPaletteFromFile(arena2.filePath("FMAP_PAL.COL"), palette);
}

bool tryGetDaggerfallLocationTypeColor(const GameDaggerfall* game, int locationType,
                                       QColor& outColor)
{
  const int paletteIndex = Daggerfall::Data::locationTypePaletteIndex(locationType);
  if (paletteIndex < 0 || paletteIndex > 255) {
    return false;
  }

  std::array<QColor, 256> palette{};
  if (!loadDaggerfallFmapPalette(game, palette)) {
    return false;
  }

  outColor = palette[static_cast<size_t>(paletteIndex)];
  return outColor.isValid();
}
}  // namespace

DaggerfallsSaveGame::DaggerfallsSaveGame(const QString& saveFolder,
                                         const GameDaggerfall* game)
    : XngineSaveGame(saveFolder, game), m_SaveFolder(saveFolder), m_Game(game)
{
  QFileInfo info(saveFolder);
  m_DisplayName = info.fileName();
  const QRegularExpression saveSlotRegex("(?i)^SAVE(\\d+)$");
  const QRegularExpressionMatch slotMatch = saveSlotRegex.match(info.fileName());
  if (slotMatch.hasMatch()) {
    bool ok = false;
    const int slot = slotMatch.captured(1).toInt(&ok);
    if (ok && slot >= 0) {
      m_SaveNumber = static_cast<unsigned long>(slot);
    }
  }
  parseSaveName();
  parseSaveTree();
  parseSaveVars();
}

std::unique_ptr<XngineSaveGame::DataFields> DaggerfallsSaveGame::fetchDataFields() const
{
  auto fields = std::make_unique<DataFields>();

  QFile imageFile(saveFilePath("IMAGE.RAW"));
  if (!imageFile.open(QIODevice::ReadOnly)) {
    return fields;
  }

  QByteArray raw = imageFile.readAll();
  if (raw.size() < kImageRawSize8) {
    return fields;
  }

  QImage image(static_cast<int>(kImageWidth), static_cast<int>(kImageHeight),
               QImage::Format_RGB32);
  const auto* ptr = reinterpret_cast<const uchar*>(raw.constData());

  if (raw.size() >= kImageRawSize15) {
    // 15-bit RGB (5:5:5) raw.
    for (qsizetype y = 0; y < kImageHeight; ++y) {
      for (qsizetype x = 0; x < kImageWidth; ++x) {
        const qsizetype i = (y * kImageWidth + x) * kImageBytesPerPixel;
        const quint16 pixel = qFromLittleEndian<quint16>(ptr + i);
        const int r = ((pixel >> 10) & 0x1F) * 255 / 31;
        const int g = ((pixel >> 5) & 0x1F) * 255 / 31;
        const int b = (pixel & 0x1F) * 255 / 31;
        image.setPixelColor(static_cast<int>(x), static_cast<int>(y), QColor(r, g, b));
      }
    }
  } else {
    // 8-bit indexed raw: decode using game palette when available.
    std::array<QColor, 256> palette{};
    const bool hasPalette = loadDaggerfallPalette(m_Game, palette);
    for (qsizetype y = 0; y < kImageHeight; ++y) {
      for (qsizetype x = 0; x < kImageWidth; ++x) {
        const qsizetype i = y * kImageWidth + x;
        const int v = static_cast<unsigned char>(ptr[i]);
        if (hasPalette) {
          image.setPixelColor(static_cast<int>(x), static_cast<int>(y),
                              palette[static_cast<size_t>(v)]);
        } else {
          image.setPixelColor(static_cast<int>(x), static_cast<int>(y), QColor(v, v, v));
        }
      }
    }
  }

  fields->Screenshot = image;
  return fields;
}

bool DaggerfallsSaveGame::parseSaveName()
{
  QFile saveNameFile(saveFilePath("SAVENAME.TXT"));
  if (!saveNameFile.open(QIODevice::ReadOnly)) {
    return false;
  }

  QByteArray bytes = saveNameFile.read(kSaveNameLength);
  if (bytes.isEmpty()) {
    return false;
  }

  const int nullPos = bytes.indexOf('\0');
  if (nullPos >= 0) {
    bytes.truncate(nullPos);
  }

  const QString saveName = QString::fromLocal8Bit(bytes).trimmed();
  if (!saveName.isEmpty()) {
    m_DisplayName = saveName;
    return true;
  }

  return false;
}

QString DaggerfallsSaveGame::getName() const
{
  const QString name = m_DisplayName.trimmed();
  if (!name.isEmpty()) {
    return name;
  }
  return XngineSaveGame::getName();
}

QString DaggerfallsSaveGame::getPCLocation() const
{
  if (m_LocationNameDetail.isEmpty()) {
    return m_PCLocation;
  }

  QStringList locationParts;
  if (!m_LocationTypeNameDetail.isEmpty()) {
    locationParts.push_back(m_LocationTypeNameDetail.toHtmlEscaped());
  }
  if (!m_LocationRegionNameDetail.isEmpty()) {
    locationParts.push_back(m_LocationRegionNameDetail.toHtmlEscaped());
  }

  QString locationName = m_LocationNameDetail.toHtmlEscaped();
  if (m_LocationTypeColorDetail.isValid()) {
    locationName = QString("<span style=\"color:%1; font-weight:600;\">%2</span>")
                       .arg(m_LocationTypeColorDetail.name(QColor::HexRgb), locationName);
  }

  if (locationParts.isEmpty()) {
    return locationName;
  }

  return QString("%1 (%2)").arg(locationName, locationParts.join(", "));
}

QString DaggerfallsSaveGame::getGameDetails() const
{
  const int developerDetailsLevel =
      (m_Game != nullptr) ? m_Game->developerSaveDetailsLevel() : 0;
  const bool showDeveloperDetails = developerDetailsLevel > 0;
  const bool fitToPaneMode = (developerDetailsLevel == 2);

  QStringList lines;
  auto appendPlain = [&lines](const QString& label, const QString& value) {
    if (value.isEmpty()) {
      return;
    }
    lines.push_back(QString("%1: %2").arg(label.toHtmlEscaped(), value.toHtmlEscaped()));
  };
  auto appendWrappedList = [&lines](const QString& label, const QStringList& parts,
                                    int maxCharsPerLine = 70) {
    if (parts.isEmpty()) {
      return;
    }
    QString current = QString("%1: ").arg(label.toHtmlEscaped());
    const QString indent = "&nbsp;&nbsp;";
    bool first = true;
    for (const auto& part : parts) {
      const QString token = first ? part : QString(", %1").arg(part);
      if (!first && (current.size() + token.size()) > maxCharsPerLine) {
        lines.push_back(current);
        current = indent + part;
      } else {
        current += token;
      }
      first = false;
    }
    if (!current.isEmpty()) {
      lines.push_back(current);
    }
  };

  if (!m_InGameDate.isEmpty()) {
    lines.push_back(QString("In-game: %1").arg(m_InGameDate.toHtmlEscaped()));
  }
  const QString race = raceName(m_Race);
  appendPlain("Race", race);
  appendPlain("Class", m_ClassName);
  const QString reflex = reflexName(m_Reflex);
  appendPlain("Reflex", reflex);
  if (m_HPMax > 0) {
    lines.push_back(QString("HP: %1/%2").arg(m_HP).arg(m_HPMax));
  }
  if (m_ManaMax > 0) {
    lines.push_back(QString("MP: %1/%2").arg(m_Mana).arg(m_ManaMax));
  }
  if (m_Gold > 0) {
    lines.push_back(QString("Gold: %1").arg(m_Gold));
  }

  if (showDeveloperDetails) {
    const int anomalyCount = m_RecordParseStats.truncatedRecords +
                             m_RecordParseStats.negativeLengths +
                             m_RecordParseStats.invalidDungeonLength;
    lines.push_back("");
    lines.push_back("[Developer Details]");

    // Compact mode (level 1): keep developer output short enough for narrow/non-scrolling panes.
    if (developerDetailsLevel == 1) {
      int unknownRecords = 0;
      QStringList topTypes;
      QList<QPair<int, QString>> rankedTypes;
      for (auto it = m_SaveTreeRecordTypeCounts.cbegin(); it != m_SaveTreeRecordTypeCounts.cend();
           ++it) {
        const QString typeName = recordTypeName(it.key());
        if (typeName.startsWith("Unknown")) {
          unknownRecords += it.value();
        }
        rankedTypes.push_back({it.value(), QString("0x%1=%2").arg(it.key(), 0, 16).arg(it.value())});
      }
      std::sort(rankedTypes.begin(), rankedTypes.end(),
                [](const QPair<int, QString>& a, const QPair<int, QString>& b) {
                  return a.first > b.first;
                });
      for (int i = 0; i < rankedTypes.size() && i < 6; ++i) {
        topTypes.push_back(rankedTypes.at(i).second);
      }

      lines.push_back(QString("Health: %1")
                          .arg(computeHealthStatus(anomalyCount, m_SaveTreeOrphanRecordCount,
                                                   m_SaveTreeParentTypeMismatchCount)));
      lines.push_back(QString("Hdr: v=0x%1 loc=0x%2 zone=0x%3")
                          .arg(m_SaveTreeHeaderVersion, 0, 16)
                          .arg(m_SaveTreeLocationCode, 0, 16)
                          .arg(m_SaveTreeZoneType, 0, 16));
      lines.push_back(QString("Tree: recs=%1 dung=%2 unk=%3")
                          .arg(m_SaveTreeRecordCount)
                          .arg(m_SaveTreeDungeonRecordCount)
                          .arg(unknownRecords));
      lines.push_back(QString("DateRaw: c=0x%1 s=0x%2")
                          .arg(m_CharacterTimestampRaw, 0, 16)
                          .arg(m_SaveVarsTimestampRaw, 0, 16));
      lines.push_back(QString("Hierarchy: roots=%1 orphans=%2 mismatch=%3")
                          .arg(m_SaveTreeRootRecordCount)
                          .arg(m_SaveTreeOrphanRecordCount)
                          .arg(m_SaveTreeParentTypeMismatchCount));
      lines.push_back(QString("Parse: z=%1 d=%2 t=%3 n=%4 id=%5")
                          .arg(m_RecordParseStats.zeroLengthSeparators)
                          .arg(m_RecordParseStats.dungeonLengthAdjusted)
                          .arg(m_RecordParseStats.truncatedRecords)
                          .arg(m_RecordParseStats.negativeLengths)
                          .arg(m_RecordParseStats.invalidDungeonLength));
      if (!topTypes.isEmpty()) {
        lines.push_back(QString("TopTypes: %1").arg(topTypes.join(", ")));
      }
      lines.push_back(QString("SaveVars: fac~%1 reg~%2")
                          .arg(m_SaveVarsFactionRecordCountEstimate)
                          .arg(m_SaveVarsRegionalRepRecordCountEstimate));
      if (m_BankRegionCount > 0) {
        lines.push_back(QString("Bank: regs=%1 debt=%2 def=%3")
                            .arg(m_BankRegionCount)
                            .arg(m_BankDebtRegions)
                            .arg(m_BankDefaultedRegions));
        lines.push_back(QString("Bank$: bal=%1 debt=%2")
                            .arg(m_BankTotalBalance)
                            .arg(m_BankTotalDebt));
      }
      if (m_SpellSummary.count > 0) {
        lines.push_back(QString("Spells: count=%1 effects=%2")
                            .arg(m_SpellSummary.count)
                            .arg(m_SpellSummary.effectSlotsUsed));
      }
      if (m_ItemRecordCount > 0 || m_ContainerRecordCount > 0) {
        lines.push_back(QString("Items: i=%1 c=%2 q=%3")
                            .arg(m_ItemRecordCount)
                            .arg(m_ContainerRecordCount)
                            .arg(m_ItemQuestBoundCount));
      }

      return lines.join("<br/>");
    }

    lines.push_back(
        QString("Save Health: %1")
            .arg(computeHealthStatus(anomalyCount, m_SaveTreeOrphanRecordCount,
                                     m_SaveTreeParentTypeMismatchCount)
                     .toHtmlEscaped()));
    lines.push_back(QString("Hdr version=0x%1").arg(m_SaveTreeHeaderVersion, 0, 16));
    lines.push_back(QString("Hdr locCode=0x%1").arg(m_SaveTreeLocationCode, 0, 16));
    lines.push_back(QString("Hdr zoneType=0x%1").arg(m_SaveTreeZoneType, 0, 16));
    lines.push_back(
        QString("&nbsp;&nbsp;xyz=(%1,%2,%3)")
            .arg(m_SaveTreeHeaderX)
            .arg(m_SaveTreeHeaderY)
            .arg(m_SaveTreeHeaderZ));
    lines.push_back(QString("layout locDet=%1").arg(m_SaveTreeLocationDetailCount));
    lines.push_back(QString("&nbsp;&nbsp;rsStart=%1").arg(m_SaveTreeRecordStreamStart));
    lines.push_back(QString("&nbsp;&nbsp;rsEnd=%1").arg(m_SaveTreeRecordStreamEnd));
    lines.push_back(QString("&nbsp;&nbsp;recs=%1").arg(m_SaveTreeRecordCount));
    lines.push_back(QString("&nbsp;&nbsp;dung=%1").arg(m_SaveTreeDungeonRecordCount));
    lines.push_back(QString("Record Hierarchy: roots=%1").arg(m_SaveTreeRootRecordCount));
    lines.push_back(QString("&nbsp;&nbsp;orphans=%1").arg(m_SaveTreeOrphanRecordCount));
    lines.push_back(QString("&nbsp;&nbsp;parentTypeMismatch=%1").arg(m_SaveTreeParentTypeMismatchCount));
    lines.push_back(
        QString("Record Parse: zeroLenSeparators=%1, dungeonLenAdjusted=%2")
            .arg(m_RecordParseStats.zeroLengthSeparators)
            .arg(m_RecordParseStats.dungeonLengthAdjusted));
    lines.push_back(
        QString("&nbsp;&nbsp;truncated=%1, negativeLen=%2, invalidDungeonLen=%3")
            .arg(m_RecordParseStats.truncatedRecords)
            .arg(m_RecordParseStats.negativeLengths)
            .arg(m_RecordParseStats.invalidDungeonLength));

    QStringList recordCountParts;
    for (auto it = m_SaveTreeRecordTypeCounts.cbegin(); it != m_SaveTreeRecordTypeCounts.cend();
         ++it) {
      recordCountParts.push_back(
          QString("0x%1=%2")
              .arg(it.key(), 0, 16)
              .arg(it.value()));
    }
    if (!recordCountParts.isEmpty()) {
      appendWrappedList("Record Types", recordCountParts, 38);
    }

    if (m_CharacterTimestampRaw > 0 || m_SaveVarsTimestampRaw > 0) {
      lines.push_back(
          QString("DateTime Raw: char=0x%1 (%2)")
              .arg(m_CharacterTimestampRaw, 0, 16)
              .arg(m_CharacterTimestampRaw));
      lines.push_back(
          QString("&nbsp;&nbsp;savevars=0x%1 (%2)")
              .arg(m_SaveVarsTimestampRaw, 0, 16)
              .arg(m_SaveVarsTimestampRaw));
      lines.push_back(QString("&nbsp;&nbsp;savevarsTimestampOffset=0x%1")
                          .arg(m_SaveVarsTimestampOffset, 0, 16));
    }

    lines.push_back(
        QString("Character Flags: resist=[%1], immune=[%2]")
            .arg(decodeElementMask(m_ResistMask).join(", ").toHtmlEscaped())
            .arg(decodeElementMask(m_ImmuneMask).join(", ").toHtmlEscaped()));
    lines.push_back(
        QString("&nbsp;&nbsp;lowTolerance=[%1], criticalWeakness=[%2]")
            .arg(decodeElementMask(m_LowToleranceMask).join(", ").toHtmlEscaped())
            .arg(decodeElementMask(m_CriticalWeaknessMask).join(", ").toHtmlEscaped()));
    lines.push_back(
        QString("Character Traits: rapidHeal=[%1], regen=[%2], spellAbsorb=%3")
            .arg(decodeRapidHealMask(m_RapidHealMask).join(", ").toHtmlEscaped())
            .arg(decodeRegenHealthMask(m_RegenHealthMask).join(", ").toHtmlEscaped())
            .arg(decodeSpellAbsorbMask(m_SpellAbsorbMask).toHtmlEscaped()));
    lines.push_back(
        QString("&nbsp;&nbsp;hitPhobia=[%1]")
            .arg(decodeHitPhobiaMask(m_HitPhobiaMask).join(", ").toHtmlEscaped()));
    lines.push_back(
        QString("Character Limits: forbiddenMaterial=[%1], weaponExpert=[%2]")
            .arg(decodeForbiddenMaterialMask(m_ForbiddenMaterialMask).join(", ").toHtmlEscaped())
            .arg(decodeWeaponExpertMask(m_WeaponExpertMask).join(", ").toHtmlEscaped())
            );
    lines.push_back(
        QString("&nbsp;&nbsp;forbiddenArmor=[%1], flags1=0x%2")
            .arg(decodeForbiddenArmorMask(m_ForbiddenArmorMask).join(", ").toHtmlEscaped())
            .arg(m_Flags1Mask, 0, 16)
            );

    if (m_SaveVarsFactionRecordCountEstimate > 0 ||
        m_SaveVarsRegionalRepRecordCountEstimate > 0) {
      lines.push_back(
          QString("SaveVars: factionRecs~%1")
              .arg(m_SaveVarsFactionRecordCountEstimate));
      lines.push_back(
          QString("&nbsp;&nbsp;regionalRecs~%1")
              .arg(m_SaveVarsRegionalRepRecordCountEstimate));
    }
    if (m_SaveVarsRegionalRepRecordCountEstimate > 0) {
      lines.push_back(
          QString("Regional Reputation: min=%1, max=%2")
              .arg(m_MinRegionalReputation)
              .arg(m_MaxRegionalReputation));
    }
    if (!m_TopFactionReputations.isEmpty()) {
      QStringList repLines;
      for (const auto& row : m_TopFactionReputations) {
        const QString name = row.name.isEmpty() ? QString("<unnamed>") : row.name;
        repLines.push_back(
            QString("id=%1 rep=%2 type=0x%3 region=0x%4 name=%5")
                .arg(row.factionId)
                .arg(row.reputation)
                .arg(row.type, 0, 16)
                .arg(row.region, 0, 16)
                .arg(name.toHtmlEscaped()));
      }
      appendWrappedList("Faction Top", repLines, 32);
    }

    if (!m_GuildMemberships.isEmpty()) {
      QStringList guildLines;
      for (const auto& guild : m_GuildMemberships) {
        QString promoted = "n/a";
        if (guild.promotionDateRaw > 0) {
          promoted = formatDaggerfallDate(guild.promotionDateRaw).toHtmlEscaped();
        }
        guildLines.push_back(
            QString("guildIdRaw=%1 rankRaw=%2 promoted=%3")
                .arg(guild.guildIdRaw)
                .arg(guild.rankRaw)
                .arg(promoted));
      }
      appendWrappedList("Guilds", guildLines, 32);
    }

    if (m_BankRegionCount > 0) {
      QString earliestDue = "n/a";
      if (m_BankEarliestDueDateRaw > 0) {
        earliestDue = formatDaggerfallDate(m_BankEarliestDueDateRaw).toHtmlEscaped();
      }
      lines.push_back(
          QString("Bank regions=%1").arg(m_BankRegionCount));
      lines.push_back(
          QString("&nbsp;&nbsp;balRegs=%1").arg(m_BankPositiveBalanceRegions));
      lines.push_back(
          QString("&nbsp;&nbsp;debtRegs=%1").arg(m_BankDebtRegions));
      lines.push_back(
          QString("&nbsp;&nbsp;defaulted=%1, totalBalance=%2, totalDebt=%3")
              .arg(m_BankDefaultedRegions)
              .arg(m_BankTotalBalance)
              .arg(m_BankTotalDebt));
      lines.push_back(QString("&nbsp;&nbsp;earliestDue=%1").arg(earliestDue));
    }

    if (m_SpellSummary.count > 0) {
      QStringList sampleNames;
      for (auto it = m_SpellSummary.names.cbegin(); it != m_SpellSummary.names.cend(); ++it) {
        sampleNames.push_back(QString("%1(%2)")
                                  .arg(it.key().toHtmlEscaped())
                                  .arg(it.value()));
        if (sampleNames.size() >= 6) {
          break;
        }
      }
      lines.push_back(
          QString("Spells count=%1").arg(m_SpellSummary.count));
      lines.push_back(
          QString("&nbsp;&nbsp;effectsUsed=%1").arg(m_SpellSummary.effectSlotsUsed));
      lines.push_back(
          QString("&nbsp;&nbsp;named=%1").arg(m_SpellSummary.names.size()));
      if (!sampleNames.isEmpty()) {
        appendWrappedList("Spell Sample", sampleNames, 32);
      }
    }

    if (m_ItemRecordCount > 0 || m_ContainerRecordCount > 0) {
      QStringList categoryParts;
      for (auto it = m_ItemCategoryCounts.cbegin(); it != m_ItemCategoryCounts.cend(); ++it) {
        QString likelyGroup = "Unknown";
        const auto groupIt = m_ItemCategoryLikelyGroups.constFind(it.key());
        if (groupIt != m_ItemCategoryLikelyGroups.cend() && !groupIt.value().isEmpty()) {
          int bestCount = -1;
          for (auto git = groupIt.value().cbegin(); git != groupIt.value().cend(); ++git) {
            if (git.value() > bestCount) {
              bestCount = git.value();
              likelyGroup = git.key();
            }
          }
        }
        categoryParts.push_back(
            QString("0x%1=%2(%3)")
                .arg(it.key(), 0, 16)
                .arg(it.value())
                .arg(likelyGroup.toHtmlEscaped()));
      }
      lines.push_back(
          QString("Inv items=%1").arg(m_ItemRecordCount));
      lines.push_back(
          QString("&nbsp;&nbsp;inCont=%1").arg(m_ItemInContainerCount));
      lines.push_back(
          QString("&nbsp;&nbsp;questBound=%1").arg(m_ItemQuestBoundCount));
      lines.push_back(QString("&nbsp;&nbsp;totalValueRaw=%1, containers=%2")
                          .arg(m_ItemTotalValueRaw)
                          .arg(m_ContainerRecordCount));
      if (!categoryParts.isEmpty()) {
        appendWrappedList("Item Categories", categoryParts, 38);
      }
    }
  }

  if (fitToPaneMode) {
    constexpr int kMaxDevLineChars = 44;
    constexpr int kMaxDevLines = 24;
    bool inDeveloperSection = false;
    int devLineCount = 0;
    QStringList bounded;
    bounded.reserve(lines.size());
    bool truncated = false;

    for (const QString& srcLine : lines) {
      if (srcLine == "[Developer Details]") {
        inDeveloperSection = true;
        devLineCount = 0;
        bounded.push_back(srcLine);
        continue;
      }

      if (!inDeveloperSection) {
        bounded.push_back(srcLine);
        continue;
      }

      if (devLineCount >= kMaxDevLines) {
        truncated = true;
        continue;
      }

      QString line = srcLine;
      line.replace("&nbsp;", " ");
      if (line.size() > kMaxDevLineChars) {
        line = line.left(kMaxDevLineChars - 3) + "...";
      }
      bounded.push_back(line);
      ++devLineCount;
    }

    if (truncated) {
      bounded.push_back("... (set level 3 for full details)");
    }
    return bounded.join("<br/>");
  }

  return lines.join("<br/>");
}

bool DaggerfallsSaveGame::parseSaveTree()
{
  QFile saveTreeFile(saveFilePath("SAVETREE.DAT"));
  if (!saveTreeFile.open(QIODevice::ReadOnly)) {
    return false;
  }

  const QByteArray data = saveTreeFile.readAll();
  if (data.size() < 16) {
    return false;
  }

  // File header (0x13 bytes): version + world position + location metadata.
  quint32 version = 0;
  qint32 headerX = 0;
  qint32 headerY = 0;
  qint32 headerZ = 0;
  quint16 locationCode = 0;
  quint8 zoneType = 0;
  if (data.size() >= kHeaderSize &&
      readLE32U(data, 0x00, version) &&
      readLE32(data, 0x04, headerX) &&
      readLE32(data, 0x08, headerY) &&
      readLE32(data, 0x0c, headerZ) &&
      readLE16U(data, 0x10, locationCode) &&
      readU8(data, 0x12, zoneType)) {
    m_SaveTreeHeaderVersion = version;
    m_SaveTreeHeaderX = headerX;
    m_SaveTreeHeaderY = headerY;
    m_SaveTreeHeaderZ = headerZ;
    m_SaveTreeLocationCode = locationCode;
    m_SaveTreeZoneType = zoneType;
    quint8 locationDetailCount = 0;
    if (readU8(data, kHeaderSize, locationDetailCount)) {
      m_SaveTreeLocationDetailCount = locationDetailCount;
    }
    // Keep this as fallback if we do not find a better in-record player position.
    m_PCLocation = formatHeaderLocationText(locationCode, zoneType, headerX, headerY, headerZ);
    Q_UNUSED(version);
  }

  qsizetype streamStart = 0;
  qsizetype streamEnd = 0;
  const auto records = findBestRecordStream(data, &streamStart, &streamEnd, &m_RecordParseStats);
  if (records.empty()) {
    return false;
  }
  m_SaveTreeRecordStreamStart = streamStart;
  m_SaveTreeRecordStreamEnd = streamEnd;
  m_SaveTreeRecordCount = static_cast<int>(records.size());
  m_SaveTreeRecordTypeCounts.clear();
  m_SaveTreeDungeonRecordCount = 0;
  m_SaveTreeRootRecordCount = 0;
  m_SaveTreeOrphanRecordCount = 0;
  m_SaveTreeParentTypeMismatchCount = 0;
  m_GuildMemberships.clear();
  m_BankRegionCount = 0;
  m_BankPositiveBalanceRegions = 0;
  m_BankDebtRegions = 0;
  m_BankDefaultedRegions = 0;
  m_BankTotalBalance = 0;
  m_BankTotalDebt = 0;
  m_BankEarliestDueDateRaw = 0;
  m_SpellSummary = {};
  m_ItemRecordCount = 0;
  m_ItemInContainerCount = 0;
  m_ItemQuestBoundCount = 0;
  m_ItemTotalValueRaw = 0;
  m_ContainerRecordCount = 0;
  m_ItemCategoryCounts.clear();
  m_ItemCategoryLikelyGroups.clear();
  std::unordered_map<quint32, quint8> recordTypeById;
  std::unordered_set<quint32> containerRecordIds;
  struct ParentLinkInfo {
    quint32 parentId = 0;
    qint32 parentType = -1;
  };
  std::vector<ParentLinkInfo> parentLinks;
  parentLinks.reserve(records.size());
  for (const auto& r : records) {
    m_SaveTreeRecordTypeCounts[r.type] += 1;
    if (r.type == kRecordTypeDungeonInformation) {
      ++m_SaveTreeDungeonRecordCount;
    }

    if (r.payloadLength >= kRecordBaseSize && r.type != kRecordTypeDungeonInformation) {
      quint32 recordId = 0;
      quint32 parentId = 0;
      qint32 parentType = -1;
      if (readLE32U(data, r.payloadOffset + 31, recordId) &&
          readLE32U(data, r.payloadOffset + 39, parentId) &&
          readLE32(data, r.payloadOffset + 67, parentType)) {
        if (recordId != 0) {
          recordTypeById[recordId] = r.type;
          if (r.type == kRecordTypeContainer) {
            containerRecordIds.insert(recordId);
          }
        }
        parentLinks.push_back({parentId, parentType});
      }
    }
  }

  for (const auto& link : parentLinks) {
    if (link.parentId == 0) {
      ++m_SaveTreeRootRecordCount;
      continue;
    }
    const auto it = recordTypeById.find(link.parentId);
    if (it == recordTypeById.end()) {
      ++m_SaveTreeOrphanRecordCount;
      continue;
    }
    if (link.parentType >= 0 && static_cast<qint32>(it->second) != link.parentType) {
      ++m_SaveTreeParentTypeMismatchCount;
    }
  }

  // Prefer a character-position record whose parent type references character.
  const ParsedRecord* bestPosition = nullptr;
  const ParsedRecord* characterRecord = nullptr;
  for (const auto& record : records) {
    if (record.type != kRecordTypeCharacterPosition ||
        record.payloadLength < kRecordBaseSize) {
      if (record.type == kRecordTypeCharacter &&
          record.payloadLength >=
              kRecordBaseSize + kCharacterRecordTimestampOffset + sizeof(quint32)) {
        characterRecord = &record;
      }
      continue;
    }

    qint32 parentType = 0;
    if (readLE32(data, record.payloadOffset + 67, parentType) &&
        parentType == kRecordTypeCharacter) {
      bestPosition = &record;
      break;
    }

    if (bestPosition == nullptr) {
      bestPosition = &record;
    }
  }

  if (characterRecord != nullptr) {
    const qsizetype recordDataOffset = characterRecord->payloadOffset + kRecordBaseSize;
    const QString characterName =
        readFixedString(data, recordDataOffset + kCharacterRecordNameOffset,
                        kCharacterRecordNameLength);
    if (!characterName.isEmpty()) {
      m_PCName = characterName;
    }

    const QString className =
        readFixedString(data, recordDataOffset + kCharacterRecordClassOffset,
                        kCharacterRecordClassLength);
    if (isLikelyClassName(className)) {
      m_ClassName = className;
    }

    quint8 level = 0;
    if (readU8(data, recordDataOffset + kCharacterRecordLevelOffset, level) && level > 0) {
      m_PCLevel = level;
    }

    readU8(data, recordDataOffset + kCharacterRecordRaceOffset, m_Race);
    readU8(data, recordDataOffset + kCharacterRecordReflexOffset, m_Reflex);
    readLE16U(data, recordDataOffset + kCharacterRecordHealthOffset, m_HP);
    readLE16U(data, recordDataOffset + kCharacterRecordHealthOffset + 2, m_HPMax);
    readLE16U(data, recordDataOffset + kCharacterRecordManaOffset, m_Mana);
    readLE16U(data, recordDataOffset + kCharacterRecordManaOffset + 2, m_ManaMax);
    readLE32U(data, recordDataOffset + kCharacterRecordGoldOffset, m_Gold);
    readU8(data, recordDataOffset + 0x230, m_ResistMask);
    readU8(data, recordDataOffset + 0x231, m_ImmuneMask);
    readU8(data, recordDataOffset + 0x232, m_LowToleranceMask);
    readU8(data, recordDataOffset + 0x233, m_CriticalWeaknessMask);
    readLE16U(data, recordDataOffset + 0x234, m_Flags1Mask);
    readU8(data, recordDataOffset + 0x236, m_RapidHealMask);
    readU8(data, recordDataOffset + 0x237, m_RegenHealthMask);
    readU8(data, recordDataOffset + 0x239, m_SpellAbsorbMask);
    readU8(data, recordDataOffset + 0x23A, m_HitPhobiaMask);
    readLE16U(data, recordDataOffset + 0x23B, m_ForbiddenMaterialMask);
    readU8(data, recordDataOffset + 0x23D, m_WeaponExpertMask);
    readLE16U(data, recordDataOffset + 0x23E, m_ForbiddenArmorMask);

    quint32 timestamp = 0;
    if (readLE32U(data, recordDataOffset + kCharacterRecordTimestampOffset, timestamp) &&
        timestamp > 0) {
      m_CharacterTimestampRaw = timestamp;
      m_InGameDate = formatDaggerfallDate(timestamp);
    }
  }

  constexpr qsizetype kBankEntrySize = 13;
  for (const auto& record : records) {
    if (record.payloadLength < kRecordBaseSize) {
      continue;
    }
    const qsizetype recordDataOffset = record.payloadOffset + kRecordBaseSize;
    const qsizetype recordDataLength = record.payloadLength - kRecordBaseSize;

    if (record.type == kRecordTypeGuildMembership) {
      // Guild records are partially documented; expose stable raw fields.
      GuildMembershipEntry entry;
      bool parsedAny = false;
      parsedAny = readLE32U(data, recordDataOffset + 0x00, entry.rankRaw) || parsedAny;
      parsedAny = readLE32U(data, recordDataOffset + 0x03, entry.guildIdRaw) || parsedAny;
      parsedAny = readLE32U(data, recordDataOffset + 0x05, entry.promotionDateRaw) || parsedAny;
      if (parsedAny) {
        m_GuildMemberships.push_back(entry);
      }
      continue;
    }

    if (record.type == kRecordTypeBankAccount) {
      const int entryCount = static_cast<int>(recordDataLength / kBankEntrySize);
      if (entryCount <= 0) {
        continue;
      }
      m_BankRegionCount = qMax(m_BankRegionCount, entryCount);
      for (int i = 0; i < entryCount; ++i) {
        const qsizetype base = recordDataOffset + static_cast<qsizetype>(i) * kBankEntrySize;
        qint32 balance = 0;
        qint32 debt = 0;
        quint32 dueDate = 0;
        quint8 defaultFlag = 0;
        if (!readLE32(data, base + 0x00, balance) ||
            !readLE32(data, base + 0x04, debt) ||
            !readLE32U(data, base + 0x08, dueDate) ||
            !readU8(data, base + 0x0C, defaultFlag)) {
          continue;
        }
        m_BankTotalBalance += static_cast<qint64>(balance);
        m_BankTotalDebt += static_cast<qint64>(debt);
        if (balance > 0) {
          ++m_BankPositiveBalanceRegions;
        }
        if (debt > 0) {
          ++m_BankDebtRegions;
          if (dueDate > 0 &&
              (m_BankEarliestDueDateRaw == 0 || dueDate < m_BankEarliestDueDateRaw)) {
            m_BankEarliestDueDateRaw = dueDate;
          }
        }
        if (defaultFlag != 0) {
          ++m_BankDefaultedRegions;
        }
      }
      continue;
    }

    if (record.type == kRecordTypeSpell) {
      ++m_SpellSummary.count;
      if (recordDataLength >= 0x31) {
        const QString spellName = readFixedString(data, recordDataOffset + 0x2F, 0x14);
        if (!spellName.isEmpty()) {
          m_SpellSummary.names[spellName] += 1;
        }
      }
      if (recordDataLength >= 0x06) {
        for (qsizetype off = 0; off + 1 < 0x06; off += 2) {
          quint16 effect = 0;
          if (readLE16U(data, recordDataOffset + off, effect) && effect != 0xFFFF) {
            ++m_SpellSummary.effectSlotsUsed;
          }
        }
      }
      continue;
    }

    if (record.type == kRecordTypeContainer) {
      ++m_ContainerRecordCount;
      continue;
    }

    if (record.type == kRecordTypeItem) {
      ++m_ItemRecordCount;
      quint32 parentId = 0;
      quint16 category = 0;
      qint32 value1 = 0;
      qint32 value2 = 0;
      quint8 questId = 0;
      QString likelyGroup = "Other";
      readLE32U(data, record.payloadOffset + 39, parentId);
      readU8(data, record.payloadOffset + 38, questId);
      if (questId != 0) {
        ++m_ItemQuestBoundCount;
        likelyGroup = "Quest";
      }
      if (parentId != 0 && containerRecordIds.find(parentId) != containerRecordIds.end()) {
        ++m_ItemInContainerCount;
        if (likelyGroup != "Quest") {
          likelyGroup = "Container";
        }
      } else if (parentId == 0x00075541) {
        likelyGroup = "WeaponsArmor";
      } else if (parentId == 0x00075542) {
        likelyGroup = "Magic";
      } else if (parentId == 0x00075543) {
        likelyGroup = "ClothingMisc";
      } else if (parentId == 0x00075544) {
        likelyGroup = "Ingredient";
      } else if (parentId == 0x00000700) {
        likelyGroup = "Quest";
      }
      if (recordDataLength >= 0x2C) {
        if (readLE16U(data, recordDataOffset + 0x20, category)) {
          m_ItemCategoryCounts[category] += 1;
          m_ItemCategoryLikelyGroups[category][likelyGroup] += 1;
        }
        // Raw value fields documented as two 32-bit slots.
        if (readLE32(data, recordDataOffset + 0x24, value1)) {
          m_ItemTotalValueRaw += static_cast<qint64>(value1);
        }
        if (readLE32(data, recordDataOffset + 0x28, value2)) {
          m_ItemTotalValueRaw += static_cast<qint64>(value2);
        }
      }
      continue;
    }
  }

  if (bestPosition != nullptr) {
    qint32 x = 0;
    quint16 yOffset = 0;
    quint16 yBase = 0;
    qint32 z = 0;
    if (readLE32(data, bestPosition->payloadOffset + 7, x) &&
        readLE16U(data, bestPosition->payloadOffset + 11, yOffset) &&
        readLE16U(data, bestPosition->payloadOffset + 13, yBase) &&
        readLE32(data, bestPosition->payloadOffset + 15, z)) {
      if (m_PCLocation.isEmpty()) {
        m_PCLocation = formatPositionText(x, yOffset, yBase, z);
      }
    }
  }

  return true;
}

bool DaggerfallsSaveGame::parseSaveVars()
{
  QFile f(saveFilePath("SAVEVARS.DAT"));
  if (!f.open(QIODevice::ReadOnly)) {
    return false;
  }
  const QByteArray data = f.readAll();
  if (data.size() < 0x3cd) {
    return false;
  }

  constexpr qint32 kSaveVarsTimestampOffset = 0x3c9;
  constexpr qint32 kSaveVarsFactionTableOffset = 0x17D0;
  constexpr qint32 kSaveVarsFactionRecordSize = 92;
  constexpr qint32 kSaveVarsRegionalRepOffset = 0x424;
  constexpr qint32 kSaveVarsRegionalRepRecordSize = 80;
  constexpr qint32 kExpectedRegionalCount = 62;
  m_SaveVarsTimestampOffset = kSaveVarsTimestampOffset;
  m_TopFactionReputations.clear();
  m_MinRegionalReputation = 0;
  m_MaxRegionalReputation = 0;

  if (data.size() >= kSaveVarsFactionTableOffset + kSaveVarsFactionRecordSize) {
    m_SaveVarsFactionRecordCountEstimate =
        static_cast<int>((data.size() - kSaveVarsFactionTableOffset) / kSaveVarsFactionRecordSize);
  }
  if (data.size() >= kSaveVarsRegionalRepOffset + kSaveVarsRegionalRepRecordSize * kExpectedRegionalCount) {
    m_SaveVarsRegionalRepRecordCountEstimate = kExpectedRegionalCount;
  } else if (data.size() >= kSaveVarsRegionalRepOffset + kSaveVarsRegionalRepRecordSize) {
    m_SaveVarsRegionalRepRecordCountEstimate =
        static_cast<int>((data.size() - kSaveVarsRegionalRepOffset) / kSaveVarsRegionalRepRecordSize);
  }

  if (m_SaveVarsRegionalRepRecordCountEstimate > 0) {
    bool initialized = false;
    for (int i = 0; i < m_SaveVarsRegionalRepRecordCountEstimate; ++i) {
      const qsizetype recOffset = kSaveVarsRegionalRepOffset + i * kSaveVarsRegionalRepRecordSize;
      if (recOffset >= data.size()) {
        break;
      }
      const qint8 rep = static_cast<qint8>(data.at(recOffset));
      if (!initialized) {
        m_MinRegionalReputation = rep;
        m_MaxRegionalReputation = rep;
        initialized = true;
      } else {
        m_MinRegionalReputation = qMin(m_MinRegionalReputation, rep);
        m_MaxRegionalReputation = qMax(m_MaxRegionalReputation, rep);
      }
    }
  }

  if (m_SaveVarsFactionRecordCountEstimate > 0) {
    QList<FactionReputationEntry> factionRows;
    factionRows.reserve(m_SaveVarsFactionRecordCountEstimate);
    for (int i = 0; i < m_SaveVarsFactionRecordCountEstimate; ++i) {
      const qsizetype recOffset = kSaveVarsFactionTableOffset + i * kSaveVarsFactionRecordSize;
      if (recOffset + kSaveVarsFactionRecordSize > data.size()) {
        break;
      }

      FactionReputationEntry row;
      row.type = static_cast<quint8>(data.at(recOffset + 0x00));
      row.region = static_cast<quint8>(data.at(recOffset + 0x01));
      row.name = readFixedString(data, recOffset + 0x03, 0x1A);

      quint16 repU16 = 0;
      quint16 idU16 = 0;
      readLE16U(data, recOffset + 0x1D, repU16);
      readLE16U(data, recOffset + 0x21, idU16);
      row.reputation = static_cast<qint16>(repU16);
      row.factionId = static_cast<qint16>(idU16);

      factionRows.push_back(row);
    }

    std::sort(factionRows.begin(), factionRows.end(),
              [](const FactionReputationEntry& a, const FactionReputationEntry& b) {
                const int aa = std::abs(static_cast<int>(a.reputation));
                const int bb = std::abs(static_cast<int>(b.reputation));
                if (aa != bb) {
                  return aa > bb;
                }
                return a.factionId < b.factionId;
              });

    constexpr int kTopFactionCount = 6;
    for (int i = 0; i < factionRows.size() && i < kTopFactionCount; ++i) {
      m_TopFactionReputations.push_back(factionRows.at(i));
    }
  }

  quint32 timestamp = 0;
  if (!readLE32U(data, kSaveVarsTimestampOffset, timestamp) || timestamp == 0) {
    return false;
  }
  m_SaveVarsTimestampRaw = timestamp;

  // Prefer character timestamp if available, fallback to SAVEVARS timestamp.
  if (m_InGameDate.isEmpty()) {
    m_InGameDate = formatDaggerfallDate(timestamp);
  }
  return true;
}

bool DaggerfallsSaveGame::readLE32(const QByteArray& data, qsizetype offset,
                                   qint32& value)
{
  if (offset < 0 || offset + static_cast<qsizetype>(sizeof(qint32)) > data.size()) {
    return false;
  }

  qint32 tmp = 0;
  std::memcpy(&tmp, data.constData() + offset, sizeof(tmp));
  value = qFromLittleEndian(tmp);
  return true;
}

bool DaggerfallsSaveGame::readLE32U(const QByteArray& data, qsizetype offset,
                                    quint32& value)
{
  if (offset < 0 || offset + static_cast<qsizetype>(sizeof(quint32)) > data.size()) {
    return false;
  }

  quint32 tmp = 0;
  std::memcpy(&tmp, data.constData() + offset, sizeof(tmp));
  value = qFromLittleEndian(tmp);
  return true;
}

bool DaggerfallsSaveGame::readLE16U(const QByteArray& data, qsizetype offset,
                                    quint16& value)
{
  if (offset < 0 || offset + static_cast<qsizetype>(sizeof(quint16)) > data.size()) {
    return false;
  }

  quint16 tmp = 0;
  std::memcpy(&tmp, data.constData() + offset, sizeof(tmp));
  value = qFromLittleEndian(tmp);
  return true;
}

bool DaggerfallsSaveGame::readU8(const QByteArray& data, qsizetype offset, quint8& value)
{
  if (offset < 0 || offset >= data.size()) {
    return false;
  }
  value = static_cast<quint8>(data.at(offset));
  return true;
}

std::vector<DaggerfallsSaveGame::ParsedRecord> DaggerfallsSaveGame::parseRecordStream(
    const QByteArray& data, qsizetype startOffset, qsizetype* endOffset,
    RecordParseStats* stats)
{
  std::vector<ParsedRecord> records;
  qsizetype pos = startOffset;

  while (pos + 4 <= data.size()) {
    qint32 recordLength = 0;
    if (!readLE32(data, pos, recordLength)) {
      break;
    }
    if (recordLength < 0) {
      if (stats != nullptr) {
        ++stats->negativeLengths;
      }
      break;
    }

    if (recordLength == 0) {
      if (stats != nullptr) {
        ++stats->zeroLengthSeparators;
      }
      pos += 4;
      continue;
    }

    if (pos + 4 + recordLength > data.size()) {
      if (stats != nullptr) {
        ++stats->truncatedRecords;
      }
      break;
    }

    const qsizetype payloadOffset = pos + 4;
    qsizetype payloadLength = recordLength;
    const quint8 recordType = static_cast<quint8>(data.at(payloadOffset));

    // Daggerfall's dungeon-information records report compressed length units.
    if (recordType == kRecordTypeDungeonInformation) {
      const qsizetype correctedLength = payloadLength * 39;
      if (payloadLength <= 0 || pos + 4 + correctedLength > data.size()) {
        if (stats != nullptr) {
          ++stats->invalidDungeonLength;
        }
        break;
      }
      payloadLength = correctedLength;
      if (stats != nullptr) {
        ++stats->dungeonLengthAdjusted;
      }
    }

    records.push_back({recordType, payloadOffset, payloadLength});
    pos += 4 + payloadLength;
  }

  if (endOffset != nullptr) {
    *endOffset = pos;
  }

  return records;
}

std::vector<DaggerfallsSaveGame::ParsedRecord> DaggerfallsSaveGame::findBestRecordStream(
    const QByteArray& data, qsizetype* startOffset, qsizetype* endOffset,
    RecordParseStats* stats)
{
  std::vector<ParsedRecord> bestRecords;
  qsizetype bestStart = 0;
  qsizetype bestEnd = 0;
  RecordParseStats bestStats;
  int bestScore = -1;

  // Header and location-detail are before the record stream. Probe a small window.
  const qsizetype maxProbe = std::min<qsizetype>(2048, std::max<qsizetype>(0, data.size() - 4));
  for (qsizetype probe = 0; probe <= maxProbe; ++probe) {
    qsizetype end = 0;
    RecordParseStats probeStats;
    auto records = parseRecordStream(data, probe, &end, &probeStats);
    if (records.empty()) {
      continue;
    }

    int score = static_cast<int>(records.size());
    if (end >= data.size() - 8) {
      score += 1000;
    }

    bool hasCharacterPos = false;
    for (const auto& r : records) {
      if (r.type == kRecordTypeCharacterPosition) {
        hasCharacterPos = true;
        break;
      }
    }
    if (hasCharacterPos) {
      score += 50;
    }

    if (score > bestScore) {
      bestScore = score;
      bestStart = probe;
      bestEnd = end;
      bestStats = probeStats;
      bestRecords = std::move(records);
    }
  }

  if (startOffset != nullptr) {
    *startOffset = bestStart;
  }
  if (endOffset != nullptr) {
    *endOffset = bestEnd;
  }
  if (stats != nullptr) {
    *stats = bestStats;
  }

  return bestRecords;
}

QString DaggerfallsSaveGame::formatPositionText(qint32 x, quint16 yOffset, quint16 yBase,
                                                qint32 z)
{
  return QString("Position X %1, YOff %2, YBase %3, Z %4")
      .arg(x)
      .arg(yOffset)
      .arg(yBase)
      .arg(z);
}

QString DaggerfallsSaveGame::formatHeaderLocationText(quint16 locationCode, quint8 zoneType,
                                                      qint32 x, qint32 y, qint32 z)
{
  Q_UNUSED(y);
  const auto nearest = DaggerfallMapsBsa::resolveNearestLocation(m_Game, x, z);
  const auto info = nearest.isValid() ? nearest
                                      : DaggerfallMapsBsa::resolveLocationInfo(m_Game, locationCode);
  if (info.isValid()) {
    m_LocationNameDetail = info.name;
    m_LocationTypeIndexDetail = info.locationType;
    m_LocationTypeNameDetail = DaggerfallMapsBsa::locationTypeName(info.locationType);
    m_LocationRegionNameDetail = DaggerfallMapsBsa::regionName(info.regionIndex);
    m_LocationTypeColorDetail = QColor();
    tryGetDaggerfallLocationTypeColor(m_Game, info.locationType, m_LocationTypeColorDetail);

    const QString typeName = DaggerfallMapsBsa::locationTypeName(info.locationType);
    const QString regionName = DaggerfallMapsBsa::regionName(info.regionIndex);
    if (!typeName.isEmpty() && !regionName.isEmpty()) {
      return QString("%1 (%2, %3)")
          .arg(info.name)
          .arg(typeName, regionName);
    }
    if (!typeName.isEmpty()) {
      return QString("%1 (%2)").arg(info.name, typeName);
    }
    if (!regionName.isEmpty()) {
      return QString("%1 (%2)").arg(info.name, regionName);
    }
    return info.name;
  }

  m_LocationNameDetail.clear();
  m_LocationTypeNameDetail.clear();
  m_LocationRegionNameDetail.clear();
  m_LocationTypeIndexDetail = -1;
  m_LocationTypeColorDetail = QColor();

  return QString("Location 0x%1 (Zone %2)")
      .arg(locationCode, 4, 16, QChar('0'))
      .arg(zoneType);
}

QString DaggerfallsSaveGame::formatDaggerfallDate(quint32 minutes)
{
  static const QStringList months = {"Morning Star", "Sun's Dawn", "First Seed",
                                     "Rain's Hand", "Second Seed", "Mid Year",
                                     "Sun's Height", "Last Seed", "Hearthfire",
                                     "Frost Fall", "Sun's Dusk", "Evening Star"};

  constexpr quint32 minsPerHour = 60;
  constexpr quint32 minsPerDay = 24 * minsPerHour;
  constexpr quint32 minsPerMonth = 30 * minsPerDay;
  constexpr quint32 minsPerYear = 12 * minsPerMonth;

  quint32 remaining = minutes;
  const quint32 year = 404 + (remaining / minsPerYear);
  remaining %= minsPerYear;
  const quint32 month = remaining / minsPerMonth;
  remaining %= minsPerMonth;
  const quint32 day = 1 + (remaining / minsPerDay);
  remaining %= minsPerDay;
  const quint32 hour = remaining / minsPerHour;
  const quint32 minute = remaining % minsPerHour;

  const QString monthName =
      (month < static_cast<quint32>(months.size())) ? months.at(static_cast<int>(month))
                                                    : QString("Month %1").arg(month + 1);
  return QString("%1 %2, 3E %3 %4:%5")
      .arg(monthName)
      .arg(day)
      .arg(year)
      .arg(hour, 2, 10, QChar('0'))
      .arg(minute, 2, 10, QChar('0'));
}

QStringList DaggerfallsSaveGame::decodeElementMask(quint8 mask)
{
  static const QList<QPair<quint8, QString>> bits = {
      {0x01, "Paralysis"}, {0x02, "Magic"},   {0x04, "Poison"}, {0x08, "Fire"},
      {0x10, "Frost"},     {0x40, "Shock"},   {0x80, "Disease"}};
  QStringList out;
  for (const auto& bit : bits) {
    if (mask & bit.first) {
      out.push_back(bit.second);
    }
  }
  if (out.isEmpty()) {
    out.push_back("None");
  }
  return out;
}

QStringList DaggerfallsSaveGame::decodeRapidHealMask(quint8 mask)
{
  static const QList<QPair<quint8, QString>> bits = {
      {0x01, "InLight"}, {0x02, "InDark"}, {0x04, "General"}};
  QStringList out;
  for (const auto& bit : bits) {
    if (mask & bit.first) {
      out.push_back(bit.second);
    }
  }
  if (out.isEmpty()) {
    out.push_back("None");
  }
  return out;
}

QStringList DaggerfallsSaveGame::decodeRegenHealthMask(quint8 mask)
{
  static const QList<QPair<quint8, QString>> bits = {
      {0x01, "InLight"}, {0x02, "InDark"}, {0x04, "InWater"}, {0x08, "General"}};
  QStringList out;
  for (const auto& bit : bits) {
    if (mask & bit.first) {
      out.push_back(bit.second);
    }
  }
  if (out.isEmpty()) {
    out.push_back("None");
  }
  return out;
}

QString DaggerfallsSaveGame::decodeSpellAbsorbMask(quint8 mask)
{
  if (mask == 0x00) {
    return "None";
  }
  if (mask == 0x01) {
    return "InLight";
  }
  if (mask == 0x02) {
    return "InDark";
  }
  if (mask == 0x04) {
    return "General";
  }
  return QString("Mixed(0x%1)").arg(mask, 0, 16);
}

QStringList DaggerfallsSaveGame::decodeHitPhobiaMask(quint8 mask)
{
  static const QList<QPair<quint8, QString>> bits = {
      {0x01, "HitUndead"},   {0x02, "HitDaedra"},   {0x04, "HitHuman"},
      {0x08, "HitAnimals"},  {0x10, "PhobiaUndead"},{0x20, "PhobiaDaedra"},
      {0x40, "PhobiaHuman"}, {0x80, "PhobiaAnimals"}};
  QStringList out;
  for (const auto& bit : bits) {
    if (mask & bit.first) {
      out.push_back(bit.second);
    }
  }
  if (out.isEmpty()) {
    out.push_back("None");
  }
  return out;
}

QStringList DaggerfallsSaveGame::decodeWeaponExpertMask(quint8 mask)
{
  static const QList<QPair<quint8, QString>> bits = {
      {0x01, "ShortBlade"}, {0x02, "LongBlade"}, {0x04, "HandToHand"},
      {0x08, "Axe"},        {0x10, "Blunt"},     {0x20, "Missile"}};
  QStringList out;
  for (const auto& bit : bits) {
    if (mask & bit.first) {
      out.push_back(bit.second);
    }
  }
  if (out.isEmpty()) {
    out.push_back("None");
  }
  return out;
}

QStringList DaggerfallsSaveGame::decodeForbiddenMaterialMask(quint16 mask)
{
  static const QList<QPair<quint16, QString>> bits = {
      {0x0001, "Iron"},     {0x0002, "Steel"},    {0x0004, "Silver"},
      {0x0008, "Elven"},    {0x0010, "Dwarven"},  {0x0020, "Mithril"},
      {0x0040, "Adamantium"},{0x0080, "Ebony"},   {0x0100, "Orcish"},
      {0x0200, "Daedric"}};
  QStringList out;
  for (const auto& bit : bits) {
    if (mask & bit.first) {
      out.push_back(bit.second);
    }
  }
  if (out.isEmpty()) {
    out.push_back("None");
  }
  return out;
}

QStringList DaggerfallsSaveGame::decodeForbiddenArmorMask(quint16 mask)
{
  static const QList<QPair<quint16, QString>> bits = {
      {0x0001, "ShortBlade"}, {0x0002, "LongBlade"}, {0x0004, "HandToHand"},
      {0x0008, "Axe"},        {0x0010, "Blunt"},     {0x0020, "Missile"},
      {0x0040, "Leather"},    {0x0080, "Chain"},     {0x0100, "Plate"},
      {0x0200, "Buckler"},    {0x0400, "RoundShield"},{0x0800, "KiteShield"},
      {0x1000, "TowerShield"}};
  QStringList out;
  for (const auto& bit : bits) {
    if (mask & bit.first) {
      out.push_back(bit.second);
    }
  }
  if (out.isEmpty()) {
    out.push_back("None");
  }
  return out;
}

QString DaggerfallsSaveGame::computeHealthStatus(int anomalies, int orphans,
                                                 int parentTypeMismatches)
{
  if (anomalies > 0) {
    return "Corrupt-risk";
  }
  if (orphans > 0 || parentTypeMismatches > 0) {
    return "Warning";
  }
  return "OK";
}

QString DaggerfallsSaveGame::recordTypeName(quint8 type)
{
  switch (type) {
    case 0x01: return "CharacterPosition";
    case 0x02: return "Item";
    case 0x03: return "Character";
    case 0x04: return "CharacterParent";
    case 0x05: return "CharacterParent";
    case 0x06: return "Unknown06";
    case 0x07: return "DungeonInformation";
    case 0x08: return "Unknown08";
    case 0x09: return "Spell";
    case 0x0A: return "GuildMembership";
    case 0x0E: return "QuestBinary";
    case 0x10: return "QuestParent";
    case 0x12: return "QuestParent";
    case 0x16: return "SpellcastingCreatureList";
    case 0x17: return "ControlSetting";
    case 0x18: return "LocationNameLogbook";
    case 0x19: return "BankAccount";
    case 0x1F: return "PotionMixing";
    case 0x20: return "UnknownTownLinked";
    case 0x21: return "UnknownDungeon";
    case 0x22: return "Mobile1";
    case 0x24: return "StoreShelfItem?";
    case 0x27: return "MissingReferenced";
    case 0x28: return "Destination";
    case 0x29: return "Destination";
    case 0x2B: return "MissingReferenced";
    case 0x2C: return "Mobile2";
    case 0x2D: return "NPC";
    case 0x2E: return "GenericNPC";
    case 0x33: return "DungeonRecord?";
    case 0x34: return "Container";
    case 0x36: return "Repair";
    case 0x40: return "UnknownContainerChild";
    case 0x41: return "QuestInfo?";
    default: return QString("Unknown0x%1").arg(type, 2, 16, QChar('0')).toUpper();
  }
}

QString DaggerfallsSaveGame::raceName(quint8 race)
{
  switch (race) {
    case 0: return "Breton";
    case 1: return "Redguard";
    case 2: return "Nord";
    case 3: return "Dark Elf";
    case 4: return "High Elf";
    case 5: return "Wood Elf";
    case 6: return "Khajiit";
    case 7: return "Argonian";
    case 8: return "Vampire";
    case 9: return "Werewolf";
    case 10: return "Wereboar";
    default: return {};
  }
}

bool DaggerfallsSaveGame::isLikelyClassName(const QString& value)
{
  const QString trimmed = value.trimmed();
  if (trimmed.isEmpty() || trimmed.size() > 32) {
    return false;
  }

  bool hasLetter = false;
  for (const QChar c : trimmed) {
    if (c.isLetter()) {
      hasLetter = true;
      continue;
    }
    if (c == ' ' || c == '\'' || c == '-') {
      continue;
    }
    return false;
  }

  return hasLetter;
}

QString DaggerfallsSaveGame::reflexName(quint8 reflex)
{
  switch (reflex) {
    case 0: return "Very Low";
    case 1: return "Low";
    case 2: return "Average";
    case 3: return "High";
    case 4: return "Very High";
    default: return {};
  }
}

QString DaggerfallsSaveGame::readFixedString(const QByteArray& data, qsizetype offset,
                                             qsizetype size)
{
  if (offset < 0 || size <= 0 || offset + size > data.size()) {
    return {};
  }

  QByteArray bytes = data.mid(offset, size);
  const int nullPos = bytes.indexOf('\0');
  if (nullPos >= 0) {
    bytes.truncate(nullPos);
  }
  return QString::fromLocal8Bit(bytes).trimmed();
}

QString DaggerfallsSaveGame::saveFilePath(const QString& fileName) const
{
  return QDir(m_SaveFolder).filePath(fileName);
}
