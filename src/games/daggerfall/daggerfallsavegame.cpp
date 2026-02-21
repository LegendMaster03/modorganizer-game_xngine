#include "daggerfallsavegame.h"

#include "gamedaggerfall.h"
#include "daggerfallmapsbsa.h"

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
#include <cstring>

namespace {
constexpr quint8 kRecordTypeCharacterPosition = 0x01;
constexpr quint8 kRecordTypeCharacter = 0x03;
constexpr quint8 kRecordTypeDungeonInformation = 0x07;
constexpr qsizetype kRecordBaseSize = 71;
constexpr qsizetype kHeaderSize = 0x13;
constexpr qsizetype kCharacterRecordNameOffset = 0x00;
constexpr qsizetype kCharacterRecordNameLength = 0x20;
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
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly)) {
    return false;
  }
  const QByteArray bytes = f.readAll();
  if (bytes.size() < 768) {
    return false;
  }

  // Several DF palette files have 8-byte headers (size 776). Use trailing 768 bytes.
  const qsizetype base = bytes.size() - 768;
  bool sixBitRange = true;
  for (qsizetype i = 0; i < 768; ++i) {
    if (static_cast<unsigned char>(bytes.at(base + i)) > 63) {
      sixBitRange = false;
      break;
    }
  }

  for (int i = 0; i < 256; ++i) {
    const int rRaw = static_cast<unsigned char>(bytes.at(base + i * 3 + 0));
    const int gRaw = static_cast<unsigned char>(bytes.at(base + i * 3 + 1));
    const int bRaw = static_cast<unsigned char>(bytes.at(base + i * 3 + 2));
    const int r = sixBitRange ? (rRaw * 255 / 63) : rRaw;
    const int g = sixBitRange ? (gRaw * 255 / 63) : gRaw;
    const int b = sixBitRange ? (bRaw * 255 / 63) : bRaw;
    palette[static_cast<size_t>(i)] = QColor(r, g, b);
  }

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

QString DaggerfallsSaveGame::getGameDetails() const
{
  QStringList lines;
  if (!m_InGameDate.isEmpty()) {
    lines.push_back(QString("In-game: %1").arg(m_InGameDate));
  }
  const QString race = raceName(m_Race);
  if (!race.isEmpty()) {
    lines.push_back(QString("Race: %1").arg(race));
  }
  const QString reflex = reflexName(m_Reflex);
  if (!reflex.isEmpty()) {
    lines.push_back(QString("Reflex: %1").arg(reflex));
  }
  if (m_HPMax > 0) {
    lines.push_back(QString("HP: %1/%2").arg(m_HP).arg(m_HPMax));
  }
  if (m_ManaMax > 0) {
    lines.push_back(QString("MP: %1/%2").arg(m_Mana).arg(m_ManaMax));
  }
  if (m_Gold > 0) {
    lines.push_back(QString("Gold: %1").arg(m_Gold));
  }
  return lines.join('\n');
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
    // Keep this as fallback if we do not find a better in-record player position.
    m_PCLocation = formatHeaderLocationText(locationCode, zoneType, headerX, headerY, headerZ);
    Q_UNUSED(version);
  }

  qsizetype streamStart = 0;
  qsizetype streamEnd = 0;
  const auto records = findBestRecordStream(data, &streamStart, &streamEnd);
  if (records.empty()) {
    return false;
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

    quint32 timestamp = 0;
    if (readLE32U(data, recordDataOffset + kCharacterRecordTimestampOffset, timestamp) &&
        timestamp > 0) {
      m_InGameDate = formatDaggerfallDate(timestamp);
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

  quint32 timestamp = 0;
  if (!readLE32U(data, 0x3c9, timestamp) || timestamp == 0) {
    return false;
  }

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
    const QByteArray& data, qsizetype startOffset, qsizetype* endOffset)
{
  std::vector<ParsedRecord> records;
  qsizetype pos = startOffset;

  while (pos + 4 <= data.size()) {
    qint32 recordLength = 0;
    if (!readLE32(data, pos, recordLength) || recordLength < 0) {
      break;
    }

    if (recordLength == 0) {
      pos += 4;
      continue;
    }

    if (pos + 4 + recordLength > data.size()) {
      break;
    }

    const qsizetype payloadOffset = pos + 4;
    qsizetype payloadLength = recordLength;
    const quint8 recordType = static_cast<quint8>(data.at(payloadOffset));

    // Daggerfall's dungeon-information records report compressed length units.
    if (recordType == kRecordTypeDungeonInformation) {
      const qsizetype correctedLength = payloadLength * 39;
      if (payloadLength <= 0 || pos + 4 + correctedLength > data.size()) {
        break;
      }
      payloadLength = correctedLength;
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
    const QByteArray& data, qsizetype* startOffset, qsizetype* endOffset)
{
  std::vector<ParsedRecord> bestRecords;
  qsizetype bestStart = 0;
  qsizetype bestEnd = 0;
  int bestScore = -1;

  // Header and location-detail are before the record stream. Probe a small window.
  const qsizetype maxProbe = std::min<qsizetype>(2048, std::max<qsizetype>(0, data.size() - 4));
  for (qsizetype probe = 0; probe <= maxProbe; ++probe) {
    qsizetype end = 0;
    auto records = parseRecordStream(data, probe, &end);
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
      bestRecords = std::move(records);
    }
  }

  if (startOffset != nullptr) {
    *startOffset = bestStart;
  }
  if (endOffset != nullptr) {
    *endOffset = bestEnd;
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
                                                      qint32 x, qint32 y, qint32 z) const
{
  Q_UNUSED(y);
  const auto nearest = DaggerfallMapsBsa::resolveNearestLocation(m_Game, x, z);
  const auto info = nearest.isValid() ? nearest
                                      : DaggerfallMapsBsa::resolveLocationInfo(m_Game, locationCode);
  if (info.isValid()) {
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
