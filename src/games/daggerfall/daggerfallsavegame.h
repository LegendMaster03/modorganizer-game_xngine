#ifndef DAGGERFALLS_SAVEGAME_H
#define DAGGERFALLS_SAVEGAME_H

#include <xnginesavegame.h>

#include <QColor>
#include <QString>
#include <QByteArray>
#include <QtGlobal>
#include <memory>
#include <vector>

class GameDaggerfall;

/**
 * Daggerfall-specific save game handler.
 * Daggerfall saves are numbered SAVE0 through SAVE5 (6 save slots)
 */
class DaggerfallsSaveGame : public XngineSaveGame
{
public:
  DaggerfallsSaveGame(const QString& saveFolder, const GameDaggerfall* game);
  virtual QString getName() const override;
  virtual QString getPCLocation() const override;
  virtual QString getGameDetails() const override;

protected:
  virtual std::unique_ptr<DataFields> fetchDataFields() const override;

private:
  struct ParsedRecord
  {
    quint8 type = 0;
    qsizetype payloadOffset = 0;
    qsizetype payloadLength = 0;
  };

  bool parseSaveTree();
  bool parseSaveName();
  bool parseSaveVars();
  static bool readLE32(const QByteArray& data, qsizetype offset, qint32& value);
  static bool readLE32U(const QByteArray& data, qsizetype offset, quint32& value);
  static bool readLE16U(const QByteArray& data, qsizetype offset, quint16& value);
  static bool readU8(const QByteArray& data, qsizetype offset, quint8& value);
  static std::vector<ParsedRecord> parseRecordStream(const QByteArray& data,
                                                     qsizetype startOffset,
                                                     qsizetype* endOffset);
  static std::vector<ParsedRecord> findBestRecordStream(const QByteArray& data,
                                                        qsizetype* startOffset,
                                                        qsizetype* endOffset);
  static QString readFixedString(const QByteArray& data, qsizetype offset, qsizetype size);
  static QString formatPositionText(qint32 x, quint16 yOffset, quint16 yBase, qint32 z);
  QString formatHeaderLocationText(quint16 locationCode, quint8 zoneType, qint32 x, qint32 y,
                                   qint32 z);
  static QString formatDaggerfallDate(quint32 minutes);
  static QString raceName(quint8 race);
  static bool isLikelyClassName(const QString& value);
  static QString reflexName(quint8 reflex);
  QString saveFilePath(const QString& fileName) const;

private:
  QString m_SaveFolder;
  const GameDaggerfall* m_Game;
  quint16 m_HP = 0;
  quint16 m_HPMax = 0;
  quint16 m_Mana = 0;
  quint16 m_ManaMax = 0;
  quint32 m_Gold = 0;
  quint8 m_Race = 0xFF;
  QString m_ClassName;
  quint8 m_Reflex = 0xFF;
  QString m_InGameDate;
  QString m_LocationNameDetail;
  QString m_LocationTypeNameDetail;
  QString m_LocationRegionNameDetail;
  int m_LocationTypeIndexDetail = -1;
  QColor m_LocationTypeColorDetail;
};

#endif  // DAGGERFALLS_SAVEGAME_H
