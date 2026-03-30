#include "redguardsavegame.h"
#include "gameredguard.h"
#include "redguardsmapdatabase.h"
#include "redguardsmapfile.h"
#include "redguardsrtxdatabase.h"
#include <xnginepaletteformat.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QMap>
#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QtEndian>

#include <functional>
#include <iterator>
#include <limits>

namespace
{
constexpr int kSvitRecordSize = 0x16;
constexpr int kSvitCurrentCountOffset = 0x0A;
constexpr quint32 kSvitSavePayloadLength = 0x768;
constexpr int kSvitItemCount = 86;
constexpr qsizetype kSvmdHeaderSize = 8;
constexpr qsizetype kSvmdRecordSize = 30;
constexpr qsizetype kSvmdRecordCodeOffset = 25;

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

struct OutdoorMapNameCache
{
  QMutex mutex;
  QHash<QString, QSet<QString>> outdoorMapsByDataPath;
};

struct WorldMapNameCache
{
  QMutex mutex;
  QHash<QString, QSet<QString>> mapNamesByDataPath;
};

struct MapDisplayNameCache
{
  QMutex mutex;
  QHash<QString, QHash<QString, QString>> displayNamesByDataPath;
};

struct MapLabelCache
{
  QMutex mutex;
  QHash<QString, QHash<QString, QSet<QString>>> labelsByDataPath;
};

struct MapTransitionDisplayNameCache
{
  QMutex mutex;
  QHash<QString, QHash<QString, QString>> displayNamesByDataPath;
};

struct MapPaletteCache
{
  QMutex mutex;
  QHash<QString, QHash<QString, XnginePaletteFormat::Palette>> palettesByDataPath;
};

QString findSoupPathForDataDirectory(const QDir& dataDir);

struct AreaTokenCandidate
{
  QString token;
  int score = (std::numeric_limits<int>::lowest)();
};

struct TsgNearestMatchDebug
{
  QString baseName;
  int totalScore = 0;
  int svmdCodeOverlap = 0;
  int svmdCodeUnion = 0;
  int svrgPrefixBytes = 0;
  bool svcbWord5Match = false;
  bool svcbWord3Match = false;
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

QString configuredSystemPathForDataDirectory(const GameRedguard* game, const QDir& dataDir,
                                             const QString& key, const QString& fallback)
{
  if (game != nullptr) {
    const QString configuredPath = game->configuredSystemPath(key, fallback);
    if (!configuredPath.isEmpty()) {
      return configuredPath;
    }
  }

  if (!dataDir.exists()) {
    return {};
  }

  const QString fallbackPath = dataDir.absoluteFilePath(fallback);
  return QFileInfo::exists(fallbackPath) ? fallbackPath : QString{};
}

QString configuredRtxPathForDataDirectory(const GameRedguard* game, const QDir& dataDir)
{
  return configuredSystemPathForDataDirectory(
      game, dataDir, QStringLiteral("rtx_filename"), QStringLiteral("ENGLISH.RTX"));
}

QString configuredWorldIniPathForDataDirectory(const GameRedguard* game, const QDir& dataDir)
{
  return configuredSystemPathForDataDirectory(
      game, dataDir, QStringLiteral("world_ini"), QStringLiteral("WORLD.INI"));
}

QString configuredItemIniPathForDataDirectory(const GameRedguard* game, const QDir& dataDir)
{
  return configuredSystemPathForDataDirectory(
      game, dataDir, QStringLiteral("item_ini"), QStringLiteral("ITEM.INI"));
}

QString buildRtxCacheKey(const GameRedguard* game, const QDir& dataDir)
{
  if (!dataDir.exists()) {
    return {};
  }

  const QString rtxPath = configuredRtxPathForDataDirectory(game, dataDir);
  const QString worldPath = configuredWorldIniPathForDataDirectory(game, dataDir);
  const QString itemPath = configuredItemIniPathForDataDirectory(game, dataDir);
  return dataDir.absolutePath() + QStringLiteral("|") + rtxPath + QStringLiteral("|") +
         worldPath + QStringLiteral("|") + itemPath;
}

QStringList loadSvitItemNamesFromDataDirectory(const GameRedguard* game, const QDir& dataDir)
{
  QStringList names = buildDefaultSvitItemNames();
  const QVector<SvitItemDisplayInfo> defaults = buildDefaultSvitItemDisplayInfo();
  if (!dataDir.exists()) {
    return names;
  }

  const QString rtxPath = configuredRtxPathForDataDirectory(game, dataDir);
  if (rtxPath.isEmpty()) {
    return names;
  }
  const QString itemPath = configuredItemIniPathForDataDirectory(game, dataDir);
  if (itemPath.isEmpty()) {
    return names;
  }

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

QVector<SvitItemDisplayInfo> loadSvitItemDisplayInfoFromDataDirectory(const GameRedguard* game,
                                                                      const QDir& dataDir)
{
  QVector<SvitItemDisplayInfo> items = buildDefaultSvitItemDisplayInfo();
  if (!dataDir.exists()) {
    return items;
  }

  const QString rtxPath = configuredRtxPathForDataDirectory(game, dataDir);
  if (rtxPath.isEmpty()) {
    return items;
  }
  const QString itemPath = configuredItemIniPathForDataDirectory(game, dataDir);
  if (itemPath.isEmpty()) {
    return items;
  }

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

QStringList loadCachedSvitItemNames(const GameRedguard* game, const QDir& dataDir)
{
  if (!dataDir.exists()) {
    return buildDefaultSvitItemNames();
  }

  const QString cacheKey = buildRtxCacheKey(game, dataDir);
  if (cacheKey.isEmpty()) {
    return buildDefaultSvitItemNames();
  }

  static SvitItemNameCache cache;
  QMutexLocker lock(&cache.mutex);
  const auto cacheIt = cache.namesByDataPath.constFind(cacheKey);
  if (cacheIt != cache.namesByDataPath.cend()) {
    return cacheIt.value();
  }

  const QStringList names = loadSvitItemNamesFromDataDirectory(game, dataDir);
  cache.namesByDataPath.insert(cacheKey, names);
  return names;
}

QStringList loadSvitItemNames(const GameRedguard* game)
{
  if (game == nullptr) {
    return buildDefaultSvitItemNames();
  }

  return loadCachedSvitItemNames(game, game->dataDirectory());
}

QVector<SvitItemDisplayInfo> loadCachedSvitItemDisplayInfo(const GameRedguard* game,
                                                           const QDir& dataDir)
{
  if (!dataDir.exists()) {
    return buildDefaultSvitItemDisplayInfo();
  }

  const QString cacheKey = buildRtxCacheKey(game, dataDir);
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
      loadSvitItemDisplayInfoFromDataDirectory(game, dataDir);
  cache.itemsByDataPath.insert(cacheKey, items);
  return items;
}

QVector<SvitItemDisplayInfo> loadSvitItemDisplayInfo(const GameRedguard* game)
{
  if (game == nullptr) {
    return buildDefaultSvitItemDisplayInfo();
  }

  return loadCachedSvitItemDisplayInfo(game, game->dataDirectory());
}

QHash<QString, QString> loadConfiguredRtxSubtitlesFromDataDirectory(const GameRedguard* game,
                                                                    const QDir& dataDir)
{
  QHash<QString, QString> subtitles;
  if (!dataDir.exists()) {
    return subtitles;
  }

  const QString rtxPath = configuredRtxPathForDataDirectory(game, dataDir);
  if (rtxPath.isEmpty()) {
    return subtitles;
  }
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

QHash<QString, QString> loadCachedConfiguredRtxSubtitles(const GameRedguard* game)
{
  if (game == nullptr) {
    return {};
  }

  const QDir dataDir = game->dataDirectory();
  if (!dataDir.exists()) {
    return {};
  }

  const QString cacheKey = buildRtxCacheKey(game, dataDir);
  if (cacheKey.isEmpty()) {
    return {};
  }

  static RtxSubtitleCache cache;
  QMutexLocker lock(&cache.mutex);
  const auto cacheIt = cache.subtitlesByDataPath.constFind(cacheKey);
  if (cacheIt != cache.subtitlesByDataPath.cend()) {
    return cacheIt.value();
  }

  const QHash<QString, QString> subtitles =
      loadConfiguredRtxSubtitlesFromDataDirectory(game, dataDir);
  cache.subtitlesByDataPath.insert(cacheKey, subtitles);
  return subtitles;
}

QSet<QString> loadWorldMapNamesFromDataDirectory(const GameRedguard* game, const QDir& dataDir)
{
  if (!dataDir.exists()) {
    return {};
  }

  const QString worldPath = configuredWorldIniPathForDataDirectory(game, dataDir);
  if (!QFileInfo::exists(worldPath)) {
    return {};
  }

  RedguardsRtxDatabase rtxDatabase;
  RedguardsMapDatabase mapDatabase(rtxDatabase);
  if (!mapDatabase.readWorldFile(worldPath)) {
    return {};
  }

  QSet<QString> names;
  for (auto* mapFile : mapDatabase.mapFiles()) {
    if (mapFile != nullptr && !mapFile->name().trimmed().isEmpty()) {
      names.insert(mapFile->name().trimmed().toUpper());
    }
  }
  return names;
}

QSet<QString> loadOutdoorWorldMapNamesFromDataDirectory(const GameRedguard* game,
                                                        const QDir& dataDir)
{
  const QString worldPath = configuredWorldIniPathForDataDirectory(game, dataDir);
  QFile file(worldPath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return {};
  }

  QHash<int, QString> mapNameById;
  QSet<int> outdoorIds;
  static const QRegularExpression mapRx(
      QStringLiteral(R"(^\s*world_map\[(\d+)\]\s*=\s*(.+?)\s*$)"),
      QRegularExpression::CaseInsensitiveOption);
  static const QRegularExpression worldRx(
      QStringLiteral(R"(^\s*world_world\[(\d+)\]\s*=\s*(.+?)\s*$)"),
      QRegularExpression::CaseInsensitiveOption);

  QTextStream in(&file);
  while (!in.atEnd()) {
    const QString line = in.readLine();
    const QRegularExpressionMatch mapMatch = mapRx.match(line);
    if (mapMatch.hasMatch()) {
      bool ok = false;
      const int id = mapMatch.captured(1).toInt(&ok);
      if (ok) {
        QString name = mapMatch.captured(2).trimmed().toUpper();
        name.replace('\\', '/');
        if (name.startsWith(QStringLiteral("MAPS/"))) {
          name = name.mid(5);
        }
        const int dot = name.indexOf('.');
        if (dot > 0) {
          name = name.left(dot);
        }
        if (!name.isEmpty()) {
          mapNameById.insert(id, name);
        }
      }
      continue;
    }

    const QRegularExpressionMatch worldMatch = worldRx.match(line);
    if (worldMatch.hasMatch()) {
      bool ok = false;
      const int id = worldMatch.captured(1).toInt(&ok);
      if (ok && !worldMatch.captured(2).trimmed().isEmpty()) {
        outdoorIds.insert(id);
      }
    }
  }

  QSet<QString> names;
  for (int id : outdoorIds) {
    const QString name = mapNameById.value(id);
    if (!name.isEmpty()) {
      names.insert(name);
    }
  }
  return names;
}

QSet<QString> loadCachedOutdoorWorldMapNames(const GameRedguard* game)
{
  if (game == nullptr) {
    return {};
  }

  const QDir dataDir = game->dataDirectory();
  if (!dataDir.exists()) {
    return {};
  }

  const QString cacheKey = buildRtxCacheKey(game, dataDir);
  if (cacheKey.isEmpty()) {
    return {};
  }

  static OutdoorMapNameCache cache;
  QMutexLocker lock(&cache.mutex);
  const auto cacheIt = cache.outdoorMapsByDataPath.constFind(cacheKey);
  if (cacheIt != cache.outdoorMapsByDataPath.cend()) {
    return cacheIt.value();
  }

  const QSet<QString> names = loadOutdoorWorldMapNamesFromDataDirectory(game, dataDir);
  cache.outdoorMapsByDataPath.insert(cacheKey, names);
  return names;
}

QSet<QString> loadCachedWorldMapNames(const GameRedguard* game)
{
  if (game == nullptr) {
    return {};
  }

  const QDir dataDir = game->dataDirectory();
  if (!dataDir.exists()) {
    return {};
  }

  const QString cacheKey = buildRtxCacheKey(game, dataDir);
  if (cacheKey.isEmpty()) {
    return {};
  }

  static WorldMapNameCache cache;
  QMutexLocker lock(&cache.mutex);
  const auto cacheIt = cache.mapNamesByDataPath.constFind(cacheKey);
  if (cacheIt != cache.mapNamesByDataPath.cend()) {
    return cacheIt.value();
  }

  const QSet<QString> names = loadWorldMapNamesFromDataDirectory(game, dataDir);
  cache.mapNamesByDataPath.insert(cacheKey, names);
  return names;
}

QString normalizeResolvedLocationSubtitle(QString subtitle)
{
  subtitle = subtitle.trimmed();
  if (subtitle.startsWith(QStringLiteral("ENTER "), Qt::CaseInsensitive)) {
    subtitle = subtitle.mid(6).trimmed();
  } else if (subtitle.startsWith(QStringLiteral("EXIT "), Qt::CaseInsensitive)) {
    subtitle = subtitle.mid(5).trimmed();
  }
  return subtitle;
}

bool isLocationStyleSubtitle(const QString& subtitle)
{
  const QString trimmed = normalizeResolvedLocationSubtitle(subtitle);
  if (trimmed.isEmpty()) {
    return false;
  }

  bool hasLetter = false;
  for (const QChar ch : trimmed) {
    if (ch.isLetter()) {
      hasLetter = true;
    }
    if (ch == QChar('?') || ch == QChar('!') || ch == QChar('.') || ch == QChar(':') ||
        ch == QChar(';')) {
      return false;
    }
  }
  return hasLetter;
}

QHash<QString, QString> loadCachedMapDisplayNames(const GameRedguard* game)
{
  if (game == nullptr) {
    return {};
  }

  const QDir dataDir = game->dataDirectory();
  if (!dataDir.exists()) {
    return {};
  }

  const QString cacheKey = buildRtxCacheKey(game, dataDir);
  if (cacheKey.isEmpty()) {
    return {};
  }

  static MapDisplayNameCache cache;
  QMutexLocker lock(&cache.mutex);
  const auto cacheIt = cache.displayNamesByDataPath.constFind(cacheKey);
  if (cacheIt != cache.displayNamesByDataPath.cend()) {
    return cacheIt.value();
  }

  const QSet<QString> mapNames = loadWorldMapNamesFromDataDirectory(game, dataDir);
  const QSet<QString> outdoorMapNames = loadOutdoorWorldMapNamesFromDataDirectory(game, dataDir);
  QHash<QString, QString> displayByBasename;

  const QString rtxPath = configuredRtxPathForDataDirectory(game, dataDir);
  const QString worldPath = configuredWorldIniPathForDataDirectory(game, dataDir);
  const QString itemPath = configuredItemIniPathForDataDirectory(game, dataDir);
  const QString soupPath = findSoupPathForDataDirectory(dataDir);
  const QString mapsRoot = dataDir.absoluteFilePath(QStringLiteral("maps"));
  if (!rtxPath.isEmpty() && !worldPath.isEmpty() && !itemPath.isEmpty() && !soupPath.isEmpty() &&
      QFileInfo::exists(rtxPath) && QFileInfo::exists(worldPath) && QFileInfo::exists(itemPath) &&
      QFileInfo::exists(soupPath) && QDir(mapsRoot).exists()) {
    RedguardsRtxDatabase rtxDatabase;
    if (rtxDatabase.readFile(rtxPath)) {
      RedguardsMapDatabase mapDatabase(rtxDatabase);
      if (mapDatabase.readWorldFile(worldPath) && mapDatabase.readSoupFile(soupPath)) {
        mapDatabase.readItemsFile(itemPath);

        static const QRegularExpression dlgCommentRx(
            QStringLiteral(R"(//\s*Dlg\s+([?$][A-Za-z0-9]{3})\s*=\s*(.+)$)"),
            QRegularExpression::CaseInsensitiveOption);

        for (auto* mapFile : mapDatabase.mapFiles()) {
          if (mapFile == nullptr) {
            continue;
          }
          const QString baseName = mapFile->name().trimmed().toUpper();
          if (!mapNames.contains(baseName)) {
            continue;
          }

          const QString mapPath =
              QDir(mapsRoot).absoluteFilePath(mapFile->name() + QStringLiteral(".RGM"));
          if (!QFileInfo::exists(mapPath) || !mapFile->readMap(mapPath)) {
            continue;
          }

          QHash<QString, int> scoreByDisplay;
          const QStringList lines = mapFile->getScript().split('\n', Qt::KeepEmptyParts);
          for (const QString& line : lines) {
            const QRegularExpressionMatch dlgMatch = dlgCommentRx.match(line.trimmed());
            if (!dlgMatch.hasMatch()) {
              continue;
            }

            const QString subtitle = normalizeResolvedLocationSubtitle(dlgMatch.captured(2));
            if (!isLocationStyleSubtitle(subtitle)) {
              continue;
            }

            int score = 1;
            if (outdoorMapNames.contains(baseName)) {
              score += 1;
            }
            scoreByDisplay[subtitle] += score;
          }

          int bestScore = std::numeric_limits<int>::lowest();
          QString bestDisplay;
          bool ambiguousBest = false;
          for (auto itScore = scoreByDisplay.cbegin(); itScore != scoreByDisplay.cend(); ++itScore) {
            if (itScore.value() > bestScore) {
              bestScore = itScore.value();
              bestDisplay = itScore.key();
              ambiguousBest = false;
            } else if (itScore.value() == bestScore && itScore.key() != bestDisplay) {
              ambiguousBest = true;
            }
          }

          if (bestScore > 0 && !ambiguousBest && !bestDisplay.isEmpty()) {
            displayByBasename.insert(baseName, bestDisplay);
          }
        }
      }
    }
  }

  cache.displayNamesByDataPath.insert(cacheKey, displayByBasename);
  return displayByBasename;
}

QSet<QString> scanMapLabels(const QString& mapPath)
{
  QFile mapFile(mapPath);
  if (!mapFile.open(QIODevice::ReadOnly)) {
    return {};
  }

  const QString mapText = QString::fromLatin1(mapFile.readAll());
  mapFile.close();

  static const QRegularExpression labelRx(QStringLiteral(R"(([?$][A-Za-z0-9]{3}))"));
  QSet<QString> labels;
  auto it = labelRx.globalMatch(mapText);
  while (it.hasNext()) {
    labels.insert(it.next().captured(1).toLower());
  }
  return labels;
}

QHash<QString, QSet<QString>> loadCachedMapLabels(const GameRedguard* game)
{
  if (game == nullptr) {
    return {};
  }

  const QDir dataDir = game->dataDirectory();
  if (!dataDir.exists()) {
    return {};
  }

  const QString cacheKey = buildRtxCacheKey(game, dataDir);
  if (cacheKey.isEmpty()) {
    return {};
  }

  static MapLabelCache cache;
  QMutexLocker lock(&cache.mutex);
  const auto cacheIt = cache.labelsByDataPath.constFind(cacheKey);
  if (cacheIt != cache.labelsByDataPath.cend()) {
    return cacheIt.value();
  }

  QHash<QString, QSet<QString>> labelsByBasename;
  const QSet<QString> mapNames = loadWorldMapNamesFromDataDirectory(game, dataDir);
  for (const QString& baseName : mapNames) {
    const QString mapPath =
        dataDir.absoluteFilePath(QStringLiteral("maps/%1.RGM").arg(baseName.trimmed()));
    const QSet<QString> labels = scanMapLabels(mapPath);
    if (!labels.isEmpty()) {
      labelsByBasename.insert(baseName.toUpper(), labels);
    }
  }

  cache.labelsByDataPath.insert(cacheKey, labelsByBasename);
  return labelsByBasename;
}

QString findSoupPathForDataDirectory(const QDir& dataDir)
{
  const QStringList candidates = {
      dataDir.absoluteFilePath(QStringLiteral("soup386/SOUP386.DEF")),
      dataDir.absoluteFilePath(QStringLiteral("SOUP386.DEF")),
  };
  for (const QString& path : candidates) {
    if (QFileInfo::exists(path)) {
      return path;
    }
  }
  return {};
}

QHash<QString, QString> loadTransitionDerivedMapDisplayNames(const GameRedguard* game,
                                                             const QDir& dataDir)
{
  QHash<QString, QString> resolved;
  if (!dataDir.exists()) {
    return resolved;
  }

  const QString rtxPath = configuredRtxPathForDataDirectory(game, dataDir);
  const QString worldPath = configuredWorldIniPathForDataDirectory(game, dataDir);
  const QString itemPath = configuredItemIniPathForDataDirectory(game, dataDir);
  const QString soupPath = findSoupPathForDataDirectory(dataDir);
  const QString mapsRoot = dataDir.absoluteFilePath(QStringLiteral("maps"));
  if (rtxPath.isEmpty() || itemPath.isEmpty() || worldPath.isEmpty() ||
      !QFileInfo::exists(rtxPath) || !QFileInfo::exists(worldPath) ||
      !QFileInfo::exists(itemPath) ||
      soupPath.isEmpty() ||
      !QDir(mapsRoot).exists()) {
    return resolved;
  }

  RedguardsRtxDatabase rtxDatabase;
  if (!rtxDatabase.readFile(rtxPath)) {
    return resolved;
  }

  RedguardsMapDatabase mapDatabase(rtxDatabase);
  if (!mapDatabase.readWorldFile(worldPath) || !mapDatabase.readSoupFile(soupPath)) {
    return resolved;
  }
  if (QFileInfo::exists(itemPath)) {
    mapDatabase.readItemsFile(itemPath);
  }

  static const QRegularExpression loadWorldRx(
      QStringLiteral(R"(LoadWorld\(<([^>]+)>\))"),
      QRegularExpression::CaseInsensitiveOption);
  static const QRegularExpression dlgCommentRx(
      QStringLiteral(R"(//\s*Dlg\s+([?$][A-Za-z0-9]{3})\s*=\s*(.+)$)"),
      QRegularExpression::CaseInsensitiveOption);

  auto findNearestDialogueDisplayInBlock = [&](const QStringList& lines, int loadWorldIndex) -> QString {
    constexpr int kMaxProbeDistance = 24;

    for (int distance = 0; distance <= kMaxProbeDistance; ++distance) {
      const QList<int> probes = (distance == 0)
                                    ? QList<int>{loadWorldIndex}
                                    : QList<int>{loadWorldIndex - distance, loadWorldIndex + distance};
      for (const int probe : probes) {
        if (probe < 0 || probe >= lines.size()) {
          continue;
        }

        const QString probeLine = lines[probe].trimmed();
        if (probe != loadWorldIndex &&
            (probeLine == QStringLiteral("{") || probeLine == QStringLiteral("}"))) {
          continue;
        }
        if (probeLine.startsWith('#')) {
          continue;
        }

        const QRegularExpressionMatch dlgMatch = dlgCommentRx.match(probeLine);
        if (!dlgMatch.hasMatch()) {
          continue;
        }

        const QString subtitle = normalizeResolvedLocationSubtitle(dlgMatch.captured(2));
        if (isLocationStyleSubtitle(subtitle)) {
          return subtitle;
        }
      }
    }

    return {};
  };

  QHash<QString, QSet<QString>> candidatesByTarget;
  for (auto* mapFile : mapDatabase.mapFiles()) {
    if (mapFile == nullptr) {
      continue;
    }

    const QString mapPath =
        QDir(mapsRoot).absoluteFilePath(mapFile->name() + QStringLiteral(".RGM"));
    if (!QFileInfo::exists(mapPath) || !mapFile->readMap(mapPath)) {
      continue;
    }

    const QStringList lines = mapFile->getScript().split('\n', Qt::KeepEmptyParts);
    for (int index = 0; index < lines.size(); ++index) {
      const QString line = lines[index].trimmed();
      const QRegularExpressionMatch loadMatch = loadWorldRx.match(line);
      if (!loadMatch.hasMatch()) {
        continue;
      }

      const QString targetMap = loadMatch.captured(1).trimmed().toUpper();
      if (targetMap.isEmpty()) {
        continue;
      }

      const QString display = findNearestDialogueDisplayInBlock(lines, index);
      if (!display.isEmpty()) {
        candidatesByTarget[targetMap].insert(display);
      }
    }
  }

  for (auto it = candidatesByTarget.cbegin(); it != candidatesByTarget.cend(); ++it) {
    if (it.value().size() == 1) {
      resolved.insert(it.key(), *it.value().cbegin());
    }
  }

  return resolved;
}

QHash<QString, QString> loadWorldPalettePathsFromDataDirectory(const GameRedguard* game,
                                                               const QDir& dataDir)
{
  const QString worldPath = configuredWorldIniPathForDataDirectory(game, dataDir);
  QFile file(worldPath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return {};
  }

  QHash<int, QString> mapNameById;
  QHash<int, QString> palettePathById;
  static const QRegularExpression mapRx(
      QStringLiteral(R"(^\s*world_map\[(\d+)\]\s*=\s*(.+?)\s*$)"),
      QRegularExpression::CaseInsensitiveOption);
  static const QRegularExpression paletteRx(
      QStringLiteral(R"(^\s*world_palette\[(\d+)\]\s*=\s*(.+?)\s*$)"),
      QRegularExpression::CaseInsensitiveOption);

  QTextStream in(&file);
  while (!in.atEnd()) {
    const QString line = in.readLine();

    const QRegularExpressionMatch mapMatch = mapRx.match(line);
    if (mapMatch.hasMatch()) {
      bool ok = false;
      const int id = mapMatch.captured(1).toInt(&ok);
      if (ok) {
        QString name = mapMatch.captured(2).trimmed().toUpper();
        name.replace('\\', '/');
        if (name.startsWith(QStringLiteral("MAPS/"))) {
          name = name.mid(5);
        }
        const int dot = name.indexOf('.');
        if (dot > 0) {
          name = name.left(dot);
        }
        if (!name.isEmpty()) {
          mapNameById.insert(id, name);
        }
      }
      continue;
    }

    const QRegularExpressionMatch paletteMatch = paletteRx.match(line);
    if (paletteMatch.hasMatch()) {
      bool ok = false;
      const int id = paletteMatch.captured(1).toInt(&ok);
      if (ok) {
        QString palettePath = paletteMatch.captured(2).trimmed();
        palettePath.replace('\\', '/');
        if (!palettePath.isEmpty()) {
          palettePathById.insert(id, palettePath);
        }
      }
    }
  }

  QHash<QString, QString> paletteByBasename;
  for (auto it = mapNameById.cbegin(); it != mapNameById.cend(); ++it) {
    const QString palettePath = palettePathById.value(it.key()).trimmed();
    if (!palettePath.isEmpty()) {
      paletteByBasename.insert(it.value().toUpper(), palettePath);
    }
  }
  return paletteByBasename;
}

QHash<QString, XnginePaletteFormat::Palette> loadMapPalettesFromDataDirectory(
    const GameRedguard* game, const QDir& dataDir)
{
  QHash<QString, XnginePaletteFormat::Palette> palettes;
  if (!dataDir.exists()) {
    return palettes;
  }

  const QHash<QString, QString> palettePaths =
      loadWorldPalettePathsFromDataDirectory(game, dataDir);
  for (auto it = palettePaths.cbegin(); it != palettePaths.cend(); ++it) {
    const QString fullPath = dataDir.absoluteFilePath(it.value());
    XnginePaletteFormat::Document paletteDocument;
    if (XnginePaletteFormat::readFile(fullPath, paletteDocument)) {
      palettes.insert(it.key(), paletteDocument.palette);
    }
  }

  return palettes;
}

QHash<QString, XnginePaletteFormat::Palette> loadCachedMapPalettes(const GameRedguard* game)
{
  if (game == nullptr) {
    return {};
  }

  const QDir dataDir = game->dataDirectory();
  if (!dataDir.exists()) {
    return {};
  }

  const QString cacheKey = buildRtxCacheKey(game, dataDir);
  if (cacheKey.isEmpty()) {
    return {};
  }

  static MapPaletteCache cache;
  QMutexLocker lock(&cache.mutex);
  const auto cacheIt = cache.palettesByDataPath.constFind(cacheKey);
  if (cacheIt != cache.palettesByDataPath.cend()) {
    return cacheIt.value();
  }

  const auto palettes = loadMapPalettesFromDataDirectory(game, dataDir);
  cache.palettesByDataPath.insert(cacheKey, palettes);
  return palettes;
}

bool decodeIndexedImage(const uchar* src, int width, int height,
                        const XnginePaletteFormat::Palette& palette, QImage& outImage)
{
  if (src == nullptr || width <= 0 || height <= 0 || palette.colors.size() != 256) {
    return false;
  }

  outImage = QImage(width, height, QImage::Format_RGB32);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const int index = src[static_cast<qsizetype>(y) * width + x];
      outImage.setPixelColor(x, y, palette.colors.at(index));
    }
  }
  return true;
}

QHash<QString, QString> loadCachedTransitionDerivedMapDisplayNames(const GameRedguard* game)
{
  if (game == nullptr) {
    return {};
  }

  const QDir dataDir = game->dataDirectory();
  if (!dataDir.exists()) {
    return {};
  }

  const QString cacheKey = buildRtxCacheKey(game, dataDir);
  if (cacheKey.isEmpty()) {
    return {};
  }

  static MapTransitionDisplayNameCache cache;
  QMutexLocker lock(&cache.mutex);
  const auto cacheIt = cache.displayNamesByDataPath.constFind(cacheKey);
  if (cacheIt != cache.displayNamesByDataPath.cend()) {
    return cacheIt.value();
  }

  const QHash<QString, QString> resolved = loadTransitionDerivedMapDisplayNames(game, dataDir);
  cache.displayNamesByDataPath.insert(cacheKey, resolved);
  return resolved;
}

void appendInventoryCountLine(QStringList& lines, const QString& label, quint32 count)
{
  if (count > 0) {
    lines << QString("%1: %2").arg(label).arg(count);
  }
}

QString itemDisplayLabel(const QVector<SvitItemDisplayInfo>& items, int itemId, const QString& fallback)
{
  if (itemId >= 0 && itemId < items.size()) {
    const QString label = items[itemId].name.trimmed();
    if (!label.isEmpty()) {
      return label;
    }
  }
  return fallback;
}

QStringList buildCompactInventoryDetails(const QVector<SvitItemDisplayInfo>& items, quint32 gold,
                                         quint32 ironSkinPotions, quint32 healthPotions,
                                         quint32 strengthPotions)
{
  QStringList lines;
  appendInventoryCountLine(lines, itemDisplayLabel(items, 2, QStringLiteral("Gold")), gold);
  appendInventoryCountLine(lines, itemDisplayLabel(items, 3, QStringLiteral("Potion of Ironskin")),
                           ironSkinPotions);
  appendInventoryCountLine(lines, itemDisplayLabel(items, 4, QStringLiteral("Health Potions")),
                           healthPotions);
  appendInventoryCountLine(lines, itemDisplayLabel(items, 67, QStringLiteral("Strength Potions")),
                           strengthPotions);
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
    const quint32 canonicalPayloadLen = qFromBigEndian<quint32>(
        reinterpret_cast<const uchar*>(bytes.constData() + kCanonicalOffset + 4));
    const qsizetype canonicalPayloadOff = kCanonicalOffset + kHeaderBytes;
    if (canonicalPayloadLen > 0 &&
        canonicalPayloadOff + static_cast<qsizetype>(canonicalPayloadLen) <= bytes.size()) {
      return kCanonicalOffset;
    }
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

QByteArray extractLastChunkPayloadCopy(const QByteArray& bytes, const char tag[4],
                                       qsizetype minOffset = 0)
{
  qsizetype payloadOffset = -1;
  quint32 payloadLength = 0;
  if (!findLastChunkPayload(bytes, tag, minOffset, &payloadOffset, &payloadLength) ||
      payloadOffset < 0 || payloadLength == 0) {
    return {};
  }
  return bytes.mid(payloadOffset, payloadLength);
}

bool readLE32(const QByteArray& data, qsizetype offset, quint32& outValue)
{
  if (offset < 0 || offset + 4 > data.size()) {
    return false;
  }

  outValue = qFromLittleEndian<quint32>(
      reinterpret_cast<const uchar*>(data.constData() + offset));
  return true;
}

bool readLE16(const QByteArray& data, qsizetype offset, quint16& outValue)
{
  if (offset < 0 || offset + 2 > data.size()) {
    return false;
  }

  outValue = qFromLittleEndian<quint16>(
      reinterpret_cast<const uchar*>(data.constData() + offset));
  return true;
}

bool readSvcbWord(const QByteArray& payload, qsizetype wordIndex, quint32& outValue)
{
  const qsizetype offset = wordIndex * static_cast<qsizetype>(sizeof(quint32));
  return readLE32(payload, offset, outValue);
}

QString readSvmdCode(const QByteArray& payload, qsizetype recordOffset)
{
  if (recordOffset < 0 ||
      recordOffset + kSvmdRecordCodeOffset + 4 > payload.size()) {
    return {};
  }

  const QByteArray raw =
      payload.mid(recordOffset + kSvmdRecordCodeOffset, 4);
  if (raw.size() != 4 || raw[0] != '?') {
    return {};
  }

  return QString::fromLatin1(raw).trimmed().toLower();
}

QSet<QString> readSvmdCodeSet(const QByteArray& payload)
{
  QSet<QString> codes;
  if (payload.size() < kSvmdHeaderSize + 4) {
    return codes;
  }

  for (qsizetype recordOffset = kSvmdHeaderSize;
       recordOffset + kSvmdRecordCodeOffset + 4 <= payload.size();
       recordOffset += kSvmdRecordSize) {
    const QString code = readSvmdCode(payload, recordOffset);
    if (code.isEmpty()) {
      break;
    }
    codes.insert(code);
  }

  return codes;
}

QString formatHexBytes(const QByteArray& data)
{
  if (data.isEmpty()) {
    return QStringLiteral("(none)");
  }

  QStringList parts;
  parts.reserve(data.size());
  for (uchar byte : data) {
    parts.push_back(QStringLiteral("%1").arg(byte, 2, 16, QLatin1Char('0')).toUpper());
  }
  return parts.join(' ');
}

int commonPrefixLength(const QByteArray& lhs, const QByteArray& rhs)
{
  const int limit = qMin(lhs.size(), rhs.size());
  int index = 0;
  while (index < limit && lhs[index] == rhs[index]) {
    ++index;
  }
  return index;
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
  if (showDeveloperDetails && !m_ExactSvmdTsgMatches.isEmpty()) {
    lines << QString("SVMD Match: %1").arg(m_ExactSvmdTsgMatches.join(", "));
  }
  if (showDeveloperDetails && !m_SvmdHeaderDebug.isEmpty()) {
    lines << QString("SVMD Header: %1").arg(m_SvmdHeaderDebug);
  }
  if (showDeveloperDetails && !m_SvmdRecordDebug.isEmpty()) {
    lines << QString("SVMD Records: %1").arg(m_SvmdRecordDebug.join(" | "));
  }
  if (showDeveloperDetails && !m_SvmdTrailerDebug.isEmpty()) {
    lines << QString("SVMD Trailer: %1").arg(m_SvmdTrailerDebug);
  }
  if (showDeveloperDetails && !m_SvmdCandidates.isEmpty()) {
    lines << QString("SVMD Candidates: %1").arg(m_SvmdCandidates.join(", "));
  }
  if (showDeveloperDetails && !m_SvcbCandidates.isEmpty()) {
    lines << QString("After SVCB: %1").arg(m_SvcbCandidates.join(", "));
  }
  if (showDeveloperDetails && !m_SvcbWord5Candidates.isEmpty()) {
    lines << QString("After SVCB word5: %1").arg(m_SvcbWord5Candidates.join(", "));
  }
  if (showDeveloperDetails && !m_SvcbWord3Candidates.isEmpty()) {
    lines << QString("After SVCB word3: %1").arg(m_SvcbWord3Candidates.join(", "));
  }
  if (showDeveloperDetails && !m_SvrgCandidates.isEmpty()) {
    lines << QString("After SVRG: %1").arg(m_SvrgCandidates.join(", "));
  }
  if (showDeveloperDetails && !m_NearestTsgCandidates.isEmpty()) {
    lines << QString("Nearest TSG: %1").arg(m_NearestTsgCandidates.join(" | "));
  }
  if (showDeveloperDetails) {
    lines << QString("SVMD Display: %1")
                 .arg(m_SvmdDisplayCandidates.isEmpty()
                          ? QStringLiteral("(none)")
                          : m_SvmdDisplayCandidates.join(", "));
  }
  if (showDeveloperDetails) {
    lines << QString("Map Display: %1")
                 .arg(m_MapDisplayCandidates.isEmpty() ? QStringLiteral("(none)")
                                                       : m_MapDisplayCandidates.join(", "));
    lines << QString("Transition Display: %1")
                 .arg(m_TransitionDisplayCandidates.isEmpty()
                          ? QStringLiteral("(none)")
                          : m_TransitionDisplayCandidates.join(", "));
    lines << QString("Resolved Map: %1")
                 .arg(m_ResolvedLocationBasename.isEmpty() ? QStringLiteral("(none)")
                                                           : m_ResolvedLocationBasename);
    lines << QString("Resolved By: %1")
                 .arg(m_ResolvedLocationSource.isEmpty() ? QStringLiteral("(none)")
                                                         : m_ResolvedLocationSource);
  }

  const QVector<SvitItemDisplayInfo> itemDisplayInfo = loadSvitItemDisplayInfo(m_Game);
  const QStringList inventoryLines =
      showFullInventory ? buildFullInventoryDetails(m_SvitCurrentCounts, itemDisplayInfo)
                        : buildCompactInventoryDetails(itemDisplayInfo, m_Gold, m_IronSkinPotions,
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
  struct TsgMatchCandidate
  {
    QString baseName;
    QByteArray svcbPayload;
    QByteArray svrgPayload;
  };

  m_AreaToken.clear();
  m_SvmdCandidates.clear();
  m_SvcbCandidates.clear();
  m_SvcbWord5Candidates.clear();
  m_SvcbWord3Candidates.clear();
  m_SvrgCandidates.clear();
  m_NearestTsgCandidates.clear();
  m_ExactSvmdTsgMatches.clear();
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
  const QSet<QString> worldMapNames = loadCachedWorldMapNames(m_Game);
  const QSet<QString> saveSvmdCodeSet = QSet<QString>(m_LocationCodes.cbegin(), m_LocationCodes.cend());
  QList<TsgMatchCandidate> exactSvmdCandidates;
  QList<TsgMatchCandidate> svmdCodeSetCandidates;
  QList<TsgNearestMatchDebug> nearestMatches;
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

    if (!m_SaveSvmdPayload.isEmpty()) {
      const QByteArray tsgSvmd = extractLastChunkPayloadCopy(bytes, "SVMD");
      if (!tsgSvmd.isEmpty()) {
        const QString baseName = tsgInfo.completeBaseName().trimmed().toUpper();
        if (!baseName.isEmpty() &&
            (worldMapNames.isEmpty() || worldMapNames.contains(baseName))) {
          const QByteArray svcbPayload = extractLastChunkPayloadCopy(bytes, "SVCB");
          const QByteArray svrgPayload = extractLastChunkPayloadCopy(bytes, "SVRG");
          const TsgMatchCandidate candidate{
              baseName,
              svcbPayload,
              svrgPayload,
          };
          if (tsgSvmd == m_SaveSvmdPayload) {
            exactSvmdCandidates.push_back(candidate);
          } else if (!saveSvmdCodeSet.isEmpty() && readSvmdCodeSet(tsgSvmd) == saveSvmdCodeSet) {
            svmdCodeSetCandidates.push_back(candidate);
          }

          TsgNearestMatchDebug debugRow;
          debugRow.baseName = baseName;
          const QSet<QString> tsgCodeSet = readSvmdCodeSet(tsgSvmd);
          int overlapCount = 0;
          for (const QString& code : saveSvmdCodeSet) {
            if (tsgCodeSet.contains(code)) {
              ++overlapCount;
            }
          }
          debugRow.svmdCodeOverlap = overlapCount;
          debugRow.svmdCodeUnion = saveSvmdCodeSet.size();
          for (const QString& code : tsgCodeSet) {
            if (!saveSvmdCodeSet.contains(code)) {
              ++debugRow.svmdCodeUnion;
            }
          }

          quint32 saveWord = 0;
          quint32 candidateWord = 0;
          if (readSvcbWord(m_SaveSvcbPayload, 5, saveWord) && readSvcbWord(svcbPayload, 5, candidateWord)) {
            debugRow.svcbWord5Match = (saveWord == candidateWord);
          }
          if (readSvcbWord(m_SaveSvcbPayload, 3, saveWord) && readSvcbWord(svcbPayload, 3, candidateWord)) {
            debugRow.svcbWord3Match = (saveWord == candidateWord);
          }
          debugRow.svrgPrefixBytes = commonPrefixLength(m_SaveSvrgPayload, svrgPayload);
          debugRow.totalScore = debugRow.svmdCodeOverlap * 100 +
                                (debugRow.svcbWord5Match ? 25 : 0) +
                                (debugRow.svcbWord3Match ? 15 : 0) +
                                qMin(debugRow.svrgPrefixBytes, 50);
          nearestMatches.push_back(debugRow);
        }
      }
    }
  }

  auto narrowCandidatesByPayload = [](const QList<TsgMatchCandidate>& candidates,
                                      const QByteArray& savePayload,
                                      const std::function<QByteArray(const TsgMatchCandidate&)>& getPayload)
      -> QList<TsgMatchCandidate> {
    if (candidates.size() <= 1 || savePayload.isEmpty()) {
      return candidates;
    }

    QList<TsgMatchCandidate> narrowed;
    for (const TsgMatchCandidate& candidate : candidates) {
      const QByteArray candidatePayload = getPayload(candidate);
      if (!candidatePayload.isEmpty() && candidatePayload == savePayload) {
        narrowed.push_back(candidate);
      }
    }

    return narrowed.isEmpty() ? candidates : narrowed;
  };

  auto narrowCandidatesBySvcbWord = [](const QList<TsgMatchCandidate>& candidates,
                                       const QByteArray& saveSvcbPayload, qsizetype wordIndex)
      -> QList<TsgMatchCandidate> {
    if (candidates.size() <= 1 || saveSvcbPayload.size() < 32) {
      return candidates;
    }

    quint32 saveWord = 0;
    if (!readSvcbWord(saveSvcbPayload, wordIndex, saveWord)) {
      return candidates;
    }

    QList<TsgMatchCandidate> narrowed;
    for (const TsgMatchCandidate& candidate : candidates) {
      quint32 candidateWord = 0;
      if (candidate.svcbPayload.size() < 32 ||
          !readSvcbWord(candidate.svcbPayload, wordIndex, candidateWord)) {
        continue;
      }
      if (candidateWord == saveWord) {
        narrowed.push_back(candidate);
      }
    }

    return (narrowed.size() == 1) ? narrowed : candidates;
  };

  auto candidateNames = [](const QList<TsgMatchCandidate>& candidates) {
    QStringList names;
    for (const TsgMatchCandidate& candidate : candidates) {
      if (!candidate.baseName.isEmpty() && !names.contains(candidate.baseName)) {
        names.push_back(candidate.baseName);
      }
    }
    return names;
  };

  QList<TsgMatchCandidate> narrowedCandidates =
      !exactSvmdCandidates.isEmpty() ? exactSvmdCandidates : svmdCodeSetCandidates;

  m_SvmdCandidates = candidateNames(narrowedCandidates);

  narrowedCandidates = narrowCandidatesByPayload(
      narrowedCandidates, m_SaveSvcbPayload,
      [](const TsgMatchCandidate& candidate) { return candidate.svcbPayload; });
  m_SvcbCandidates = candidateNames(narrowedCandidates);
  narrowedCandidates = narrowCandidatesBySvcbWord(
      narrowedCandidates, m_SaveSvcbPayload, 5);
  m_SvcbWord5Candidates = candidateNames(narrowedCandidates);
  narrowedCandidates = narrowCandidatesBySvcbWord(
      narrowedCandidates, m_SaveSvcbPayload, 3);
  m_SvcbWord3Candidates = candidateNames(narrowedCandidates);
  narrowedCandidates = narrowCandidatesByPayload(
      narrowedCandidates, m_SaveSvrgPayload,
      [](const TsgMatchCandidate& candidate) { return candidate.svrgPayload; });
  m_SvrgCandidates = candidateNames(narrowedCandidates);

  for (const TsgMatchCandidate& candidate : narrowedCandidates) {
    if (!m_ExactSvmdTsgMatches.contains(candidate.baseName)) {
      m_ExactSvmdTsgMatches.push_back(candidate.baseName);
    }
  }

  std::sort(nearestMatches.begin(), nearestMatches.end(),
            [](const TsgNearestMatchDebug& lhs, const TsgNearestMatchDebug& rhs) {
              if (lhs.totalScore != rhs.totalScore) {
                return lhs.totalScore > rhs.totalScore;
              }
              if (lhs.svmdCodeOverlap != rhs.svmdCodeOverlap) {
                return lhs.svmdCodeOverlap > rhs.svmdCodeOverlap;
              }
              return lhs.baseName < rhs.baseName;
            });

  for (int index = 0; index < nearestMatches.size() && index < 3; ++index) {
    const TsgNearestMatchDebug& row = nearestMatches[index];
    m_NearestTsgCandidates.push_back(
        QStringLiteral("%1(score=%2,svmd=%3/%4,svc5=%5,svc3=%6,svrg=%7)")
            .arg(row.baseName)
            .arg(row.totalScore)
            .arg(row.svmdCodeOverlap)
            .arg(row.svmdCodeUnion)
            .arg(row.svcbWord5Match ? 1 : 0)
            .arg(row.svcbWord3Match ? 1 : 0)
            .arg(row.svrgPrefixBytes));
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

  // SVMD contains location/state records. We now read these directly by their
  // fixed retail layout instead of regex-scanning the whole payload for tokens.
  qsizetype svmdOffset = -1;
  quint32 svmdLen = 0;
  m_SaveSvmdPayload.clear();
  m_SaveSvcbPayload = extractLastChunkPayloadCopy(bytes, "SVCB", kTailScanOffset);
  m_SaveSvrgPayload = extractLastChunkPayloadCopy(bytes, "SVRG", kTailScanOffset);
  if (!findLastChunkPayload(bytes, "SVMD", kTailScanOffset, &svmdOffset, &svmdLen) ||
      svmdLen < 10) {
    return;
  }

  const QByteArray payload = bytes.mid(svmdOffset, svmdLen);
  m_SaveSvmdPayload = payload;

  m_LocationCodes.clear();
  m_SvmdRecordDebug.clear();
  m_SvmdHeaderDebug.clear();
  m_SvmdTrailerDebug.clear();
  QSet<QString> seenCodes;
  m_LocationCode.clear();

  if (payload.size() < kSvmdHeaderSize + 4) {
    return;
  }

  m_SvmdHeaderDebug = formatHexBytes(payload.left(kSvmdHeaderSize));

  qsizetype consumedBytes = kSvmdHeaderSize;
  int recordIndex = 0;

  for (qsizetype recordOffset = kSvmdHeaderSize;
       recordOffset + kSvmdRecordCodeOffset + 4 <= payload.size();
       recordOffset += kSvmdRecordSize) {
    const QString code = readSvmdCode(payload, recordOffset);
    if (code.isEmpty()) {
      break;
    }

    if (!seenCodes.contains(code)) {
      seenCodes.insert(code);
      m_LocationCodes.push_back(code);
    }

    quint32 field08 = 0;
    quint32 field0C = 0;
    quint32 field10 = 0;
    quint32 field14 = 0;
    quint16 flag1C = 0;
    readLE32(payload, recordOffset + 0x08, field08);
    readLE32(payload, recordOffset + 0x0C, field0C);
    readLE32(payload, recordOffset + 0x10, field10);
    readLE32(payload, recordOffset + 0x14, field14);
    readLE16(payload, recordOffset + 0x1C, flag1C);
    const QByteArray prefix8 = payload.mid(recordOffset, 8);
    m_SvmdRecordDebug.push_back(
        QStringLiteral("[%1] code=%2 prefix=%3 08=%4 0C=%5 10=%6 14=%7 flag=%8")
            .arg(recordIndex)
            .arg(code)
            .arg(formatHexBytes(prefix8))
            .arg(field08)
            .arg(field0C)
            .arg(field10)
            .arg(field14)
            .arg(flag1C));
    consumedBytes = recordOffset + kSvmdRecordSize;
    ++recordIndex;
  }

  if (consumedBytes < payload.size()) {
    m_SvmdTrailerDebug = formatHexBytes(payload.mid(consumedBytes));
  } else {
    m_SvmdTrailerDebug = QStringLiteral("(none)");
  }
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
  m_ResolvedLocationSource.clear();
  m_ResolvedLocationBasename.clear();
  m_SvmdDisplayCandidates.clear();
  m_MapDisplayCandidates.clear();
  m_TransitionDisplayCandidates.clear();

  if (m_LocationCode.isEmpty() && m_LocationCodes.isEmpty() && m_AreaToken.isEmpty()) {
    return;
  }

  // SVMD record scanning is structurally understood, but the active/current record
  // selector is not yet proven. Do not promote arbitrary record codes into the
  // user-facing location field until that selector is solved.
  const QHash<QString, QSet<QString>> labelsByBasename = loadCachedMapLabels(m_Game);
  const QHash<QString, QString> subtitleByLabel = loadCachedConfiguredRtxSubtitles(m_Game);
  QSet<QString> resolvedSvmdDisplays;
  for (const QString& baseName : m_ExactSvmdTsgMatches) {
    const QSet<QString> mapLabels = labelsByBasename.value(baseName.trimmed().toUpper());
    if (mapLabels.isEmpty()) {
      continue;
    }
    for (const QString& code : m_LocationCodes) {
      const QString label = code.toLower();
      if (!mapLabels.contains(label)) {
        continue;
      }
      const QString subtitle = subtitleByLabel.value(label).trimmed();
      if (isLocationStyleSubtitle(subtitle)) {
        const QString normalized = normalizeResolvedLocationSubtitle(subtitle);
        resolvedSvmdDisplays.insert(normalized);
        const QString candidate = QStringLiteral("%1:%2=%3").arg(baseName, code, normalized);
        if (!m_SvmdDisplayCandidates.contains(candidate)) {
          m_SvmdDisplayCandidates.push_back(candidate);
        }
      }
    }
  }
  // Keep SVMD label intersection as developer evidence only. The common map-local
  // SVMD record tables still mix genuine place labels with dialogue/topic labels,
  // and until the active/current record selector is proven, promoting a unique
  // surviving subtitle here can still produce false positives like "BYE".

  const QHash<QString, QString> displayByBasename = loadCachedMapDisplayNames(m_Game);
  const QHash<QString, QString> transitionDisplayByBasename =
      loadCachedTransitionDerivedMapDisplayNames(m_Game);
  for (const QString& baseName : m_ExactSvmdTsgMatches) {
    const QString key = baseName.trimmed().toUpper();
    QString display = displayByBasename.value(key).trimmed();
    if (!display.isEmpty() && !m_MapDisplayCandidates.contains(QStringLiteral("%1=%2").arg(key, display))) {
      m_MapDisplayCandidates.push_back(QStringLiteral("%1=%2").arg(key, display));
    }
    if (display.isEmpty()) {
      display = transitionDisplayByBasename.value(key).trimmed();
      if (!display.isEmpty() &&
          !m_TransitionDisplayCandidates.contains(QStringLiteral("%1=%2").arg(key, display))) {
        m_TransitionDisplayCandidates.push_back(QStringLiteral("%1=%2").arg(key, display));
      }
    }
  }
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

  XnginePaletteFormat::Palette activePalette;
  bool hasActivePalette = false;
  if (m_Game != nullptr) {
    const QHash<QString, XnginePaletteFormat::Palette> palettesByBasename =
        loadCachedMapPalettes(m_Game);
    QSet<QString> paletteCandidates;
    for (const QString& baseName : m_ExactSvmdTsgMatches) {
      const QString key = baseName.trimmed().toUpper();
      if (palettesByBasename.contains(key)) {
        paletteCandidates.insert(key);
      }
    }
    if (paletteCandidates.isEmpty()) {
      const auto tsgFiles = QDir(m_SaveFolder).entryInfoList(QStringList{"*.TSG", "*.tsg"},
                                                             QDir::Files, QDir::Name);
      for (const QFileInfo& fi : tsgFiles) {
        const QString key = fi.completeBaseName().trimmed().toUpper();
        if (palettesByBasename.contains(key)) {
          paletteCandidates.insert(key);
        }
      }
    }
    if (paletteCandidates.size() == 1) {
      activePalette = palettesByBasename.value(*paletteCandidates.cbegin());
      hasActivePalette = true;
    }
  }

  const qsizetype sig = findThumbnailSignature(bytes);
  if (sig >= 0 && sig + 8 < bytes.size()) {
    const quint32 rawLenBE = qFromBigEndian<quint32>(
        reinterpret_cast<const uchar*>(bytes.constData() + sig + 4));
    const quint16 width = qFromLittleEndian<quint16>(
        reinterpret_cast<const uchar*>(bytes.constData() + sig + 4));
    const quint16 height = qFromLittleEndian<quint16>(
        reinterpret_cast<const uchar*>(bytes.constData() + sig + 6));
    const qsizetype dataOff = sig + 8;

    // Canonical Redguard thumbnail payload:
    // 0x18000 bytes = 256 * 192 * 2 (RGB565, little-endian pixels).
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

    // Software rendering thumbnail payload:
    // 0x0C000 bytes = 256 * 192 * 1 indexed pixels using the active map palette.
    if (rawLenBE == 0x0C000U && hasActivePalette &&
        dataOff + static_cast<qsizetype>(rawLenBE) <= bytes.size()) {
      const auto* src = reinterpret_cast<const uchar*>(bytes.constData() + dataOff);
      QImage image;
      if (decodeIndexedImage(src, 256, 192, activePalette, image)) {
        fields->Screenshot = image;
        return fields;
      }
    }

    // Legacy/fallback planar attempt retained for non-canonical saves.
    if (width > 0 && height > 0 && width <= 2048 && height <= 2048) {
      const qsizetype planeSize = static_cast<qsizetype>(width) * height;
      const qsizetype payloadSize = planeSize * 3;
      if (dataOff + payloadSize <= bytes.size()) {
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
    }
  }

  // Third decode path: SCR3 framebuffer chunk (RGB565 LE, 640x480) used by glide rendering.
  qsizetype scr3Offset = -1;
  quint32 scr3Length = 0;
  if (findLastChunkPayload(bytes, "SCR3", 0, &scr3Offset, &scr3Length) &&
      scr3Length == 614400U && scr3Offset >= 0 &&
      scr3Offset + static_cast<qsizetype>(scr3Length) <= bytes.size()) {
    const int scrW = 640;
    const int scrH = 480;
    QImage image(scrW, scrH, QImage::Format_RGB32);
    const auto* src = reinterpret_cast<const uchar*>(bytes.constData() + scr3Offset);
    for (int y = 0; y < scrH; ++y) {
      for (int x = 0; x < scrW; ++x) {
        const qsizetype i = static_cast<qsizetype>(y) * scrW * 2 + x * 2;
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

  // Fourth decode path: SCRN framebuffer chunk (indexed 640x480) used by software rendering.
  qsizetype scrnOffset = -1;
  quint32 scrnLength = 0;
  if (hasActivePalette && findLastChunkPayload(bytes, "SCRN", 0, &scrnOffset, &scrnLength) &&
      scrnLength == 307200U && scrnOffset >= 0 &&
      scrnOffset + static_cast<qsizetype>(scrnLength) <= bytes.size()) {
    const auto* src = reinterpret_cast<const uchar*>(bytes.constData() + scrnOffset);
    QImage image;
    if (decodeIndexedImage(src, 640, 480, activePalette, image)) {
      fields->Screenshot = image;
      return fields;
    }
  }

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
