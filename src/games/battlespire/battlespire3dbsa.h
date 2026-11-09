#ifndef BATTLESPIRE_3DBSA_H
#define BATTLESPIRE_3DBSA_H

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

#include <utility>

#include <xngine3dformat.h>

// Battlespire archive mesh wrapper around shared Xngine3dFormat. Some
// v2.7 records require a heuristic face-header normalization; success
// does not prove every original field was reconstructed losslessly.
class Battlespire3dBsa
{
public:
  struct NativeFaceHeader
  {
    quint8 pointCount = 0;
    quint8 unknown1 = 0;
    quint16 textureRaw = 0;
    QByteArray unknownBytes;  // Six bytes from the native ten-byte header.
  };

  struct NativeMeshRecord
  {
    Xngine3dFormat::MeshRecord mesh;
    QVector<NativeFaceHeader> nativeFaceHeaders;
    bool usesNativeTenByteFaces = false;
  };

  static bool listRecordNames(const QString& archivePath, QStringList& outRecordNames,
                              QString* errorMessage = nullptr);

  static bool loadMeshFile(const QString& filePath, Xngine3dFormat::MeshRecord& outMesh,
                           QString* errorMessage = nullptr);

  static bool loadMeshRecordByName(const QString& archivePath, const QString& recordName,
                                   Xngine3dFormat::MeshRecord& outMesh,
                                   QString* errorMessage = nullptr);

  static bool loadNativeMeshFile(const QString& filePath, NativeMeshRecord& outMesh,
                                 QString* errorMessage = nullptr);

  static bool loadNativeMeshRecordByName(const QString& archivePath, const QString& recordName,
                                         NativeMeshRecord& outMesh,
                                         QString* errorMessage = nullptr);
};

#endif  // BATTLESPIRE_3DBSA_H
