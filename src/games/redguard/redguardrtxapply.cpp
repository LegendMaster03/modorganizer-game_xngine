#include "redguardsrtxdatabase.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QTextStream>

namespace
{
void writeUsage()
{
  QTextStream err(stderr);
  err << "Usage: redguardrtxapply <base_rtx> <changes_txt> <output_rtx>\n";
}
}

int main(int argc, char* argv[])
{
  QCoreApplication app(argc, argv);
  const QStringList args = app.arguments();
  if (args.size() != 4) {
    writeUsage();
    return 1;
  }

  const QString baseRtxPath = QFileInfo(args.at(1)).absoluteFilePath();
  const QString changesPath = QFileInfo(args.at(2)).absoluteFilePath();
  const QString outputPath = QFileInfo(args.at(3)).absoluteFilePath();

  QTextStream out(stdout);
  QTextStream err(stderr);

  RedguardsRtxDatabase rtxDb;
  if (!rtxDb.readFile(baseRtxPath)) {
    err << "Failed to read base RTX: " << baseRtxPath << "\n";
    return 2;
  }

  if (!rtxDb.applyChanges(changesPath)) {
    err << "Failed to apply changes file: " << changesPath << "\n";
    return 3;
  }

  if (!rtxDb.writeFile(outputPath)) {
    err << "Failed to write output RTX: " << outputPath << "\n";
    return 4;
  }

  out << "Wrote RTX: " << outputPath << "\n";
  out << "Entries: " << rtxDb.size() << "\n";
  return 0;
}
