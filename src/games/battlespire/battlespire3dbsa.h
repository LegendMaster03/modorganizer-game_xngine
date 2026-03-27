#ifndef BATTLESPIRE_3DBSA_H
#define BATTLESPIRE_3DBSA_H

#include <QString>
#include <QStringList>

#include <xngine3dformat.h>

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
