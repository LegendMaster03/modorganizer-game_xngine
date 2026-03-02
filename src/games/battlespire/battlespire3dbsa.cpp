#include "battlespire3dbsa.h"

#include <xnginebsaformat.h>

namespace {

bool setError(QString* errorMessage, const QString& text)
{
  if (errorMessage != nullptr) {
    *errorMessage = text;
  }
  return false;
}

}  // namespace

bool Battlespire3dBsa::listRecordNames(const QString& archivePath, QStringList& outRecordNames,
                                       QString* errorMessage)
{
  outRecordNames.clear();

  XngineBSAFormat::Archive archive;
  if (!XngineBSAFormat::readArchive(archivePath, archive, errorMessage)) {
    return false;
  }
  if (archive.type != XngineBSAFormat::IndexType::NameRecord) {
    return setError(errorMessage, "Battlespire 3D archive is not a NameRecord BSA");
  }

  outRecordNames.reserve(archive.entries.size());
  for (const auto& e : archive.entries) {
    outRecordNames.push_back(e.name);
  }
  return true;
}

bool Battlespire3dBsa::loadMeshRecordByName(const QString& archivePath, const QString& recordName,
                                            Xngine3dFormat::MeshRecord& outMesh,
                                            QString* errorMessage)
{
  XngineBSAFormat::Archive archive;
  if (!XngineBSAFormat::readArchive(archivePath, archive, errorMessage)) {
    return false;
  }
  if (archive.type != XngineBSAFormat::IndexType::NameRecord) {
    return setError(errorMessage, "Battlespire 3D archive is not a NameRecord BSA");
  }

  const QString wanted = recordName.trimmed().toUpper();
  const XngineBSAFormat::Entry* target = nullptr;
  for (const auto& e : archive.entries) {
    if (e.name.trimmed().toUpper() == wanted) {
      target = &e;
      break;
    }
  }
  if (target == nullptr) {
    return setError(errorMessage, QString("3D record '%1' not found").arg(recordName));
  }

  if (!Xngine3dFormat::parseRecord(target->data, outMesh, errorMessage)) {
    return false;
  }

  // Battlespire 3D.BSA content is expected in v2.x family.
  const auto tag = outMesh.header.versionTag;
  if (tag != Xngine3dFormat::VersionTag::V2_5 &&
      tag != Xngine3dFormat::VersionTag::V2_6 &&
      tag != Xngine3dFormat::VersionTag::V2_7) {
    return setError(errorMessage,
                    QString("Battlespire 3D record '%1' uses unsupported version '%2'")
                        .arg(recordName)
                        .arg(outMesh.header.versionString));
  }

  return true;
}
