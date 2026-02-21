#include "daggerfallqbn.h"
#include "daggerfallformatutils.h"

#include <QFile>

#include <algorithm>

namespace {

constexpr qsizetype kHeaderSize = 60;

constexpr qsizetype kSizeItems = 0x13;
constexpr qsizetype kSizeSection1 = 0x5e;
constexpr qsizetype kSizeSection2 = 0x22;
constexpr qsizetype kSizeNpcs = 0x14;
constexpr qsizetype kSizeLocations = 0x18;
constexpr qsizetype kSizeSection5 = 0x10;
constexpr qsizetype kSizeTimers = 0x21;
constexpr qsizetype kSizeMobs = 0x0e;
constexpr qsizetype kSizeOpCodes = 0x57;
constexpr qsizetype kSizeStates = 0x08;
constexpr qsizetype kSizeTextVariables = 0x1b;

using Daggerfall::FormatUtil::appendWarning;
using Daggerfall::FormatUtil::readLE16;
using Daggerfall::FormatUtil::readLE16U;
using Daggerfall::FormatUtil::readLE32;
using Daggerfall::FormatUtil::readLE32U;
using Daggerfall::FormatUtil::readU8;
using Daggerfall::FormatUtil::setError;

bool parseHeader(const QByteArray& data, DaggerfallQbn::Header& out, QString* errorMessage)
{
  if (data.size() < kHeaderSize) {
    return setError(errorMessage, "QBN file is smaller than 60-byte header");
  }
  if (!readLE16U(data, 0, out.questId) || !readLE16U(data, 2, out.factionId) ||
      !readLE16U(data, 4, out.resourceId) || !readU8(data, 15, out.hasDebugInfo) ||
      !readLE16U(data, 58, out.null2)) {
    return setError(errorMessage, "Failed reading QBN header fields");
  }
  out.resourceFilename = data.mid(6, 9);
  for (int i = 0; i < 10; ++i) {
    if (!readLE16U(data, 16 + i * 2, out.sectionRecordCount[i])) {
      return setError(errorMessage, "Failed reading sectionRecordCount");
    }
  }
  for (int i = 0; i < 11; ++i) {
    if (!readLE16U(data, 36 + i * 2, out.sectionOffset[i])) {
      return setError(errorMessage, "Failed reading sectionOffset");
    }
  }
  return true;
}

qsizetype sectionSizeForIndex(int i)
{
  switch (i) {
    case 0: return kSizeItems;
    case 1: return kSizeSection1;
    case 2: return kSizeSection2;
    case 3: return kSizeNpcs;
    case 4: return kSizeLocations;
    case 5: return kSizeSection5;
    case 6: return kSizeTimers;
    case 7: return kSizeMobs;
    case 8: return kSizeOpCodes;
    case 9: return kSizeStates;
    case 10: return kSizeTextVariables;
    default: return 0;
  }
}

QVector<DaggerfallQbn::SectionRange> buildSectionRanges(const QByteArray& data,
                                                        const DaggerfallQbn::Header& h)
{
  QVector<DaggerfallQbn::SectionRange> ranges;
  ranges.reserve(11);
  for (int i = 0; i < 11; ++i) {
    const qsizetype off = static_cast<qsizetype>(h.sectionOffset[i]);
    if (off <= 0 || off >= data.size()) {
      continue;
    }
    ranges.push_back({i, off, 0});
  }
  std::sort(ranges.begin(), ranges.end(),
            [](const auto& a, const auto& b) { return a.offset < b.offset; });
  for (int i = 0; i < ranges.size(); ++i) {
    const qsizetype next = (i + 1 < ranges.size()) ? ranges[i + 1].offset : data.size();
    ranges[i].size = std::max<qsizetype>(0, next - ranges[i].offset);
  }
  return ranges;
}

const DaggerfallQbn::SectionRange* findRange(const QVector<DaggerfallQbn::SectionRange>& ranges,
                                             int index)
{
  for (const auto& r : ranges) {
    if (r.index == index) {
      return &r;
    }
  }
  return nullptr;
}

template <typename TRecord>
bool parseFixedSection(const QByteArray& data, qsizetype off, qsizetype bytesSize,
                       qsizetype recSize, quint16 expectedCount,
                       QVector<TRecord>& outVec, QString* warning,
                       QString* errorMessage)
{
  if (off < 0 || bytesSize < 0 || off + bytesSize > data.size()) {
    return setError(errorMessage, "Section range is outside file");
  }
  if (recSize <= 0) {
    return setError(errorMessage, "Section record size is invalid");
  }
  const int bySize = static_cast<int>(bytesSize / recSize);
  if ((bytesSize % recSize) != 0) {
    appendWarning(*warning, QString("Section has %1 trailing bytes").arg(bytesSize % recSize));
  }
  if (expectedCount != static_cast<quint16>(bySize)) {
    appendWarning(*warning,
                  QString("Header count %1 differs from size-derived count %2")
                      .arg(expectedCount)
                      .arg(bySize));
  }
  outVec.resize(bySize);
  return true;
}

}  // namespace

