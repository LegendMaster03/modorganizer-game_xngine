#include "redguardsavegame.h"
#include "gameredguard.h"
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

namespace
{
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

QString extractAreaTokenFromTsg(const QByteArray& bytes)
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

  QString bestToken;
  int bestScore = -9999;
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
    if (score > bestScore) {
      bestScore = score;
      bestToken = cIt.key();
    }
  }
  return bestToken;
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

QString RedguardsSaveGame::getGameDetails() const
{
  QStringList lines;
  if (m_Gold > 0) {
    lines << QString("Gold: %1").arg(m_Gold);
  }
  if (m_HealthPotions > 0) {
    lines << QString("Health Potions: %1").arg(m_HealthPotions);
  }
  if (!m_AreaToken.isEmpty()) {
    lines << QString("Area: %1").arg(m_AreaToken);
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

  QFile f(tsgFiles.first().absoluteFilePath());
  if (!f.open(QIODevice::ReadOnly)) {
    return;
  }
  const QByteArray bytes = f.readAll();
  f.close();

  if (bytes.size() < 16) {
    return;
  }

  m_AreaToken = extractAreaTokenFromTsg(bytes);
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

  // SVIT appears to hold most player state.
  qsizetype svitOffset = -1;
  quint32 svitLen = 0;
  if (findLastChunkPayload(bytes, "SVIT", kTailScanOffset, &svitOffset, &svitLen) &&
      svitLen >= 0x64) {
    const auto* svit = reinterpret_cast<const uchar*>(bytes.constData() + svitOffset);

    // Observed in provided saves: offset 0x36 tracks carried gold.
    // (offset 0x32 is another value and was causing incorrect Gold=500 readings)
    m_Gold = qFromLittleEndian<quint32>(svit + 0x36);

    // Observed in sample saves: offset 0x62 tracks health potion count.
    m_HealthPotions = qFromLittleEndian<quint32>(svit + 0x62);
  }

  // SVMD contains location code records and an active location index.
  qsizetype svmdOffset = -1;
  quint32 svmdLen = 0;
  if (!findLastChunkPayload(bytes, "SVMD", kTailScanOffset, &svmdOffset, &svmdLen) ||
      svmdLen < 10) {
    return;
  }

  if (svmdLen < 10) {
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

void RedguardsSaveGame::resolveLocationFromCode()
{
  if (m_LocationCode.isEmpty() && m_LocationCodes.isEmpty()) {
    return;
  }

  // Resolve code labels from ENGLISH.RTX (e.g. ?smk -> STROS M'KAI)
  // instead of relying on hardcoded guesses.
  static QMutex cacheMutex;
  static QHash<QString, QString> subtitleByLabel;
  static QString loadedFromPath;

  if (m_Game != nullptr) {
    const QString rtxPath = m_Game->dataDirectory().absoluteFilePath("ENGLISH.RTX");
    QMutexLocker lock(&cacheMutex);
    if (loadedFromPath != rtxPath) {
      subtitleByLabel.clear();
      loadedFromPath.clear();

      RedguardsRtxDatabase rtx;
      if (rtx.readFile(rtxPath)) {
        for (auto it = rtx.entries().cbegin(); it != rtx.entries().cend(); ++it) {
          subtitleByLabel.insert(it.key().trimmed().toLower(), it.value().subtitle.trimmed());
        }
        loadedFromPath = rtxPath;
      }
    }
  }

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
  QString fallbackCode;

  for (const QString& code : candidates) {
    const QString key = QStringLiteral("?") + code;
    const QString subtitle = subtitleByLabel.value(key).trimmed();
    if (subtitle.isEmpty()) {
      continue;
    }
    if (fallbackSubtitle.isEmpty()) {
      fallbackSubtitle = subtitle;
      fallbackCode = code;
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
