#include "battlespire3dbsa.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtEndian>

#include <iostream>

namespace {

void appendLE32(QByteArray& out, quint32 value)
{
  const quint32 raw = qToLittleEndian(value);
  out.append(reinterpret_cast<const char*>(&raw), sizeof(raw));
}

QByteArray makeNativeFaceFixture()
{
  QByteArray data("v2.7", 4);
  const quint32 headerFields[] = {
      1,    // numVertices
      1,    // numFaces
      0,    // radius
      0,    // numFrames
      130,  // offsetFrameData
      106,  // planeData offset (legacy field name numUVOffsets)
      130,  // offsetSection4
      0,    // section4Count
      0,    // unknown4
      130,  // offsetUVOffsets
      130,  // offsetUVData
      64,   // offsetVertexCoors
      94,   // offsetFaceNormals
      0,    // numUVOffsets2
      76,   // offsetFaceData
  };
  for (quint32 value : headerFields) {
    appendLE32(data, value);
  }

  // Vertex at offset 64.
  appendLE32(data, 0);
  appendLE32(data, 0);
  appendLE32(data, 0);

  // Native 10-byte face header at offset 76: <count, unknown1, texture, unknown[6]>.
  data.append(static_cast<char>(1));
  data.append(static_cast<char>(0xAA));
  data.append(static_cast<char>(0x34));
  data.append(static_cast<char>(0x12));
  const char unknown[6] = {1, 2, 3, 4, 5, 6};
  data.append(unknown, 6);

  // One point reference: point offset 0, U=0, V=0.
  appendLE32(data, 0);
  data.append(4, '\0');

  // One face normal at offset 94.
  data.append(12, '\0');

  // One 24-byte plane-data entry at offset 106.
  data.append(24, '\0');
  return data;
}

bool check(bool condition, const char* message)
{
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

}  // namespace

int main()
{
  QTemporaryDir temp;
  if (!check(temp.isValid(), "create temporary directory")) {
    return 1;
  }

  const QString path = temp.filePath("native.3d");
  QFile file(path);
  if (!check(file.open(QIODevice::WriteOnly | QIODevice::Truncate), "open fixture file")) {
    return 1;
  }
  const QByteArray fixture = makeNativeFaceFixture();
  if (!check(file.write(fixture) == fixture.size(), "write native face fixture")) {
    return 1;
  }
  file.close();

  Battlespire3dBsa::NativeMeshRecord native;
  QString error;
  bool ok = check(Battlespire3dBsa::loadNativeMeshFile(path, native, &error),
                  "parse native ten-byte face layout");
  if (!ok) {
    std::cerr << error.toStdString() << '\n';
    return 1;
  }

  ok &= check(native.usesNativeTenByteFaces, "select native layout before inherited layout");
  ok &= check(native.nativeFaceHeaders.size() == 1, "preserve native face header");
  if (native.nativeFaceHeaders.size() == 1) {
    const auto& face = native.nativeFaceHeaders.at(0);
    ok &= check(face.pointCount == 1, "native point count");
    ok &= check(face.unknown1 == 0xAA, "native unknown1 byte");
    ok &= check(face.textureRaw == 0x1234, "native texture field");
    ok &= check(face.unknownBytes == QByteArray("\x01\x02\x03\x04\x05\x06", 6),
                "preserve all six native unknown bytes");
  }
  ok &= check(native.mesh.planes.size() == 1 && native.mesh.planes.at(0).points.size() == 1,
              "project native face into shared mesh parser");

  if (!ok) {
    return 1;
  }
  std::cout << "Battlespire native 3D face tests passed.\n";
  return 0;
}
