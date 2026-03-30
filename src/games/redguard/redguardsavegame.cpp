#include "redguardsavegame.h"
#include "gameredguard.h"
#include "redguardsmapdatabase.h"
#include "redguardsrtxdatabase.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QMap>
#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QtEndian>

#include <iterator>
#include <limits>

namespace
{
constexpr int kSvitRecordSize = 0x16;
constexpr int kSvitCurrentCountOffset = 0x0A;
constexpr quint32 kSvitSavePayloadLength = 0x768;
constexpr int kSvitItemCount = 86;

struct SvitItemNameCache
{
  QMutex mutex;
  QHash<QString, QStringList> namesByDataPath;
};

struct SvitItemDisplayInfo
{
  QString name;
  int playerMax = 0;
};

struct SvitItemDisplayInfoCache
{
  QMutex mutex;
  QHash<QString, QVector<SvitItemDisplayInfo>> itemsByDataPath;
};

struct RtxSubtitleCache
{
  QMutex mutex;
  QHash<QString, QHash<QString, QString>> subtitlesByDataPath;
};

struct AreaTokenCandidate
{
  QString token;
  int score = (std::numeric_limits<int>::lowest)();
};

QStringList buildDefaultSvitItemNames()
{
  QStringList names;
  names.reserve(kSvitItemCount + 1);
  for (int i = 0; i < kSvitItemCount; ++i) {
    names.push_back(QStringLiteral("Item %1").arg(i));
  }
  names.push_back(QStringLiteral("LAST"));
  return names;
}

QVector<SvitItemDisplayInfo> buildDefaultSvitItemDisplayInfo()
{
  QVector<SvitItemDisplayInfo> items;
  items.reserve(kSvitItemCount + 1);
  for (int i = 0; i < kSvitItemCount; ++i) {
    items.push_back({QStringLiteral("Item %1").arg(i), 0});
  }
  items.push_back({QStringLiteral("LAST"), 0});
  return items;
}

QStringList loadSvitItemNamesFromDataDirectory(const QDir& dataDir)
{
  QStringList names = buildDefaultSvitItemNames();
  const QVector<SvitItemDisplayInfo> defaults = buildDefaultSvitItemDisplayInfo();
  if (!dataDir.exists()) {
    return names;
  }

  const QString rtxPath = dataDir.absoluteFilePath("ENGLISH.RTX");
  const QString itemPath = dataDir.absoluteFilePath("ITEM.INI");

  RedguardsRtxDatabase rtxDatabase;
  if (!rtxDatabase.readFile(rtxPath)) {
    return names;
  }

  RedguardsMapDatabase mapDatabase(rtxDatabase);
  if (!mapDatabase.readItemsFile(itemPath)) {
    return names;
  }

  const auto& items = mapDatabase.items();
  for (int i = 0; i < items.size() && i < kSvitItemCount; ++i) {
    names[i] = items[i].name.trimmed().isEmpty() ? defaults[i].name : items[i].name.trimmed();
  }

  return names;
}

QVector<SvitItemDisplayInfo> loadSvitItemDisplayInfoFromDataDirectory(const QDir& dataDir)
{
  QVector<SvitItemDisplayInfo> items = buildDefaultSvitItemDisplayInfo();
  if (!dataDir.exists()) {
    return items;
  }

  const QString rtxPath = dataDir.absoluteFilePath("ENGLISH.RTX");
  const QString itemPath = dataDir.absoluteFilePath("ITEM.INI");

  RedguardsRtxDatabase rtxDatabase;
  if (!rtxDatabase.readFile(rtxPath)) {
    return items;
  }

  RedguardsMapDatabase mapDatabase(rtxDatabase);
  if (!mapDatabase.readItemsFile(itemPath)) {
    return items;
  }

  const auto& mapItems = mapDatabase.items();
  for (int i = 0; i < mapItems.size() && i < kSvitItemCount; ++i) {
    if (!mapItems[i].name.trimmed().isEmpty()) {
      items[i].name = mapItems[i].name.trimmed();
    }
    items[i].playerMax = mapItems[i].playerMax;
  }

  return items;
}

QStringList loadCachedSvitItemNames(const QDir& dataDir)
{
  if (!dataDir.exists()) {
    return buildDefaultSvitItemNames();
  }

  const QString cacheKey = dataDir.absolutePath();
  if (cacheKey.isEmpty()) {
    return buildDefaultSvitItemNames();
  }

  static SvitItemNameCache cache;
  QMutexLocker lock(&cache.mutex);
  const auto cacheIt = cache.namesByDataPath.constFind(cacheKey);
  if (cacheIt != cache.namesByDataPath.cend()) {
    return cacheIt.value();
  }

  const QStringList names = loadSvitItemNamesFromDataDirectory(dataDir);
  cache.namesByDataPath.insert(cacheKey, names);
  return names;
}

QStringList loadSvitItemNames(const GameRedguard* game)
{
  if (game == nullptr) {
    return buildDefaultSvitItemNames();
  }

  return loadCachedSvitItemNames(game->dataDirectory());
}

QVector<SvitItemDisplayInfo> loadCachedSvitItemDisplayInfo(const QDir& dataDir)
{
  if (!dataDir.exists()) {
    return buildDefaultSvitItemDisplayInfo();
  }

  const QString cacheKey = dataDir.absolutePath();
  if (cacheKey.isEmpty()) {
    return buildDefaultSvitItemDisplayInfo();
  }

  static SvitItemDisplayInfoCache cache;
  QMutexLocker lock(&cache.mutex);
  const auto cacheIt = cache.itemsByDataPath.constFind(cacheKey);
  if (cacheIt != cache.itemsByDataPath.cend()) {
    return cacheIt.value();
  }

  const QVector<SvitItemDisplayInfo> items =
      loadSvitItemDisplayInfoFromDataDirectory(dataDir);
  cache.itemsByDataPath.insert(cacheKey, items);
  return items;
}

QVector<SvitItemDisplayInfo> loadSvitItemDisplayInfo(const GameRedguard* game)
{
  if (game == nullptr) {
    return buildDefaultSvitItemDisplayInfo();
  }

  return loadCachedSvitItemDisplayInfo(game->dataDirectory());
}

QHash<QString, QString> loadEnglishRtxSubtitlesFromDataDirectory(const QDir& dataDir)
{
  QHash<QString, QString> subtitles;
  if (!dataDir.exists()) {
    return subtitles;
  }

  const QString rtxPath = dataDir.absoluteFilePath("ENGLISH.RTX");
  RedguardsRtxDatabase rtxDatabase;
  if (!rtxDatabase.readFile(rtxPath)) {
    return subtitles;
  }

  const auto& entries = rtxDatabase.entries();
  subtitles.reserve(entries.size());
  for (auto it = entries.cbegin(); it != entries.cend(); ++it) {
    subtitles.insert(it.key().trimmed().toLower(), it.value().subtitle.trimmed());
  }
  return subtitles;
}

QHash<QString, QString> loadCachedEnglishRtxSubtitles(const GameRedguard* game)
{
  if (game == nullptr) {
    return {};
  }

  const QDir dataDir = game->dataDirectory();
  if (!dataDir.exists()) {
    return {};
  }

  const QString cacheKey = dataDir.absolutePath();
  if (cacheKey.isEmpty()) {
    return {};
  }

  static RtxSubtitleCache cache;
  QMutexLocker lock(&cache.mutex);
  const auto cacheIt = cache.subtitlesByDataPath.constFind(cacheKey);
  if (cacheIt != cache.subtitlesByDataPath.cend()) {
    return cacheIt.value();
  }

  const QHash<QString, QString> subtitles = loadEnglishRtxSubtitlesFromDataDirectory(dataDir);
  cache.subtitlesByDataPath.insert(cacheKey, subtitles);
  return subtitles;
}

void appendInventoryCountLine(QStringList& lines, const QString& label, quint32 count)
{
  if (count > 0) {
    lines << QString("%1: %2").arg(label).arg(count);
  }
}

QStringList buildCompactInventoryDetails(quint32 gold, quint32 ironSkinPotions,
                                         quint32 healthPotions, quint32 strengthPotions)
{
  QStringList lines;
  appendInventoryCountLine(lines, QStringLiteral("Gold"), gold);
  appendInventoryCountLine(lines, QStringLiteral("Potion of Ironskin"), ironSkinPotions);
  appendInventoryCountLine(lines, QStringLiteral("Health Potions"), healthPotions);
  appendInventoryCountLine(lines, QStringLiteral("Strength Potions"), strengthPotions);
  if (!lines.isEmpty()) {
    lines.prepend(QStringLiteral("Inventory:"));
  }
  return lines;
}

QStringList buildFullInventoryDetails(const QVector<quint32>& counts, const QStringList& names)
{
  QStringList lines;
  if (counts.isEmpty()) {
    return lines;
  }

  lines << QStringLiteral("Inventory:");
  const int limit = qMin(counts.size(), names.size());
  for (int itemId = 0; itemId < limit; ++itemId) {
    lines << QString("%1: %2").arg(names[itemId]).arg(counts[itemId]);
  }
  return lines;
}

QStringList buildFullInventoryDetails(const QVector<quint32>& counts,
                                      const QVector<SvitItemDisplayInfo>& items)
{
  QStringList lines;
  if (counts.isEmpty()) {
    return lines;
  }

  QStringList itemLines;
  const int limit = qMin(counts.size(), items.size());
  for (int itemId = 0; itemId < limit; ++itemId) {
    const quint32 count = counts[itemId];
    if (count == 0) {
      continue;
    }

    const QString label = items[itemId].name.trimmed().isEmpty()
                              ? QStringLiteral("Item %1").arg(itemId)
                              : items[itemId].name.trimmed();
    if (items[itemId].playerMax == 1) {
      itemLines << label;
    } else {
      itemLines << QString("%1: %2").arg(label).arg(count);
    }
  }

  if (!itemLines.isEmpty()) {
    lines << QStringLiteral("Inventory:");
    lines.append(itemLines);
  }
  return lines;
}

struct SaveChunkRow
{
  QByteArray tag;
  qsizetype tagOffset = -1;
  qsizetype payloadOffset = -1;
  quint32 payloadLength = 0;
  bool bounded = false;
};

QList<SaveChunkRow> buildChunkTable(const QByteArray& bytes)
{
  QList<SaveChunkRow> rows;
  if (bytes.size() < 8) {
    return rows;
  }

  qsizetype pos = 0;
  int guardCount = 0;
  constexpr int kMaxRows = 8192;
  while (pos + 8 <= bytes.size() && guardCount < kMaxRows) {
    SaveChunkRow row;
    row.tag = bytes.mid(pos, 4);
    row.tagOffset = pos;
    row.payloadLength = qFromBigEndian<quint32>(
        reinterpret_cast<const uchar*>(bytes.constData() + pos + 4));
    row.payloadOffset = pos + 8;
    row.bounded = (row.payloadOffset + static_cast<qsizetype>(row.payloadLength) <= bytes.size());
    rows.push_back(row);

    if (!row.bounded) {
      break;
    }

    pos = row.payloadOffset + static_cast<qsizetype>(row.payloadLength);
    ++guardCount;
  }

  return rows;
}

QString chunkDecodeStatus(const QByteArray& tag)
{
  if (tag == "THMB" || tag == "SVIT" || tag == "SVMD") {
    return QStringLiteral("parsed");
  }

  static const QSet<QByteArray> knownOpaque = {
      QByteArrayLiteral("SVGM"),
      QByteArrayLiteral("SCR3"),
      QByteArrayLiteral("SVGF"),
      QByteArrayLiteral("SMGR"),
      QByteArrayLiteral("SMOB"),
      QByteArrayLiteral("SMRP"),
      QByteArrayLiteral("SMCO"),
      QByteArrayLiteral("SMSH"),
      QByteArrayLiteral("SVCM"),
      QByteArrayLiteral("SMFM"),
      QByteArrayLiteral("SVRG"),
      QByteArrayLiteral("SVGV"),
      QByteArrayLiteral("SVG2"),
      QByteArrayLiteral("SVCB"),
      QByteArrayLiteral("SVMV"),
  };

  if (knownOpaque.contains(tag)) {
    return QStringLiteral("known-opaque");
  }
  return QStringLiteral("unknown");
}

QString formatChunkTable(const QList<SaveChunkRow>& rows)
{
  QStringList lines;
  lines << QStringLiteral("tag\ttag_offset\tpayload_offset\tlength_be\tbounds\tdecode_status");
  for (const SaveChunkRow& row : rows) {
    lines << QStringLiteral("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg(QString::fromLatin1(row.tag))
                 .arg(row.tagOffset)
                 .arg(row.payloadOffset)
                 .arg(row.payloadLength)
                 .arg(row.bounded ? QStringLiteral("ok") : QStringLiteral("out-of-bounds"))
                 .arg(chunkDecodeStatus(row.tag));
  }
  return lines.join('\n');
}

qsizetype findThumbnailSignature(const QByteArray& bytes)
{
  constexpr qsizetype kCanonicalOffset = 0x118;
  constexpr qsizetype kHeaderBytes     = 8;

  if (bytes.size() >= kCanonicalOffset + kHeaderBytes &&
      bytes.mid(kCanonicalOffset, 4) == "THMB") {
    return kCanonicalOffset;
  }

  qsizetype searchFrom = 0;
  while (searchFrom >= 0 && searchFrom + kHeaderBytes <= bytes.size()) {
    const qsizetype sig = bytes.indexOf("THMB", searchFrom);
    if (sig < 0 || sig + kHeaderBytes > bytes.size()) {
      return -1;
    }

    const quint32 payloadLen = qFromBigEndian<quint32>(
        reinterpret_cast<const uchar*>(bytes.constData() + sig + 4));
    const qsizetype payloadOff = sig + kHeaderBytes;
    if (payloadLen > 0 &&
        payloadOff + static_cast<qsizetype>(payloadLen) <= bytes.size()) {
      return sig;
    }
    searchFrom = sig + 1;
  }

  return -1;
}

bool findLastChunkPayload(const QByteArray& bytes, const char tag[4], qsizetype minOffset,
                          qsizetype* payloadOffset, quint32* payloadLength)
{
  qsizetype foundOffset = -1;
  quint32 foundLength = 0;
  qsizetype from = minOffset;
  while (from >= 0 && from + 8 <= bytes.size()) {
    const qsizetype pos = bytes.indexOf(tag, from);
    if (pos < 0 || pos + 8 > bytes.size()) {
      break;
    }
    const quint32 len = qFromBigEndian<quint32>(
        reinterpret_cast<const uchar*>(bytes.constData() + pos + 4));
    const qsizetype dataPos = pos + 8;
    if (dataPos + static_cast<qsizetype>(len) <= bytes.size()) {
      foundOffset = dataPos;
      foundLength = len;
    }
    from = pos + 1;
  }

  if (foundOffset < 0) {
    return false;
  }

  if (payloadOffset != nullptr) {
    *payloadOffset = foundOffset;
  }
  if (payloadLength != nullptr) {
    *payloadLength = foundLength;
  }
  return true;
}

AreaTokenCandidate extractAreaTokenFromTsg(const QByteArray& bytes)
{
  static const QRegularExpression tokenRx(R"(\b([A-Z]{2}[A-Z]{4}\d{2})\b)");
  QHash<QString, int> counts;
  auto it = tokenRx.globalMatch(QString::fromLatin1(bytes));
  while (it.hasNext()) {
    const QString token = it.next().captured(1);
    if (token == QStringLiteral("NCPLAK01")) {
      continue;
    }
    counts[token] += 1;
  }

  AreaTokenCandidate best;
  for (auto cIt = counts.cbegin(); cIt != counts.cend(); ++cIt) {
    int score = 0;
    if (cIt.key().startsWith(QStringLiteral("HB"))) {
      score += 10;
    }
    if (cIt.value() <= 3) {
      score += 5;
    } else {
      score -= cIt.value();
    }
    if (score > best.score) {
      best.score = score;
      best.token = cIt.key();
    }
  }
  return best;
}
}  // namespace

RedguardsSaveGame::RedguardsSaveGame(const QString& saveFolder,
                                     const GameRedguard* game)
    : XngineSaveGame(saveFolder, game), m_SaveFolder(saveFolder), m_SaveFile(saveFolder),
      m_Game(game)
{
  resolveSavePath();
  scanAuxiliaryFiles();
  detectSlotFromFolder();
  parseSaveHeader();
}

QString RedguardsSaveGame::getName() const
{
  QStringList parts;
  if (!m_SaveTitle.isEmpty()) {
    parts << m_SaveTitle;
  } else if (!m_DisplayName.isEmpty()) {
    parts << m_DisplayName;
  } else {
    parts << QStringLiteral("Redguard Save");
  }

  if (m_SaveNumber > 0) {
    parts << QString("Slot %1").arg(m_SaveNumber);
  }
  return parts.join(", ");
}

QString RedguardsSaveGame::getPCLevelText() const
{
  return {};
}

QString RedguardsSaveGame::getGameDetails() const
{
  const bool showDeveloperDetails =
      (m_Game != nullptr) ? m_Game->showDeveloperSaveDetails() : false;
  const bool showFullInventory =
      (m_Game != nullptr) ? m_Game->showFullSvitInventory() : false;

  QStringList lines;
  if (showDeveloperDetails && !m_AreaToken.isEmpty()) {
    lines << QString("Area: %1").arg(m_AreaToken);
  }

  const QStringList inventoryLines =
      showFullInventory ? buildFullInventoryDetails(m_SvitCurrentCounts,
                                                    loadSvitItemDisplayInfo(m_Game))
                        : buildCompactInventoryDetails(m_Gold, m_IronSkinPotions,
                                                        m_HealthPotions, m_StrengthPotions);

  if (!inventoryLines.isEmpty()) {
    if (!lines.isEmpty()) {
      lines << QString();
    }
    lines.append(inventoryLines);
  }

  return lines.join('\n');
}

QStringList RedguardsSaveGame::allFiles() const
{
  const QFileInfo fi(getFilepath());
  if (fi.isDir()) {
    return XngineSaveGame::allFiles();
  }
  if (!m_SaveFile.isEmpty()) {
    QStringList files{m_SaveFile};
    if (!files.contains(getFilepath())) {
      files.push_back(getFilepath());
    }
    return files;
  }
  return XngineSaveGame::allFiles();
}

QString RedguardsSaveGame::chunkTableDebugText() const
{
  if (!m_ChunkTableDebugText.isEmpty()) {
    return m_ChunkTableDebugText;
  }

  QFile f(m_SaveFile);
  if (!f.open(QIODevice::ReadOnly)) {
    return {};
  }

  const QByteArray bytes = f.readAll();
  f.close();
  m_ChunkTableDebugText = formatChunkTable(buildChunkTable(bytes));
  return m_ChunkTableDebugText;
}

void RedguardsSaveGame::resolveSavePath()
{
  const QFileInfo fi(m_SaveFile);
  if (!fi.exists()) {
    return;
  }

  if (fi.isDir()) {
    m_SaveFolder = fi.absoluteFilePath();
    const QString candidate = QDir(m_SaveFolder).filePath("SAVEGAME.SAV");
    if (QFileInfo::exists(candidate)) {
      m_SaveFile = candidate;
      return;
    }
    const auto matches = QDir(m_SaveFolder).entryInfoList(
        QStringList{"*.SAV", "*.sav"}, QDir::Files, QDir::Name);
    if (!matches.isEmpty()) {
      m_SaveFile = matches.first().absoluteFilePath();
    }
  } else {
    m_SaveFolder = fi.absoluteDir().absolutePath();
    m_SaveFile = fi.absoluteFilePath();
  }
}

void RedguardsSaveGame::scanAuxiliaryFiles()
{
  m_AuxiliaryFiles.clear();
  if (m_SaveFolder.isEmpty()) {
    return;
  }

  const QDir dir(m_SaveFolder);
  if (!dir.exists()) {
    return;
  }

  const auto entries = dir.entryInfoList(QDir::Files, QDir::Name);
  for (const auto& entry : entries) {
    const QString fileName = entry.fileName();
    if (fileName.compare("SAVEGAME.SAV", Qt::CaseInsensitive) == 0) {
      continue;
    }
    m_AuxiliaryFiles.push_back(fileName);
  }
}

void RedguardsSaveGame::parseAuxiliaryMetadata()
{
  m_AreaToken.clear();
  if (m_SaveFolder.isEmpty()) {
    return;
  }
  const QDir dir(m_SaveFolder);
  if (!dir.exists()) {
    return;
  }

  const auto tsgFiles = dir.entryInfoList(QStringList{"*.TSG", "*.tsg"}, QDir::Files,
                                          QDir::Name);
  if (tsgFiles.isEmpty()) {
    return;
  }

  AreaTokenCandidate bestCandidate;
  for (const auto& tsgInfo : tsgFiles) {
    QFile f(tsgInfo.absoluteFilePath());
    if (!f.open(QIODevice::ReadOnly)) {
      continue;
    }
    const QByteArray bytes = f.readAll();
    f.close();

    if (bytes.size() < 16) {
      continue;
    }

    const AreaTokenCandidate candidate = extractAreaTokenFromTsg(bytes);
    if (!candidate.token.isEmpty() && candidate.score > bestCandidate.score) {
      bestCandidate = candidate;
    }
  }

  m_AreaToken = bestCandidate.token;
}

void RedguardsSaveGame::detectSlotFromFolder()
{
  QFileInfo fi(m_SaveFolder);
  if (!fi.exists()) {
    fi = QFileInfo(m_SaveFile);
  }

  const QRegularExpression slotRe("(?i)^SAVEGAME\\.(\\d+)$");
  const auto match = slotRe.match(fi.fileName());
  if (!match.hasMatch()) {
    return;
  }

  bool ok = false;
  const int slot = match.captured(1).toInt(&ok);
  if (!ok || slot < 0) {
    return;
  }
  m_SaveNumber = static_cast<unsigned long>(slot);
}

bool RedguardsSaveGame::parseSaveHeader()
{
  QFile f(m_SaveFile);
  if (!f.open(QIODevice::ReadOnly)) {
    return false;
  }

  const QByteArray bytes = f.readAll();
  m_FileSize = static_cast<quint64>(f.size());
  f.close();
  if (bytes.size() < 0x30) {
    return false;
  }

  m_ValidSignature = (bytes.mid(0, 4) == "SVGM");
  m_FormatVersion = readFixedCString(bytes, 0x08, 8);
  m_SaveTitle = readFixedCString(bytes, 0x14, 128);
  m_HasThumbnail = (bytes.indexOf("THMB") >= 0);

  // Redguard has a fixed protagonist.
  m_PCName = QStringLiteral("Cyrus");

  parseStructuredMetadata(bytes);
  parseAuxiliaryMetadata();
  resolveLocationFromCode();

  if (m_SaveTitle.isEmpty()) {
    m_SaveTitle = m_DisplayName;
  }
  return true;
}

void RedguardsSaveGame::parseStructuredMetadata(const QByteArray& bytes)
{
  constexpr qsizetype kTailScanOffset = 0;

  // In SAVEGAME.SAV, the trailing bounded-valid SVIT chunk is a fixed-stride item table:
  // item ID n lives at base (n * 0x16), and current_count[n] is LE u32 at base + 0x0A.
  // This layout is confirmed for the 0x768-byte SAVEGAME.SAV payload. SVIT also appears in
  // some TSG files, but those variants should not be interpreted with SAVEGAME.SAV semantics.
  qsizetype svitOffset = -1;
  quint32 svitLen = 0;
  m_Gold = 0;
  m_IronSkinPotions = 0;
  m_HealthPotions = 0;
  m_StrengthPotions = 0;
  m_SvitCurrentCounts.fill(0, kSvitItemCount);
  if (findLastChunkPayload(bytes, "SVIT", kTailScanOffset, &svitOffset, &svitLen) &&
      svitLen >= kSvitSavePayloadLength) {
    const auto* svit = reinterpret_cast<const uchar*>(bytes.constData() + svitOffset);

    for (int itemId = 0; itemId < kSvitItemCount; ++itemId) {
      m_SvitCurrentCounts[itemId] = readSvitCurrentCount(svit, svitLen, itemId);
    }

    m_Gold = readSvitCurrentCount(svit, svitLen, 2);
    m_IronSkinPotions = readSvitCurrentCount(svit, svitLen, 3);
    m_HealthPotions = readSvitCurrentCount(svit, svitLen, 4);
    m_StrengthPotions = readSvitCurrentCount(svit, svitLen, 67);
  }

  // SVMD contains location code records and an active location index.
  qsizetype svmdOffset = -1;
  quint32 svmdLen = 0;
  if (!findLastChunkPayload(bytes, "SVMD", kTailScanOffset, &svmdOffset, &svmdLen) ||
      svmdLen < 10) {
    return;
  }

  const QByteArray payload = bytes.mid(svmdOffset, svmdLen);
  const quint16 activeLocationId = qFromLittleEndian<quint16>(
      reinterpret_cast<const uchar*>(payload.constData() + payload.size() - 2));

  QMap<quint32, QString> locationById;
  m_LocationCodes.clear();
  QSet<QString> seenCodes;
  QRegularExpression rx(R"(\?[A-Za-z]{3})");
  auto it = rx.globalMatch(QString::fromLatin1(payload));
  while (it.hasNext()) {
    const auto match = it.next();
    const qsizetype pos = match.capturedStart();
    if (pos < 4) {
      continue;
    }
    const quint32 locationId = qFromLittleEndian<quint32>(
        reinterpret_cast<const uchar*>(payload.constData() + pos - 4));
    QString code = match.captured(0);
    if (code.startsWith('?')) {
      code.remove(0, 1);
    }
    code = code.trimmed().toLower();
    locationById.insert(locationId, code);
    if (!code.isEmpty() && !seenCodes.contains(code)) {
      seenCodes.insert(code);
      m_LocationCodes.push_back(code);
    }
  }

  if (!locationById.contains(activeLocationId)) {
    return;
  }
  m_LocationCode = locationById.value(activeLocationId);
}

quint32 RedguardsSaveGame::readSvitCurrentCount(const uchar* svit, quint32 svitLen, int itemId)
{
  if (svit == nullptr || itemId < 0) {
    return 0;
  }

  const qsizetype offset =
      static_cast<qsizetype>(itemId) * kSvitRecordSize + kSvitCurrentCountOffset;
  if (offset + static_cast<qsizetype>(sizeof(quint32)) > static_cast<qsizetype>(svitLen)) {
    return 0;
  }

  return qFromLittleEndian<quint32>(svit + offset);
}

void RedguardsSaveGame::resolveLocationFromCode()
{
  if (m_LocationCode.isEmpty() && m_LocationCodes.isEmpty() && m_AreaToken.isEmpty()) {
    return;
  }

  // Resolve code labels from ENGLISH.RTX (e.g. ?smk -> STROS M'KAI)
  // instead of relying on hardcoded guesses.
  const QHash<QString, QString> subtitleByLabel = loadCachedEnglishRtxSubtitles(m_Game);

  QStringList candidates;
  if (!m_LocationCode.isEmpty()) {
    candidates.push_back(m_LocationCode.toLower());
  }
  for (const QString& code : m_LocationCodes) {
    const QString c = code.toLower();
    if (!candidates.contains(c)) {
      candidates.push_back(c);
    }
  }

  QString bestSubtitle;
  QString fallbackSubtitle;

  for (const QString& code : candidates) {
    const QString key = QStringLiteral("?") + code;
    const QString subtitle = subtitleByLabel.value(key).trimmed();
    if (subtitle.isEmpty()) {
      continue;
    }
    if (fallbackSubtitle.isEmpty()) {
      fallbackSubtitle = subtitle;
    }
    if (!isWeakLocationToken(code, subtitle)) {
      bestSubtitle = subtitle;
      break;
    }
  }

  if (!bestSubtitle.isEmpty()) {
    m_PCLocation = bestSubtitle;
    return;
  }
  if (!fallbackSubtitle.isEmpty()) {
    m_PCLocation = fallbackSubtitle;
    return;
  }

  if (!m_AreaToken.isEmpty()) {
    m_PCLocation = m_AreaToken.toUpper();
    return;
  }

  m_PCLocation = !m_LocationCode.isEmpty() ? m_LocationCode.toUpper()
                                           : m_LocationCodes.value(0).toUpper();
}

bool RedguardsSaveGame::isWeakLocationToken(const QString& code, const QString& subtitle)
{
  static const QSet<QString> weakCodes = {
      QStringLiteral("bye"),
      QStringLiteral("mus"),
      QStringLiteral("leg"),
      QStringLiteral("isz"),
  };

  static const QSet<QString> weakSubtitles = {
      QStringLiteral("BYE"),
      QStringLiteral("MUSIC"),
      QStringLiteral("LEAGUE"),
      QStringLiteral("ISZARA"),
  };

  if (weakCodes.contains(code.toLower())) {
    return true;
  }
  if (weakSubtitles.contains(subtitle.trimmed().toUpper())) {
    return true;
  }
  return false;
}

std::unique_ptr<XngineSaveGame::DataFields> RedguardsSaveGame::fetchDataFields() const
{
  auto fields = std::make_unique<DataFields>();

  QFile f(m_SaveFile);
  if (!f.open(QIODevice::ReadOnly)) {
    return fields;
  }
  const QByteArray bytes = f.readAll();
  f.close();
  if (bytes.size() < 0x120) {
    return fields;
  }

  const qsizetype sig = findThumbnailSignature(bytes);
  if (sig < 0 || sig + 8 >= bytes.size()) {
    return fields;
  }

  const quint32 rawLenBE = qFromBigEndian<quint32>(
      reinterpret_cast<const uchar*>(bytes.constData() + sig + 4));
  const quint16 width = qFromLittleEndian<quint16>(
      reinterpret_cast<const uchar*>(bytes.constData() + sig + 4));
  const quint16 height = qFromLittleEndian<quint16>(
      reinterpret_cast<const uchar*>(bytes.constData() + sig + 6));
  const qsizetype dataOff = sig + 8;

  // Common Redguard thumbnail payload observed in sample saves:
  // 0x18000 bytes = 256 * 192 * 2 (16-bit RGB).
  if (rawLenBE == 0x18000U && dataOff + static_cast<qsizetype>(rawLenBE) <= bytes.size()) {
    const int thmbW = 256;
    const int thmbH = 192;
    QImage image(thmbW, thmbH, QImage::Format_RGB32);
    const auto* src = reinterpret_cast<const uchar*>(bytes.constData() + dataOff);
    for (int y = 0; y < thmbH; ++y) {
      for (int x = 0; x < thmbW; ++x) {
        const qsizetype i = static_cast<qsizetype>(y) * thmbW * 2 + x * 2;
        const quint16 px = qFromLittleEndian<quint16>(src + i);
        const int r = ((px >> 11) & 0x1F) * 255 / 31;
        const int g = ((px >> 5) & 0x3F) * 255 / 63;
        const int b = (px & 0x1F) * 255 / 31;
        image.setPixelColor(x, y, QColor(r, g, b));
      }
    }
    fields->Screenshot = image;
    return fields;
  }

  if (width == 0 || height == 0 || width > 2048 || height > 2048) {
    return fields;
  }
  const qsizetype planeSize = static_cast<qsizetype>(width) * height;
  const qsizetype payloadSize = planeSize * 3;
  if (dataOff + payloadSize > bytes.size()) {
    return fields;
  }
  const auto* data = reinterpret_cast<const uchar*>(bytes.constData() + dataOff);
  const qsizetype rowStride = static_cast<qsizetype>(width) * 3;
  QImage image(static_cast<int>(width), static_cast<int>(height), QImage::Format_RGB32);
  for (int y = 0; y < static_cast<int>(height); ++y) {
    const qsizetype rowBase = static_cast<qsizetype>(y) * rowStride;
    const auto* rRow = data + rowBase;
    const auto* gRow = data + rowBase + width;
    const auto* bRow = data + rowBase + (width * 2);
    for (int x = 0; x < static_cast<int>(width); ++x) {
      image.setPixelColor(x, y, QColor(rRow[x], gRow[x], bRow[x]));
    }
  }
  fields->Screenshot = image;
  return fields;
}

QString RedguardsSaveGame::readFixedCString(const QByteArray& data, qsizetype offset,
                                            qsizetype maxLen)
{
  if (offset < 0 || maxLen <= 0 || offset + maxLen > data.size()) {
    return {};
  }

  QByteArray value = data.mid(offset, maxLen);
  const int nul = value.indexOf('\0');
  if (nul >= 0) {
    value.truncate(nul);
  }
  return QString::fromLocal8Bit(value).trimmed();
}
