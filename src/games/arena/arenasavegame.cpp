#include "arenasavegame.h"

#include "gamearena.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringList>

namespace
{
using Codec = ArenaSaveGame::NibbleCodec;

Codec makeCodec(std::initializer_list<quint8> left, std::initializer_list<quint8> right)
{
  Codec c;
  int i = 0;
  for (auto v : left) {
    c.left[static_cast<size_t>(i++)] = v;
  }
  i = 0;
  for (auto v : right) {
    c.right[static_cast<size_t>(i++)] = v;
  }
  return c;
}

const Codec kLevelCodec = makeCodec(
    {0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7},
    {0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0, 0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8});

const Codec kExp0Codec = makeCodec(
    {0x2, 0x3, 0x0, 0x1, 0x6, 0x7, 0x4, 0x5, 0xA, 0xB, 0x8, 0x9, 0xE, 0xF, 0xC, 0xD},
    {0x9, 0x8, 0xB, 0xA, 0xD, 0xC, 0xF, 0xE, 0x1, 0x0, 0x3, 0x2, 0x5, 0x4, 0x7, 0x6});
const Codec kExp1Codec = makeCodec(
    {0x5, 0x4, 0x7, 0x6, 0x1, 0x0, 0x3, 0x2, 0xD, 0xC, 0xF, 0xE, 0x9, 0x8, 0xB, 0xA},
    {0x2, 0x3, 0x0, 0x1, 0x6, 0x7, 0x4, 0x5, 0xA, 0xB, 0x8, 0x9, 0xE, 0xF, 0xC, 0xD});
const Codec kExp2Codec = makeCodec(
    {0xA, 0xB, 0x8, 0x9, 0xE, 0xF, 0xC, 0xD, 0x2, 0x3, 0x0, 0x1, 0x6, 0x7, 0x4, 0x5},
    {0x4, 0x5, 0x6, 0x7, 0x0, 0x1, 0x2, 0x3, 0xC, 0xD, 0xE, 0xF, 0x8, 0x9, 0xA, 0xB});
const Codec kExp3Codec = makeCodec(
    {0x4, 0x5, 0x6, 0x7, 0x0, 0x1, 0x2, 0x3, 0xC, 0xD, 0xE, 0xF, 0x8, 0x9, 0xA, 0xB},
    {0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7});

const Codec kGold0Codec = makeCodec(
    {0x1, 0x0, 0x3, 0x2, 0x5, 0x4, 0x7, 0x6, 0x9, 0x8, 0xB, 0xA, 0xD, 0xC, 0xF, 0xE},
    {0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7});
const Codec kGold1Codec = makeCodec(
    {0x3, 0x2, 0x1, 0x0, 0x7, 0x6, 0x5, 0x4, 0xB, 0xA, 0x9, 0x8, 0xF, 0xE, 0xD, 0xC},
    {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF});
const Codec kGold2Codec = makeCodec(
    {0x5, 0x4, 0x7, 0x6, 0x1, 0x0, 0x3, 0x2, 0xD, 0xC, 0xF, 0xE, 0x9, 0x8, 0xB, 0xA},
    {0xE, 0xF, 0xC, 0xD, 0xA, 0xB, 0x8, 0x9, 0x6, 0x7, 0x4, 0x5, 0x2, 0x3, 0x0, 0x1});
const Codec kGold3Codec = makeCodec(
    {0xB, 0xA, 0x9, 0x8, 0xF, 0xE, 0xD, 0xC, 0x3, 0x2, 0x1, 0x0, 0x7, 0x6, 0x5, 0x4},
    {0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7});

const Codec kBless0Codec = makeCodec(
    {0x6, 0x7, 0x4, 0x5, 0x2, 0x3, 0x0, 0x1, 0xE, 0xF, 0xC, 0xD, 0xA, 0xB, 0x8, 0x9},
    {0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7});
const Codec kBless1Codec = makeCodec(
    {0xC, 0xD, 0xE, 0xF, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB},
    {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF});
}  // namespace

ArenaSaveGame::ArenaSaveGame(const QString& saveFile, const GameArena* game)
    : XngineSaveGame(saveFile, game), m_SaveFile(saveFile), m_Game(game)
{
  resolveSavePath();
  detectSlotFromFilename();
  parseNamesDat();
  parseSaveEngn();
  parseCityData();
  parseLog();
  parseSpells();
}

