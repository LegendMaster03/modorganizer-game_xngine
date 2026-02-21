#ifndef ARENA_SAVEGAME_H
#define ARENA_SAVEGAME_H

#include <xnginesavegame.h>

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include <array>

class GameArena;

class ArenaSaveGame : public XngineSaveGame
{
public:
  struct NibbleCodec
  {
    std::array<quint8, 16> left{};
    std::array<quint8, 16> right{};
  };

  ArenaSaveGame(const QString& saveFile, const GameArena* game);

  virtual QString getName() const override;
  virtual QString getGameDetails() const override;
  virtual QStringList allFiles() const override;

private:
  void resolveSavePath();
  bool parseSaveEngn();
  bool parseLog();
  bool parseSpells();
  bool parseNamesDat();
  bool parseCityData();

  bool readByte(qsizetype offset, quint8& out) const;
  bool readDword(qsizetype offset, quint32& out) const;
  bool readWord(qsizetype offset, quint16& out) const;

  static quint8 decodeNibble(quint8 scrambled, const std::array<quint8, 16>& map);
  static quint8 decodeByte(quint8 scrambled, const NibbleCodec& codec);

  static quint32 decodeDword(const QByteArray& data, qsizetype offset,
                             const NibbleCodec& b0, const NibbleCodec& b1,
                             const NibbleCodec& b2, const NibbleCodec& b3,
                             bool* ok = nullptr);
  static quint16 decodeWord(const QByteArray& data, qsizetype offset,
                            const NibbleCodec& b0, const NibbleCodec& b1,
                            bool* ok = nullptr);
  static QString extractLikelyPersonName(const QByteArray& data);
  static QString extractLikelyLocation(const QByteArray& data);

  QString companionPath(const QString& stem) const;
  QString slotSuffix() const;
  void detectSlotFromFilename();
  void parseQuestFlags();

private:
  QString m_SaveFile;
  QString m_SaveDir;
  QString m_SlotSuffix;
  const GameArena* m_Game;
  QByteArray m_SaveEngnData;

  int m_Slot = -1;

  quint16 m_Level = 0;
  quint32 m_Experience = 0;
  quint32 m_Gold = 0;
  quint16 m_BlessingRaw = 0;
  double m_BlessingScaled = 0.0;

  int m_StaffPieces = -1;
  bool m_RiaVisionEnabled = false;
  bool m_HasMainQuestItem = false;

  quint32 m_DetailRaw = 0;
  int m_LogEntryCount = 0;
  QString m_LastQuestTitle;
  int m_SpellRecordCount = 0;
  int m_SpellActiveCount = 0;
  QStringList m_SpellPreview;
  bool m_HasSlotDisplayName = false;
  bool m_IsEmptySlot = false;
};

#endif  // ARENA_SAVEGAME_H
