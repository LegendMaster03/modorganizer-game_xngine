#include "battlespirebs6.h"

#include <QFile>
#include <QSet>
#include <QtEndian>

#include <cstring>

namespace {

bool setError(QString* errorMessage, const QString& text)
{
  if (errorMessage != nullptr) {
    *errorMessage = text;
  }
  return false;
}

bool hasRange(const QByteArray& data, qsizetype offset, qsizetype size)
{
  if (offset < 0 || size < 0 || offset > data.size()) {
    return false;
  }
  return size <= data.size() - offset;
}

template <typename T>
bool readLE(const QByteArray& data, qsizetype offset, T& outValue)
{
  if (!hasRange(data, offset, static_cast<qsizetype>(sizeof(T)))) {
    return false;
  }
  T raw{};
  std::memcpy(&raw, data.constData() + offset, sizeof(raw));
  outValue = qFromLittleEndian(raw);
  return true;
}

QString readCString(const QByteArray& data)
{
  const qsizetype nullPos = data.indexOf('\0');
  const qsizetype length = nullPos >= 0 ? nullPos : data.size();
  return QString::fromLatin1(data.constData(), length);
}

bool isGroupTag(const QString& tag)
{
  static const QSet<QString> groups = {
      "GNRL", "TEXI", "STRU", "SNAP", "VIEW", "CTRL", "LINK", "OBJS",
      "OBJD", "LITS", "LITD", "FLAS", "FLAD",
  };
  return groups.contains(tag);
}

bool isUInt32Tag(const QString& tag)
{
  static const QSet<QString> values = {
      "RADI", "IDNB", "IDTY", "BRIT", "SELE", "SCAL", "AMBI", "IDFI",
      "BITS", "WATR",
  };
  return values.contains(tag);
}

bool isVec3Tag(const QString& tag)
{
  return tag == "POSI" || tag == "OFST" || tag == "CENT" || tag == "ANGS";
}

bool parseChunkList(const QByteArray& data, QVector<BattlespireBs6::Chunk>& outChunks,
                    QStringList& diagnostics, QString* errorMessage, int depth)
{
  if (depth > 64) {
    return setError(errorMessage, "BS6 chunk nesting exceeds 64 levels");
  }

  qsizetype pos = 0;
  while (pos < data.size()) {
    if (!hasRange(data, pos, 8)) {
      return setError(errorMessage,
                      QString("BS6 trailing bytes at offset %1 are smaller than a chunk header")
                          .arg(pos));
    }

    const QString tag = QString::fromLatin1(data.constData() + pos, 4);
    quint32 declaredLength = 0;
    if (!readLE(data, pos + 4, declaredLength)) {
      return setError(errorMessage, QString("Failed reading BS6 chunk length at %1").arg(pos));
    }

    const qsizetype payloadOffset = pos + 8;
    const qsizetype remaining = data.size() - payloadOffset;
    if (declaredLength > static_cast<quint64>(remaining)) {
      return setError(errorMessage,
                      QString("BS6 chunk %1 at %2 declares %3 bytes but only %4 remain")
                          .arg(tag)
                          .arg(pos)
                          .arg(declaredLength)
                          .arg(remaining));
    }

    BattlespireBs6::Chunk chunk;
    chunk.tag = tag;
    chunk.declaredLength = declaredLength;
    chunk.rawData = data.mid(payloadOffset, static_cast<qsizetype>(declaredLength));

    if (isGroupTag(tag)) {
      if (!parseChunkList(chunk.rawData, chunk.children, diagnostics, errorMessage, depth + 1)) {
        return false;
      }
    } else if (tag == "FILN" || tag == "DIRN" || tag == "NAME") {
      chunk.stringValue = readCString(chunk.rawData);
      if (!chunk.rawData.contains('\0')) {
        diagnostics.push_back(QString("BS6 %1 chunk has no NUL terminator").arg(tag));
      }
    } else if (isUInt32Tag(tag)) {
      if (chunk.rawData.size() != 4) {
        return setError(errorMessage,
                        QString("BS6 %1 chunk must be 4 bytes, got %2")
                            .arg(tag)
                            .arg(chunk.rawData.size()));
      }
      if (!readLE(chunk.rawData, 0, chunk.unsignedValue)) {
        return setError(errorMessage, QString("Failed reading BS6 %1 value").arg(tag));
      }
      chunk.hasUnsignedValue = true;
    } else if (isVec3Tag(tag)) {
      if (chunk.rawData.size() != 12) {
        return setError(errorMessage,
                        QString("BS6 %1 chunk must be 12 bytes, got %2")
                            .arg(tag)
                            .arg(chunk.rawData.size()));
      }
      if (!readLE(chunk.rawData, 0, chunk.vectorValue.x) ||
          !readLE(chunk.rawData, 4, chunk.vectorValue.y) ||
          !readLE(chunk.rawData, 8, chunk.vectorValue.z)) {
        return setError(errorMessage, QString("Failed reading BS6 %1 vector").arg(tag));
      }
      chunk.hasVectorValue = true;
    } else if (tag == "BBOX") {
      if (chunk.rawData.size() != 24) {
        return setError(errorMessage,
                        QString("BS6 BBOX chunk must be 24 bytes, got %1")
                            .arg(chunk.rawData.size()));
      }
      for (qsizetype off = 0; off < 24; off += 12) {
        BattlespireBs6::Vec3 point;
        if (!readLE(chunk.rawData, off, point.x) ||
            !readLE(chunk.rawData, off + 4, point.y) ||
            !readLE(chunk.rawData, off + 8, point.z)) {
          return setError(errorMessage, "Failed reading BS6 BBOX corner");
        }
        chunk.vectorList.push_back(point);
      }
    } else if (tag == "LFIL") {
      if ((chunk.rawData.size() % 260) != 0) {
        return setError(errorMessage,
                        QString("BS6 LFIL length %1 is not a multiple of 260")
                            .arg(chunk.rawData.size()));
      }
      for (qsizetype off = 0; off < chunk.rawData.size(); off += 260) {
        chunk.fileNames.push_back(readCString(chunk.rawData.mid(off, 260)));
      }
    }

    outChunks.push_back(std::move(chunk));
    pos = payloadOffset + static_cast<qsizetype>(declaredLength);
  }

  return true;
}

void collectStrings(const BattlespireBs6::Chunk& chunk, const QString& tag,
                    QStringList& outValues)
{
  if (chunk.tag == tag && !chunk.stringValue.isEmpty()) {
    outValues.push_back(chunk.stringValue);
  }
  for (const auto& child : chunk.children) {
    collectStrings(child, tag, outValues);
  }
}

const BattlespireBs6::Chunk* directChild(const BattlespireBs6::Chunk& parent,
                                         const QString& tag)
{
  for (const auto& child : parent.children) {
    if (child.tag == tag) {
      return &child;
    }
  }
  return nullptr;
}

void buildDerivedViews(const QVector<BattlespireBs6::Chunk>& chunks,
                       BattlespireBs6::Document& document)
{
  for (const auto& chunk : chunks) {
    if (chunk.tag == "WATR" && chunk.hasUnsignedValue) {
      document.waterValues.push_back(chunk.unsignedValue);
    }

    if (chunk.tag == "OBJD") {
      BattlespireBs6::ObjectInstance instance;
      if (const auto* model = directChild(chunk, "FILN")) {
        instance.modelFilename = model->stringValue;
      }
      if (const auto* position = directChild(chunk, "POSI")) {
        if (position->hasVectorValue) {
          instance.hasPosition = true;
          instance.position = position->vectorValue;
        }
      }
      if (const auto* angles = directChild(chunk, "ANGS")) {
        if (angles->hasVectorValue) {
          instance.hasAngles = true;
          instance.angles = angles->vectorValue;
        }
      }
      if (const auto* scale = directChild(chunk, "SCAL")) {
        if (scale->hasUnsignedValue) {
          instance.hasScale = true;
          instance.scale = scale->unsignedValue;
        }
      }
      if (const auto* textures = directChild(chunk, "TEXI")) {
        collectStrings(*textures, "FILN", instance.textureFiles);
      }

      if (!instance.modelFilename.isEmpty() || instance.hasPosition || instance.hasAngles ||
          instance.hasScale || !instance.textureFiles.isEmpty()) {
        document.objects.push_back(std::move(instance));
      }
    }

    buildDerivedViews(chunk.children, document);
  }
}

}  // namespace

bool BattlespireBs6::parseData(const QByteArray& data, Document& outDocument,
                               QString* errorMessage)
{
  outDocument = {};
  if (data.isEmpty()) {
    return setError(errorMessage, "BS6 file is empty");
  }

  if (!parseChunkList(data, outDocument.chunks, outDocument.diagnostics, errorMessage, 0)) {
    return false;
  }
  buildDerivedViews(outDocument.chunks, outDocument);
  return true;
}

bool BattlespireBs6::readFile(const QString& filePath, Document& outDocument,
                              QString* errorMessage)
{
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return setError(errorMessage, QString("Unable to open BS6 file: %1").arg(filePath));
  }
  return parseData(file.readAll(), outDocument, errorMessage);
}
