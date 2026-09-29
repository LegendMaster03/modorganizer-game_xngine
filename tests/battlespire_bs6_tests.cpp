#include "battlespirebs6.h"

#include <QtEndian>

#include <cstring>
#include <iostream>

namespace {

void appendLE32(QByteArray& out, quint32 value)
{
  const quint32 raw = qToLittleEndian(value);
  out.append(reinterpret_cast<const char*>(&raw), sizeof(raw));
}

void appendLE32S(QByteArray& out, qint32 value)
{
  appendLE32(out, static_cast<quint32>(value));
}

QByteArray makeChunk(const char tag[5], const QByteArray& payload)
{
  QByteArray out(tag, 4);
  appendLE32(out, static_cast<quint32>(payload.size()));
  out.append(payload);
  return out;
}

QByteArray makeVec3(qint32 x, qint32 y, qint32 z)
{
  QByteArray out;
  appendLE32S(out, x);
  appendLE32S(out, y);
  appendLE32S(out, z);
  return out;
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
  QByteArray objectPayload;
  objectPayload += makeChunk("FILN", QByteArray("MODEL.3D\0", 9));
  objectPayload += makeChunk("POSI", makeVec3(10, 20, 30));
  objectPayload += makeChunk("ANGS", makeVec3(1, 2, 3));
  QByteArray scale;
  appendLE32(scale, 4096);
  objectPayload += makeChunk("SCAL", scale);

  QByteArray texturePayload;
  texturePayload += makeChunk("FILN", QByteArray("STONE.BSI\0", 10));
  objectPayload += makeChunk("TEXI", texturePayload);

  QByteArray objectsPayload;
  objectsPayload += makeChunk("OBJD", objectPayload);

  QByteArray rootPayload;
  rootPayload += makeChunk("OBJS", objectsPayload);
  QByteArray water;
  appendLE32(water, 7);
  rootPayload += makeChunk("WATR", water);

  const QByteArray fixture = makeChunk("GNRL", rootPayload);

  BattlespireBs6::Document document;
  QString error;
  bool ok = check(BattlespireBs6::parseData(fixture, document, &error),
                  "parse synthetic BS6 fixture");
  if (!ok) {
    std::cerr << error.toStdString() << '\n';
    return 1;
  }

  ok &= check(document.objects.size() == 1, "derive one BS6 object instance");
  if (document.objects.size() == 1) {
    const auto& object = document.objects.at(0);
    ok &= check(object.modelFilename == "MODEL.3D", "read object model filename");
    ok &= check(object.hasPosition && object.position.x == 10 && object.position.y == 20 &&
                    object.position.z == 30,
                "read object position");
    ok &= check(object.hasAngles && object.angles.x == 1 && object.angles.y == 2 &&
                    object.angles.z == 3,
                "read object angles");
    ok &= check(object.hasScale && object.scale == 4096, "read object scale");
    ok &= check(object.textureFiles.size() == 1 && object.textureFiles.at(0) == "STONE.BSI",
                "associate TEXI filename with object instance");
  }
  ok &= check(document.waterValues.size() == 1 && document.waterValues.at(0) == 7,
              "decode WATR value");

  QByteArray malformed = fixture;
  malformed.chop(1);
  BattlespireBs6::Document badDocument;
  error.clear();
  ok &= check(!BattlespireBs6::parseData(malformed, badDocument, &error),
              "reject truncated BS6 chunk");
  ok &= check(!error.isEmpty(), "report actionable BS6 truncation error");

  if (!ok) {
    return 1;
  }
  std::cout << "Battlespire BS6 tests passed.\n";
  return 0;
}
