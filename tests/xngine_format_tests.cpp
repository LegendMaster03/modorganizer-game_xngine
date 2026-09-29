#include "battlespiresaveformat.h"
#include "xnginebsaformat.h"
#include "xnginerecordgraph.h"

#include <QByteArray>
#include <QDataStream>
#include <QFile>
#include <QTemporaryDir>

#include <algorithm>
#include <iostream>

namespace {

bool check(bool condition, const char* message)
{
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool writeBattlespireCompressedFixture(const QString& path)
{
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return false;
  }
  QDataStream stream(&file);
  stream.setByteOrder(QDataStream::LittleEndian);
  stream << static_cast<quint16>(1)
         << static_cast<quint16>(XngineBSAFormat::IndexType::NameRecord);

  QByteArray encoded;
  encoded.append(static_cast<char>(0x07));
  encoded.append("ABC", 3);
  if (file.write(encoded) != encoded.size()) {
    return false;
  }

  QByteArray name(12, '\0');
  const QByteArray sourceName("TEST.3D");
  std::copy(sourceName.cbegin(), sourceName.cend(), name.begin());
  if (file.write(name) != name.size()) {
    return false;
  }
  stream << static_cast<qint16>(1) << static_cast<qint32>(encoded.size());
  return stream.status() == QDataStream::Ok;
}

bool writeDaggerfallNumericFixture(const QString& path)
{
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return false;
  }
  QDataStream stream(&file);
  stream.setByteOrder(QDataStream::LittleEndian);
  stream << static_cast<quint16>(1)
         << static_cast<quint16>(XngineBSAFormat::IndexType::NumberRecord);
  const QByteArray payload("XYZ", 3);
  if (file.write(payload) != payload.size()) {
    return false;
  }
  stream << static_cast<quint32>(0x12345678U)
         << static_cast<qint32>(payload.size());
  return stream.status() == QDataStream::Ok;
}

bool testSaveLayoutConstants()
{
  using namespace BattlespireSaveFormat;
  bool ok = true;
  ok &= check(PlayerVarsOffset::ActiveSpells == 370, "ActiveSpells offset is 370");
  ok &= check(PlayerVarsOffset::CharacterFlags == 550, "Flags offset is 550");
  ok &= check(PlayerVarsOffset::Team == 558, "Team offset is 558");
  ok &= check(PlayerVarsOffset::Goal == 562, "Goal offset is 562");
  ok &= check(kCurrentMapLevelOffset == 1051, "Current map offset is 1051");
  ok &= check(classifyPlayerRecord(3, true, 50000).canonical(),
              "type 3 and ID 50000 is canonical");
  ok &= check(classifyPlayerRecord(3, true, 1).recoveryCandidate(),
              "type-only match is recovery-only");
  ok &= check(classifyPlayerRecord(18, true, 50000).recoveryCandidate(),
              "ID-only match is recovery-only");
  return ok;
}

bool testRecordGraph()
{
  XngineRecordGraph graph;
  QStringList diagnostics;
  bool ok = true;
  ok &= check(graph.addNode({50000, 0, 3, -1}, &diagnostics), "add player node");
  ok &= check(graph.addNode({100, 50000, 2, -1}, &diagnostics), "add container node");
  ok &= check(graph.addNode({101, 100, 2, -1}, &diagnostics), "add nested item node");
  ok &= check(graph.traceToAncestor(101, 50000).reachesAncestor,
              "nested ownership reaches player");

  ok &= check(graph.addNode({200, 201, 2, -1}, &diagnostics), "add cycle node A");
  ok &= check(graph.addNode({201, 200, 2, -1}, &diagnostics), "add cycle node B");
  ok &= check(graph.traceToAncestor(200, 50000).cycleDetected,
              "ownership cycle is detected");

  ok &= check(graph.addNode({300, 999, 2, -1}, &diagnostics), "add orphan node");
  const auto missing = graph.traceToAncestor(300, 50000);
  ok &= check(missing.missingReference && missing.missingId == 999,
              "missing ownership reference is reported");
  ok &= check(!graph.validateLinks().isEmpty(), "graph validation reports malformed links");
  return ok;
}

