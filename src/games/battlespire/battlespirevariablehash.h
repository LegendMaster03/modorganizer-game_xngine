#ifndef BATTLESPIRE_VARIABLEHASH_H
#define BATTLESPIRE_VARIABLEHASH_H

#include <QStringView>
#include <QtGlobal>

#include <string_view>

// Hash names using the observed 32-bit rotate-by-four and ASCII case fold.
// This is a byte-oriented algorithm, not Unicode case-insensitive hashing.
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

// QString input is first converted to Latin-1 bytes, then passed to hashAscii.
quint32 hashName(QStringView name);

// Known input/output pairs prevent accidental changes to the hash convention.
static_assert(hashAscii("PCMale") == 0x05481605U);
static_assert(hashAscii("PCFemale") == 0x47AA160AU);
static_assert(hashAscii("Dagger") == 0x0485BBA2U);
static_assert(hashAscii("Armor") == 0x00467242U);
static_assert(hashAscii("QuestStart") == 0xA8988BDDU);
static_assert(hashAscii("SumeerName") == 0x19A72B9FU);

}  // namespace BattlespireVariableHash

#endif  // BATTLESPIRE_VARIABLEHASH_H