QString ArenaSaveGame::getName() const
{
  QStringList parts;
  if (m_HasSlotDisplayName && !m_DisplayName.trimmed().isEmpty()) {
    parts << m_DisplayName.trimmed();
  }
  if (m_Slot >= 0) {
    parts << QString("Arena Slot %1").arg(m_Slot);
  } else {
    parts << QString("Arena Save");
  }
  if (m_Level > 0) {
    parts << QString("Level %1").arg(m_Level);
  }
  if (m_IsEmptySlot) {
    parts << QString("Empty");
  }
  if (m_Gold > 0) {
    parts << QString("%1 gold").arg(m_Gold);
  }
  return parts.join(", ");
}

QString ArenaSaveGame::getGameDetails() const
{
  QStringList lines;
  if (m_Experience > 0) {
    lines << QString("Experience: %1").arg(m_Experience);
  }
  if (m_Gold > 0) {
    lines << QString("Gold: %1").arg(m_Gold);
  }
  lines << QString("Blessing: %1 (raw %2)")
               .arg(QString::number(m_BlessingScaled, 'f', 2))
               .arg(m_BlessingRaw);
  if (m_DetailRaw > 0) {
    lines << QString("Detail Raw: 0x%1")
                 .arg(m_DetailRaw, 8, 16, QChar('0'));
  }

  if (m_StaffPieces >= 0) {
    lines << QString("Main Quest Staff Pieces: %1").arg(m_StaffPieces);
  }
  lines << QString("Ria Vision Trigger: %1").arg(m_RiaVisionEnabled ? "enabled" : "disabled");
  lines << QString("Main Quest Item Flag: %1").arg(m_HasMainQuestItem ? "set" : "clear");

  if (m_LogEntryCount > 0) {
    lines << QString("Quest Log Entries: %1").arg(m_LogEntryCount);
  }
  if (!m_LastQuestTitle.isEmpty()) {
    lines << QString("Latest Log Title: %1").arg(m_LastQuestTitle);
  }
  if (m_SpellRecordCount > 0) {
    lines << QString("Spellbook Records: %1").arg(m_SpellRecordCount);
    lines << QString("Spellbook Active: %1").arg(m_SpellActiveCount);
    if (!m_SpellPreview.isEmpty()) {
      lines << QString("Spells: %1").arg(m_SpellPreview.join(", "));
    }
  }

  return lines.join('\n');
}

QStringList ArenaSaveGame::allFiles() const
{
  QStringList files;
  const QDir dir(m_SaveDir.isEmpty() ? QFileInfo(m_SaveFile).absoluteDir().absolutePath()
                                     : m_SaveDir);
  const QString suffix = slotSuffix();
  if (!suffix.isEmpty()) {
    const auto slotFiles =
        dir.entryInfoList(QStringList{QString("*.%1").arg(suffix)}, QDir::Files, QDir::Name);
    for (const auto& fi : slotFiles) {
      const QString abs = fi.absoluteFilePath();
      if (!files.contains(abs)) {
        files.push_back(abs);
      }
    }
  } else {
    files = XngineSaveGame::allFiles();
  }

  const QString namesInDir = dir.filePath("NAMES.DAT");
  if (QFileInfo::exists(namesInDir) && !files.contains(namesInDir)) {
    files.push_back(namesInDir);
  } else {
    const QString namesInParent = QDir(dir.absolutePath() + "/..").filePath("NAMES.DAT");
    if (QFileInfo::exists(namesInParent) && !files.contains(namesInParent)) {
      files.push_back(namesInParent);
    }
  }

  return files;
}

