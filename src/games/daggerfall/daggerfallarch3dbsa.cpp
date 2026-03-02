#include "daggerfallarch3dbsa.h"

#include "daggerfallformatutils.h"

#include <xnginebsaformat.h>

namespace {

using Daggerfall::FormatUtil::setError;

}  // namespace

bool DaggerfallArch3dBsa::listRecordIds(const QString& arch3dBsaPath,
                                        QVector<quint16>& outRecordIds,
                                        QString* errorMessage)
{
  outRecordIds.clear();

  XngineBSAFormat::Archive archive;
  if (!XngineBSAFormat::readArchive(arch3dBsaPath, archive, errorMessage)) {
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

bool DaggerfallArch3dBsa::loadMeshRecord(const QString& arch3dBsaPath, quint16 recordId,
                                         MeshRecord& outMesh, QString* errorMessage)
{
  XngineBSAFormat::Archive archive;
  if (!XngineBSAFormat::readArchive(arch3dBsaPath, archive, errorMessage)) {
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

  return Xngine3dFormat::parseRecord(target->data, outMesh, errorMessage);
}
