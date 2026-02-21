#include "daggerfallblocksbsa.h"
#include "daggerfallformatutils.h"

#include "daggerfallcommon.h"
#include "xnginebsaformat.h"

#include <algorithm>

namespace {

constexpr qsizetype kRmbHeaderSize = 6776;
constexpr int kRmbMaxBlocks = 32;
constexpr int kRdbModelRefCount = 750;
constexpr qsizetype kRdiSize = 512;

using Daggerfall::FormatUtil::appendWarning;
using Daggerfall::FormatUtil::readLE16U;
using Daggerfall::FormatUtil::readLE32;
using Daggerfall::FormatUtil::readLE32U;
using Daggerfall::FormatUtil::setError;

bool readLE16S(const QByteArray& data, qsizetype off, qint16& value)
{
  quint16 raw = 0;
  if (!readLE16U(data, off, raw)) {
    return false;
  }
  value = static_cast<qint16>(raw);
  return true;
}

bool readPoint(const QByteArray& data, qsizetype off, DaggerfallBlocksBsa::Point& outPoint)
{
  return readLE32(data, off + 0, outPoint.x) &&
         readLE32(data, off + 4, outPoint.y) &&
         readLE32(data, off + 8, outPoint.z);
}

QString readFixedText(const QByteArray& data, qsizetype off, qsizetype len)
{
  if (off < 0 || len <= 0 || off + len > data.size()) {
    return {};
  }
  QByteArray bytes = data.mid(off, len);
  const int nul = bytes.indexOf('\0');
  if (nul >= 0) {
    bytes.truncate(nul);
  }
  return QString::fromLatin1(bytes).trimmed();
}

DaggerfallBlocksBsa::TextureRef decodeTexture(quint16 raw)
{
  DaggerfallBlocksBsa::TextureRef t;
  t.raw = raw;
  t.imageIndex = static_cast<int>(raw & 0x7f);
  t.fileIndex = static_cast<int>(raw >> 7);
  return t;
}

bool decodeActionData(const QByteArray& rawData, DaggerfallBlocksBsa::RdbAction& action)
{
  if (rawData.size() < 5) {
    return false;
  }
  action.axis = static_cast<DaggerfallBlocksBsa::RdbAction::Axis>(
      static_cast<quint8>(rawData.at(0)));

  quint16 duration = 0;
  quint16 magnitude = 0;
  if (!readLE16U(rawData, 1, duration) || !readLE16U(rawData, 3, magnitude)) {
    return false;
  }
  action.duration = duration;
  action.magnitude = magnitude;

  const quint8 t = static_cast<quint8>(action.type);
  action.hasAxisData = (t == 0x01 || t == 0x08 || t == 0x09);
  return true;
}

void fillRdiStats(const QByteArray& data, DaggerfallBlocksBsa::RdiStats& outStats)
{
  outStats = {};
  outStats.size = data.size();
  for (const char c : data) {
    const quint8 b = static_cast<quint8>(c);
    if (b == 0x00) {
      ++outStats.count00;
    } else if (b == 0x01) {
      ++outStats.count01;
    } else {
      ++outStats.countOther;
    }
  }
  outStats.isBinary01Only = (outStats.countOther == 0);
}

bool parseRmbModel(const QByteArray& data, qsizetype off, DaggerfallBlocksBsa::RmbModel& outModel)
{
  if (off < 0 || off + 66 > data.size()) {
    return false;
  }
  if (!readLE16U(data, off + 0, outModel.modelId1)) {
    return false;
  }
  outModel.modelId2 = static_cast<quint8>(data.at(off + 2));
  outModel.unknown1 = static_cast<quint8>(data.at(off + 3));
  if (!readLE32U(data, off + 4, outModel.unknown2) ||
      !readLE32U(data, off + 8, outModel.unknown3) ||
      !readLE32U(data, off + 12, outModel.unknown4)) {
    return false;
  }
  quint32 lo = 0;
  quint32 hi = 0;
  if (!readLE32U(data, off + 16, lo) || !readLE32U(data, off + 20, hi)) {
    return false;
  }
  outModel.nullValue1 = (static_cast<quint64>(hi) << 32) | lo;
  if (!readPoint(data, off + 24, outModel.point1) ||
      !readPoint(data, off + 36, outModel.point2) ||
      !readLE32U(data, off + 48, outModel.nullValue2) ||
      !readLE16S(data, off + 52, outModel.rotationAngle) ||
      !readLE16U(data, off + 54, outModel.unknown5) ||
      !readLE32U(data, off + 56, outModel.unknown6) ||
      !readLE32U(data, off + 60, outModel.unknown8) ||
      !readLE16U(data, off + 64, outModel.nullValue4)) {
    return false;
  }
  return true;
}

bool parseRmbFlat(const QByteArray& data, qsizetype off, DaggerfallBlocksBsa::RmbFlat& outFlat)
{
  if (off < 0 || off + 17 > data.size()) {
    return false;
  }
  quint16 tex = 0;
  if (!readPoint(data, off + 0, outFlat.position) ||
      !readLE16U(data, off + 12, tex) ||
      !readLE16U(data, off + 14, outFlat.unknown1)) {
    return false;
  }
  outFlat.texture = decodeTexture(tex);
  outFlat.flag = static_cast<quint8>(data.at(off + 16));
  return true;
}

bool parseRmbPerson(const QByteArray& data, qsizetype off, DaggerfallBlocksBsa::RmbPerson& outPerson)
{
  if (off < 0 || off + 17 > data.size()) {
    return false;
  }
  quint16 tex = 0;
  if (!readPoint(data, off + 0, outPerson.position) ||
      !readLE16U(data, off + 12, tex) ||
      !readLE16U(data, off + 14, outPerson.factionId)) {
    return false;
  }
  outPerson.texture = decodeTexture(tex);
  outPerson.flag = static_cast<quint8>(data.at(off + 16));
  return true;
}

bool parseRmbSection3(const QByteArray& data, qsizetype off, DaggerfallBlocksBsa::RmbSection3& outS3)
{
  if (off < 0 || off + 16 > data.size()) {
    return false;
  }
  return readPoint(data, off + 0, outS3.position) &&
         readLE16U(data, off + 12, outS3.unknown1) &&
         readLE16U(data, off + 14, outS3.unknown2);
}

bool parseRmbDoor(const QByteArray& data, qsizetype off, DaggerfallBlocksBsa::RmbDoor& outDoor)
{
  if (off < 0 || off + 19 > data.size()) {
    return false;
  }
  if (!readPoint(data, off + 0, outDoor.position) ||
      !readLE16U(data, off + 12, outDoor.unknown1) ||
      !readLE16S(data, off + 14, outDoor.rotationAngle) ||
      !readLE16U(data, off + 16, outDoor.unknown2)) {
    return false;
  }
  outDoor.nullValue = static_cast<quint8>(data.at(off + 18));
  return true;
}

bool parseRmbBlockData(const QByteArray& data, qsizetype& cursor,
                       DaggerfallBlocksBsa::RmbBlockData& outData)
{
  if (cursor < 0 || cursor + 17 > data.size()) {
    return false;
  }
  outData.header.modelCount = static_cast<quint8>(data.at(cursor + 0));
  outData.header.flatCount = static_cast<quint8>(data.at(cursor + 1));
  outData.header.section3Count = static_cast<quint8>(data.at(cursor + 2));
  outData.header.personCount = static_cast<quint8>(data.at(cursor + 3));
  outData.header.doorCount = static_cast<quint8>(data.at(cursor + 4));
  cursor += 17;

  outData.models.reserve(outData.header.modelCount);
  for (quint8 i = 0; i < outData.header.modelCount; ++i) {
    DaggerfallBlocksBsa::RmbModel model;
    if (!parseRmbModel(data, cursor, model)) {
      return false;
    }
    outData.models.push_back(model);
    cursor += 66;
  }

  outData.flats.reserve(outData.header.flatCount);
  for (quint8 i = 0; i < outData.header.flatCount; ++i) {
    DaggerfallBlocksBsa::RmbFlat flat;
    if (!parseRmbFlat(data, cursor, flat)) {
      return false;
    }
    outData.flats.push_back(flat);
    cursor += 17;
  }

  outData.section3.reserve(outData.header.section3Count);
  for (quint8 i = 0; i < outData.header.section3Count; ++i) {
    DaggerfallBlocksBsa::RmbSection3 s3;
    if (!parseRmbSection3(data, cursor, s3)) {
      return false;
    }
    outData.section3.push_back(s3);
    cursor += 16;
  }

  outData.persons.reserve(outData.header.personCount);
  for (quint8 i = 0; i < outData.header.personCount; ++i) {
    DaggerfallBlocksBsa::RmbPerson person;
    if (!parseRmbPerson(data, cursor, person)) {
      return false;
    }
    outData.persons.push_back(person);
    cursor += 17;
  }

  outData.doors.reserve(outData.header.doorCount);
  for (quint8 i = 0; i < outData.header.doorCount; ++i) {
    DaggerfallBlocksBsa::RmbDoor door;
    if (!parseRmbDoor(data, cursor, door)) {
      return false;
    }
    outData.doors.push_back(door);
    cursor += 19;
  }

  return true;
}

bool parseRmb(const QByteArray& data, DaggerfallBlocksBsa::RmbRecord& outRmb, QString* errorMessage)
{
  outRmb = {};
  outRmb.raw = data;
  if (data.size() < kRmbHeaderSize) {
    return setError(errorMessage, "RMB record is smaller than fixed header size");
  }

  outRmb.header.blockCount = static_cast<quint8>(data.at(0));
  outRmb.header.modelCount = static_cast<quint8>(data.at(1));
  outRmb.header.flatCount = static_cast<quint8>(data.at(2));

  if (outRmb.header.blockCount > kRmbMaxBlocks) {
    return setError(errorMessage, "RMB header block count exceeds 32");
  }

  // BlockPositionList (32 x 20)
  outRmb.header.blockPositions.reserve(32);
  qsizetype off = 3;
  for (int i = 0; i < 32; ++i) {
    DaggerfallBlocksBsa::RmbBlockPosition bp;
    if (!readLE32U(data, off + 0, bp.unknown1) ||
        !readLE32U(data, off + 4, bp.unknown2) ||
        !readLE32(data, off + 8, bp.x) ||
        !readLE32(data, off + 12, bp.z) ||
        !readLE32(data, off + 16, bp.rotationAngle)) {
      return setError(errorMessage, "Failed parsing RMB block position list");
    }
    outRmb.header.blockPositions.push_back(bp);
    off += 20;
  }

  // BuildingDataList (32 x 26)
  outRmb.header.buildingData.reserve(32);
  for (int i = 0; i < 32; ++i) {
    if (off + 26 > data.size()) {
      return setError(errorMessage, "Failed parsing RMB building data list");
    }
    outRmb.header.buildingData.push_back(data.mid(off, 26));
    off += 26;
  }

  // Section2
  outRmb.header.blockPtr.reserve(32);
  for (int i = 0; i < 32; ++i) {
    quint32 ptr = 0;
    if (!readLE32U(data, off, ptr)) {
      return setError(errorMessage, "Failed parsing RMB block pointer list");
    }
    outRmb.header.blockPtr.push_back(ptr);
    off += 4;
  }

  outRmb.header.blockSize.reserve(32);
  for (int i = 0; i < 32; ++i) {
    quint32 sz = 0;
    if (!readLE32U(data, off, sz)) {
      return setError(errorMessage, "Failed parsing RMB block size list");
    }
    outRmb.header.blockSize.push_back(sz);
    off += 4;
  }
  // modelPtr / flatPtr
  if (!readLE32U(data, off + 0, outRmb.header.modelPtr) ||
      !readLE32U(data, off + 4, outRmb.header.flatPtr)) {
    return setError(errorMessage, "Failed parsing RMB model/flat pointers");
  }
  off += 8;

  // GroundData
  outRmb.header.groundTextures.reserve(256);
  for (int i = 0; i < 256; ++i) {
    outRmb.header.groundTextures.push_back(static_cast<quint8>(data.at(off + i)));
  }
  off += 256;
  outRmb.header.groundDecoration.reserve(256);
  for (int i = 0; i < 256; ++i) {
    outRmb.header.groundDecoration.push_back(static_cast<quint8>(data.at(off + i)));
  }
  off += 256;

  // Automap (4096)
  outRmb.header.automap = data.mid(off, 4096);
  off += 4096;

  // FileNameList
  outRmb.header.blockFileName = readFixedText(data, off, 13);
  off += 13;
  for (int i = 0; i < 32; ++i) {
    outRmb.header.fileNameList.push_back(readFixedText(data, off, 13));
    off += 13;
  }

  if (off != kRmbHeaderSize) {
    return setError(errorMessage, "Unexpected RMB header decode size mismatch");
  }

  // BlockList (blockCount elements, sizes in blockSize list)
  qsizetype cursor = kRmbHeaderSize;
  quint64 declaredUsedBlockBytes = 0;
  for (int i = 0; i < outRmb.header.blockCount; ++i) {
    declaredUsedBlockBytes += outRmb.header.blockSize.at(i);
  }
  outRmb.blocks.reserve(outRmb.header.blockCount);
  for (int i = 0; i < outRmb.header.blockCount; ++i) {
    const quint32 declaredSize = outRmb.header.blockSize.at(i);
    if (declaredSize == 0) {
      // Some records can have empty slots.
      appendWarning(outRmb.warning, QString("Block %1 has declared size 0").arg(i));
      outRmb.blocks.push_back({});
      continue;
    }
    if (cursor + static_cast<qsizetype>(declaredSize) > data.size()) {
      return setError(errorMessage, "RMB block data exceeds record size");
    }

    const qsizetype blockStart = cursor;
    const qsizetype blockEnd = cursor + static_cast<qsizetype>(declaredSize);

    DaggerfallBlocksBsa::RmbBlock block;
    qsizetype blockCursor = blockStart;
    if (!parseRmbBlockData(data, blockCursor, block.exterior) ||
        !parseRmbBlockData(data, blockCursor, block.interior)) {
      return setError(errorMessage, "Failed parsing RMB block blockdata pair");
    }
    if (blockCursor < blockEnd) {
      block.trailingBytes = data.mid(blockCursor, blockEnd - blockCursor);
    }
    outRmb.blocks.push_back(block);
    cursor = blockEnd;
  }

  // ModelList
  outRmb.modelList.reserve(outRmb.header.modelCount);
  for (int i = 0; i < outRmb.header.modelCount; ++i) {
    DaggerfallBlocksBsa::RmbModel model;
    if (!parseRmbModel(data, cursor, model)) {
      return setError(errorMessage, "Failed parsing RMB model list");
    }
    outRmb.modelList.push_back(model);
    cursor += 66;
  }

  // FlatList
  outRmb.flatList.reserve(outRmb.header.flatCount);
  for (int i = 0; i < outRmb.header.flatCount; ++i) {
    DaggerfallBlocksBsa::RmbFlat flat;
    if (!parseRmbFlat(data, cursor, flat)) {
      return setError(errorMessage, "Failed parsing RMB flat list");
    }
    outRmb.flatList.push_back(flat);
    cursor += 17;
  }

  if (cursor < data.size()) {
    appendWarning(outRmb.warning, QString("RMB parse left %1 trailing bytes")
                                      .arg(data.size() - cursor));
  }

  const quint64 minExpectedSize = static_cast<quint64>(kRmbHeaderSize) +
                                  declaredUsedBlockBytes +
                                  static_cast<quint64>(outRmb.header.modelCount) * 66u +
                                  static_cast<quint64>(outRmb.header.flatCount) * 17u;
  if (minExpectedSize > static_cast<quint64>(data.size())) {
    appendWarning(outRmb.warning,
                  QString("RMB declared sections require %1 bytes but record is %2 bytes")
                      .arg(minExpectedSize)
                      .arg(data.size()));
  }

  if (outRmb.header.modelPtr != 0) {
    appendWarning(outRmb.warning,
                  QString("RMB ModelPtr is non-zero (0x%1)")
                      .arg(outRmb.header.modelPtr, 8, 16, QChar('0')));
  }
  if (outRmb.header.flatPtr != 0) {
    appendWarning(outRmb.warning,
                  QString("RMB FlatPtr is non-zero (0x%1)")
                      .arg(outRmb.header.flatPtr, 8, 16, QChar('0')));
  }

  return true;
}

bool parseRdb(const QByteArray& data, DaggerfallBlocksBsa::RdbRecord& outRdb, QString* errorMessage)
{
  outRdb = {};
  outRdb.raw = data;

  if (data.size() < 20 + (kRdbModelRefCount * 8) + (kRdbModelRefCount * 4)) {
    return setError(errorMessage, "RDB record is smaller than fixed header/reference sections");
  }

  qsizetype cursor = 0;
  if (!readLE32U(data, cursor + 0, outRdb.header.unknown1) ||
      !readLE32U(data, cursor + 4, outRdb.header.width) ||
      !readLE32U(data, cursor + 8, outRdb.header.height) ||
      !readLE32U(data, cursor + 12, outRdb.header.objectRootOffset) ||
      !readLE32U(data, cursor + 16, outRdb.header.unknown2)) {
    return setError(errorMessage, "Failed parsing RDB header");
  }
  cursor += 20;

  outRdb.modelReferenceList.reserve(kRdbModelRefCount);
  for (int i = 0; i < kRdbModelRefCount; ++i) {
    DaggerfallBlocksBsa::RdbModelReference ref;
    ref.modelIdText = QString::fromLatin1(data.constData() + cursor + 0, 5).trimmed();
    ref.descriptionText = QString::fromLatin1(data.constData() + cursor + 5, 3).trimmed();
    outRdb.modelReferenceList.push_back(ref);
    cursor += 8;
  }

  outRdb.modelDataList.reserve(kRdbModelRefCount);
  for (int i = 0; i < kRdbModelRefCount; ++i) {
    quint32 v = 0;
    if (!readLE32U(data, cursor, v)) {
      return setError(errorMessage, "Failed parsing RDB model data list");
    }
    outRdb.modelDataList.push_back(v);
    cursor += 4;
  }

  if (outRdb.header.objectRootOffset >= static_cast<quint32>(data.size())) {
    return setError(errorMessage, "RDB object root offset is outside record");
  }
  if (outRdb.header.objectRootOffset < cursor) {
    appendWarning(outRdb.warning,
                  QString("RDB object root offset (%1) is before fixed sections (%2)")
                      .arg(outRdb.header.objectRootOffset)
                      .arg(cursor));
  }

  // ObjectSection header at objectRootOffset.
  const qsizetype objSection = static_cast<qsizetype>(outRdb.header.objectRootOffset);
  if (objSection + 512 > data.size()) {
    return setError(errorMessage, "RDB object section header is truncated");
  }

  if (!readLE32U(data, objSection + 0, outRdb.unknownOffset) ||
      !readLE32U(data, objSection + 16, outRdb.objectFileSize)) {
    return setError(errorMessage, "Failed parsing RDB object section header");
  }
  outRdb.objectHeaderRaw = data.mid(objSection, 512);
  if (outRdb.objectFileSize != static_cast<quint32>(data.size())) {
    appendWarning(outRdb.warning,
                  QString("RDB object header file size (%1) != record size (%2)")
                      .arg(outRdb.objectFileSize)
                      .arg(data.size()));
  }

  const qsizetype rootListOffset = objSection + 512;
  const qsizetype rootCount =
      static_cast<qsizetype>(outRdb.header.width) * static_cast<qsizetype>(outRdb.header.height);
  const qsizetype rootBytes = rootCount * 4;
  if (rootListOffset + rootBytes > data.size()) {
    return setError(errorMessage, "RDB object root list exceeds record");
  }

  outRdb.objectRootList.reserve(rootCount);
  for (qsizetype i = 0; i < rootCount; ++i) {
    qint32 off = -1;
    if (!readLE32(data, rootListOffset + i * 4, off)) {
      return setError(errorMessage, "Failed parsing RDB object root list entry");
    }
    outRdb.objectRootList.push_back(off);
  }

  // Walk linked object lists from every root.
  QHash<qint32, bool> visited;
  for (qint32 root : outRdb.objectRootList) {
    qint32 current = root;
    while (current >= 0) {
      if (visited.contains(current)) {
        break;
      }
      visited.insert(current, true);

      if (current + 25 > data.size()) {
        outRdb.warning = "At least one RDB object points outside record";
        break;
      }

      DaggerfallBlocksBsa::RdbObject obj;
      obj.offset = current;
      if (!readLE32(data, current + 0, obj.nextOffset) ||
          !readLE32(data, current + 4, obj.previousOffset) ||
          !readPoint(data, current + 8, obj.position) ||
          !readLE32U(data, current + 21, obj.dataOffset)) {
        outRdb.warning = "Failed decoding at least one RDB object";
        break;
      }
      obj.type = static_cast<quint8>(data.at(current + 20));
      outRdb.objects.push_back(obj);
      if (obj.nextOffset >= 0 &&
          static_cast<qsizetype>(obj.nextOffset) + 25 > data.size()) {
        appendWarning(outRdb.warning, "RDB object Next pointer targets invalid offset");
      }
      if (obj.previousOffset >= 0 &&
          static_cast<qsizetype>(obj.previousOffset) + 25 > data.size()) {
        appendWarning(outRdb.warning, "RDB object Previous pointer targets invalid offset");
      }

      const qsizetype dOff = static_cast<qsizetype>(obj.dataOffset);
      if (obj.type == 0x01 && dOff + 23 <= data.size()) {
        DaggerfallBlocksBsa::RdbModelData md;
        if (readPoint(data, dOff + 0, md.rotation) &&
            readLE16U(data, dOff + 12, md.modelIndex) &&
            readLE32U(data, dOff + 14, md.triggerFlagStartingLock)) {
          md.soundIndex = static_cast<quint8>(data.at(dOff + 18));
          readLE32(data, dOff + 19, md.actionOffset);
          outRdb.modelObjects.insert(obj.offset, md);
          outRdb.objects.back().hasValidData = true;

          if (md.actionOffset >= 0 &&
              static_cast<qsizetype>(md.actionOffset) + 10 <= data.size()) {
            DaggerfallBlocksBsa::RdbAction action;
            const qsizetype aOff = static_cast<qsizetype>(md.actionOffset);
            action.rawData = data.mid(aOff, 5);
            readLE32(data, aOff + 5, action.targetOffset);
            action.type = static_cast<quint8>(data.at(aOff + 9));
            action.typed = static_cast<DaggerfallBlocksBsa::RdbAction::ActionType>(action.type);
            decodeActionData(action.rawData, action);
            outRdb.actions.insert(md.actionOffset, action);
          }
        }
      } else if (obj.type == 0x02 && dOff + 10 <= data.size()) {
        DaggerfallBlocksBsa::RdbLightData ld;
        if (readLE32U(data, dOff + 0, ld.unknown1) &&
            readLE32U(data, dOff + 4, ld.unknown2) &&
            readLE16U(data, dOff + 8, ld.unknown3)) {
          outRdb.lightObjects.insert(obj.offset, ld);
          outRdb.objects.back().hasValidData = true;
        }
      } else if (obj.type == 0x03 && dOff + 11 <= data.size()) {
        DaggerfallBlocksBsa::RdbFlatData fd;
        quint16 tex = 0;
        if (readLE16U(data, dOff + 0, tex) &&
            readLE16U(data, dOff + 2, fd.gender) &&
            readLE16U(data, dOff + 4, fd.factionId)) {
          fd.texture = decodeTexture(tex);
          fd.unknown = data.mid(dOff + 6, 5);
          outRdb.flatObjects.insert(obj.offset, fd);
          outRdb.objects.back().hasValidData = true;
        }
      }

      current = obj.nextOffset;
    }
  }

  // Build resolved object index and action graph links.
  outRdb.objectIndexByOffset.clear();
  outRdb.objectIndexByOffset.reserve(outRdb.objects.size());
  for (int i = 0; i < outRdb.objects.size(); ++i) {
    outRdb.objectIndexByOffset.insert(outRdb.objects.at(i).offset, i);
  }

  outRdb.actionLinks.clear();
  outRdb.outgoingActionLinksByObject.clear();
  outRdb.incomingActionLinksByObject.clear();
  int unresolvedActions = 0;
  int unresolvedTargets = 0;

  for (auto it = outRdb.modelObjects.constBegin(); it != outRdb.modelObjects.constEnd(); ++it) {
    const qint32 sourceOffset = it.key();
    const auto& md = it.value();
    if (md.actionOffset < 0) {
      continue;
    }

    DaggerfallBlocksBsa::RdbRecord::ActionLink link;
    link.sourceObjectOffset = sourceOffset;
    link.actionOffset = md.actionOffset;

    const int srcIdx = outRdb.objectIndexByOffset.value(sourceOffset, -1);
    if (srcIdx >= 0 && srcIdx < outRdb.objects.size()) {
      link.sourceObjectType = outRdb.objects.at(srcIdx).type;
    }

    const auto actionIt = outRdb.actions.constFind(md.actionOffset);
    if (actionIt != outRdb.actions.constEnd()) {
      link.actionDecoded = true;
      link.actionType = actionIt.value().type;
      link.targetObjectOffset = actionIt.value().targetOffset;
    } else {
      ++unresolvedActions;
    }

    const int tgtIdx = outRdb.objectIndexByOffset.value(link.targetObjectOffset, -1);
    if (tgtIdx >= 0 && tgtIdx < outRdb.objects.size()) {
      link.targetExists = true;
      link.targetObjectType = outRdb.objects.at(tgtIdx).type;
    } else if (link.actionDecoded && link.targetObjectOffset >= 0) {
      ++unresolvedTargets;
    }

    const int linkIndex = outRdb.actionLinks.size();
    outRdb.actionLinks.push_back(link);
    outRdb.outgoingActionLinksByObject[sourceOffset].push_back(linkIndex);
    if (link.targetExists) {
      outRdb.incomingActionLinksByObject[link.targetObjectOffset].push_back(linkIndex);
    }
  }
  if (unresolvedActions > 0) {
    appendWarning(outRdb.warning,
                  QString("RDB has %1 model action offsets without decodable action records")
                      .arg(unresolvedActions));
  }
  if (unresolvedTargets > 0) {
    appendWarning(outRdb.warning,
                  QString("RDB has %1 action targets that do not resolve to objects")
                      .arg(unresolvedTargets));
  }

  return true;
}

QString rmbPrefixForIndex(quint8 blockIndex)
{
  return Daggerfall::Data::rmbPrefixForBlockIndex(blockIndex);
}

}  // namespace

