#ifndef DAGGERFALL_QBNPSEUDO_H
#define DAGGERFALL_QBNPSEUDO_H

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QVector>
#include <QtGlobal>

class DaggerfallQbnPseudo
{
public:
  static constexpr qsizetype RecordSize = 87;
  static constexpr int MaxSubrecords = 5;

  struct Subrecord
  {
    quint8 notFlag = 0;
    quint32 localPtr = 0;
    quint16 sectionId = 0;
    quint32 value = 0;
    quint32 objectPtr = 0;

    quint8 localPtrRecordIndex() const
    {
      return static_cast<quint8>(localPtr & 0xFF);
    }

    quint8 localPtrSectionIndex() const
    {
      return static_cast<quint8>((localPtr >> 8) & 0xFF);
    }
  };

  struct Record
  {
    quint16 opCode = 0;
    quint16 flags = 0;
    quint16 argumentCount = 0;
    Subrecord subrecords[MaxSubrecords];
    quint16 messageId = 0xFFFF;
    quint32 lastUpdate = 0;
    QString warning;

    bool isCompleted() const { return (flags & 0x0100u) != 0; }
  };

  struct OpcodeSpec
  {
    quint16 opCode = 0;
    QString name;
    int minArgs = 0;
    int maxArgs = 0;
  };

  struct Section
  {
    QVector<Record> records;
    QString warning;
  };

  static bool parseSectionBytes(const QByteArray& bytes, Section& outSection,
                                QString* errorMessage = nullptr);

  static bool parseRecordBytes(const QByteArray& bytes, Record& outRecord,
                               QString* errorMessage = nullptr);

  static QHash<quint16, OpcodeSpec> opcodeSpecs();
  static QString opcodeName(quint16 opCode);
};

#endif  // DAGGERFALL_QBNPSEUDO_H
