#ifndef BATTLESPIRE_TEXTRSC_H
#define BATTLESPIRE_TEXTRSC_H

#include <QHash>
#include <QByteArray>
#include <QString>
#include <QVector>
#include <QtGlobal>

// Speculative Daggerfall-derived compatibility adapter. The confirmed Battlespire
// resource set uses TXT.BSA; no real Battlespire TEXT.RSC file or production caller
// is currently established. Keep this isolated for compatibility/research and do not
// treat it as an authoritative Battlespire text path without fixture evidence.
class BattlespireTextRsc
{
public:
  struct TextRecordHeader
  {
    quint16 textRecordId = 0;
    quint32 offset = 0;
  };

  struct TextSubrecord
  {
    QByteArray rawBytes;
    QString text;
  };

  struct TextRecord
  {
    int index = -1;
    quint16 textRecordId = 0;
    QByteArray rawBytes;
    QVector<TextSubrecord> subrecords;
    QString warning;
    bool isValid() const { return index >= 0; }
  };

  struct Database
  {
    quint16 textRecordHeaderLength = 0;
    int textRecordCount = 0;
    QVector<TextRecordHeader> headers;
    QHash<int, TextRecord> records;
    QHash<quint16, TextRecord> recordsById;
    QString warning;
  };

  static bool loadTextRsc(const QString& filePath, Database& outDb,
                          QString* errorMessage = nullptr);

  static bool parseTextRecordBytes(const QByteArray& data, int index, TextRecord& outRecord,
                                   QString* errorMessage = nullptr);
};

#endif  // BATTLESPIRE_TEXTRSC_H