bool ArenaSaveGame::parseSaveEngn()
{
  QFile f(m_SaveFile);
  if (!f.open(QIODevice::ReadOnly)) {
    m_IsEmptySlot = true;
    m_PCName = "Empty";
    return false;
  }
  m_SaveEngnData = f.readAll();
  if (m_SaveEngnData.isEmpty()) {
    m_IsEmptySlot = true;
    m_PCName = "Empty";
    return false;
  }

  bool anyNonZero = false;
  for (const char c : m_SaveEngnData) {
    if (static_cast<unsigned char>(c) != 0) {
      anyNonZero = true;
      break;
    }
  }
  if (!anyNonZero) {
    m_IsEmptySlot = true;
    m_PCName = "Empty";
    m_PCLevel = 0;
    m_Level = 0;
    m_Gold = 0;
    m_Experience = 0;
    m_BlessingRaw = 0;
    m_BlessingScaled = 0.0;
    return true;
  }

  quint8 levelScrambled = 0;
  if (readByte(0x06, levelScrambled)) {
    const quint8 levelRaw = decodeByte(levelScrambled, kLevelCodec);
    m_Level = static_cast<quint16>(levelRaw) + 1;
    m_PCLevel = m_Level;
  }

  bool ok = false;
  m_Experience = decodeDword(m_SaveEngnData, 0x0409, kExp0Codec, kExp1Codec, kExp2Codec,
                             kExp3Codec, &ok);
  if (!ok) {
    m_Experience = 0;
  }

  ok = false;
  m_Gold = decodeDword(m_SaveEngnData, 0x041E, kGold0Codec, kGold1Codec, kGold2Codec,
                       kGold3Codec, &ok);
  if (!ok) {
    m_Gold = 0;
  }

  ok = false;
  m_BlessingRaw = decodeWord(m_SaveEngnData, 0x0422, kBless0Codec, kBless1Codec, &ok);
  if (!ok) {
    m_BlessingRaw = 0;
  }
  m_BlessingScaled = static_cast<double>(m_BlessingRaw) / 2.56;

  readDword(0x0B90, m_DetailRaw);
  parseQuestFlags();

  if (m_PCName.isEmpty()) {
    const QString extracted = extractLikelyPersonName(m_SaveEngnData);
    if (!extracted.isEmpty()) {
      m_PCName = extracted;
    }
  }
  return true;
}

bool ArenaSaveGame::parseLog()
{
  const QString logPath = companionPath("LOG");
  QFile f(logPath);
  if (!f.open(QIODevice::ReadOnly)) {
    return false;
  }

  const QByteArray data = f.readAll();
  if (data.isEmpty()) {
    return false;
  }

  const QRegularExpression entryRe("(?s)&(.*?) \\*");
  QRegularExpressionMatchIterator it = entryRe.globalMatch(QString::fromLocal8Bit(data));

  QString last;
  while (it.hasNext()) {
    const auto match = it.next();
    const QString entry = match.captured(1);
    if (entry.isEmpty()) {
      continue;
    }
    ++m_LogEntryCount;
    const QString title = entry.split(QRegularExpression("\\r?\\n")).first().trimmed();
    if (!title.isEmpty()) {
      last = title;
    }
  }
  m_LastQuestTitle = last;
  return true;
}

bool ArenaSaveGame::parseSpells()
{
  const QString spellsPath = companionPath("SPELLS");
  QFileInfo fi(spellsPath);
  if (!fi.exists() || !fi.isFile()) {
    return false;
  }
  constexpr qint64 kSpellRecordSize = 85;
  constexpr qsizetype kSpellCostOffset = 0x32;
  constexpr qsizetype kSpellNameOffset = 0x34;
  constexpr qsizetype kSpellNameLength = 33;
  constexpr int kMaxSpellRecords = 32;
  const qint64 count = fi.size() / kSpellRecordSize;
  m_SpellRecordCount = static_cast<int>(qMin<qint64>(count, kMaxSpellRecords));

  QFile f(spellsPath);
  if (!f.open(QIODevice::ReadOnly)) {
    return true;
  }
  const QByteArray data = f.readAll();
  m_SpellActiveCount = 0;
  m_SpellPreview.clear();
  for (int i = 0; i < m_SpellRecordCount; ++i) {
    const qsizetype off = static_cast<qsizetype>(i) * kSpellRecordSize;
    if (off + kSpellRecordSize > data.size()) {
      break;
    }
    bool anyNonZero = false;
    for (qsizetype j = 0; j < kSpellRecordSize; ++j) {
      if (static_cast<unsigned char>(data.at(off + j)) != 0) {
        anyNonZero = true;
        break;
      }
    }
    if (anyNonZero) {
      ++m_SpellActiveCount;

      QString name;
      if (off + kSpellNameOffset + kSpellNameLength <= data.size()) {
        QByteArray rawName =
            data.mid(off + kSpellNameOffset, kSpellNameLength);
        const int nul = rawName.indexOf('\0');
        if (nul >= 0) {
          rawName.truncate(nul);
        }
        for (int j = rawName.size() - 1; j >= 0; --j) {
          const unsigned char ch = static_cast<unsigned char>(rawName.at(j));
          if (ch < 32 || ch > 126) {
            rawName.remove(j, 1);
          }
        }
        name = QString::fromLocal8Bit(rawName).trimmed();
      }

      quint16 cost = 0;
      if (off + kSpellCostOffset + 2 <= data.size()) {
        const auto* p = reinterpret_cast<const uchar*>(data.constData() + off + kSpellCostOffset);
        cost = static_cast<quint16>(p[0]) |
               (static_cast<quint16>(p[1]) << 8);
      }

      if (name.isEmpty()) {
        name = QString("slot%1").arg(i + 1);
      }

      if (m_SpellPreview.size() < 6) {
        m_SpellPreview.push_back(QString("%1 (%2)").arg(name).arg(cost));
      }
    }
  }
  return true;
}

