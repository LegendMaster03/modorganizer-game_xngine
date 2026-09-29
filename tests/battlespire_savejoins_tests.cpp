#include "battlespiresavejoins.h"

#include <QtEndian>

#include <cstring>
#include <iostream>

namespace {

void writeLE32(QByteArray& data, qsizetype offset, quint32 value)
{
  const quint32 raw = qToLittleEndian(value);
  std::memcpy(data.data() + offset, &raw, sizeof(raw));
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
  QByteArray data(24, '\0');
  writeLE32(data, 0, 100);
  writeLE32(data, 8, 200);
  writeLE32(data, 16, 300);

  QHash<quint32, int> recordTypes;
  recordTypes.insert(100, 6);
  recordTypes.insert(300, 18);

  const auto summary = BattlespireSaveJoins::joinRecordIds(data, 0, 8, 3, recordTypes);
  bool ok = true;
  ok &= check(summary.populated == 3, "count populated SAVEVARS records");
  ok &= check(summary.resolved == 2, "resolve SAVEVARS IDs through SAVETREE");
  ok &= check(summary.missing == 1, "count unresolved SAVEVARS IDs");
  ok &= check(summary.missingRecordIds.size() == 1 && summary.missingRecordIds.at(0) == 200,
              "report unresolved SAVEVARS record ID");

  const auto truncated = BattlespireSaveJoins::joinRecordIds(data.left(13), 0, 8, 3, recordTypes);
  ok &= check(truncated.populated == 1 && truncated.resolved == 1,
              "stop safely at truncated fixed-record block");

  if (!ok) {
    return 1;
  }
  std::cout << "Battlespire SAVEVARS join tests passed.\n";
  return 0;
}
