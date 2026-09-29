#include "battlespiresavegame.h"

#include "battlespiresaveformat.h"
#include "gamebattlespire.h"
#include "xnginepaletteformat.h"
#include "xnginerecordgraph.h"

#include <QColor>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QRegularExpression>
#include <QStringList>
#include <QtEndian>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace {
constexpr qsizetype kSaveNameLength = 32;
constexpr qsizetype kImageWidth = 80;
constexpr qsizetype kImageHeight = 50;
constexpr qsizetype kImageBytesPerPixel8 = 1;
constexpr qsizetype kImageBytesPerPixel = 2;
constexpr qsizetype kImageRawSize8 = kImageWidth * kImageHeight * kImageBytesPerPixel8;
constexpr qsizetype kImageRawSize = kImageWidth * kImageHeight * kImageBytesPerPixel;
constexpr bool kLogSaveParsing = false;

int expectedRecordLength(quint8 type)
{
  // SAVETREE "RecordLength" excludes its own 4-byte length field.
  switch (type) {
    case 2: return 816;
    case 3: return 852;
    case 6: return 172;
    case 7: return 62;
    case 9: return 180;
    case 18: return 852;
    case 23: return 66;
    case 51: return 1266;
    case 66: return 510;
    default: return -1;
  }
}

bool hasRange(const QByteArray& data, qsizetype offset, qsizetype size)
{
  if (offset < 0 || size < 0 || offset > data.size()) {
    return false;
  }
  return size <= data.size() - offset;
}

template <typename T>
bool readLE(const QByteArray& data, qsizetype offset, T& value)
{
  if (!hasRange(data, offset, static_cast<qsizetype>(sizeof(T)))) {
    return false;
  }

  T tmp{};
  std::memcpy(&tmp, data.constData() + offset, sizeof(T));
  value = qFromLittleEndian(tmp);
  return true;
}

bool readF32LE(const QByteArray& data, qsizetype offset, float& value)
{
  quint32 bits = 0;
  if (!readLE(data, offset, bits)) {
    return false;
  }
  static_assert(sizeof(float) == sizeof(quint32));
  std::memcpy(&value, &bits, sizeof(float));
  return true;
}

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
  palette[0].setAlpha(255);
  return true;
}

bool loadBattlespirePalette(const GameBattlespire* game, std::array<QColor, 256>& palette)
{
  if (game == nullptr) {
    return false;
  }

  const QDir gameData(game->gameDirectory().filePath("GAMEDATA"));
  const QStringList candidates = {
      "PAL.RAW",
      "LOWCOLOR.COL",
      "PAL.COL",
  };

  for (const auto& name : candidates) {
    const QString filePath = gameData.filePath(name);
    if (loadPaletteFromFile(filePath, palette)) {
      return true;
    }
  }
  return false;
}

QStringList decodeBitFlags(quint32 mask, const QStringList& names)
{
  QStringList out;
  for (int bit = 0; bit < names.size() && bit < 32; ++bit) {
    if (mask & (1u << bit)) {
      const auto& name = names.at(bit);
      if (!name.isEmpty()) {
        out.push_back(name);
      }
    }
  }
  return out;
}

QStringList activeSpellNamesFromMask(quint32 mask)
{
  static const QStringList kSpellArray = {
      "Unused",         "Monster Summoning", "Detect Spell",     "Detect Enemy",
      "Detect Invis",   "Invisibility",      "Shadow",           "Chameleon",
      "Slow Fall",      "Continuous Damage", "Poison",           "Confusion",
      "Vampiric Drain", "Delayed Damage",    "Dispel Magic",     "Spell Reflect",
      "Spell Resist",   "Spell Absorption",  "Cause Damage",     "Fire Shield",
      "Cure Poison",    "Cure Health",       "Jumping",          "Running",
      "Etherealness",   "Teleport",          "Shield",           "Resistance",
      "Slow",           "Haste",             "Strength",         "Dispel Sigil",
  };
  return decodeBitFlags(mask, kSpellArray);
}

