#ifndef REDGUARDS_SAVEGAME_H
#define REDGUARDS_SAVEGAME_H

#include <xnginesavegame.h>

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QSet>

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
  virtual QString getGameDetails() const override;
  virtual QStringList allFiles() const override;

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
  static bool isWeakLocationToken(const QString& code, const QString& subtitle);

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
  quint32 m_HealthPotions = 0;
  QString m_LocationCode;
  QStringList m_LocationCodes;
  QString m_AreaToken;
  QStringList m_AuxiliaryFiles;
};

#endif  // REDGUARDS_SAVEGAME_H
