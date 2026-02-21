#ifndef DAGGERFALL_MAPSBSA_H
#define DAGGERFALL_MAPSBSA_H

#include <QHash>
#include <QString>
#include <QtGlobal>

class GameDaggerfall;

class DaggerfallMapsBsa
{
public:
  struct LocationInfo
  {
    QString name;
    int regionIndex = -1;
    int locationType = -1;
    quint32 mapId = 0;
    bool discovered = false;
    bool hidden = false;

    bool isValid() const { return !name.isEmpty(); }
  };

  static bool loadLocationNameIndex(const QString& mapsBsaPath,
                                    QHash<quint16, QString>& outIndex,
                                    QString* errorMessage = nullptr);

  static QString resolveLocationName(const GameDaggerfall* game, quint16 locationCode);
  static bool loadLocationInfoIndex(const QString& mapsBsaPath,
                                    QHash<quint16, LocationInfo>& outIndex,
                                    QString* errorMessage = nullptr);
  static LocationInfo resolveLocationInfo(const GameDaggerfall* game, quint16 locationCode);
  static LocationInfo resolveNearestLocation(const GameDaggerfall* game, qint32 x, qint32 z);
  static QString resolveExteriorBlockRecordName(const GameDaggerfall* game, int regionIndex,
                                                quint8 blockIndex, quint8 blockNumber,
                                                quint8 blockCharacter);
  static QString resolveDungeonBlockRecordName(quint8 blockIndex, quint16 blockNumber);

  static QString regionName(int regionIndex);
  static QString locationTypeName(int locationType);
};

#endif  // DAGGERFALL_MAPSBSA_H
