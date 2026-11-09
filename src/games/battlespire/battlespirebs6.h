#ifndef BATTLESPIRE_BS6_H
#define BATTLESPIRE_BS6_H

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

#include <utility>

// Read Battlespire BS6 tagged level data without discarding unfamiliar chunks.
// Known tags receive typed views; raw payloads and nonfatal diagnostics remain
// available while the original format is still being investigated.
class BattlespireBs6
{
public:
  struct Vec3
  {
    qint32 x = 0;
    qint32 y = 0;
    qint32 z = 0;
  };

  // Raw payload is authoritative. Derived values are populated only when
  // their corresponding has* flags or field types establish a known layout.
  struct Chunk
  {
    QString tag;
    quint32 declaredLength = 0;
    QByteArray rawData;
    QVector<Chunk> children;

    bool hasUnsignedValue = false;
    quint32 unsignedValue = 0;
    bool hasVectorValue = false;
    Vec3 vectorValue;
    QVector<Vec3> vectorList;
    QString stringValue;
    QStringList fileNames;
  };

  struct ObjectInstance
  {
    QString modelFilename;
    bool hasPosition = false;
    Vec3 position;
    bool hasAngles = false;
    Vec3 angles;
    bool hasScale = false;
    quint32 scale = 0;
    QStringList textureFiles;
  };

  // Objects and water values are convenience views over the original tree;
  // they are not a complete interpretation of the level format.
  struct Document
  {
    QVector<Chunk> chunks;
    QVector<ObjectInstance> objects;
    QVector<quint32> waterValues;
    QStringList diagnostics;
  };

  static bool parseData(const QByteArray& data, Document& outDocument,
                        QString* errorMessage = nullptr);
  static bool readFile(const QString& filePath, Document& outDocument,
                       QString* errorMessage = nullptr);
};

#endif  // BATTLESPIRE_BS6_H
