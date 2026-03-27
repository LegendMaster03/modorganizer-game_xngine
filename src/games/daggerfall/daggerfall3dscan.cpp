#include "daggerfallarch3dbsa.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QTextStream>

namespace
{
struct ScanStats
{
  int recordsSeen = 0;
  int recordsParsed = 0;
  int recordsFailed = 0;
  int recordsSkipped = 0;
  int solidColorFaces = 0;
  int texturedFaces = 0;
};

bool scanArch3d(const QString& arch3dPath, QTextStream& out)
{
  QVector<quint16> recordIds;
  QString error;
  if (!DaggerfallArch3dBsa::listRecordIds(arch3dPath, recordIds, &error)) {
    out << "FAIL " << arch3dPath << " :: " << error << "\n";
    return false;
  }

  ScanStats stats;
  stats.recordsSeen = recordIds.size();
  for (const quint16 recordId : recordIds) {
    QByteArray recordData;
    QString loadError;
    if (!DaggerfallArch3dBsa::loadRecordBytes(arch3dPath, recordId, recordData, &loadError)) {
      ++stats.recordsFailed;
      out << "FAIL " << arch3dPath << " :: record " << recordId << " :: "
          << loadError << "\n";
      continue;
    }

    if (DaggerfallArch3dBsa::looksLikeBlankVersionRecord(recordData)) {
      ++stats.recordsSkipped;
      out << "SKIP " << arch3dPath << " :: record " << recordId
          << " :: blank-version ARCH3D special record\n";
      continue;
    }

    DaggerfallArch3dBsa::MeshRecord mesh;
    if (!DaggerfallArch3dBsa::loadMeshRecord(recordData, mesh, &loadError)) {
      ++stats.recordsFailed;
      out << "FAIL " << arch3dPath << " :: record " << recordId << " :: "
          << loadError << "\n";
      continue;
    }

    ++stats.recordsParsed;
    for (const auto& plane : mesh.planes) {
      if (plane.texture.isSolidColor) {
        ++stats.solidColorFaces;
      } else {
        ++stats.texturedFaces;
      }
    }

    if (!mesh.warning.isEmpty()) {
      out << "WARN " << arch3dPath << " :: record " << recordId << " :: "
          << mesh.warning << "\n";
    }
  }

  out << "ARCH3D: " << arch3dPath << "\n";
  out << "Records seen: " << stats.recordsSeen << "\n";
  out << "Records parsed: " << stats.recordsParsed << "\n";
  out << "Records failed: " << stats.recordsFailed << "\n";
  out << "Records skipped: " << stats.recordsSkipped << "\n";
  out << "Solid-color faces: " << stats.solidColorFaces << "\n";
  out << "Textured faces: " << stats.texturedFaces << "\n";
  return stats.recordsFailed == 0;
}

}  // namespace

int main(int argc, char* argv[])
{
  QCoreApplication app(argc, argv);
  QTextStream out(stdout);
  QTextStream err(stderr);

  const QStringList args = app.arguments();
  if (args.size() != 2) {
    err << "Usage: daggerfall3dscan <ARCH3D.BSA>\n";
    return 2;
  }

  const QFileInfo info(args[1]);
  if (!info.exists()) {
    err << "Missing path: " << args[1] << "\n";
    return 1;
  }

  return scanArch3d(info.absoluteFilePath(), out) ? 0 : 1;
}
