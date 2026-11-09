#ifndef BATTLESPIRE_BSI_H
#define BATTLESPIRE_BSI_H

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

#include <utility>

// Experimental BSI decoder based on current format research. The implementation
// models big-endian chunk lengths and separate pixel/header encodings, but its
// agreement with synthetic fixtures is not proof that it handles original assets.
// Keep unknown fields and palette semantics unresolved where evidence is lacking.
class BattlespireBsi
{
public:
  struct Rgb
  {
    quint8 r = 0;
    quint8 g = 0;
    quint8 b = 0;
  };

  struct Header
  {
    quint16 xOffset = 0;
    quint16 yOffset = 0;
    quint16 width = 0;
    quint16 height = 0;
    QByteArray unknown6;
    quint16 frames = 0;
    quint16 unknown3 = 0;
    quint16 unknown4 = 0;
    quint16 unknown5 = 0;
    quint16 unknown6Value = 0;
    quint16 compression = 0;
  };

  struct Chunk
  {
    QString tag;
    quint32 declaredLength = 0;
    QByteArray data;
  };

  // Keep encoded bytes and raw palette tables as well as decoded views so
  // later format discoveries do not depend on a lossy conversion.
  struct Document
  {
    bool hasBsifPreamble = false;
    quint32 bsifDeclaredLength = 0;
    QString name;
    bool hasHeader = false;
    Header header;
    QByteArray ifhd;
    QByteArray hiclRaw;
    QByteArray htblRaw;
    QByteArray cmapRaw;
    QByteArray encodedPixels;
    QByteArray decodedPixels;
    QVector<Rgb> hiclPalette;
    QVector<QVector<Rgb>> lightingPalettes;
    QVector<Rgb> cmapPalette;
    QVector<Chunk> unknownChunks;
    QStringList diagnostics;
  };

  static bool parseData(const QByteArray& data, Document& outDocument,
                        QString* errorMessage = nullptr);
  static bool readFile(const QString& filePath, Document& outDocument,
                       QString* errorMessage = nullptr);
};

#endif  // BATTLESPIRE_BSI_H
