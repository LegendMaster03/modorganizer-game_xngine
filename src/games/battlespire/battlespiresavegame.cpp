#include "battlespiresavegame.h"

#include "gamebattlespire.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QColor>
#include <QImage>
#include <QRegularExpression>
#include <QDebug>
#include <QtEndian>

#include <algorithm>
#include <cstring>

namespace {
constexpr qsizetype kSaveNameLength = 32;
constexpr qsizetype kRecordHeaderLength = 65;
constexpr quint8 kRecordTypePlayer = 3;
constexpr qsizetype kImageWidth = 80;
constexpr qsizetype kImageHeight = 50;
constexpr qsizetype kImageBytesPerPixel = 2;
constexpr qsizetype kImageRawSize = kImageWidth * kImageHeight * kImageBytesPerPixel;
constexpr bool kLogSaveParsing = false;

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

std::unique_ptr<XngineSaveGame::DataFields> BattlespireSaveGame::fetchDataFields() const
{
  auto fields = std::make_unique<DataFields>();

  QFile imageFile(saveFilePath("IMAGE.RAW"));
  if (!imageFile.open(QIODevice::ReadOnly)) {
    return fields;
  }

  QByteArray raw = imageFile.readAll();
  if (raw.size() < kImageRawSize) {
    return fields;
  }

  QImage image(static_cast<int>(kImageWidth), static_cast<int>(kImageHeight),
               QImage::Format_RGB32);
  const auto* ptr = reinterpret_cast<const uchar*>(raw.constData());

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
    if (recordType == kRecordTypePlayer &&
        recordLength >= static_cast<quint32>(kRecordHeaderLength + 791)) {
      m_PCName = readFixedString(data, pos + 65, 32);

      float posX = 0.0F;
      float posY = 0.0F;
      float posZ = 0.0F;
      if (readF32LE(data, pos + 11, posX) && readF32LE(data, pos + 15, posY) &&
          readF32LE(data, pos + 19, posZ)) {
        m_PositionText = QString("X %1, Y %2, Z %3")
                             .arg(QString::number(posX, 'f', 2))
                             .arg(QString::number(posY, 'f', 2))
                             .arg(QString::number(posZ, 'f', 2));
      }

      quint32 level = 0;
      if (readLE(data, pos + 736, level) && level > 0 && level < 200) {
        m_PCLevel = static_cast<unsigned short>(level);
      }

      qint32 wounds = 0;
      qint32 woundsMax = 0;
      quint16 sp = 0;
      quint16 spMax = 0;
      if (readLE(data, pos + 167, wounds) && readLE(data, pos + 171, woundsMax) &&
          readLE(data, pos + 161, sp) && readLE(data, pos + 163, spMax)) {
        Q_UNUSED(wounds);
        Q_UNUSED(woundsMax);
        Q_UNUSED(sp);
        Q_UNUSED(spMax);
      }

      return true;
    }

    pos += totalLength;
  }

  return false;
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
    case 8: return "Deathmatch level 1";
    case 9: return "Deathmatch level 2";
    case 10: return "Deathmatch level 3";
    case 11: return "Deathmatch level 4";
    case 12: return "Castle flag capture (foggy)";
    case 13: return "Castle flag capture (night)";
    case 14: return "Capture the flag 2";
    case 15: return "Deathmatch level 5";
    case 16: return "Deathmatch level 6";
    case 17: return "Deathmatch level 7";
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
