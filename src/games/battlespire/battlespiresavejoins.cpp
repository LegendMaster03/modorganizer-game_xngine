#include "battlespiresavejoins.h"

#include <QtEndian>

#include <cstring>

namespace BattlespireSaveJoins {

Summary joinRecordIds(const QByteArray& data, qsizetype start, qsizetype recordSize,
                      qsizetype maxRecords, const QHash<quint32, int>& saveTreeRecordTypes)
{
  Summary summary;
  if (start < 0 || recordSize < static_cast<qsizetype>(sizeof(quint32)) || maxRecords <= 0 ||
      start > data.size()) {
    return summary;
  }

  for (qsizetype i = 0; i < maxRecords; ++i) {
    if (i > (data.size() - start) / recordSize) {
      break;
    }
    const qsizetype offset = start + i * recordSize;
    if (offset > data.size() || recordSize > data.size() - offset) {
      break;
    }

    quint32 rawId = 0;
    std::memcpy(&rawId, data.constData() + offset, sizeof(rawId));
    const quint32 recordId = qFromLittleEndian(rawId);
    if (recordId == 0) {
      continue;
    }

    ++summary.populated;
    if (saveTreeRecordTypes.contains(recordId)) {
      ++summary.resolved;
    } else {
      ++summary.missing;
      summary.missingRecordIds.push_back(recordId);
    }
  }

  return summary;
}

}  // namespace BattlespireSaveJoins
