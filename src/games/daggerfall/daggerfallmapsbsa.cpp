#include "daggerfallmapsbsa.h"
#include "daggerfallformatutils.h"

#include "daggerfallblocksbsa.h"
#include "daggerfallcommon.h"
#include "gamedaggerfall.h"
#include "xnginebsaformat.h"

#include <QDir>
#include <QHash>
#include <QtEndian>
#include <QStringList>
#include <QVector>

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <limits>

namespace {

using Daggerfall::FormatUtil::setError;

QString readFixedStringLocal(const QByteArray& data, qsizetype offset, qsizetype size)
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

bool parseRegionIndex(const QString& entryName, int* regionIndex)
{
  const int dot = entryName.lastIndexOf('.');
  if (dot < 0 || dot + 4 != entryName.size()) {
    return false;
  }
  bool ok = false;
  const int region = entryName.mid(dot + 1, 3).toInt(&ok);
  if (!ok || region < 0 || region > 999) {
    return false;
  }
  *regionIndex = region;
  return true;
}

QHash<int, QStringList> readMapNamesByRegion(const XngineBSAFormat::Archive& archive)
{
  QHash<int, QStringList> namesByRegion;
  for (const auto& entry : archive.entries) {
    if (!entry.name.startsWith("MAPNAMES.", Qt::CaseInsensitive)) {
      continue;
    }
    int region = -1;
    if (!parseRegionIndex(entry.name, &region) || entry.data.size() < 4) {
      continue;
    }

    quint32 count = 0;
    std::memcpy(&count, entry.data.constData(), sizeof(count));
    count = qFromLittleEndian(count);
    if (count == 0) {
      continue;
    }

    const qsizetype needed = 4 + static_cast<qsizetype>(count) * 32;
    if (entry.data.size() < needed) {
      continue;
    }

    QStringList names;
    names.reserve(static_cast<int>(count));
    for (quint32 i = 0; i < count; ++i) {
      const qsizetype off = 4 + static_cast<qsizetype>(i) * 32;
      names.push_back(readFixedStringLocal(entry.data, off, 32));
    }
    namesByRegion.insert(region, names);
  }
  return namesByRegion;
}

struct MapTableElement
{
  quint32 mapId = 0;
  quint32 latitudeType = 0;
  quint32 longitudeType = 0;
  quint8 flavor = 0;
  quint32 services = 0;
};

struct CoordinateEntry
{
  qint32 latitude = 0;
  qint32 longitude = 0;
  DaggerfallMapsBsa::LocationInfo info;
};

QHash<int, QVector<MapTableElement>> readMapTableByRegion(
    const XngineBSAFormat::Archive& archive, const QHash<int, QStringList>& namesByRegion)
{
  QHash<int, QVector<MapTableElement>> tablesByRegion;

  for (const auto& entry : archive.entries) {
    if (!entry.name.startsWith("MAPTABLE.", Qt::CaseInsensitive)) {
      continue;
    }
    int region = -1;
    if (!parseRegionIndex(entry.name, &region)) {
      continue;
    }

    const int expectedCount = namesByRegion.value(region).size();
    if (expectedCount <= 0) {
      continue;
    }
    const qsizetype required = static_cast<qsizetype>(expectedCount) * 17;
    if (entry.data.size() < required) {
      continue;
    }

    QVector<MapTableElement> list;
    list.reserve(expectedCount);
    for (int i = 0; i < expectedCount; ++i) {
      const qsizetype off = static_cast<qsizetype>(i) * 17;
      MapTableElement e{};
      std::memcpy(&e.mapId, entry.data.constData() + off + 0, sizeof(e.mapId));
      std::memcpy(&e.latitudeType, entry.data.constData() + off + 4, sizeof(e.latitudeType));
      std::memcpy(&e.longitudeType, entry.data.constData() + off + 8, sizeof(e.longitudeType));
      e.flavor = static_cast<quint8>(entry.data.at(off + 12));
      std::memcpy(&e.services, entry.data.constData() + off + 13, sizeof(e.services));

      e.mapId = qFromLittleEndian(e.mapId);
      e.latitudeType = qFromLittleEndian(e.latitudeType);
      e.longitudeType = qFromLittleEndian(e.longitudeType);
      e.services = qFromLittleEndian(e.services);
      list.push_back(e);
    }
    tablesByRegion.insert(region, list);
  }

  return tablesByRegion;
}

void populateLocationNumIndex(const XngineBSAFormat::Archive& archive,
                              const QHash<int, QStringList>& namesByRegion,
                              const QHash<int, QVector<MapTableElement>>& tablesByRegion,
                              QHash<quint16, DaggerfallMapsBsa::LocationInfo>& outIndex)
{
  for (const auto& entry : archive.entries) {
    if (!entry.name.startsWith("MAPPITEM.", Qt::CaseInsensitive)) {
      continue;
    }

    int region = -1;
    if (!parseRegionIndex(entry.name, &region)) {
      continue;
    }
    const auto names = namesByRegion.value(region);
    if (names.isEmpty()) {
      continue;
    }
    const auto table = tablesByRegion.value(region);

    const QByteArray& data = entry.data;
    const quint32 locationCount = static_cast<quint32>(names.size());
    const qsizetype tableSize = static_cast<qsizetype>(locationCount) * 4;
    if (data.size() < tableSize) {
      continue;
    }

    for (quint32 i = 0; i < locationCount; ++i) {
      quint32 relOffset = 0;
      std::memcpy(&relOffset, data.constData() + static_cast<qsizetype>(i) * 4,
                  sizeof(relOffset));
      relOffset = qFromLittleEndian(relOffset);

      const qsizetype elemOffset = tableSize + static_cast<qsizetype>(relOffset);
      if (elemOffset + 4 > data.size()) {
        continue;
      }

      quint32 doorCount = 0;
      std::memcpy(&doorCount, data.constData() + elemOffset, sizeof(doorCount));
      doorCount = qFromLittleEndian(doorCount);
      const qsizetype objectHeaderOffset =
          elemOffset + 4 + static_cast<qsizetype>(doorCount) * 6;
      const qsizetype locationHeaderOffset = objectHeaderOffset + 71;
      if (locationHeaderOffset + 48 > data.size()) {
        continue;
      }

      quint16 locationNum = 0;
      std::memcpy(&locationNum, data.constData() + objectHeaderOffset + 33,
                  sizeof(locationNum));
      locationNum = qFromLittleEndian(locationNum);

      const QString headerName = readFixedStringLocal(data, locationHeaderOffset, 32);
      const QString fallbackName =
          (static_cast<int>(i) < names.size()) ? names.at(static_cast<int>(i)) : QString();
      const QString name = headerName.isEmpty() ? fallbackName : headerName;
      if (locationNum != 0 && !name.isEmpty() && !outIndex.contains(locationNum)) {
        DaggerfallMapsBsa::LocationInfo info;
        info.name = name;
        info.regionIndex = region;
        if (i < static_cast<quint32>(table.size())) {
          const auto& mt = table.at(static_cast<int>(i));
          info.mapId = mt.mapId;
          info.locationType = static_cast<int>((mt.latitudeType >> 25) & 0x1f);
          info.discovered = (mt.latitudeType & 0x40000000u) != 0;
          info.hidden = (mt.latitudeType & 0x80000000u) != 0;
        }
        outIndex.insert(locationNum, info);
      }

      // Save header location code is UInt16 and appears to map to MAPTABLE map IDs.
      // Keep a secondary key using low 16 bits so callers can resolve header codes.
      if (!name.isEmpty() && i < static_cast<quint32>(table.size())) {
        const auto& mt = table.at(static_cast<int>(i));
        const quint16 headerCode = static_cast<quint16>(mt.mapId & 0xffffu);
        if (!outIndex.contains(headerCode)) {
          DaggerfallMapsBsa::LocationInfo info;
          info.name = name;
          info.regionIndex = region;
          info.mapId = mt.mapId;
          info.locationType = static_cast<int>((mt.latitudeType >> 25) & 0x1f);
          info.discovered = (mt.latitudeType & 0x40000000u) != 0;
          info.hidden = (mt.latitudeType & 0x80000000u) != 0;
          outIndex.insert(headerCode, info);
        }
      }
    }
  }
}

}  // namespace

