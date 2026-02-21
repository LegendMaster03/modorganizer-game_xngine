#ifndef DAGGERFALL_BLOCKSBSA_H
#define DAGGERFALL_BLOCKSBSA_H

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

class DaggerfallBlocksBsa
{
public:
  enum class RecordType
  {
    FOO,
    RMB,
    RDB,
    RDI,
    Unknown
  };

  struct Point
  {
    qint32 x = 0;
    qint32 y = 0;
    qint32 z = 0;
  };

  struct TextureRef
  {
    int fileIndex = 0;
    int imageIndex = 0;
    quint16 raw = 0;
  };

  struct RmbModel
  {
    quint16 modelId1 = 0;
    quint8 modelId2 = 0;
    quint8 unknown1 = 0;
    quint32 unknown2 = 0;
    quint32 unknown3 = 0;
    quint32 unknown4 = 0;
    quint64 nullValue1 = 0;
    Point point1;
    Point point2;
    quint32 nullValue2 = 0;
    qint16 rotationAngle = 0;
    quint16 unknown5 = 0;
    quint32 unknown6 = 0;
    quint32 unknown8 = 0;
    quint16 nullValue4 = 0;

    quint32 arch3dRecordId() const
    {
      return static_cast<quint32>(modelId1) * 100u + static_cast<quint32>(modelId2);
    }
  };

  struct RmbFlat
  {
    Point position;
    TextureRef texture;
    quint16 unknown1 = 0;
    quint8 flag = 0;
  };

  struct RmbPerson
  {
    Point position;
    TextureRef texture;
    quint16 factionId = 0;
    quint8 flag = 0;
  };

  struct RmbDoor
  {
    Point position;
    quint16 unknown1 = 0;
    qint16 rotationAngle = 0;
    quint16 unknown2 = 0;
    quint8 nullValue = 0;
  };

  struct RmbSection3
  {
    Point position;
    quint16 unknown1 = 0;
    quint16 unknown2 = 0;
  };

  struct RmbBlockDataHeader
  {
    quint8 modelCount = 0;
    quint8 flatCount = 0;
    quint8 section3Count = 0;
    quint8 personCount = 0;
    quint8 doorCount = 0;
  };

  struct RmbBlockData
  {
    RmbBlockDataHeader header;
    QVector<RmbModel> models;
    QVector<RmbFlat> flats;
    QVector<RmbSection3> section3;
    QVector<RmbPerson> persons;
    QVector<RmbDoor> doors;
  };

  struct RmbBlock
  {
    RmbBlockData exterior;
    RmbBlockData interior;
    QByteArray trailingBytes;
  };

  struct RmbBlockPosition
  {
    quint32 unknown1 = 0;
    quint32 unknown2 = 0;
    qint32 x = 0;
    qint32 z = 0;
    qint32 rotationAngle = 0;
  };

  struct RmbHeader
  {
    quint8 blockCount = 0;
    quint8 modelCount = 0;
    quint8 flatCount = 0;

    QVector<RmbBlockPosition> blockPositions;  // 32 entries
    QVector<QByteArray> buildingData;          // 32 entries, 26 bytes each
    QVector<quint32> blockPtr;                 // 32 entries (reserved)
    QVector<quint32> blockSize;                // 32 entries
    quint32 modelPtr = 0;
    quint32 flatPtr = 0;

    QVector<quint8> groundTextures;     // 256 entries
    QVector<quint8> groundDecoration;   // 256 entries
    QByteArray automap;                 // 4096 bytes
    QString blockFileName;
    QStringList fileNameList;           // 32 entries
  };

  struct RmbRecord
  {
    RmbHeader header;
    QVector<RmbBlock> blocks;
    QVector<RmbModel> modelList;
    QVector<RmbFlat> flatList;
    QByteArray raw;
    QString warning;
  };

  struct RdbHeader
  {
    quint32 unknown1 = 0;
    quint32 width = 0;
    quint32 height = 0;
    quint32 objectRootOffset = 0;
    quint32 unknown2 = 0;
  };

  struct RdbModelReference
  {
    QString modelIdText;      // 5 chars
    QString descriptionText;  // 3 chars
  };

  struct RdbObject
  {
    qint32 offset = -1;
    qint32 nextOffset = -1;
    qint32 previousOffset = -1;
    Point position;
    quint8 type = 0;
    quint32 dataOffset = 0;
    bool hasValidData = false;
  };

  struct RdbModelData
  {
    Point rotation;
    quint16 modelIndex = 0;
    quint32 triggerFlagStartingLock = 0;
    quint8 soundIndex = 0;
    qint32 actionOffset = -1;
  };