bool testBattlespireCompressedRoundTrip()
{
  QTemporaryDir temp;
  bool ok = check(temp.isValid(), "create temporary directory");
  if (!ok) return false;

  const QString sourcePath = temp.filePath("source.bsa");
  const QString outputPath = temp.filePath("output.bsa");
  ok &= check(writeBattlespireCompressedFixture(sourcePath),
              "write compressed Battlespire fixture");
  if (!ok) return false;

  XngineBSAFormat::Traits traits;
  traits.allowCompressed = true;
  traits.allowCompressedPassthroughWrite = true;
  traits.compressionMode = XngineBSAFormat::CompressionMode::BattlespireLzss;
  traits.descriptorLayout = XngineBSAFormat::DescriptorLayout::Battlespire;

  QString error;
  XngineBSAFormat::Archive archive;
  ok &= check(XngineBSAFormat::readArchive(sourcePath, archive, &error, traits),
              "read compressed Battlespire archive");
  if (!ok) {
    std::cerr << error.toStdString() << '\n';
    return false;
  }
  ok &= check(archive.entries.size() == 1, "Battlespire entry count");
  ok &= check(archive.entries.at(0).data == QByteArray("ABC", 3),
              "Battlespire payload is decoded");
  ok &= check(archive.entries.at(0).compressed == 0,
              "decoded payload clears compression flag");

  error.clear();
  ok &= check(XngineBSAFormat::writeArchive(outputPath, archive, &error, traits),
              "write decoded Battlespire archive");
  if (!ok) {
    std::cerr << error.toStdString() << '\n';
    return false;
  }

  XngineBSAFormat::Archive reread;
  error.clear();
  ok &= check(XngineBSAFormat::readArchive(outputPath, reread, &error, traits),
              "read rewritten Battlespire archive");
  if (!ok) {
    std::cerr << error.toStdString() << '\n';
    return false;
  }
  ok &= check(reread.entries.size() == 1, "rewritten Battlespire entry count");
  ok &= check(reread.entries.at(0).data == QByteArray("ABC", 3),
              "rewritten payload agrees with descriptor");
  ok &= check(reread.entries.at(0).compressed == 0,
              "rewritten descriptor is uncompressed");
  return ok;
}

bool testDaggerfallDescriptorRoundTrip()
{
  QTemporaryDir temp;
  bool ok = check(temp.isValid(), "create Daggerfall temporary directory");
  if (!ok) return false;

  const QString sourcePath = temp.filePath("source.bsa");
  const QString outputPath = temp.filePath("output.bsa");
  ok &= check(writeDaggerfallNumericFixture(sourcePath), "write Daggerfall fixture");
  if (!ok) return false;

  XngineBSAFormat::Traits traits;
  traits.descriptorLayout = XngineBSAFormat::DescriptorLayout::Daggerfall;

  QString error;
  XngineBSAFormat::Archive archive;
  ok &= check(XngineBSAFormat::readArchive(sourcePath, archive, &error, traits),
              "read Daggerfall numeric archive");
  if (!ok) {
    std::cerr << error.toStdString() << '\n';
    return false;
  }
  ok &= check(archive.entries.size() == 1, "Daggerfall entry count");
  ok &= check(archive.entries.at(0).recordId == 0x12345678U,
              "Daggerfall keeps 32-bit numeric ID");
  ok &= check(archive.entries.at(0).data == QByteArray("XYZ", 3),
              "Daggerfall payload read");

  error.clear();
  ok &= check(XngineBSAFormat::writeArchive(outputPath, archive, &error, traits),
              "write Daggerfall numeric archive");
  if (!ok) {
    std::cerr << error.toStdString() << '\n';
    return false;
  }

  XngineBSAFormat::Archive reread;
  error.clear();
  ok &= check(XngineBSAFormat::readArchive(outputPath, reread, &error, traits),
              "read rewritten Daggerfall archive");
  if (!ok) {
    std::cerr << error.toStdString() << '\n';
    return false;
  }
  ok &= check(reread.entries.at(0).recordId == 0x12345678U,
              "Daggerfall 32-bit ID survives round trip");
  ok &= check(reread.entries.at(0).data == QByteArray("XYZ", 3),
              "Daggerfall payload survives round trip");
  return ok;
}

}  // namespace

int main()
{
  bool ok = true;
  ok &= testSaveLayoutConstants();
  ok &= testRecordGraph();
  ok &= testBattlespireCompressedRoundTrip();
  ok &= testDaggerfallDescriptorRoundTrip();
  if (!ok) return 1;
  std::cout << "All XnGine format tests passed.\n";
  return 0;
}
