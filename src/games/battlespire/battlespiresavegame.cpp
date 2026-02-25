#include "battlespiresavegame.h"

#include "gamebattlespire.h"
#include "xnginepaletteformat.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QColor>
#include <QImage>
#include <QRegularExpression>
#include <QDebug>
#include <QStringList>
#include <QtEndian>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace {
constexpr qsizetype kSaveNameLength = 32;
constexpr quint8 kRecordTypePlayer = 3;
constexpr quint8 kRecordTypeItem = 2;
constexpr qsizetype kImageWidth = 80;
constexpr qsizetype kImageHeight = 50;
constexpr qsizetype kImageBytesPerPixel8 = 1;
constexpr qsizetype kImageBytesPerPixel = 2;
constexpr qsizetype kImageRawSize8 = kImageWidth * kImageHeight * kImageBytesPerPixel8;
constexpr qsizetype kImageRawSize = kImageWidth * kImageHeight * kImageBytesPerPixel;
constexpr bool kLogSaveParsing = false;
constexpr quint32 kPlayerRecordId = 50000U;  // 0x0000C350
constexpr quint16 kItemIdGoldPieces = 33;

template <typename T>
bool readLE(const QByteArray& data, qsizetype offset, T& value)
{
  if (offset < 0 || offset + static_cast<qsizetype>(sizeof(T)) > data.size()) {
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
}  // namespace

BattlespireSaveGame::BattlespireSaveGame(QString const& folder,
                                         GameBattlespire const* game)
    : XngineSaveGame(folder, game), m_SaveFolder(folder)
{
  QFileInfo info(folder);
  m_DisplayName = info.fileName();

  parseSaveName();
  parseSaveTree();
  parseSaveVars();

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
  if (m_Gold > 0) {
    lines.push_back(QString("Gold: %1").arg(m_Gold));
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

bool BattlespireSaveGame::parseSaveName()
{
  QFile saveNameFile(saveFilePath("SAVENAME.DAT"));
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

bool BattlespireSaveGame::parseSaveTree()
{
  QFile saveTreeFile(saveFilePath("SAVETREE.DAT"));
  if (!saveTreeFile.open(QIODevice::ReadOnly)) {
    return false;
  }

  const QByteArray data = saveTreeFile.readAll();
  if (data.size() < 8) {
    return false;
  }

  qsizetype pos = 4;  // 4-byte file version/header
  bool foundPlayerRecord = false;
  quint64 goldTotal = 0;
  m_Gold = 0;
  while (pos + 5 <= data.size()) {
    quint32 recordLength = 0;
    if (!readLE(data, pos, recordLength)) {
      break;
    }
    if (recordLength == 0) {
      break;
    }

    const qsizetype totalLength = static_cast<qsizetype>(recordLength) + 4;
    if (pos + totalLength > data.size()) {
      break;
    }

    const quint8 recordType = static_cast<quint8>(data.at(pos + 4));
    const qsizetype recordEnd = pos + totalLength;
    auto hasBytes = [recordEnd](qsizetype at, qsizetype size) {
        return at >= 0 && size >= 0 && at + size <= recordEnd;
    };

    quint32 recordId = 0;
    const bool hasRecordId = hasBytes(pos + 33, 4) && readLE(data, pos + 33, recordId);
    const bool isPlayerRecordByType = (recordType == kRecordTypePlayer);
    const bool isPlayerRecordById = hasRecordId && (recordId == 50000U);

    if (isPlayerRecordByType || isPlayerRecordById) {
      foundPlayerRecord = true;

      if (hasBytes(pos + 65, 32)) {
        const QString name = readFixedString(data, pos + 65, 32);
        if (!name.isEmpty()) {
          m_PCName = name;
        }
      }

      float posX = 0.0F;
      float posY = 0.0F;
      float posZ = 0.0F;
      if (hasBytes(pos + 11, 12) && readF32LE(data, pos + 11, posX) &&
          readF32LE(data, pos + 15, posY) && readF32LE(data, pos + 19, posZ) &&
          std::isfinite(posX) && std::isfinite(posY) && std::isfinite(posZ)) {
        m_PositionText = QString("X %1, Y %2, Z %3")
                             .arg(QString::number(posX, 'f', 2))
                             .arg(QString::number(posY, 'f', 2))
                             .arg(QString::number(posZ, 'f', 2));
      }

      quint32 level = 0;
      if (hasBytes(pos + 736, 4) && readLE(data, pos + 736, level) && level > 0 &&
          level < 200) {
        m_PCLevel = static_cast<unsigned short>(level);
      }

      qint32 wounds = 0;
      qint32 woundsMax = 0;
      quint16 sp = 0;
      quint16 spMax = 0;
      if (hasBytes(pos + 161, 12) && readLE(data, pos + 167, wounds) &&
          readLE(data, pos + 171, woundsMax) && readLE(data, pos + 161, sp) &&
          readLE(data, pos + 163, spMax) && woundsMax >= 0) {
        m_Wounds = wounds;
        m_WoundsMax = woundsMax;
        m_SpellPoints = sp;
        m_SpellPointsMax = spMax;
      }

      // Keep scanning; in malformed files we want the last valid player-like record.
    }

    if (recordType == kRecordTypeItem) {
      quint16 itemId = 0;
      quint32 quantity = 0;
      quint32 parentId = 0;
      if (hasBytes(pos + 97, 2) && hasBytes(pos + 127, 4) && hasBytes(pos + 61, 4) &&
          readLE(data, pos + 97, itemId) && readLE(data, pos + 127, quantity) &&
          readLE(data, pos + 61, parentId) && itemId == kItemIdGoldPieces &&
          parentId == kPlayerRecordId) {
        goldTotal += quantity;
      }
    }

    pos += totalLength;
  }

  m_Gold = static_cast<quint32>(std::min<quint64>(goldTotal, 0xFFFFFFFFULL));

  return foundPlayerRecord;
}

bool BattlespireSaveGame::parseSaveVars()
{
  QFile saveVarsFile(saveFilePath("SAVEVARS.DAT"));
  if (!saveVarsFile.open(QIODevice::ReadOnly)) {
    return false;
  }

  const QByteArray data = saveVarsFile.readAll();
  if (data.size() < 1072) {
    return false;
  }

  // The first block is a copy of the player record without the 65-byte SAVETREE header.
  parsePlayerBlockFromSaveVars(data);

  // Misc block starts right after the 1052-byte player block.
  quint32 currentLevel = 0;
  qsizetype levelOffset = -1;
  if (!readLE(data, 1052, currentLevel) || currentLevel == 0 || currentLevel > 100) {
    if (!readLE(data, 1051, currentLevel)) {
      return false;
    }
    levelOffset = 1051;
  } else {
    levelOffset = 1052;
  }

  if (currentLevel == 0 || currentLevel > 100) {
    return false;
  }

  m_CurrentLevelId = currentLevel;

  const QString locationName = levelLocationName(currentLevel);
  if (!locationName.isEmpty()) {
    m_PCLocation = locationName;
  } else {
    m_PCLocation = QString("Unknown (ID: %1)").arg(currentLevel);
  }

  if (m_PCLevel == 0) {
    m_PCLevel = static_cast<unsigned short>(currentLevel);
  }

  if constexpr (kLogSaveParsing) {
    qInfo().noquote() << "[BattlespireSaveGame] parseSaveVars() slot="
                      << QFileInfo(m_SaveFolder).fileName() << " levelId=" << currentLevel
                      << " offset=" << levelOffset << " location=" << m_PCLocation;
  }

  return true;
}

bool BattlespireSaveGame::parsePlayerBlockFromSaveVars(const QByteArray& data)
{
  constexpr qsizetype kPlayerBlockSize = 787;
  if (data.size() < kPlayerBlockSize) {
    return false;
  }

  // Offsets are SAVETREE player offsets shifted by -65 (header removed).
  const QString playerName = readFixedString(data, 0, 32);
  if (!playerName.isEmpty()) {
    m_PCName = playerName;
  }

  const QString className = readFixedString(data, 378, 24);
  if (!className.isEmpty()) {
    m_ClassName = className;
  }

  quint32 level = 0;
  if (readLE(data, 671, level) && level > 0 && level < 200) {
    m_PCLevel = static_cast<unsigned short>(level);
  }

  quint8 race = 0xFF;
  if (readLE(data, 605, race)) {
    m_Race = race;
  }

  quint16 sp = 0;
  quint16 spMax = 0;
  qint32 wounds = 0;
  qint32 woundsMax = 0;
  if (readLE(data, 96, sp)) {
    m_SpellPoints = sp;
  }
  if (readLE(data, 98, spMax)) {
    m_SpellPointsMax = spMax;
  }
  if (readLE(data, 102, wounds)) {
    m_Wounds = wounds;
  }
  if (readLE(data, 106, woundsMax)) {
    m_WoundsMax = woundsMax;
  }

  return true;
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