bool DaggerfallBlocksBsa::listRecordNames(const QString& blocksBsaPath,
                                          QVector<QString>& outNames,
                                          QString* errorMessage)
{
  outNames.clear();
  XngineBSAFormat::Archive archive;
  if (!XngineBSAFormat::readArchive(blocksBsaPath, archive, errorMessage)) {
    return false;
  }
  if (archive.type != XngineBSAFormat::IndexType::NameRecord) {
    return setError(errorMessage, "BLOCKS.BSA is not a NameRecord archive");
  }

  outNames.reserve(archive.entries.size());
  for (const auto& e : archive.entries) {
    outNames.push_back(e.name);
  }
  return true;
}

bool DaggerfallBlocksBsa::loadRecord(const QString& blocksBsaPath,
                                     const QString& recordName,
                                     Record& outRecord,
                                     QString* errorMessage)
{
  outRecord = {};

  XngineBSAFormat::Archive archive;
  if (!XngineBSAFormat::readArchive(blocksBsaPath, archive, errorMessage)) {
    return false;
  }
  if (archive.type != XngineBSAFormat::IndexType::NameRecord) {
    return setError(errorMessage, "BLOCKS.BSA is not a NameRecord archive");
  }

  const QString wanted = recordName.toUpper();
  const XngineBSAFormat::Entry* found = nullptr;
  for (const auto& e : archive.entries) {
    if (e.name.toUpper() == wanted) {
      found = &e;
      break;
    }
  }
  if (found == nullptr) {
    return setError(errorMessage, QString("BLOCKS record not found: %1").arg(recordName));
  }

  outRecord.name = found->name;
  outRecord.type = detectType(found->name);
  outRecord.raw = found->data;

  if (outRecord.type == RecordType::RMB) {
    if (!parseRmb(found->data, outRecord.rmb, errorMessage)) {
      return false;
    }
  } else if (outRecord.type == RecordType::RDB) {
    if (!parseRdb(found->data, outRecord.rdb, errorMessage)) {
      return false;
    }
  } else if (outRecord.type == RecordType::RDI) {
    outRecord.rdi = found->data;
    if (outRecord.rdi.size() != kRdiSize) {
      return setError(errorMessage,
                      QString("RDI record size is %1, expected 512").arg(outRecord.rdi.size()));
    }
    fillRdiStats(outRecord.rdi, outRecord.rdiStats);
  }

  return true;
}

