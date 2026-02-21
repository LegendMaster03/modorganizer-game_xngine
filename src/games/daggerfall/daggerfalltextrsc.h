#ifndef DAGGERFALL_TEXTRSC_H
#define DAGGERFALL_TEXTRSC_H

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

class DaggerfallTextRsc
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
    QString text;  // 0xFD/0x00 converted to '\n'
    QStringList variables;
  };

  struct TextRecord
  {
    int index = -1;
    quint16 textRecordId = 0;
    QString displayLabel;
    QString categoryLabel;
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
  static QString categoryLabelForRecordId(quint16 textRecordId);
  static QString formatRecordLabel(quint16 textRecordId, const QString& categoryLabel);
};

#endif  // DAGGERFALL_TEXTRSC_H
