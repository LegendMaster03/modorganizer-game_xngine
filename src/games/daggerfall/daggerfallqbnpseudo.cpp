#include "daggerfallqbnpseudo.h"
#include "daggerfallformatutils.h"

#include <array>

namespace {

using Daggerfall::FormatUtil::appendWarning;
using Daggerfall::FormatUtil::readLE16U;
using Daggerfall::FormatUtil::readLE32U;
using Daggerfall::FormatUtil::setError;

}  // namespace

QHash<quint16, DaggerfallQbnPseudo::OpcodeSpec> DaggerfallQbnPseudo::opcodeSpecs()
{
  using Spec = OpcodeSpec;
  static const std::array<Spec, 77> kSpecs = {{
      {0x00, "Item & Location", 3, 3},
      {0x01, "Item & NPC", 3, 3},
      {0x02, "Check Kill Count", 3, 3},
      {0x03, "PC finds Item", 2, 2},
      {0x04, "Items", 5, 5},
      {0x05, "Unknown", 3, 3},
      {0x06, "States", 1, 1},
      {0x07, "States", 2, 5},
      {0x08, "Quest", 3, 3},
      {0x09, "Repeating Spawn", 5, 5},
      {0x0A, "Add Topics", 4, 4},
      {0x0B, "Remove Topics", 4, 4},
      {0x0C, "Start timer", 2, 2},
      {0x0D, "Stop timer", 2, 2},
      {0x11, "Locations", 4, 4},
      {0x13, "Add Location to Map", 2, 2},
      {0x15, "Mob hurt by PC", 2, 2},
      {0x16, "Place Mob at Location", 3, 3},
      {0x17, "Create Log Entry", 3, 3},
      {0x18, "Remove Log", 2, 2},
      {0x1A, "Give Item to NPC", 3, 3},
      {0x1B, "Add Global Map", 4, 4},
      {0x1C, "PC meets NPC", 2, 2},
      {0x1D, "Yes/No Question", 4, 4},
      {0x1E, "NPC, Location", 3, 3},
      {0x1F, "Daily Clock", 3, 3},
      {0x22, "Random State", 5, 5},
      {0x23, "Cycle state", 5, 5},
      {0x24, "Give Item to PC", 2, 2},
      {0x25, "Item", 4, 4},
      {0x26, "Rumors", 2, 2},
      {0x27, "Item and Mob", 3, 3},
      {0x2B, "PC at Location", 3, 3},
      {0x2C, "Delete NPC", 2, 2},
      {0x2E, "Hide NPC", 2, 2},
      {0x30, "Show NPC", 2, 2},
      {0x31, "Cure disease", 2, 2},
      {0x32, "Play movie", 2, 2},
      {0x33, "Display Message", 1, 1},
      {0x34, "AND States", 5, 5},
      {0x35, "OR States", 5, 5},
      {0x36, "Make Item ordinary", 2, 2},
      {0x37, "Escort NPC", 2, 2},
      {0x38, "End Escort NPC", 2, 2},
      {0x39, "Use Item", 3, 3},
      {0x3A, "Cure Vampirism", 1, 1},
      {0x3B, "Cure Lycanthropy", 1, 1},
      {0x3C, "Play Sound", 2, 2},
      {0x3D, "Reputation", 3, 3},
      {0x3E, "Weather Override", 3, 3},
      {0x3F, "Unknown", 2, 2},
      {0x40, "Unknown", 2, 2},
      {0x41, "Legal Reputation", 2, 2},
      {0x44, "Mob", 3, 3},
      {0x45, "Mob", 3, 3},
      {0x46, "PC has Items", 5, 5},
      {0x47, "Take PC Gold", 4, 4},
      {0x48, "Unknown", 2, 2},
      {0x49, "PC casts Spell", 3, 3},
      {0x4C, "Give Item to PC", 2, 2},
      {0x4D, "Check Player Level", 2, 2},
      {0x4E, "Unknown", 2, 2},
      // Spec documents list this as [2], but some descriptions show third arg usage.
      {0x4F, "Check Faction Reputation", 2, 3},
      {0x51, "Locations, NPC", 3, 3},
      {0x52, "Stop NPC talk", 3, 3},
      {0x53, "Location", 4, 4},
      {0x54, "Play Sound", 4, 4},
      {0x55, "Choose Questor", 4, 4},
      {0x56, "End Questor", 4, 4},
      {0x57, "Unknown", 5, 5},
  }};

  QHash<quint16, OpcodeSpec> out;
  out.reserve(static_cast<int>(kSpecs.size()));
  for (const auto& s : kSpecs) {
    out.insert(s.opCode, s);
  }
  return out;
}