bool DaggerfallMapsBsa::loadLocationNameIndex(const QString& mapsBsaPath,
                                              QHash<quint16, QString>& outIndex,
                                              QString* errorMessage)
{
  QHash<quint16, LocationInfo> infoIndex;
  if (!loadLocationInfoIndex(mapsBsaPath, infoIndex, errorMessage)) {
    outIndex.clear();
    return false;
  }

  outIndex.clear();
  for (auto it = infoIndex.constBegin(); it != infoIndex.constEnd(); ++it) {
    outIndex.insert(it.key(), it.value().name);
  }
  return !outIndex.isEmpty();
}

bool DaggerfallMapsBsa::loadLocationInfoIndex(const QString& mapsBsaPath,
                                              QHash<quint16, LocationInfo>& outIndex,
                                              QString* errorMessage)
{
  outIndex.clear();

  XngineBSAFormat::Archive archive;
  if (!XngineBSAFormat::readArchive(mapsBsaPath, archive, errorMessage)) {
    return false;
  }

  const auto namesByRegion = readMapNamesByRegion(archive);
  if (namesByRegion.isEmpty()) {
    return setError(errorMessage,
                    QString("No MAPNAMES records found in %1").arg(mapsBsaPath));
  }

  const auto tablesByRegion = readMapTableByRegion(archive, namesByRegion);
  populateLocationNumIndex(archive, namesByRegion, tablesByRegion, outIndex);
  return !outIndex.isEmpty() ||
         setError(errorMessage,
                  QString("No location mapping records decoded from %1").arg(mapsBsaPath));
}

QString DaggerfallMapsBsa::resolveLocationName(const GameDaggerfall* game,
                                               quint16 locationCode)
{
  return resolveLocationInfo(game, locationCode).name;
}

