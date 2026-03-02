#ifndef DAGGERFALL_ARCH3DBSA_H
#define DAGGERFALL_ARCH3DBSA_H

#include <QString>
#include <QVector>

#include <xngine3dformat.h>

class DaggerfallArch3dBsa
{
public:
  using MeshRecord = Xngine3dFormat::MeshRecord;

  static bool listRecordIds(const QString& arch3dBsaPath, QVector<quint16>& outRecordIds,
                            QString* errorMessage = nullptr);

  static bool loadMeshRecord(const QString& arch3dBsaPath, quint16 recordId,
                             MeshRecord& outMesh, QString* errorMessage = nullptr);
};

#endif  // DAGGERFALL_ARCH3DBSA_H
