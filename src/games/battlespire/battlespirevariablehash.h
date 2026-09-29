#ifndef BATTLESPIRE_VARIABLEHASH_H
#define BATTLESPIRE_VARIABLEHASH_H

#include <QStringView>
#include <QtGlobal>

#include <string_view>

namespace BattlespireVariableHash {

constexpr quint8 uppercaseAscii(quint8 value)
{
  return (value >= 'a' && value <= 'z') ? static_cast<quint8>(value - ('a' - 'A')) : value;
}

constexpr quint32 rotateLeft4(quint32 value)
{
  return static_cast<quint32>((value << 4) | (value >> 28));
}

constexpr quint32 hashAscii(std::string_view name)
{
  quint32 hash = 0;
  for (const unsigned char byte : name) {
    hash = static_cast<quint32>(rotateLeft4(hash) + uppercaseAscii(byte));
  }
  return hash;
}

quint32 hashName(QStringView name);

static_assert(hashAscii("PCMale") == 0x05481605U);
static_assert(hashAscii("PCFemale") == 0x47AA160AU);
static_assert(hashAscii("Dagger") == 0x0485BBA2U);
static_assert(hashAscii("Armor") == 0x00467242U);
static_assert(hashAscii("QuestStart") == 0xA8988BDDU);
static_assert(hashAscii("SumeerName") == 0x19A72B9FU);

}  // namespace BattlespireVariableHash

#endif  // BATTLESPIRE_VARIABLEHASH_H
