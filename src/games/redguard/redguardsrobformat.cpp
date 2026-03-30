#include "redguardsrobformat.h"

#include <QFile>
#include <QFileInfo>
#include <QtEndian>

#include <cstring>

namespace
{
bool setError(QString* errorMessage, const QString& text)
{
  if (errorMessage != nullptr) {
    *errorMessage = text;
  }
  return false;
}

bool readFileBytes(const QString& filePath, QByteArray& outBytes,
                   QString* errorMessage)
{
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return setError(errorMessage,
                    QString("Unable to open ROB file: %1").arg(filePath));
  }

  outBytes = file.readAll();
  return true;
}

bool readLE32(const QByteArray& data, qsizetype offset, quint32& outValue)
{
  if (offset < 0 || offset + 4 > data.size()) {
    return false;
  }

  quint32 raw = 0;
  std::memcpy(&raw, data.constData() + offset, sizeof(raw));
  outValue = qFromLittleEndian(raw);
  return true;
}

QString readFixedString8(const QByteArray& data, qsizetype offset)
{
  if (offset < 0 || offset + 8 > data.size()) {
    return {};
  }

  const QByteArray raw = data.mid(offset, 8);
  const int nul = raw.indexOf('\0');
  const QByteArray trimmed = (nul >= 0) ? raw.left(nul) : raw;
  return QString::fromLatin1(trimmed).trimmed();
}

bool parseHeader(const QByteArray& bytes, RedguardsRobFormat::Header& outHeader,
                 QString* errorMessage)
{
  if (bytes.size() < 20) {
    return setError(errorMessage,
                    QString("ROB file too short: %1 bytes").arg(bytes.size()));
  }

  if (bytes.mid(0, 4) != "OARC") {
    return setError(errorMessage, "ROB missing OARC signature");
  }
  if (bytes.mid(12, 4) != "OARD") {
    return setError(errorMessage, "ROB missing OARD signature");
  }

  if (!readLE32(bytes, 4, outHeader.unknown1) ||
      !readLE32(bytes, 8, outHeader.segmentCount) ||
      !readLE32(bytes, 16, outHeader.unknown2)) {
    return setError(errorMessage, "Failed reading ROB header");
  }

  return true;
}

bool parseSegment(const QByteArray& bytes, qsizetype cursor,
                  RedguardsRobFormat::Segment& outSegment,
                  QString* errorMessage)
{
  constexpr qsizetype kSegmentHeaderSize = 80;
  if (cursor < 0 || cursor + kSegmentHeaderSize > bytes.size()) {
    return setError(errorMessage, "ROB segment header overruns file");
  }

  outSegment = {};
  outSegment.headerOffset = cursor;
  outSegment.payloadOffset = cursor + kSegmentHeaderSize;

  if (!readLE32(bytes, cursor + 0, outSegment.unknown1) ||
      !readLE32(bytes, cursor + 12, outSegment.mode) ||
      !readLE32(bytes, cursor + 76, outSegment.payloadSize)) {
    return setError(errorMessage, "Failed reading ROB segment header");
  }

  outSegment.name = readFixedString8(bytes, cursor + 4);
  for (int i = 0; i < static_cast<int>(outSegment.unknownFields.size()); ++i) {
    if (!readLE32(bytes, cursor + 16 + i * 4, outSegment.unknownFields[i])) {
      return setError(errorMessage,
                      QString("Failed reading ROB segment field %1").arg(i));
    }
  }

  if (outSegment.payloadOffset + static_cast<qsizetype>(outSegment.payloadSize) >
      bytes.size()) {
    return setError(errorMessage,
                    QString("ROB segment '%1' payload overruns file")
                        .arg(outSegment.name));
  }

  outSegment.payloadData =
      bytes.mid(outSegment.payloadOffset,
                static_cast<qsizetype>(outSegment.payloadSize));
  return true;
}

void tryParseEmbeddedMesh(RedguardsRobFormat::Segment& segment)
{
  if (!segment.usesEmbeddedPayload()) {
    return;
  }

  QString error;
  if (Redguards3d::parseRecord(segment.payloadData, segment.mesh, &error)) {
    segment.parsedMesh = true;
    return;
  }

  segment.warning = QString("Embedded mesh parse failed: %1").arg(error);
}

void tryParseExternalMesh(const QDir& external3dcDirectory,
                          RedguardsRobFormat::Segment& segment)
{
  if (!segment.usesExternalPayload() || !external3dcDirectory.exists() ||
      segment.name.isEmpty()) {
    return;
  }

  const QString filePath = external3dcDirectory.absoluteFilePath(segment.externalFileName());
  if (!QFileInfo::exists(filePath)) {
    segment.warning =
        QString("External 3DC not found: %1").arg(segment.externalFileName());
    return;
  }

  QString error;
  if (Redguards3d::load(filePath, segment.mesh, &error)) {
    segment.parsedMesh = true;
    return;
  }

  segment.warning = QString("External 3DC parse failed: %1").arg(error);
}

}  // namespace

bool RedguardsRobFormat::loadFile(const QString& filePath, Document& outDocument,
                                  QString* errorMessage)
{
  QByteArray bytes;
  if (!readFileBytes(filePath, bytes, errorMessage)) {
    return false;
  }

  return parseBytes(bytes, outDocument, QFileInfo(filePath).dir(), errorMessage);
}

bool RedguardsRobFormat::parseBytes(const QByteArray& bytes, Document& outDocument,
                                    const QDir& external3dcDirectory,
                                    QString* errorMessage)
{
  outDocument = {};

  if (!parseHeader(bytes, outDocument.header, errorMessage)) {
    return false;
  }

  constexpr qsizetype kHeaderSize = 20;
  constexpr qsizetype kSegmentHeaderSize = 80;
  qsizetype cursor = kHeaderSize;
  outDocument.segments.reserve(static_cast<int>(outDocument.header.segmentCount));

  bool sawExternalSegment = false;
  bool sawNonCanonicalMode = false;

  for (quint32 i = 0; i < outDocument.header.segmentCount; ++i) {
    Segment segment;
    if (!parseSegment(bytes, cursor, segment, errorMessage)) {
      return false;
    }

    if (!segment.usesEmbeddedPayload()) {
      sawExternalSegment = true;
    }
    if (segment.mode != 0 && segment.mode != 256 && segment.mode != 512) {
      sawNonCanonicalMode = true;
    }

    tryParseEmbeddedMesh(segment);
    tryParseExternalMesh(external3dcDirectory, segment);

    outDocument.segments.push_back(segment);
    cursor += kSegmentHeaderSize + static_cast<qsizetype>(segment.payloadSize);
  }

  if (cursor + 4 == bytes.size() && bytes.mid(cursor, 4) == "END ") {
    cursor += 4;
  }

  if (cursor != bytes.size()) {
    outDocument.warning =
        QString("ROB parse ended at %1 of %2 bytes").arg(cursor).arg(bytes.size());
  }

  if (sawNonCanonicalMode) {
    const QString suffix = QStringLiteral(
        "Observed non-canonical ROB segment mode values outside 0/256/512");
    outDocument.warning =
        outDocument.warning.isEmpty() ? suffix
                                      : QString("%1; %2").arg(outDocument.warning, suffix);
  } else if (sawExternalSegment && outDocument.warning.isEmpty()) {
    outDocument.warning =
        QStringLiteral("ROB contains external-reference segments");
  }

  return true;
}
