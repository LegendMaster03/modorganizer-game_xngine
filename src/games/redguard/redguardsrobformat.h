#ifndef REDGUARDSROBFORMAT_H
#define REDGUARDSROBFORMAT_H

#include "redguards3d.h"

#include <QByteArray>
#include <QDir>
#include <QString>
#include <QVector>

#include <array>

class RedguardsRobFormat
{
public:
  struct Header
  {
    quint32 unknown1 = 0;
    quint32 segmentCount = 0;
    quint32 unknown2 = 0;
  };

  struct Segment
  {
    quint32 unknown1 = 0;
    QString name;
    quint32 mode = 0;
    std::array<quint32, 15> unknownFields{};
    quint32 payloadSize = 0;
    qsizetype headerOffset = 0;
    qsizetype payloadOffset = 0;
    QByteArray payloadData;

    bool usesEmbeddedPayload() const { return payloadSize > 0; }
    bool usesExternalPayload() const { return payloadSize == 0; }
    QString externalFileName() const { return name + ".3DC"; }

    bool parsedMesh = false;
    Redguards3d::MeshRecord mesh;
    QString warning;
  };

  struct Document
  {
    Header header;
    QVector<Segment> segments;
    QString warning;
  };

public:
  static bool loadFile(const QString& filePath, Document& outDocument,
                       QString* errorMessage = nullptr);
  static bool parseBytes(const QByteArray& bytes, Document& outDocument,
                         const QDir& external3dcDirectory = QDir(),
                         QString* errorMessage = nullptr);
};

#endif  // REDGUARDSROBFORMAT_H