DaggerfallBlocksBsa::RecordType DaggerfallBlocksBsa::detectType(const QString& recordName)
{
  const QString upper = recordName.toUpper();
  if (upper == "FOO") {
    return RecordType::FOO;
  }
  if (upper.endsWith(".RMB")) {
    return RecordType::RMB;
  }
  if (upper.endsWith(".RDB")) {
    return RecordType::RDB;
  }
  if (upper.endsWith(".RDI")) {
    return RecordType::RDI;
  }
  return RecordType::Unknown;
}

QString DaggerfallBlocksBsa::typeName(RecordType type)
{
  switch (type) {
    case RecordType::FOO: return "FOO";
    case RecordType::RMB: return "RMB";
    case RecordType::RDB: return "RDB";
    case RecordType::RDI: return "RDI";
    default: return "Unknown";
  }
}

QString DaggerfallBlocksBsa::rdbActionTypeName(quint8 actionType)
{
  switch (actionType) {
    case 0x00: return "None";
    case 0x01: return "Translation";
    case 0x02: return "Unknown 0x02";
    case 0x04: return "Unknown 0x04";
    case 0x08: return "Rotation";
    case 0x09: return "Translation+Rotation";
    case 0x0B: return "Unknown 0x0B";
    case 0x0C: return "Unknown 0x0C";
    case 0x0E: return "Teleport to linked flat";
    case 0x10: return "Unknown 0x10";
    case 0x11: return "Unknown 0x11";
    case 0x12: return "Unknown 0x12";
    case 0x14: return "Unknown 0x14";
    case 0x15: return "Unknown 0x15";
    case 0x16: return "Unknown 0x16";
    case 0x17: return "Unknown 0x17";
    case 0x18: return "Unknown 0x18";
    case 0x19: return "Unknown 0x19";
    case 0x1C: return "Unknown 0x1C";
    case 0x1E: return "Activate/Use";
    case 0x1F: return "Unknown 0x1F";
    case 0x20: return "Unknown 0x20";
    case 0x32: return "Unknown 0x32";
    case 0x63: return "Unknown 0x63";
    case 0x64: return "Unknown 0x64";
    default:
      return QString("Unknown 0x%1").arg(actionType, 2, 16, QChar('0')).toUpper();
  }
}