QStringList characterFlagNamesFromMask(quint32 mask)
{
  // Only expose known/useful names; unknown bits remain in raw mask.
  static const QStringList kCharacterFlags = {
      "",          "",          "Detected",   "IsFemale",
      "",          "",          "Sneaking",   "CursorMode",
      "",          "",          "LanternOn",  "IsFemale2",
      "",          "",          "",           "",
      "",          "",          "",           "",
      "",          "",          "",           "",
      "",          "",          "",           "",
      "",          "",          "",           "",
  };
  return decodeBitFlags(mask, kCharacterFlags);
}
}  // namespace

BattlespireSaveGame::BattlespireSaveGame(QString const& folder,
                                         GameBattlespire const* game)
    : XngineSaveGame(folder, game), m_SaveFolder(folder)
{
  QFileInfo info(folder);
  m_DisplayName = info.fileName();

  resetValidationState();
  parseSaveName();
  parseSaveTree();
  parseSaveVars();
  m_HasImage = QFileInfo::exists(saveFilePath("IMAGE.RAW"));
  evaluateDeveloperValidation();
  m_IsEmptySlot = !hasAnySavePayload();

  if (m_PCName.isEmpty() && !m_DisplayName.isEmpty() &&
      !m_DisplayName.startsWith("SAVE", Qt::CaseInsensitive)) {
    m_PCName = m_DisplayName;
  }

  const QRegularExpression saveSlotRegex("(?i)^SAVE(\\d+)$");
  const QRegularExpressionMatch slotMatch = saveSlotRegex.match(info.fileName());
  if (slotMatch.hasMatch()) {
    bool ok = false;
    const int slot = slotMatch.captured(1).toInt(&ok);
    if (ok && slot >= 0) {
      m_SaveNumber = static_cast<unsigned long>(slot);
    }
  }
}

QString BattlespireSaveGame::getName() const
{
  const QString slotLabel = QString("SAVE%1").arg(m_SaveNumber);
  if (!m_DisplayName.isEmpty() &&
      !m_DisplayName.startsWith("SAVE", Qt::CaseInsensitive)) {
    return QString("%1 - %2").arg(slotLabel, m_DisplayName);
  }
  return slotLabel;
}

QString BattlespireSaveGame::getSaveGroupIdentifier() const
{
  return {};
}

