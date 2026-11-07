#ifndef BATTLESPIRE_3DBSA_H
#define BATTLESPIRE_3DBSA_H

#include <QString>
#include <QStringList>

#include <xngine3dformat.h>

// Battlespire archive mesh wrapper around shared Xngine3dFormat. Some
// v2.7 records require a heuristic face-header normalization; success
// does not prove every original field was reconstructed losslessly.
class Battlespire3dBsa
{
public:
  static bool listRecordNames(const QString& archivePath, QStringList& outRecordNames,
                              QString* errorMessage = nullptr);

  static bool loadMeshFile(const QString& filePath, Xngine3dFormat::MeshRecord& outMesh,
                           QString* errorMessage = nullptr);

  static bool loadMeshRecordByName(const QString& archivePath, const QString& recordName,
                                   Xngine3dFormat::MeshRecord& outMesh,
                                   QString* errorMessage = nullptr);
};

#endif  // BATTLESPIRE_3DBSA_H
