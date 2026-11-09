#ifndef BATTLESPIRE_3DBSA_H
#define BATTLESPIRE_3DBSA_H

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

#include <utility>

#include <xngine3dformat.h>

// Experimental Battlespire mesh adapter sharing Xngine3dFormat. It tests
// whether supported v2.x records use a ten-byte native face header rather than
// the Daggerfall-compatible layout. The synthetic fixture covers v2.7 only.
// Normalization remains a hypothesis, not evidence of lossless reconstruction.
class Battlespire3dBsa
{
public:
  // Retain the six uninterpreted bytes instead of mapping them onto
  // speculative Daggerfall fields or losing them during normalization.
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

  // Native entry points also return the original ten-byte face headers when
  // that layout is detected; basic entry points expose only the shared mesh.
  static bool loadNativeMeshFile(const QString& filePath, NativeMeshRecord& outMesh,
                                 QString* errorMessage = nullptr);

  static bool loadNativeMeshRecordByName(const QString& archivePath, const QString& recordName,
                                         NativeMeshRecord& outMesh,
                                         QString* errorMessage = nullptr);
};

#endif  // BATTLESPIRE_3DBSA_H