QString DaggerfallBlocksBsa::rdbAxisName(quint8 axis)
{
  switch (axis) {
    case 0x01: return "Negative X";
    case 0x02: return "Positive X";
    case 0x03: return "Negative Y";
    case 0x04: return "Positive Y";
    case 0x05: return "Negative Z";
    case 0x06: return "Positive Z";
    default:
      return QString("Unknown 0x%1").arg(axis, 2, 16, QChar('0')).toUpper();
  }
}

QString DaggerfallBlocksBsa::buildRmbName(int regionIndex, quint8 blockIndex,
                                          quint8 blockNumber, quint8 blockCharacter)
{
  const QString prefix = rmbPrefixForIndex(blockIndex);
  if (prefix.isEmpty()) {
    return {};
  }

  QString letters;
  QString numbers;

  if (prefix == "TEMP") {
    letters = (blockCharacter > 0x07) ? "GA" : "AA";
    static const QVector<QString> templeNumbers = {"A0", "B0", "C0", "D0",
                                                   "E0", "F0", "G0", "H0"};
    numbers = templeNumbers.at(static_cast<int>(blockCharacter & 0x07));
  } else {
    int q = static_cast<int>(blockCharacter) / 16;
    if (prefix == "CUST") {
      if (regionIndex == 23) {         // Wayrest
        q = 0;
      } else if (regionIndex == 20) {  // Sentinel
        q = 8;
      } else {
        q = 0;
      }
    } else if (regionIndex == 23 && q > 0) {  // Wayrest non-CUST
      --q;
    }

    static const QVector<QString> letterTable = {"AA", "BA", "AL", "BL", "AM", "BM",
                                                  "AS", "BS", "GA", "GL", "GM", "GS"};
    if (q < 0 || q >= letterTable.size()) {
      return {};
    }
    letters = letterTable.at(q);
    numbers = QString::number(blockNumber);
  }

  return QString("%1%2%3.RMB").arg(prefix, letters, numbers).toUpper();
}