  struct RdbLightData
  {
    quint32 unknown1 = 0;
    quint32 unknown2 = 0;
    quint16 unknown3 = 0;
  };

  struct RdbFlatData
  {
    TextureRef texture;
    quint16 gender = 0;
    quint16 factionId = 0;
    QByteArray unknown;  // 5 bytes
  };

  struct RdbAction
  {
    enum class ActionType : quint8
    {
      None = 0x00,
      Translation = 0x01,
      Unknown02 = 0x02,
      Unknown04 = 0x04,
      Rotation = 0x08,
      TranslationWithRotation = 0x09,
      Unknown0B = 0x0B,
      Unknown0C = 0x0C,
      TeleportToLinkedFlat = 0x0E,
      Unknown10 = 0x10,
      Unknown11 = 0x11,
      Unknown12 = 0x12,
      Unknown14 = 0x14,
      Unknown15 = 0x15,
      Unknown16 = 0x16,
      Unknown17 = 0x17,
      Unknown18 = 0x18,
      Unknown19 = 0x19,
      Unknown1C = 0x1C,
      ActivateUse = 0x1E,
      Unknown1F = 0x1F,
      Unknown20 = 0x20,
      Unknown32 = 0x32,
      Unknown63 = 0x63,
      Unknown64 = 0x64
    };

    enum class Axis : quint8
    {
      Unknown = 0x00,
      NegativeX = 0x01,
      PositiveX = 0x02,
      NegativeY = 0x03,
      PositiveY = 0x04,
      NegativeZ = 0x05,
      PositiveZ = 0x06
    };

    QByteArray rawData;  // 5 bytes
    qint32 targetOffset = -1;
    quint8 type = 0;
    ActionType typed = ActionType::None;
    Axis axis = Axis::Unknown;
    quint16 duration = 0;
    quint16 magnitude = 0;
    bool hasAxisData = false;
  };

  struct RdbRecord
  {
    struct ActionLink
    {
      qint32 sourceObjectOffset = -1;
      quint8 sourceObjectType = 0;
      qint32 actionOffset = -1;
      quint8 actionType = 0;
      qint32 targetObjectOffset = -1;
      quint8 targetObjectType = 0;
      bool actionDecoded = false;
      bool targetExists = false;
    };

    RdbHeader header;
    QVector<RdbModelReference> modelReferenceList;  // 750
    QVector<quint32> modelDataList;                 // 750
    quint32 unknownOffset = 0;
    quint32 objectFileSize = 0;
    QByteArray objectHeaderRaw;                     // 512
    QVector<qint32> objectRootList;                 // width*height
    QVector<RdbObject> objects;
    QHash<qint32, int> objectIndexByOffset;
    QHash<qint32, RdbModelData> modelObjects;
    QHash<qint32, RdbLightData> lightObjects;
    QHash<qint32, RdbFlatData> flatObjects;
    QHash<qint32, RdbAction> actions;
    QVector<ActionLink> actionLinks;
    QHash<qint32, QVector<int>> outgoingActionLinksByObject;
    QHash<qint32, QVector<int>> incomingActionLinksByObject;
    QByteArray raw;
    QString warning;
  };

  struct RdiStats
  {
    int size = 0;
    int count00 = 0;
    int count01 = 0;
    int countOther = 0;
    bool isBinary01Only = false;
  };

  struct Record
  {
    QString name;
    RecordType type = RecordType::Unknown;
    QByteArray raw;
    RmbRecord rmb;
    RdbRecord rdb;
    QByteArray rdi;  // always 512 bytes
    RdiStats rdiStats;
    bool isValid() const { return !name.isEmpty(); }
  };

  static bool listRecordNames(const QString& blocksBsaPath, QVector<QString>& outNames,
                              QString* errorMessage = nullptr);

  static bool loadRecord(const QString& blocksBsaPath, const QString& recordName,
                         Record& outRecord, QString* errorMessage = nullptr);

  static RecordType detectType(const QString& recordName);
  static QString typeName(RecordType type);
  static QString rdbActionTypeName(quint8 actionType);
  static QString rdbAxisName(quint8 axis);

  // MAPS.BSA block-reference helpers.
  static QString buildRmbName(int regionIndex, quint8 blockIndex, quint8 blockNumber,
                              quint8 blockCharacter);
  static QString buildRdbName(quint8 blockIndex, quint16 blockNumber);
  static QString resolveExistingRmbName(const QString& blocksBsaPath,
                                        const QString& preferredRmbName,
                                        QString* errorMessage = nullptr);
};

#endif  // DAGGERFALL_BLOCKSBSA_H