bool DaggerfallQbn::loadFile(const QString& qbnPath, File& outFile, QString* errorMessage)
{
  outFile = {};
  QFile f(qbnPath);
  if (!f.open(QIODevice::ReadOnly)) {
    return setError(errorMessage, QString("Unable to open QBN file: %1").arg(qbnPath));
  }
  return parseBytes(f.readAll(), outFile, errorMessage);
}

bool DaggerfallQbn::parseBytes(const QByteArray& bytes, File& outFile,
                               QString* errorMessage)
{
  outFile = {};
  outFile.raw = bytes;

  if (!parseHeader(bytes, outFile.header, errorMessage)) {
    return false;
  }
  if (outFile.header.null2 != 0) {
    appendWarning(outFile.warning, QString("Header null2 is non-zero (%1)").arg(outFile.header.null2));
  }

  outFile.sectionRanges = buildSectionRanges(bytes, outFile.header);

  // Section 0: Items
  if (const auto* r = findRange(outFile.sectionRanges, 0)) {
    if (!parseFixedSection(bytes, r->offset, r->size, kSizeItems, outFile.header.sectionRecordCount[0],
                           outFile.items, &outFile.warning, errorMessage)) {
      return false;
    }
    for (int i = 0; i < outFile.items.size(); ++i) {
      const qsizetype off = r->offset + static_cast<qsizetype>(i) * kSizeItems;
      auto& rec = outFile.items[i];
      if (!readLE16(bytes, off + 0, rec.itemIndex) || !readU8(bytes, off + 2, rec.reward) ||
          !readLE16U(bytes, off + 3, rec.itemCategory) ||
          !readLE16U(bytes, off + 5, rec.itemCategoryIndex) ||
          !readLE32U(bytes, off + 7, rec.textVariableHash) ||
          !readLE32U(bytes, off + 11, rec.nullValue) ||
          !readLE16U(bytes, off + 15, rec.textRecordId1) ||
          !readLE16U(bytes, off + 17, rec.textRecordId2)) {
        return setError(errorMessage, QString("Failed reading Item record %1").arg(i));
      }
    }
  }

  // Section 1 / 2 / 5: unknown raw sections.
  if (const auto* r = findRange(outFile.sectionRanges, 1)) {
    outFile.section1Raw = bytes.mid(r->offset, r->size);
  }
  if (const auto* r = findRange(outFile.sectionRanges, 2)) {
    outFile.section2Raw = bytes.mid(r->offset, r->size);
  }
  if (const auto* r = findRange(outFile.sectionRanges, 5)) {
    outFile.section5Raw = bytes.mid(r->offset, r->size);
  }

  // Section 3: NPCs
  if (const auto* r = findRange(outFile.sectionRanges, 3)) {
    if (!parseFixedSection(bytes, r->offset, r->size, kSizeNpcs, outFile.header.sectionRecordCount[3],
                           outFile.npcs, &outFile.warning, errorMessage)) {
      return false;
    }
    for (int i = 0; i < outFile.npcs.size(); ++i) {
      const qsizetype off = r->offset + static_cast<qsizetype>(i) * kSizeNpcs;
      auto& rec = outFile.npcs[i];
      if (!readLE16(bytes, off + 0, rec.npcIndex) || !readU8(bytes, off + 2, rec.gender) ||
          !readU8(bytes, off + 3, rec.facePictureIndex) ||
          !readLE16U(bytes, off + 4, rec.unknown1) ||
          !readLE16U(bytes, off + 6, rec.factionIndex) ||
          !readLE32U(bytes, off + 8, rec.textVariableHash) ||
          !readLE32U(bytes, off + 12, rec.nullValue) ||
          !readLE16U(bytes, off + 16, rec.textRecordId1) ||
          !readLE16U(bytes, off + 18, rec.textRecordId2)) {
        return setError(errorMessage, QString("Failed reading NPC record %1").arg(i));
      }
    }
  }

  // Section 4: Locations
  if (const auto* r = findRange(outFile.sectionRanges, 4)) {
    if (!parseFixedSection(bytes, r->offset, r->size, kSizeLocations,
                           outFile.header.sectionRecordCount[4], outFile.locations,
                           &outFile.warning, errorMessage)) {
      return false;
    }
    for (int i = 0; i < outFile.locations.size(); ++i) {
      const qsizetype off = r->offset + static_cast<qsizetype>(i) * kSizeLocations;
      auto& rec = outFile.locations[i];
      if (!readLE16U(bytes, off + 0, rec.locationIndex) || !readU8(bytes, off + 2, rec.flags) ||
          !readU8(bytes, off + 3, rec.generalLocation) ||
          !readLE16U(bytes, off + 4, rec.fineLocation) ||
          !readLE16(bytes, off + 6, rec.locationType) ||
          !readLE16(bytes, off + 8, rec.doorSelector) ||
          !readLE16U(bytes, off + 10, rec.unknown2) ||
          !readLE32U(bytes, off + 12, rec.textVariableHash) ||
          !readLE32U(bytes, off + 16, rec.objPtr) ||
          !readLE16U(bytes, off + 20, rec.textRecordId1) ||
          !readLE16U(bytes, off + 22, rec.textRecordId2)) {
        return setError(errorMessage, QString("Failed reading Location record %1").arg(i));
      }
    }
  }

  // Section 6: Timers
  if (const auto* r = findRange(outFile.sectionRanges, 6)) {
    if (!parseFixedSection(bytes, r->offset, r->size, kSizeTimers,
                           outFile.header.sectionRecordCount[6], outFile.timers,
                           &outFile.warning, errorMessage)) {
      return false;
    }
    for (int i = 0; i < outFile.timers.size(); ++i) {
      const qsizetype off = r->offset + static_cast<qsizetype>(i) * kSizeTimers;
      auto& rec = outFile.timers[i];
      if (!readLE16(bytes, off + 0, rec.timerIndex) || !readLE16U(bytes, off + 2, rec.flags) ||
          !readU8(bytes, off + 4, rec.type) || !readLE32(bytes, off + 5, rec.minimum) ||
          !readLE32(bytes, off + 9, rec.maximum) || !readLE32U(bytes, off + 13, rec.started) ||
          !readLE32U(bytes, off + 17, rec.duration) || !readLE32(bytes, off + 21, rec.link1) ||
          !readLE32(bytes, off + 25, rec.link2) ||
          !readLE32U(bytes, off + 29, rec.textVariableHash)) {
        return setError(errorMessage, QString("Failed reading Timer record %1").arg(i));
      }
    }
  }

  // Section 7: Mobs
  if (const auto* r = findRange(outFile.sectionRanges, 7)) {
    if (!parseFixedSection(bytes, r->offset, r->size, kSizeMobs,
                           outFile.header.sectionRecordCount[7], outFile.mobs,
                           &outFile.warning, errorMessage)) {
      return false;
    }
    for (int i = 0; i < outFile.mobs.size(); ++i) {
      const qsizetype off = r->offset + static_cast<qsizetype>(i) * kSizeMobs;
      auto& rec = outFile.mobs[i];
      if (!readU8(bytes, off + 0, rec.mobIndex) || !readLE16U(bytes, off + 1, rec.null1) ||
          !readU8(bytes, off + 3, rec.mobType) || !readLE16U(bytes, off + 4, rec.mobCount) ||
          !readLE32U(bytes, off + 6, rec.textVariableHash) ||
          !readLE32U(bytes, off + 10, rec.null2)) {
        return setError(errorMessage, QString("Failed reading Mob record %1").arg(i));
      }
    }
  }

  // Section 8: Pseudo-code opcodes.
  if (const auto* r = findRange(outFile.sectionRanges, 8)) {
    QByteArray sectionBytes = bytes.mid(r->offset, r->size);
    QString pseudoError;
    if (!DaggerfallQbnPseudo::parseSectionBytes(sectionBytes, outFile.opCodes, &pseudoError)) {
      return setError(errorMessage, QString("Failed parsing opcode section: %1").arg(pseudoError));
    }
    if (!outFile.opCodes.warning.isEmpty()) {
      appendWarning(outFile.warning, QString("Opcode section: %1").arg(outFile.opCodes.warning));
    }
  }

  // Section 9: States
  if (const auto* r = findRange(outFile.sectionRanges, 9)) {
    if (!parseFixedSection(bytes, r->offset, r->size, kSizeStates,
                           outFile.header.sectionRecordCount[9], outFile.states,
                           &outFile.warning, errorMessage)) {
      return false;
    }
    for (int i = 0; i < outFile.states.size(); ++i) {
      const qsizetype off = r->offset + static_cast<qsizetype>(i) * kSizeStates;
      auto& rec = outFile.states[i];
      if (!readLE16(bytes, off + 0, rec.flagIndex) || !readU8(bytes, off + 2, rec.isGlobal) ||
          !readU8(bytes, off + 3, rec.globalIndex) ||
          !readLE32U(bytes, off + 4, rec.textVariableHash)) {
        return setError(errorMessage, QString("Failed reading State record %1").arg(i));
      }
    }
  }

  // Section 10: Optional text variable table (usually only when hasDebugInfo != 0).
  if (const auto* r = findRange(outFile.sectionRanges, 10)) {
    const QByteArray section = bytes.mid(r->offset, r->size);
    if ((section.size() % kSizeTextVariables) != 0) {
      appendWarning(outFile.warning,
                    QString("Text variable section has %1 trailing bytes")
                        .arg(section.size() % kSizeTextVariables));
    }
    const int count = static_cast<int>(section.size() / kSizeTextVariables);
    outFile.textVariables.reserve(count);
    for (int i = 0; i < count; ++i) {
      const qsizetype off = static_cast<qsizetype>(i) * kSizeTextVariables;
      TextVariableRecord rec;
      rec.textVariable = Daggerfall::FormatUtil::readFixedString(section, off + 0, 20, true);
      if (rec.textVariable.isEmpty()) {
        break;  // null string terminator ends list
      }
      if (!readU8(section, off + 20, rec.sectionId) ||
          !readLE16U(section, off + 21, rec.recordId) ||
          !readLE32U(section, off + 23, rec.recordPtr)) {
        return setError(errorMessage, QString("Failed reading TextVariable record %1").arg(i));
      }
      outFile.textVariables.push_back(rec);
    }
  } else if (outFile.header.hasDebugInfo != 0) {
    appendWarning(outFile.warning,
                  "Header hasDebugInfo is non-zero but sectionOffset[10] is not valid");
  }

  return true;
}