bool ArenaSaveGame::parseCityData()
{
  const QString cityPath = companionPath("CITYDATA");
  QFile f(cityPath);
  if (!f.open(QIODevice::ReadOnly)) {
    return false;
  }

  const QByteArray data = f.readAll();
  if (data.isEmpty()) {
    return false;
  }

  if (m_PCLocation.isEmpty()) {
    const QString location = extractLikelyLocation(data);
    if (!location.isEmpty()) {
      m_PCLocation = location;
    }
  }
  return !m_PCLocation.isEmpty();
}

bool ArenaSaveGame::parseNamesDat()
{
  if (m_Slot < 0 || m_Slot > 9) {
    return false;
  }

  QFileInfo saveInfo(m_SaveFile);
  QString namesPath = saveInfo.absoluteDir().filePath("NAMES.DAT");
  if (!QFileInfo::exists(namesPath)) {
    namesPath = QDir(saveInfo.absolutePath() + "/..").filePath("NAMES.DAT");
  }

  QFile f(namesPath);
  if (!f.open(QIODevice::ReadOnly)) {
    return false;
  }

  constexpr qsizetype kSlotNameBytes = 48;
  constexpr qsizetype kSlotCount = 10;
  const QByteArray names = f.readAll();
  if (names.size() < (kSlotNameBytes * kSlotCount)) {
    return false;
  }

  const qsizetype offset = static_cast<qsizetype>(m_Slot) * kSlotNameBytes;
  QByteArray raw = names.mid(offset, kSlotNameBytes);
  const int nul = raw.indexOf('\0');
  if (nul >= 0) {
    raw.truncate(nul);
  }

  QString slotName = QString::fromLocal8Bit(raw).trimmed();
  if (slotName.compare("EMPTY", Qt::CaseInsensitive) == 0) {
    slotName.clear();
  }
  if (!slotName.isEmpty()) {
    m_DisplayName = slotName;
    m_HasSlotDisplayName = true;
  }

  return !slotName.isEmpty();
}

bool ArenaSaveGame::readByte(qsizetype offset, quint8& out) const
{
  if (offset < 0 || offset >= m_SaveEngnData.size()) {
    return false;
  }
  out = static_cast<quint8>(m_SaveEngnData.at(offset));
  return true;
}

bool ArenaSaveGame::readDword(qsizetype offset, quint32& out) const
{
  if (offset < 0 || offset + 4 > m_SaveEngnData.size()) {
    return false;
  }
  const auto* p = reinterpret_cast<const uchar*>(m_SaveEngnData.constData() + offset);
  out = static_cast<quint32>(p[0]) | (static_cast<quint32>(p[1]) << 8) |
        (static_cast<quint32>(p[2]) << 16) | (static_cast<quint32>(p[3]) << 24);
  return true;
}

