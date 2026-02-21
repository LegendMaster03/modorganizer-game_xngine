#ifndef DAGGERFALL_TEXTRECORD_H
#define DAGGERFALL_TEXTRECORD_H

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

class DaggerfallTextRecord
{
public:
  enum class TokenType
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

  struct Token
  {
    TokenType type = TokenType::RawControl;
    QByteArray bytes;
  };

  struct Subrecord
  {
    QByteArray raw;
    QString text;
    QVector<Token> tokens;
    QStringList variables;
  };

  struct Record
  {
    quint16 id = 0xFFFF;
    quint32 offset = 0;
    QByteArray raw;
    QVector<Subrecord> subrecords;
    QString warning;
    bool isValid() const { return id != 0xFFFF; }
  };

  struct Header
  {
    quint16 id = 0xFFFF;
    quint32 offset = 0;
  };

  struct Database
  {
    quint16 headerLength = 0;
    int recordCount = 0;
    QVector<Header> headers;
    QVector<Record> records;
    QString warning;
  };

  static bool parseRecord(const QByteArray& bytes, Record& outRecord,
                          QString* errorMessage = nullptr);
  static bool parseDatabase(const QByteArray& bytes, Database& outDb,
                            QString* errorMessage = nullptr);
  static QVector<Token> tokenizeSubrecord(const QByteArray& bytes);
  static QString decodeSubrecordText(const QByteArray& bytes);
};

#endif  // DAGGERFALL_TEXTRECORD_H
