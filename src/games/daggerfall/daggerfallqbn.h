#ifndef DAGGERFALL_QBN_H
#define DAGGERFALL_QBN_H

#include "daggerfallqbnpseudo.h"

#include <QByteArray>
#include <QString>
#include <QVector>
#include <QtGlobal>

class DaggerfallQbn
{
public:
  struct Header
  {
    quint16 questId = 0;
    quint16 factionId = 0;
    quint16 resourceId = 0;
    QByteArray resourceFilename;  // 9 bytes
    quint8 hasDebugInfo = 0;
    quint16 sectionRecordCount[10] = {0};
    quint16 sectionOffset[11] = {0};
    quint16 null2 = 0;
  };

  struct ItemRecord
  {
    qint16 itemIndex = 0;
    quint8 reward = 0;
    quint16 itemCategory = 0;
    quint16 itemCategoryIndex = 0;
    quint32 textVariableHash = 0;
    quint32 nullValue = 0;
    quint16 textRecordId1 = 0;
    quint16 textRecordId2 = 0;
  };

  struct NpcRecord
  {
    qint16 npcIndex = 0;
    quint8 gender = 0;
    quint8 facePictureIndex = 0;
    quint16 unknown1 = 0;
    quint16 factionIndex = 0;
    quint32 textVariableHash = 0;
    quint32 nullValue = 0;
    quint16 textRecordId1 = 0;
    quint16 textRecordId2 = 0;
  };

  struct LocationRecord
  {
    quint16 locationIndex = 0;
    quint8 flags = 0;
    quint8 generalLocation = 0;
    quint16 fineLocation = 0;
    qint16 locationType = 0;
    qint16 doorSelector = 0;
    quint16 unknown2 = 0;
    quint32 textVariableHash = 0;
    quint32 objPtr = 0;
    quint16 textRecordId1 = 0;
    quint16 textRecordId2 = 0;
  };

  struct TimerRecord
  {
    qint16 timerIndex = 0;
    quint16 flags = 0;
    quint8 type = 0;
    qint32 minimum = 0;
    qint32 maximum = 0;
    quint32 started = 0;
    quint32 duration = 0;
    qint32 link1 = 0;
    qint32 link2 = 0;
    quint32 textVariableHash = 0;
  };

  struct MobRecord
  {
    quint8 mobIndex = 0;
    quint16 null1 = 0;
    quint8 mobType = 0;
    quint16 mobCount = 0;
    quint32 textVariableHash = 0;
    quint32 null2 = 0;
  };

  struct StateRecord
  {
    qint16 flagIndex = 0;
    quint8 isGlobal = 0;
    quint8 globalIndex = 0;
    quint32 textVariableHash = 0;
  };

  struct TextVariableRecord
  {
    QString textVariable;
    quint8 sectionId = 0;
    quint16 recordId = 0;
    quint32 recordPtr = 0;
  };

  struct SectionRange
  {
    int index = -1;
    qsizetype offset = 0;
    qsizetype size = 0;
    bool isValid() const { return index >= 0 && size >= 0; }
  };

  struct File
  {
    Header header;
    QByteArray raw;
    QVector<SectionRange> sectionRanges;

    QVector<ItemRecord> items;
    QVector<NpcRecord> npcs;
    QVector<LocationRecord> locations;
    QVector<TimerRecord> timers;
    QVector<MobRecord> mobs;
    DaggerfallQbnPseudo::Section opCodes;
    QVector<StateRecord> states;
    QVector<TextVariableRecord> textVariables;

    QByteArray section1Raw;
    QByteArray section2Raw;
    QByteArray section5Raw;

    QString warning;
  };

  static bool loadFile(const QString& qbnPath, File& outFile,
                       QString* errorMessage = nullptr);
  static bool parseBytes(const QByteArray& bytes, File& outFile,
                         QString* errorMessage = nullptr);
};

#endif  // DAGGERFALL_QBN_H