QString BattlespireSaveGame::getGameDetails() const
{
  const bool kShowDeveloperDetails =
      (static_cast<const GameBattlespire*>(m_Game) != nullptr)
          ? static_cast<const GameBattlespire*>(m_Game)->showDeveloperSaveDetails()
          : false;

  QStringList lines;

  const QString race = raceName(m_Race);
  if (!race.isEmpty()) {
    lines.push_back(QString("Race: %1").arg(race));
  }
  if (!m_ClassName.isEmpty()) {
    lines.push_back(QString("Class: %1").arg(m_ClassName));
  }
  if (m_WoundsMax > 0) {
    lines.push_back(QString("HP: %1/%2").arg(m_Wounds).arg(m_WoundsMax));
  }
  if (m_SpellPointsMax > 0) {
    lines.push_back(QString("MP: %1/%2").arg(m_SpellPoints).arg(m_SpellPointsMax));
  }
  lines.push_back(QString("Gold: %1").arg(m_Gold));

  if (kShowDeveloperDetails && !m_IsEmptySlot) {
    lines.push_back("");
    lines.push_back("[Developer Details]");
    lines.push_back(QString("Save Validation: %1")
                        .arg(m_ValidationLikelyModified ? "Has structural anomalies"
                                                        : "No structural anomalies detected"));
    if (m_ValidationLikelyModified) {
      constexpr int kMaxNotes = 5;
      const int shown =
          (std::min<int>)(kMaxNotes, static_cast<int>(m_ValidationNotes.size()));
      for (int i = 0; i < shown; ++i) {
        lines.push_back(QString(" - %1").arg(m_ValidationNotes.at(i)));
      }
      if (m_ValidationNotes.size() > shown) {
        lines.push_back(QString(" - ... %1 more").arg(m_ValidationNotes.size() - shown));
      }
    }
    if (m_SaveTreeTailBytes > 0) {
      lines.push_back("Parse Notes:");
      lines.push_back(
          QString(" - SAVETREE trailing section present (%1 bytes)").arg(m_SaveTreeTailBytes));
    }
    lines.push_back(QString("Payload Files: SAVENAME=%1, SAVETREE=%2, SAVEVARS=%3, IMAGE=%4")
                        .arg(m_HasSaveName ? "yes" : "no")
                        .arg(m_HasSaveTree ? "yes" : "no")
                        .arg(m_HasSaveVars ? "yes" : "no")
                        .arg(m_HasImage ? "yes" : "no"));
    lines.push_back(QString("SAVETREE Header Version: %1").arg(m_SaveTreeVersion));
    lines.push_back(QString("SAVETREE Record Count: %1").arg(m_RecordCountTotal));
    if (!m_RecordTypeCounts.isEmpty()) {
      QStringList typeCounts;
      const QList<int> keys = m_RecordTypeCounts.keys();
      for (const auto type : keys) {
        typeCounts.push_back(QString("T%1=%2").arg(type).arg(m_RecordTypeCounts.value(type)));
      }
      lines.push_back(QString("SAVETREE Types: %1").arg(typeCounts.join(", ")));
    }
    lines.push_back(
        QString("Player Record Found: %1 (canonical=%2, type=%3, id=%4, recovery=%5)")
            .arg(m_PlayerRecordFound ? "yes" : "no")
            .arg(m_PlayerRecordCanonicalFound ? "yes" : "no")
            .arg(m_PlayerRecordByTypeFound ? "yes" : "no")
            .arg(m_PlayerRecordByIdFound ? "yes" : "no")
            .arg(m_PlayerRecordRecoveryUsed ? "yes" : "no"));
    lines.push_back(QString("Gold Scan: total=%1 from %2 player-owned item records")
                        .arg(m_GoldAccumulator)
                        .arg(m_GoldItemRecordCount));
    lines.push_back(QString("Current Map ID: %1 (offset %2)")
                        .arg(m_CurrentLevelId)
                        .arg(m_CurrentLevelOffset >= 0 ? QString::number(m_CurrentLevelOffset)
                                                       : QString("?")));
    if (m_CurrentTimestamp > 0) {
      lines.push_back(QString("Current Timestamp: %1").arg(m_CurrentTimestamp));
    }
    lines.push_back(QString("Active Spells Mask: 0x%1")
                        .arg(m_ActiveSpellsMask, 8, 16, QChar('0')).toUpper());
    if (!m_ActiveSpellNames.isEmpty()) {
      constexpr int kMaxSpellNames = 8;
      QStringList shown = m_ActiveSpellNames.mid(0, kMaxSpellNames);
      QString suffix;
      if (m_ActiveSpellNames.size() > kMaxSpellNames) {
        suffix = QString(" (+%1 more)").arg(m_ActiveSpellNames.size() - kMaxSpellNames);
      }
      lines.push_back(QString("Active Spells: %1%2").arg(shown.join(", ")).arg(suffix));
    }
    lines.push_back(QString("Character Flags: 0x%1")
                        .arg(m_CharacterFlagsMask, 8, 16, QChar('0')).toUpper());
    if (!m_CharacterFlagNames.isEmpty()) {
      lines.push_back(QString("Character Flags Set: %1").arg(m_CharacterFlagNames.join(", ")));
    }
    lines.push_back(QString("AI Team/Goal: %1 / %2").arg(m_TeamValue).arg(m_GoalValue));
    lines.push_back(QString("SAVEVARS Summary: ConversationMap=%1, StaticEnemy=%2, GlobalVars=%3, LocalVars=%4")
                        .arg(m_ConversationMapCount)
                        .arg(m_StaticEnemyCount)
                        .arg(m_GlobalVariableCount)
                        .arg(m_LocalVariableCount));
    lines.push_back(QString("MonsterTypeCount: non-zero=%1, total=%2")
                        .arg(m_MonsterTypeCountNonZero)
                        .arg(m_MonsterTypeCountTotal));
    if (m_SaveTreeTailBytes > 0) {
      lines.push_back(QString("SAVETREE Trailing Bytes: %1").arg(m_SaveTreeTailBytes));
    }
  }

  return lines.join('\n');
}