QString DaggerfallBlocksBsa::buildRdbName(quint8 blockIndex, quint16 blockNumber)
{
  const QString prefix = Daggerfall::Data::rdbPrefixForBlockIndex(blockIndex);
  if (prefix.isEmpty()) {
    return {};
  }
  return QString("%1%2.RDB")
      .arg(prefix)
      .arg(static_cast<int>(blockNumber), 7, 10, QChar('0'));
}

QString DaggerfallBlocksBsa::resolveExistingRmbName(const QString& blocksBsaPath,
                                                    const QString& preferredRmbName,
                                                    QString* errorMessage)
{
  QVector<QString> names;
  if (!listRecordNames(blocksBsaPath, names, errorMessage)) {
    return {};
  }

  const QString preferred = preferredRmbName.toUpper();
  if (std::find(names.begin(), names.end(), preferred) != names.end()) {
    return preferred;
  }

  if (!preferred.endsWith(".RMB") || preferred.size() < 8) {
    return {};
  }
  const QString stem = preferred.left(preferred.size() - 4);
  const QString prefix = stem.left(4);
  QString suffix;
  if (stem.size() >= 2) {
    suffix = stem.right(2);
  }

  QStringList candidates;
  for (const auto& name : names) {
    const QString upper = name.toUpper();
    if (!upper.endsWith(".RMB")) {
      continue;
    }
    const QString nStem = upper.left(upper.size() - 4);
    if (nStem.startsWith(prefix) && nStem.endsWith(suffix)) {
      candidates.push_back(upper);
    }
  }

  if (candidates.isEmpty()) {
    return {};
  }
  std::sort(candidates.begin(), candidates.end());
  return candidates.front();
}