QString DaggerfallQbnPseudo::opcodeName(quint16 opCode)
{
  const auto specs = opcodeSpecs();
  const auto it = specs.constFind(opCode);
  if (it == specs.constEnd()) {
    return QString("Opcode 0x%1").arg(opCode, 2, 16, QChar('0'));
  }
  return QString("Opcode 0x%1 (%2)")
      .arg(opCode, 2, 16, QChar('0'))
      .arg(it->name);
}

bool DaggerfallQbnPseudo::parseRecordBytes(const QByteArray& bytes, Record& outRecord,
                                           QString* errorMessage)
{
  outRecord = {};
  if (bytes.size() != RecordSize) {
    return setError(errorMessage, QString("QBN pseudo record must be %1 bytes").arg(RecordSize));
  }

  if (!readLE16U(bytes, 0, outRecord.opCode) || !readLE16U(bytes, 2, outRecord.flags) ||
      !readLE16U(bytes, 4, outRecord.argumentCount) ||
      !readLE16U(bytes, 81, outRecord.messageId) ||
      !readLE32U(bytes, 83, outRecord.lastUpdate)) {
    return setError(errorMessage, "Failed reading QBN pseudo record header fields");
  }

  for (int i = 0; i < MaxSubrecords; ++i) {
    const qsizetype off = 6 + static_cast<qsizetype>(i) * 15;
    auto& sub = outRecord.subrecords[i];
    sub.notFlag = static_cast<quint8>(bytes.at(off));
    if (!readLE32U(bytes, off + 1, sub.localPtr) || !readLE16U(bytes, off + 5, sub.sectionId) ||
        !readLE32U(bytes, off + 7, sub.value) || !readLE32U(bytes, off + 11, sub.objectPtr)) {
      return setError(errorMessage, QString("Failed reading subrecord %1").arg(i));
    }
  }

  if (outRecord.argumentCount == 0 || outRecord.argumentCount > MaxSubrecords) {
    appendWarning(outRecord.warning, QString("Invalid argument count: %1").arg(outRecord.argumentCount));
  }

  const auto specs = opcodeSpecs();
  const auto it = specs.constFind(outRecord.opCode);
  if (it == specs.constEnd()) {
    appendWarning(outRecord.warning,
                  QString("Unknown opcode 0x%1").arg(outRecord.opCode, 2, 16, QChar('0')));
  } else {
    const auto& spec = *it;
    if (outRecord.argumentCount < static_cast<quint16>(spec.minArgs) ||
        outRecord.argumentCount > static_cast<quint16>(spec.maxArgs)) {
      appendWarning(outRecord.warning,
                    QString("Opcode 0x%1 expects %2-%3 args, got %4")
                        .arg(outRecord.opCode, 2, 16, QChar('0'))
                        .arg(spec.minArgs)
                        .arg(spec.maxArgs)
                        .arg(outRecord.argumentCount));
    }
  }

  if (outRecord.lastUpdate != 0) {
    appendWarning(outRecord.warning,
                  QString("LastUpdate is non-zero (%1)").arg(outRecord.lastUpdate));
  }

  return true;
}

bool DaggerfallQbnPseudo::parseSectionBytes(const QByteArray& bytes, Section& outSection,
                                            QString* errorMessage)
{
  outSection = {};
  if (bytes.isEmpty()) {
    return true;
  }

  if ((bytes.size() % RecordSize) != 0) {
    appendWarning(outSection.warning,
                  QString("Section byte-size %1 is not aligned to %2-byte records")
                      .arg(bytes.size())
                      .arg(RecordSize));
  }

  const int count = static_cast<int>(bytes.size() / RecordSize);
  outSection.records.reserve(count);
  for (int i = 0; i < count; ++i) {
    const qsizetype off = static_cast<qsizetype>(i) * RecordSize;
    const QByteArray raw = bytes.mid(off, RecordSize);
    Record record;
    QString recError;
    if (!parseRecordBytes(raw, record, &recError)) {
      return setError(errorMessage, QString("Failed parsing pseudo record %1: %2")
                                        .arg(i)
                                        .arg(recError));
    }
    if (!record.warning.isEmpty()) {
      appendWarning(outSection.warning, QString("Record %1: %2").arg(i).arg(record.warning));
    }
    outSection.records.push_back(record);
  }
  return true;
}
