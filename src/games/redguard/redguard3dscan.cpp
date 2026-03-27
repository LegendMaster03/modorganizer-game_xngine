#include "redguards3d.h"
#include "battlespire3dbsa.h"
#include "daggerfallarch3dbsa.h"

#include <QCoreApplication>
#include <QDirIterator>
#include <QFileInfo>
#include <QTextStream>

namespace
{
struct ScanStats
{
  int filesSeen = 0;
  int filesParsed = 0;
  int filesFailed = 0;
  int solidColorFaces = 0;
  int texturedFaces = 0;
};

bool scanPath(const QString& rootPath, QTextStream& out)
{
  QDirIterator it(rootPath, QStringList() << "*.3D" << "*.3DC", QDir::Files,
                  QDirIterator::Subdirectories);

  ScanStats stats;
  while (it.hasNext()) {
    const QString filePath = it.next();
    ++stats.filesSeen;

    Redguards3d::MeshRecord mesh;
    QString error;
    if (!Redguards3d::load(filePath, mesh, &error)) {
      ++stats.filesFailed;
      out << "FAIL " << filePath << " :: " << error << "\n";
      continue;
    }

    ++stats.filesParsed;
    for (const auto& plane : mesh.planes) {
      if (plane.texture.isSolidColor) {
        ++stats.solidColorFaces;
      } else {
        ++stats.texturedFaces;
      }
    }

    if (!mesh.warning.isEmpty()) {
      out << "WARN " << filePath << " :: " << mesh.warning << "\n";
    }
  }

  out << "Root: " << rootPath << "\n";
  out << "Files seen: " << stats.filesSeen << "\n";
  out << "Files parsed: " << stats.filesParsed << "\n";
  out << "Files failed: " << stats.filesFailed << "\n";
  out << "Solid-color faces: " << stats.solidColorFaces << "\n";
  out << "Textured faces: " << stats.texturedFaces << "\n";
  return stats.filesFailed == 0;
}

bool scanBattlespirePath(const QString& rootPath, QTextStream& out)
{
  QDirIterator it(rootPath, QStringList() << "*.3D" << "*.3DC", QDir::Files,
                  QDirIterator::Subdirectories);

  ScanStats stats;
  while (it.hasNext()) {
    const QString filePath = it.next();
    ++stats.filesSeen;

    Xngine3dFormat::MeshRecord mesh;
    QString error;
    if (!Battlespire3dBsa::loadMeshFile(filePath, mesh, &error)) {
      ++stats.filesFailed;
      out << "FAIL " << filePath << " :: " << error << "\n";
      continue;
    }

    ++stats.filesParsed;
    for (const auto& plane : mesh.planes) {
      if (plane.texture.isSolidColor) {
        ++stats.solidColorFaces;
      } else {
        ++stats.texturedFaces;
      }
    }

    if (!mesh.warning.isEmpty()) {
      out << "WARN " << filePath << " :: " << mesh.warning << "\n";
    }
  }

  out << "Root: " << rootPath << "\n";
  out << "Files seen: " << stats.filesSeen << "\n";
  out << "Files parsed: " << stats.filesParsed << "\n";
  out << "Files failed: " << stats.filesFailed << "\n";
  out << "Solid-color faces: " << stats.solidColorFaces << "\n";
  out << "Textured faces: " << stats.texturedFaces << "\n";
  return stats.filesFailed == 0;
}

bool scanDaggerfallArch3d(const QString& arch3dPath, QTextStream& out)
{
  QVector<quint16> recordIds;
  QString error;
  if (!DaggerfallArch3dBsa::listRecordIds(arch3dPath, recordIds, &error)) {
    out << "FAIL " << arch3dPath << " :: " << error << "\n";
    return false;
  }

  ScanStats stats;
  stats.filesSeen = recordIds.size();
  for (const quint16 recordId : recordIds) {
    DaggerfallArch3dBsa::MeshRecord mesh;
    QString loadError;
    if (!DaggerfallArch3dBsa::loadMeshRecord(arch3dPath, recordId, mesh, &loadError)) {
      ++stats.filesFailed;
      out << "FAIL " << arch3dPath << " :: record " << recordId << " :: "
          << loadError << "\n";
      continue;
    }

    ++stats.filesParsed;
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
  out << "Records seen: " << stats.filesSeen << "\n";
  out << "Records parsed: " << stats.filesParsed << "\n";
  out << "Records failed: " << stats.filesFailed << "\n";
  out << "Solid-color faces: " << stats.solidColorFaces << "\n";
  out << "Textured faces: " << stats.texturedFaces << "\n";
  return stats.filesFailed == 0;
}

}  // namespace

int main(int argc, char* argv[])
{
  QCoreApplication app(argc, argv);
  QTextStream out(stdout);
  QTextStream err(stderr);

  const QStringList args = app.arguments();
  if (args.size() < 2) {
    err << "Usage: redguard3dscan <path> [more paths...]\n";
    err << "   or: redguard3dscan --daggerfall-arch3d <ARCH3D.BSA>\n";
    err << "   or: redguard3dscan --battlespire-loose <path>\n";
    return 2;
  }

  if (args.size() == 3 && args[1] == "--daggerfall-arch3d") {
    return scanDaggerfallArch3d(QFileInfo(args[2]).absoluteFilePath(), out) ? 0 : 1;
  }

  if (args.size() == 3 && args[1] == "--battlespire-loose") {
    return scanBattlespirePath(QFileInfo(args[2]).absoluteFilePath(), out) ? 0 : 1;
  }

  bool ok = true;
  for (int i = 1; i < args.size(); ++i) {
    const QFileInfo info(args[i]);
    if (!info.exists()) {
      err << "Missing path: " << args[i] << "\n";
      ok = false;
      continue;
    }
    if (!scanPath(info.absoluteFilePath(), out)) {
      ok = false;
    }
  }

  return ok ? 0 : 1;
}