bool ArenaSaveGame::readWord(qsizetype offset, quint16& out) const
{
  if (offset < 0 || offset + 2 > m_SaveEngnData.size()) {
    return false;
  }
  const auto* p = reinterpret_cast<const uchar*>(m_SaveEngnData.constData() + offset);
  out = static_cast<quint16>(p[0]) | (static_cast<quint16>(p[1]) << 8);
  return true;
}

quint8 ArenaSaveGame::decodeNibble(quint8 scrambled, const std::array<quint8, 16>& map)
{
  for (quint8 i = 0; i < 16; ++i) {
    if (map[static_cast<size_t>(i)] == scrambled) {
      return i;
    }
  }
  return 0;
}

quint8 ArenaSaveGame::decodeByte(quint8 scrambled, const NibbleCodec& codec)
{
  const quint8 leftScr = static_cast<quint8>((scrambled >> 4) & 0x0F);
  const quint8 rightScr = static_cast<quint8>(scrambled & 0x0F);
  const quint8 left = decodeNibble(leftScr, codec.left);
  const quint8 right = decodeNibble(rightScr, codec.right);
  return static_cast<quint8>((left << 4) | right);
}

quint32 ArenaSaveGame::decodeDword(const QByteArray& data, qsizetype offset, const NibbleCodec& b0,
                                   const NibbleCodec& b1, const NibbleCodec& b2,
                                   const NibbleCodec& b3, bool* ok)
{
  if (ok != nullptr) {
    *ok = false;
  }
  if (offset < 0 || offset + 4 > data.size()) {
    return 0;
  }

  const quint8 d0 = decodeByte(static_cast<quint8>(data.at(offset + 0)), b0);
  const quint8 d1 = decodeByte(static_cast<quint8>(data.at(offset + 1)), b1);
  const quint8 d2 = decodeByte(static_cast<quint8>(data.at(offset + 2)), b2);
  const quint8 d3 = decodeByte(static_cast<quint8>(data.at(offset + 3)), b3);

  if (ok != nullptr) {
    *ok = true;
  }

  return static_cast<quint32>(d0) | (static_cast<quint32>(d1) << 8) |
         (static_cast<quint32>(d2) << 16) | (static_cast<quint32>(d3) << 24);
}

quint16 ArenaSaveGame::decodeWord(const QByteArray& data, qsizetype offset, const NibbleCodec& b0,
                                  const NibbleCodec& b1, bool* ok)
{
  if (ok != nullptr) {
    *ok = false;
  }
  if (offset < 0 || offset + 2 > data.size()) {
    return 0;
  }

  const quint8 d0 = decodeByte(static_cast<quint8>(data.at(offset + 0)), b0);
  const quint8 d1 = decodeByte(static_cast<quint8>(data.at(offset + 1)), b1);

  if (ok != nullptr) {
    *ok = true;
  }

  return static_cast<quint16>(d0) | (static_cast<quint16>(d1) << 8);
}

QString ArenaSaveGame::extractLikelyPersonName(const QByteArray& data)
{
  const QString s = QString::fromLatin1(data);
  // Two+ title-cased words, tolerate apostrophes/hyphens.
  const QRegularExpression re(
      "\\b([A-Z][a-z'\\-]{2,}(?: [A-Z][a-z'\\-]{2,}){1,3})\\b");
  auto it = re.globalMatch(s);
  while (it.hasNext()) {
    const auto m = it.next();
    const QString candidate = m.captured(1).trimmed();
    if (candidate.isEmpty()) {
      continue;
    }
    if (candidate.contains("High Rock", Qt::CaseInsensitive) ||
        candidate.contains("North Point", Qt::CaseInsensitive) ||
        candidate.contains("Old Gate", Qt::CaseInsensitive)) {
      continue;
    }
    return candidate;
  }
  return {};
}

