#include "battlespirevariablehash.h"

#include <QString>

#include <iostream>

namespace {

struct Case
{
  const char* name;
  quint32 expected;
};

}  // namespace

int main()
{
  const Case cases[] = {
      {"PCMale", 0x05481605U},
      {"PCFemale", 0x47AA160AU},
      {"Dagger", 0x0485BBA2U},
      {"Armor", 0x00467242U},
      {"QuestStart", 0xA8988BDDU},
      {"SumeerName", 0x19A72B9FU},
  };

  bool ok = true;
  for (const auto& test : cases) {
    const quint32 actual = BattlespireVariableHash::hashName(QString::fromLatin1(test.name));
    if (actual != test.expected) {
      std::cerr << "FAIL: " << test.name << " expected 0x" << std::hex << test.expected
                << " got 0x" << actual << '\n';
      ok = false;
    }
  }

  if (!ok) {
    return 1;
  }
  std::cout << "Battlespire variable hash tests passed.\n";
  return 0;
}