std::unique_ptr<XngineSaveGame::DataFields> BattlespireSaveGame::fetchDataFields() const
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
  if (raw.size() >= kImageRawSize) {
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
    std::array<QColor, 256> palette{};
    const bool hasPalette =
        loadBattlespirePalette(static_cast<const GameBattlespire*>(m_Game), palette);
    for (qsizetype y = 0; y < kImageHeight; ++y) {
      for (qsizetype x = 0; x < kImageWidth; ++x) {
        const qsizetype i = y * kImageWidth + x;
        const int v = static_cast<unsigned char>(ptr[i]);
        image.setPixelColor(static_cast<int>(x), static_cast<int>(y),
                            hasPalette ? palette[static_cast<size_t>(v)] : QColor(v, v, v));
      }
    }
  }

  fields->Screenshot = image;
  return fields;
}

void BattlespireSaveGame::resetValidationState()
{
  m_ValidationLikelyModified = false;
  m_ValidationNotes.clear();
  m_PlayerRecordFound = false;
  m_PlayerRecordByTypeFound = false;
  m_PlayerRecordByIdFound = false;
  m_PlayerRecordCanonicalFound = false;
  m_PlayerRecordRecoveryUsed = false;
  m_SaveTreeTailBytes = 0;
}

bool BattlespireSaveGame::parseSaveName()
{
  QFile saveNameFile(saveFilePath("SAVENAME.DAT"));
  if (!saveNameFile.open(QIODevice::ReadOnly)) {
    return false;
  }
  m_HasSaveName = true;

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

bool BattlespireSaveGame::parseSaveTree()
{
  using namespace BattlespireSaveFormat;

  QFile saveTreeFile(saveFilePath("SAVETREE.DAT"));
  if (!saveTreeFile.open(QIODevice::ReadOnly)) {
    return false;
  }
  m_HasSaveTree = true;

  const QByteArray data = saveTreeFile.readAll();
  if (data.size() < 8) {
    m_ValidationNotes.push_back("SAVETREE.DAT is too small to contain records");
    return false;
  }

  readLE(data, 0, m_SaveTreeVersion);
  qsizetype pos = 4;  // 4-byte file version/header
  quint64 goldTotal = 0;
  int selectedPlayerRank = 0;
  int canonicalPlayerCount = 0;
  XngineRecordGraph recordGraph;

  struct GoldCandidate
  {
    quint32 recordId = 0;
    quint32 parentId = 0;
    quint32 quantity = 0;
  };
  QVector<GoldCandidate> goldCandidates;

  m_Gold = 0;
  m_RecordCountTotal = 0;
  m_RecordTypeCounts.clear();
  m_PlayerRecordByTypeFound = false;
  m_PlayerRecordByIdFound = false;
  m_GoldItemRecordCount = 0;
  m_GoldAccumulator = 0;

  auto parsePlayerRecord = [&](qsizetype recordPos, qsizetype recordEnd) {
    auto hasBytes = [recordEnd](qsizetype at, qsizetype size) {
      return at >= 0 && size >= 0 && at <= recordEnd && size <= recordEnd - at;
    };

    if (hasBytes(recordPos + PlayerTreeOffset::Name, 32)) {
      const QString name = readFixedString(data, recordPos + PlayerTreeOffset::Name, 32);
      if (!name.isEmpty()) {
        m_PCName = name;
      }
    }

    float posX = 0.0F;
    float posY = 0.0F;
    float posZ = 0.0F;
    if (hasBytes(recordPos + 11, 12) && readF32LE(data, recordPos + 11, posX) &&
        readF32LE(data, recordPos + 15, posY) && readF32LE(data, recordPos + 19, posZ) &&
        std::isfinite(posX) && std::isfinite(posY) && std::isfinite(posZ)) {
      m_PositionText = QString("X %1, Y %2, Z %3")
                           .arg(QString::number(posX, 'f', 2))
                           .arg(QString::number(posY, 'f', 2))
                           .arg(QString::number(posZ, 'f', 2));
    }

    quint32 level = 0;
    if (hasBytes(recordPos + PlayerTreeOffset::Level, 4) &&
        readLE(data, recordPos + PlayerTreeOffset::Level, level) && level > 0 && level < 200) {
      m_PCLevel = static_cast<unsigned short>(level);
    }

    qint32 wounds = 0;
    qint32 woundsMax = 0;
    quint16 sp = 0;
    quint16 spMax = 0;
    if (hasBytes(recordPos + PlayerTreeOffset::SpellPoints, 14) &&
        readLE(data, recordPos + PlayerTreeOffset::Wounds, wounds) &&
        readLE(data, recordPos + PlayerTreeOffset::WoundsMax, woundsMax) &&
        readLE(data, recordPos + PlayerTreeOffset::SpellPoints, sp) &&
        readLE(data, recordPos + PlayerTreeOffset::SpellPointsMax, spMax) && woundsMax >= 0) {
      m_Wounds = wounds;
      m_WoundsMax = woundsMax;
      m_SpellPoints = sp;
      m_SpellPointsMax = spMax;
    }
  };

  while (pos + 5 <= data.size()) {
    quint32 recordLength = 0;
    if (!readLE(data, pos, recordLength)) {
      m_ValidationNotes.push_back(QString("Unable to read SAVETREE record length at %1").arg(pos));
      break;
    }
    if (recordLength == 0) {
      break;
    }

    const qsizetype bytesAvailableAfterLength = data.size() - pos - 4;
    if (recordLength > static_cast<quint64>(bytesAvailableAfterLength)) {
      m_ValidationNotes.push_back(
          QString("SAVETREE record at %1 declares %2 bytes but only %3 remain")
              .arg(pos)
              .arg(recordLength)
              .arg(bytesAvailableAfterLength));
      break;
    }

    const qsizetype totalLength = static_cast<qsizetype>(recordLength) + 4;
    const qsizetype recordEnd = pos + totalLength;
    const quint8 recordType = static_cast<quint8>(data.at(pos + 4));
    ++m_RecordCountTotal;
    m_RecordTypeCounts[recordType] = m_RecordTypeCounts.value(recordType) + 1;

    const int expectedLength = expectedRecordLength(recordType);
    if (expectedLength > 0 && static_cast<int>(recordLength) != expectedLength) {
      m_ValidationNotes.push_back(
          QString("Record length mismatch for type %1: got %2 expected %3")
              .arg(recordType)
              .arg(recordLength)
              .arg(expectedLength));
    }

    auto hasBytes = [recordEnd](qsizetype at, qsizetype size) {
      return at >= 0 && size >= 0 && at <= recordEnd && size <= recordEnd - at;
    };

    quint32 recordId = 0;
    quint32 parentId = 0;
    const bool hasRecordId = hasBytes(pos + PlayerTreeOffset::RecordId, 4) &&
                             readLE(data, pos + PlayerTreeOffset::RecordId, recordId);
    const bool hasParentId = hasBytes(pos + PlayerTreeOffset::ParentId, 4) &&
                             readLE(data, pos + PlayerTreeOffset::ParentId, parentId);

    if (hasRecordId && recordId != 0) {
      recordGraph.addNode({recordId, hasParentId ? parentId : 0,
                           static_cast<qint32>(recordType), -1},
                          &m_ValidationNotes);
    }

    const PlayerRecordMatch playerMatch =
        classifyPlayerRecord(recordType, hasRecordId, recordId);
    m_PlayerRecordByTypeFound = m_PlayerRecordByTypeFound || playerMatch.byType;
    m_PlayerRecordByIdFound = m_PlayerRecordByIdFound || playerMatch.byId;

    if (playerMatch.canonical()) {
      ++canonicalPlayerCount;
      m_PlayerRecordCanonicalFound = true;
      if (selectedPlayerRank < 3) {
        parsePlayerRecord(pos, recordEnd);
        selectedPlayerRank = 3;
      }
    } else if (playerMatch.recoveryCandidate()) {
      const QString idText = hasRecordId ? QString::number(recordId) : QString("<missing>");
      m_ValidationNotes.push_back(
          QString("Damaged player-record candidate: type=%1 id=%2; expected type=3 id=50000")
              .arg(recordType)
              .arg(idText));

      const int recoveryRank = playerMatch.byType ? 2 : 1;
      if (selectedPlayerRank < recoveryRank) {
        parsePlayerRecord(pos, recordEnd);
        selectedPlayerRank = recoveryRank;
      }
    }

    if (recordType == kRecordTypeItem) {
      quint16 itemId = 0;
      quint32 quantity = 0;
      if (hasBytes(pos + PlayerTreeOffset::GoldItemId, 2) &&
          hasBytes(pos + PlayerTreeOffset::GoldQuantity, 4) && hasParentId &&
          readLE(data, pos + PlayerTreeOffset::GoldItemId, itemId) &&
          readLE(data, pos + PlayerTreeOffset::GoldQuantity, quantity) &&
          itemId == kItemIdGoldPieces) {
        goldCandidates.push_back({hasRecordId ? recordId : 0, parentId, quantity});
      }
    }

    pos = recordEnd;
  }

  if (canonicalPlayerCount > 1) {
    m_ValidationNotes.push_back(
        QString("SAVETREE contains %1 canonical player records").arg(canonicalPlayerCount));
  }

  m_PlayerRecordFound = selectedPlayerRank > 0;
  m_PlayerRecordRecoveryUsed = selectedPlayerRank > 0 && selectedPlayerRank < 3;

  const QStringList graphNotes = recordGraph.validateLinks();
  for (const QString& note : graphNotes) {
    if (!m_ValidationNotes.contains(note)) {
      m_ValidationNotes.push_back(note);
    }
  }

  for (const GoldCandidate& gold : goldCandidates) {
    const auto ownership =
        recordGraph.traceParentToAncestor(gold.parentId, kPlayerRecordId);
    if (ownership.reachesAncestor) {
      goldTotal += gold.quantity;
      ++m_GoldItemRecordCount;
      continue;
    }

    if (ownership.cycleDetected) {
      m_ValidationNotes.push_back(
          QString("Gold item record %1 has a cyclic ownership chain")
              .arg(gold.recordId));
    } else if (ownership.missingReference) {
      m_ValidationNotes.push_back(
          QString("Gold item record %1 ownership chain references missing record %2")
              .arg(gold.recordId)
              .arg(ownership.missingId));
    }
  }

  if (pos < data.size()) {
    m_SaveTreeTailBytes = static_cast<quint32>(data.size() - pos);
  }
  m_Gold = static_cast<quint32>(std::min<quint64>(goldTotal, 0xFFFFFFFFULL));
  m_GoldAccumulator = goldTotal;

  return m_PlayerRecordFound;
}

bool BattlespireSaveGame::parseSaveVars()
{
  using namespace BattlespireSaveFormat;

  QFile saveVarsFile(saveFilePath("SAVEVARS.DAT"));
  if (!saveVarsFile.open(QIODevice::ReadOnly)) {
    return false;
  }
  m_HasSaveVars = true;

  const QByteArray data = saveVarsFile.readAll();
  if (data.size() < kCurrentTimestampOffset + static_cast<qsizetype>(sizeof(quint32))) {
    m_ValidationNotes.push_back("SAVEVARS.DAT is too small to contain the player and misc blocks");
    return false;
  }

  // The first block is a copy of the player record without the 65-byte SAVETREE header.
  parsePlayerBlockFromSaveVars(data);

  // UESP documents the Miscellaneous block, and therefore CurrentLevel/current map, at byte 1051.
  quint32 currentLevel = 0;
  if (!readLE(data, kCurrentMapLevelOffset, currentLevel)) {
    m_ValidationNotes.push_back("Unable to read SAVEVARS current map at offset 1051");
    return false;
  }

  m_CurrentLevelId = currentLevel;
  m_CurrentLevelOffset = static_cast<int>(kCurrentMapLevelOffset);
  readLE(data, kCurrentTimestampOffset, m_CurrentTimestamp);

  auto countFixedRecords = [&](qsizetype start, qsizetype bytesPerRecord, qsizetype maxRecords,
                               auto&& recordHasData) {
    int count = 0;
    if (start < 0 || bytesPerRecord <= 0 || maxRecords <= 0 || start > data.size()) {
      return count;
    }
    for (qsizetype i = 0; i < maxRecords; ++i) {
      if (i > (std::numeric_limits<qsizetype>::max() - start) / bytesPerRecord) {
        break;
      }
      const qsizetype off = start + i * bytesPerRecord;
      if (!hasRange(data, off, bytesPerRecord)) {
        break;
      }
      const QByteArray rec = data.mid(off, bytesPerRecord);
      if (recordHasData(rec)) {
        ++count;
      }
    }
    return count;
  };

  // SAVEVARS block summaries from documented fixed-layout offsets.
  m_ConversationMapCount = countFixedRecords(
      4492, 8, 128, [](const QByteArray& rec) {
        quint32 id = 0;
        std::memcpy(&id, rec.constData(), sizeof(id));
        return qFromLittleEndian(id) != 0;
      });

  m_StaticEnemyCount = countFixedRecords(
      5520, 56, 128, [](const QByteArray& rec) {
        quint32 id = 0;
        std::memcpy(&id, rec.constData(), sizeof(id));
        return qFromLittleEndian(id) != 0;
      });

  m_GlobalVariableCount = countFixedRecords(
      15237, 8, 1344, [](const QByteArray& rec) {
        quint32 hash = 0;
        std::memcpy(&hash, rec.constData(), sizeof(hash));
        return qFromLittleEndian(hash) != 0;
      });

  m_LocalVariableCount = countFixedRecords(
      25989, 68, 128, [](const QByteArray& rec) {
        quint32 id = 0;
        std::memcpy(&id, rec.constData(), sizeof(id));
        if (qFromLittleEndian(id) != 0) {
          return true;
        }
        // Some records may have zeroed ID but populated var hashes/values.
        for (int i = 4; i < rec.size(); ++i) {
          if (static_cast<unsigned char>(rec.at(i)) != 0) {
            return true;
          }
        }
        return false;
      });

  m_MonsterTypeCountNonZero = 0;
  m_MonsterTypeCountTotal = 0;
  if (hasRange(data, 34693, 16)) {
    for (int i = 0; i < 16; ++i) {
      const int v = static_cast<unsigned char>(data.at(34693 + i));
      m_MonsterTypeCountTotal += v;
      if (v > 0) {
        ++m_MonsterTypeCountNonZero;
      }
    }
  }

  const QString locationName = levelLocationName(currentLevel);
  if (!locationName.isEmpty()) {
    m_PCLocation = locationName;
  } else if (currentLevel != 0) {
    m_PCLocation = QString("Unknown map (ID: %1)").arg(currentLevel);
  }

  // CurrentLevel in SAVEVARS is the campaign map identifier, not character level.
  // Character level remains whatever was read from the player record/player block.

  if constexpr (kLogSaveParsing) {
    qInfo().noquote() << "[BattlespireSaveGame] parseSaveVars() slot="
                      << QFileInfo(m_SaveFolder).fileName() << " mapId=" << currentLevel
                      << " offset=" << kCurrentMapLevelOffset << " location=" << m_PCLocation;
  }

  return true;
}

bool BattlespireSaveGame::parsePlayerBlockFromSaveVars(const QByteArray& data)
{
  using namespace BattlespireSaveFormat;

  constexpr qsizetype kPlayerBlockSize = 787;
  if (data.size() < kPlayerBlockSize) {
    return false;
  }

  // SAVEVARS copies the player record body with the 65-byte SAVETREE header removed.
  const QString playerName = readFixedString(data, PlayerVarsOffset::Name, 32);
  if (!playerName.isEmpty()) {
    m_PCName = playerName;
  }

  const QString className = readFixedString(data, PlayerVarsOffset::ClassName, 24);
  if (!className.isEmpty()) {
    m_ClassName = className;
  }

  quint32 level = 0;
  if (readLE(data, PlayerVarsOffset::Level, level) && level > 0 && level < 200) {
    m_PCLevel = static_cast<unsigned short>(level);
  }

  quint8 race = 0xFF;
  if (readLE(data, PlayerVarsOffset::Race, race)) {
    m_Race = race;
  }

  readLE(data, PlayerVarsOffset::ActiveSpells, m_ActiveSpellsMask);
  readLE(data, PlayerVarsOffset::CharacterFlags, m_CharacterFlagsMask);
  readLE(data, PlayerVarsOffset::Team, m_TeamValue);
  readLE(data, PlayerVarsOffset::Goal, m_GoalValue);
  m_ActiveSpellNames = activeSpellNamesFromMask(m_ActiveSpellsMask);
  m_CharacterFlagNames = characterFlagNamesFromMask(m_CharacterFlagsMask);

  quint16 sp = 0;
  quint16 spMax = 0;
  qint32 wounds = 0;
  qint32 woundsMax = 0;
  if (readLE(data, PlayerVarsOffset::SpellPoints, sp)) {
    m_SpellPoints = sp;
  }
  if (readLE(data, PlayerVarsOffset::SpellPointsMax, spMax)) {
    m_SpellPointsMax = spMax;
  }
  if (readLE(data, PlayerVarsOffset::Wounds, wounds)) {
    m_Wounds = wounds;
  }
  if (readLE(data, PlayerVarsOffset::WoundsMax, woundsMax)) {
    m_WoundsMax = woundsMax;
  }

  return true;
}

void BattlespireSaveGame::evaluateDeveloperValidation()
{
  if (!m_HasSaveTree) {
    m_ValidationNotes.push_back("SAVETREE.DAT missing or unreadable");
  }
  if (!m_HasSaveVars) {
    m_ValidationNotes.push_back("SAVEVARS.DAT missing or unreadable");
  }
  if (!m_PlayerRecordFound) {
    m_ValidationNotes.push_back("Player record not found in SAVETREE");
  } else if (!m_PlayerRecordCanonicalFound) {
    m_ValidationNotes.push_back(
        "Canonical player record (type 3, ID 50000) not found; using damaged-file recovery candidate");
  }
  if (m_GoldAccumulator > 0xFFFFFFFFULL) {
    m_ValidationNotes.push_back("Gold total exceeded uint32 range before clamp");
  }

  m_ValidationLikelyModified = !m_ValidationNotes.isEmpty();
}

bool BattlespireSaveGame::hasAnySavePayload() const
{
  if (m_HasSaveName || m_HasSaveTree || m_HasSaveVars || m_HasImage) {
    return true;
  }

  return QFileInfo::exists(saveFilePath("SAVENAME.DAT")) ||
         QFileInfo::exists(saveFilePath("SAVETREE.DAT")) ||
         QFileInfo::exists(saveFilePath("SAVEVARS.DAT")) ||
         QFileInfo::exists(saveFilePath("IMAGE.RAW"));
}

QString BattlespireSaveGame::raceName(quint8 raceId)
{
  switch (raceId) {
    case 0: return "Redguard";
    case 1: return "Breton";
    case 2: return "Nord";
    case 3: return "High Elf";
    case 4: return "Dark Elf";
    case 5: return "Wood Elf";
    default: return {};
  }
}

QString BattlespireSaveGame::saveFilePath(const QString& fileName) const
{
  return QDir(m_SaveFolder).filePath(fileName);
}

QString BattlespireSaveGame::levelLocationName(quint32 currentLevel)
{
  // Mapping based on GAMEDATA/LEVELS.TXT row order:
  // 1..7 campaign (Lv1..Lv7), then multiplayer maps.
  switch (currentLevel) {
    case 1: return "The Weir Gate";
    case 2: return "Caitiff: Administration, Labs and Library";
    case 3: return "The Soul Cairn";
    case 4: return "Shade Perilous";
    case 5: return "The Chimera of Desolation";
    case 6: return "Havok Wellhead";
    case 7: return "Mehrunes Dagon";
    default: return {};
  }
}

QString BattlespireSaveGame::readFixedString(const QByteArray& data, qsizetype offset,
                                             qsizetype size)
{
  if (!hasRange(data, offset, size) || size <= 0) {
    return {};
  }

  QByteArray bytes = data.mid(offset, size);
  const int nullPos = bytes.indexOf('\0');
  if (nullPos >= 0) {
    bytes.truncate(nullPos);
  }

  return QString::fromLocal8Bit(bytes).trimmed();
}