QString ArenaSaveGame::extractLikelyLocation(const QByteArray& data)
{
  const QString s = QString::fromLatin1(data);
  const QRegularExpression re(
      "\\b([A-Z][A-Za-z'\\-]{2,}(?: [A-Z][A-Za-z'\\-]{2,}){0,3})\\b");
  auto it = re.globalMatch(s);

  QStringList hits;
  while (it.hasNext()) {
    QString c = it.next().captured(1).trimmed();
    if (c.isEmpty()) {
      continue;
    }
    if (c.compare("EMPTY", Qt::CaseInsensitive) == 0) {
      continue;
    }
    if (c.contains("Store", Qt::CaseInsensitive) ||
        c.contains("Equipment", Qt::CaseInsensitive) ||
        c.contains("Merchandise", Qt::CaseInsensitive)) {
      continue;
    }
    if (!hits.contains(c)) {
      hits.push_back(c);
    }
  }

  if (hits.isEmpty()) {
    return {};
  }
  if (hits.size() >= 2) {
    return QString("%1, %2").arg(hits.at(1), hits.at(0));
  }
  return hits.first();
}

QString ArenaSaveGame::companionPath(const QString& stem) const
{
  const QFileInfo fi(m_SaveFile);
  const QString fileName = fi.fileName();
  const int dot = fileName.lastIndexOf('.');
  if (dot < 0 || dot == fileName.size() - 1) {
    return fi.dir().filePath(stem);
  }

  const QString suffix = fileName.mid(dot + 1);
  return fi.dir().filePath(stem + "." + suffix);
}

QString ArenaSaveGame::slotSuffix() const
{
  if (!m_SlotSuffix.isEmpty()) {
    return m_SlotSuffix;
  }

  const QFileInfo fi(m_SaveFile);
  const QString fileName = fi.fileName();
  const int dot = fileName.lastIndexOf('.');
  if (dot < 0 || dot == fileName.size() - 1) {
    return {};
  }
  return fileName.mid(dot + 1);
}

void ArenaSaveGame::detectSlotFromFilename()
{
  {
    const QFileInfo fi(m_SaveFile);
    const QRegularExpression re("(?i)^SAVEENGN\\.(\\d{1,2})$");
    const auto m = re.match(fi.fileName());
    if (m.hasMatch()) {
      bool ok = false;
      const int slot = m.captured(1).toInt(&ok);
      if (ok && slot >= 0) {
        m_Slot = slot;
        m_SaveNumber = static_cast<unsigned long>(slot);
        m_SlotSuffix = m.captured(1);
        return;
      }
    }
  }

  const QDir dir(m_SaveDir);
  const QRegularExpression dirRe("(?i)^SAVE(\\d+)$");
  const auto m = dirRe.match(dir.dirName());
  if (!m.hasMatch()) {
    return;
  }

  bool ok = false;
  const int slot = m.captured(1).toInt(&ok);
  if (!ok || slot < 0) {
    return;
  }
  m_Slot = slot;
  m_SaveNumber = static_cast<unsigned long>(slot);
}

void ArenaSaveGame::parseQuestFlags()
{
  quint8 flag = 0;
  if (readByte(0x0DD2, flag)) {
    m_RiaVisionEnabled = (flag == 0xE8 || flag == 0xE9);
  }
  if (readByte(0x0DD3, flag)) {
    const int pieces = static_cast<int>(flag) - 0xC0;
    if (pieces >= 0 && pieces <= 8) {
      m_StaffPieces = pieces;
    }
  }
  if (readByte(0x0DDB, flag)) {
    m_HasMainQuestItem = (flag == 0x06);
  }
}

void ArenaSaveGame::resolveSavePath()
{
  QFileInfo fi(m_SaveFile);
  if (!fi.exists()) {
    return;
  }

  if (fi.isDir()) {
    m_SaveDir = fi.absoluteFilePath();
    const QDir dir(m_SaveDir);

    const auto exact = dir.entryInfoList(QStringList{"SAVEENGN.??"}, QDir::Files, QDir::Name);
    if (!exact.isEmpty()) {
      m_SaveFile = exact.first().absoluteFilePath();
      m_SlotSuffix = QFileInfo(m_SaveFile).suffix();
      return;
    }

    const auto any = dir.entryInfoList(QStringList{"SAVEENGN.*"}, QDir::Files, QDir::Name);
    if (!any.isEmpty()) {
      m_SaveFile = any.first().absoluteFilePath();
      m_SlotSuffix = QFileInfo(m_SaveFile).suffix();
      return;
    }

    return;
  }

  m_SaveDir = fi.absoluteDir().absolutePath();
  m_SlotSuffix = fi.suffix();
}
