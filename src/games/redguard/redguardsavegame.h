#ifndef REDGUARDS_SAVEGAME_H
#define REDGUARDS_SAVEGAME_H

#include <xnginesavegame.h>

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QSet>
#include <QVector>

class GameRedguard;

/**
 * Redguard-specific save game handler.
 * Redguard saves are folders named SAVEGAME.XXX containing SAVEGAME.SAV
 */
class RedguardsSaveGame : public XngineSaveGame
{
public:
  RedguardsSaveGame(const QString& saveFolder, const GameRedguard* game);
  virtual QString getName() const override;
  virtual QString getPCLevelText() const override;
  virtual QString getGameDetails() const override;
  virtual QStringList allFiles() const override;
  QString chunkTableDebugText() const;

protected:
  virtual std::unique_ptr<DataFields> fetchDataFields() const override;

private:
  void resolveSavePath();
  void detectSlotFromFolder();
  void scanAuxiliaryFiles();
  void parseAuxiliaryMetadata();
  bool parseSaveHeader();
  void parseStructuredMetadata(const QByteArray& bytes);
  void resolveLocationFromCode();
  static QString readFixedCString(const QByteArray& data, qsizetype offset, qsizetype maxLen);
  static quint32 readSvitCurrentCount(const uchar* svit, quint32 svitLen, int itemId);

private:
  QString m_SaveFolder;
  QString m_SaveFile;
  const GameRedguard* m_Game;
  bool m_ValidSignature = false;
  QString m_FormatVersion;
  QString m_SaveTitle;
  quint64 m_FileSize = 0;
  bool m_HasThumbnail = false;
  quint32 m_Gold = 0;
  quint32 m_IronSkinPotions = 0;
  quint32 m_HealthPotions = 0;
  quint32 m_StrengthPotions = 0;
  QVector<quint32> m_SvitCurrentCounts;
  QByteArray m_SaveSvmdPayload;
  QByteArray m_SaveSvcbPayload;
  QByteArray m_SaveSvrgPayload;
  QString m_LocationCode;
  QStringList m_LocationCodes;
  QStringList m_SvmdRecordDebug;
  QString m_SvmdHeaderDebug;
  QString m_SvmdTrailerDebug;
  QStringList m_SvmdCandidates;
  QStringList m_SvcbCandidates;
  QStringList m_SvcbWord5Candidates;
  QStringList m_SvcbWord3Candidates;
  QStringList m_SvrgCandidates;
  QStringList m_NearestTsgCandidates;
  QStringList m_ExactSvmdTsgMatches;
  QString m_AreaToken;
  QString m_ResolvedLocationSource;
  QString m_ResolvedLocationBasename;
  QStringList m_SvmdDisplayCandidates;
  QStringList m_MapDisplayCandidates;
  QStringList m_TransitionDisplayCandidates;
  QStringList m_AuxiliaryFiles;
  mutable QString m_ChunkTableDebugText;
};

#endif  // REDGUARDS_SAVEGAME_H
