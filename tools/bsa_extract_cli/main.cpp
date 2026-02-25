#include "../../src/xngine/xnginebsaformat.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>

namespace {

XngineBSAFormat::Traits battlespireTraits()
{
  XngineBSAFormat::Traits traits;
  traits.allowCompressed = true;
  traits.allowCompressedPassthroughWrite = true;
  traits.compressionMode = XngineBSAFormat::CompressionMode::BattlespireLzss;
  traits.allowMissingTypeHeader = true;
  traits.writeTypeHeader = true;
  return traits;
}

void printUsage()
{
  QTextStream err(stderr);
  err << "Usage: bsa_extract_cli <archive-path> <output-directory>\n";
}

}  // namespace

int main(int argc, char* argv[])
{
  QCoreApplication app(argc, argv);
  const QStringList args = app.arguments();
  if (args.size() != 3) {
    printUsage();
    return 2;
  }

  const QString archivePath = QDir::fromNativeSeparators(args.at(1));
  const QString outputDir = QDir::fromNativeSeparators(args.at(2));

  QFileInfo inInfo(archivePath);
  if (!inInfo.exists() || !inInfo.isFile()) {
    QTextStream(stderr) << "Archive not found: " << archivePath << '\n';
    return 3;
  }

  auto traits = battlespireTraits();
  const QString fileName = inInfo.fileName().toUpper();
  if (fileName == "SPIRE.SND") {
    traits.variantHint = XngineBSAFormat::ArchiveVariant::BattlespireSnd;
  }

  QString error;
  if (!XngineBSAFormat::unpackToDirectory(archivePath, outputDir, &error, traits)) {
    QTextStream(stderr) << "Extract failed for " << archivePath << ": " << error << '\n';
    return 1;
  }

  QTextStream(stdout) << "Extracted " << archivePath << " -> " << outputDir << '\n';
  return 0;
}
