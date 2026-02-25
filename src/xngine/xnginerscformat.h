#ifndef XNGINERSCFORMAT_H
#define XNGINERSCFORMAT_H

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

class XngineRscFormat
{
public:
  enum class Variant
  {
    Auto,
    TextRecordDatabase
  };

  struct Traits
  {
    Variant variant = Variant::Auto;
    bool strictValidation = true;
  };

  enum class TextTokenType
  {
    Printable,
    FontCode,      // F9 xx
    FontColor,     // FA xx
    PositionCode,  // [00|01|02..FF] FB xx xx
    BookImage,     // F7 <name>\0
    EndOfPage,     // F6
    EndOfLine,     // FC 00 or FD 00
    RawControl
  };

  struct TextToken
  {
    TextTokenType type = TextTokenType::RawControl;
    QByteArray bytes;
  };

  struct TextSubrecord
  {
    QByteArray raw;
    QString text;
    QVector<TextToken> tokens;
  };

  struct TextRecord
  {
    quint16 id = 0xFFFF;
    quint32 offset = 0;
    QByteArray raw;
    QVector<TextSubrecord> subrecords;
    QString warning;
    bool isValid() const { return id != 0xFFFF; }
  };

  struct TextRecordHeader
  {
    quint16 id = 0xFFFF;
    quint32 offset = 0;
  };

  struct TextRecordDatabase
  {
    quint16 headerLength = 0;   // bytes after first u16 until payload start
    int recordCount = 0;        // excludes sentinel entry
    QVector<TextRecordHeader> headers;   // record headers only
    TextRecordHeader sentinelHeader;     // expected id=0xFFFF, offset=end
    QVector<TextRecord> records;         // parsed records for valid headers
    QString warning;
  };

  struct Document
  {
    Variant variant = Variant::Auto;
    TextRecordDatabase textDb;
    QString warning;
  };

public:
  static bool readFile(const QString& filePath, Document& outDocument,
                       QString* errorMessage = nullptr,
                       const Traits& traits = Traits{});
  static bool writeFile(const QString& filePath, const Document& document,
                        QString* errorMessage = nullptr,
                        const Traits& traits = Traits{});

  static bool parseTextRecordDatabase(const QByteArray& bytes, TextRecordDatabase& outDb,
                                      QString* errorMessage = nullptr,
                                      bool strictValidation = true);
  static bool writeTextRecordDatabase(const TextRecordDatabase& db, QByteArray& outBytes,
                                      QString* errorMessage = nullptr);
  static bool parseTextRecord(const QByteArray& bytes, TextRecord& outRecord,
                              QString* errorMessage = nullptr);

  static QVector<TextToken> tokenizeTextSubrecord(const QByteArray& bytes);
  static QString decodeTextSubrecordText(const QByteArray& bytes);
};

#endif  // XNGINERSCFORMAT_H
