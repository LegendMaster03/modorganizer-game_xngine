#ifndef REDGUARDS3D_H
#define REDGUARDS3D_H

#include <xngine3dformat.h>

#include <QByteArray>
#include <QString>

class Redguards3d
{
public:
  using Header = Xngine3dFormat::Header;
  using MeshRecord = Xngine3dFormat::MeshRecord;
  using VersionTag = Xngine3dFormat::VersionTag;

public:
  static QString versionTagToString(VersionTag versionTag);
  static bool isSupportedRedguardVersion(VersionTag versionTag);

  static bool parseHeader(const QByteArray& data, Header& outHeader,
                          QString* errorMessage = nullptr);
  static bool parseRecord(const QByteArray& data, MeshRecord& outMesh,
                          QString* errorMessage = nullptr);

  static bool loadHeader(const QString& filePath, Header& outHeader,
                         QString* errorMessage = nullptr);
  static bool load(const QString& filePath, MeshRecord& outMesh,
                   QString* errorMessage = nullptr);

  static bool looksLikeRedguard3dName(const QString& fileName);
};

#endif  // REDGUARDS3D_H