DaggerfallMapsBsa::LocationInfo DaggerfallMapsBsa::resolveLocationInfo(
    const GameDaggerfall* game, quint16 locationCode)
{
  if (game == nullptr) {
    return {};
  }

  static QHash<QString, QHash<quint16, LocationInfo>> cacheByPath;

  const QString mapsPath =
      QDir::fromNativeSeparators(game->gameDirectory().filePath("arena2/MAPS.BSA"));
  if (!cacheByPath.contains(mapsPath)) {
    QHash<quint16, LocationInfo> loaded;
    QString error;
    if (!loadLocationInfoIndex(mapsPath, loaded, &error)) {
      cacheByPath.insert(mapsPath, {});
    } else {
      cacheByPath.insert(mapsPath, loaded);
    }
  }

  return cacheByPath.value(mapsPath).value(locationCode);
}

QString DaggerfallMapsBsa::regionName(int regionIndex)
{
  return Daggerfall::Data::regionName(regionIndex);
}

QString DaggerfallMapsBsa::locationTypeName(int locationType)
{
  return Daggerfall::Data::locationTypeName(locationType);
}

DaggerfallMapsBsa::LocationInfo DaggerfallMapsBsa::resolveNearestLocation(
    const GameDaggerfall* game, qint32 x, qint32 z)
{
  if (game == nullptr) {
    return {};
  }

  static QHash<QString, QVector<CoordinateEntry>> coordCacheByPath;
  const QString mapsPath =
      QDir::fromNativeSeparators(game->gameDirectory().filePath("arena2/MAPS.BSA"));

  if (!coordCacheByPath.contains(mapsPath)) {
    QVector<CoordinateEntry> entries;

    XngineBSAFormat::Archive archive;
    QString errorMessage;
    if (XngineBSAFormat::readArchive(mapsPath, archive, &errorMessage)) {
      const auto namesByRegion = readMapNamesByRegion(archive);
      const auto tablesByRegion = readMapTableByRegion(archive, namesByRegion);
      for (auto it = tablesByRegion.constBegin(); it != tablesByRegion.constEnd(); ++it) {
        const int region = it.key();
        const auto names = namesByRegion.value(region);
        const auto& table = it.value();
        for (int i = 0; i < table.size() && i < names.size(); ++i) {
          const auto& mt = table.at(i);
          const QString name = names.at(i);
          if (name.isEmpty()) {
            continue;
          }

          CoordinateEntry ce;
          ce.latitude = static_cast<qint32>(mt.latitudeType & 0x1ffffffu);
          ce.longitude = static_cast<qint32>(mt.longitudeType & 0xffffffu);
          ce.info.name = name;
          ce.info.regionIndex = region;
          ce.info.locationType = static_cast<int>((mt.latitudeType >> 25) & 0x1f);
          ce.info.discovered = (mt.latitudeType & 0x40000000u) != 0;
          ce.info.hidden = (mt.latitudeType & 0x80000000u) != 0;
          ce.info.mapId = mt.mapId;
          entries.push_back(ce);
        }
      }
    }

    coordCacheByPath.insert(mapsPath, entries);
  }

  const auto entries = coordCacheByPath.value(mapsPath);
  if (entries.isEmpty()) {
    return {};
  }

  qint64 bestDistance = (std::numeric_limits<qint64>::max)();
  const CoordinateEntry* best = nullptr;
  for (const auto& e : entries) {
    // Some DF coordinate contexts are swapped, so test both orientations.
    const qint64 d1 = std::llabs(static_cast<qint64>(e.longitude) - x) +
                      std::llabs(static_cast<qint64>(e.latitude) - z);
    const qint64 d2 = std::llabs(static_cast<qint64>(e.latitude) - x) +
                      std::llabs(static_cast<qint64>(e.longitude) - z);
    const qint64 d = (std::min)(d1, d2);
    if (d < bestDistance) {
      bestDistance = d;
      best = &e;
    }
  }

  // 32k map-cell granularity means nearest should usually be very close.
  constexpr qint64 kMaxAcceptableDistance = 300000;
  if (best == nullptr || bestDistance > kMaxAcceptableDistance) {
    return {};
  }

  return best->info;
}

QString DaggerfallMapsBsa::resolveExteriorBlockRecordName(const GameDaggerfall* game,
                                                          int regionIndex,
                                                          quint8 blockIndex,
                                                          quint8 blockNumber,
                                                          quint8 blockCharacter)
{
  const QString preferred =
      DaggerfallBlocksBsa::buildRmbName(regionIndex, blockIndex, blockNumber,
                                        blockCharacter);
  if (game == nullptr || preferred.isEmpty()) {
    return preferred;
  }

  const QString blocksPath =
      QDir::fromNativeSeparators(game->gameDirectory().filePath("arena2/BLOCKS.BSA"));
  QString error;
  const QString resolved =
      DaggerfallBlocksBsa::resolveExistingRmbName(blocksPath, preferred, &error);
  if (!resolved.isEmpty()) {
    return resolved;
  }
  return preferred;
}

QString DaggerfallMapsBsa::resolveDungeonBlockRecordName(quint8 blockIndex,
                                                         quint16 blockNumber)
{
  return DaggerfallBlocksBsa::buildRdbName(blockIndex, blockNumber);
}
