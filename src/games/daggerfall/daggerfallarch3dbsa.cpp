#include "daggerfallarch3dbsa.h"

#include "daggerfallformatutils.h"

#include <xnginebsaformat.h>

namespace {

using Daggerfall::FormatUtil::setError;

// ARCH3D includes records with compressed flags that are not decoded as
// regular meshes. Keep such payloads available to the Daggerfall layer.
XngineBSAFormat::Traits arch3dTraits()
{
  XngineBSAFormat::Traits traits;
  // Daggerfall ARCH3D.BSA uses the compressed flag on special records we do not
  // currently decode as meshes; keep their raw payloads so the Daggerfall layer
  // can classify them without teaching the core parser a game-specific quirk.
  traits.allowCompressedPassthroughRead = true;
  return traits;
}

}  // namespace

bool DaggerfallArch3dBsa::listRecordIds(const QString& arch3dBsaPath,
                                        QVector<quint16>& outRecordIds,
                                        QString* errorMessage)
{
  outRecordIds.clear();

  XngineBSAFormat::Archive archive;
  if (!XngineBSAFormat::readArchive(arch3dBsaPath, archive, errorMessage, arch3dTraits())) {
    return false;
  }
  if (archive.type != XngineBSAFormat::IndexType::NumberRecord) {
    return setError(errorMessage, "ARCH3D.BSA is not a NumberRecord archive");
  }

  outRecordIds.reserve(archive.entries.size());
  for (const auto& e : archive.entries) {
    outRecordIds.push_back(e.recordId);
  }
  return true;
}

bool DaggerfallArch3dBsa::loadRecordBytes(const QString& arch3dBsaPath, quint16 recordId,
                                         QByteArray& outData, QString* errorMessage)
{
  outData.clear();

  XngineBSAFormat::Archive archive;
  if (!XngineBSAFormat::readArchive(arch3dBsaPath, archive, errorMessage, arch3dTraits())) {
    return false;
  }
  if (archive.type != XngineBSAFormat::IndexType::NumberRecord) {
    return setError(errorMessage, "ARCH3D.BSA is not a NumberRecord archive");
  }

  const XngineBSAFormat::Entry* target = nullptr;
  for (const auto& e : archive.entries) {
    if (e.recordId == recordId) {
      target = &e;
      break;
    }
  }
  if (target == nullptr) {
    return setError(errorMessage, QString("ARCH3D record %1 not found").arg(recordId));
  }

  outData = target->data;
  return true;
}

// A leading four-space version field is a heuristic for special records,
// not a complete classification of every non-mesh ARCH3D payload.
bool DaggerfallArch3dBsa::looksLikeBlankVersionRecord(const QByteArray& recordData)
{
  return recordData.size() >= 4 && recordData.startsWith("    ");
}

bool DaggerfallArch3dBsa::loadMeshRecord(const QString& arch3dBsaPath, quint16 recordId,
                                         MeshRecord& outMesh, QString* errorMessage)
{
  QByteArray recordData;
  if (!loadRecordBytes(arch3dBsaPath, recordId, recordData, errorMessage)) {
    return false;
  }
  return loadMeshRecord(recordData, outMesh, errorMessage);
}

// Mesh interpretation is delegated to the shared XnGine 3D parser;
// callers must distinguish special/raw records before requesting a mesh.
bool DaggerfallArch3dBsa::loadMeshRecord(const QByteArray& recordData, MeshRecord& outMesh,
                                         QString* errorMessage)
{
  return Xngine3dFormat::parseRecord(recordData, outMesh, errorMessage);
}
