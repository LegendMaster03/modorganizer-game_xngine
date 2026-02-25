#include "xnginebsaformat.h"

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QStringList>
#include <QTextStream>

#include <array>
#include <cctype>
#include <cstring>
#include <limits>

namespace {

bool isDos83Name(const QString& name)
{
  const QString upper = name.toUpper();
  const QStringList parts = upper.split('.');
  if (parts.isEmpty() || parts.size() > 2) {
    return false;
  }

  const QString stem = parts.at(0);
  const QString ext = (parts.size() == 2) ? parts.at(1) : QString();

  auto validChunk = [](const QString& s, int maxLen) {
    if (s.isEmpty() || s.size() > maxLen) {
      return false;
    }
    for (const QChar c : s) {
      if (!c.isLetterOrNumber() && c != '_' && c != '-') {
        return false;
      }
    }
    return true;
  };

  if (!validChunk(stem, 8)) {
    return false;
  }
  if (!ext.isEmpty() && !validChunk(ext, 3)) {
    return false;
  }
  return true;
}

QString sanitizeOutputName(const QString& name)
{
  QString out = name;
  out.replace('\\', '_');
  out.replace('/', '_');
  out.replace(':', '_');
  return out;
}

bool setError(QString* errorMessage, const QString& error)
{
  if (errorMessage != nullptr) {
    *errorMessage = error;
  }
  return false;
}

bool startsWithRiffWave(const QByteArray& data)
{
  if (data.size() < 12) {
    return false;
  }
  return data.startsWith("RIFF") && data.mid(8, 4) == "WAVE";
}

int descriptorSizeForType(XngineBSAFormat::IndexType type)
{
  return (type == XngineBSAFormat::IndexType::NameRecord) ? 18 : 8;
}

void lzssOutByte(QByteArray& out, std::array<quint8, 4096>& window, int& windowPos,
                 quint8 value)
{
  window[windowPos] = value;
  windowPos = (windowPos + 1) & 0x0FFF;
  out.append(static_cast<char>(value));
}

QByteArray decompressBattlespireLzss(const QByteArray& input)
{
  std::array<quint8, 4096> window{};
  for (int i = 0; i < 4078; ++i) {
    window[static_cast<size_t>(i)] = 0x20;
  }
  for (int i = 4078; i < 4096; ++i) {
    window[static_cast<size_t>(i)] = 0x00;
  }

  int windowPos = 4078;
  int pos = 0;
  QByteArray out;

  while (pos < input.size()) {
    const quint8 marker = static_cast<quint8>(input.at(pos++));
    for (int bit = 0; bit < 8; ++bit) {
      const bool rawByte = ((marker >> bit) & 0x01) != 0;
      if (rawByte) {
        if (pos >= input.size()) {
          return out;
        }
        const quint8 value = static_cast<quint8>(input.at(pos++));
        lzssOutByte(out, window, windowPos, value);
      } else {
        if (pos + 1 >= input.size()) {
          return out;
        }
        const quint8 b0 = static_cast<quint8>(input.at(pos++));
        const quint8 b1 = static_cast<quint8>(input.at(pos++));
        const int offset = static_cast<int>(b0) | ((static_cast<int>(b1) & 0xF0) << 4);
        const int length = (static_cast<int>(b1) & 0x0F) + 3;
        for (int i = 0; i < length; ++i) {
          const quint8 value = window[static_cast<size_t>((offset + i) & 0x0FFF)];
          lzssOutByte(out, window, windowPos, value);
        }
      }
    }
  }

  return out;
}

}  // namespace

bool XngineBSAFormat::readArchive(const QString& filePath, Archive& outArchive,
                                  QString* errorMessage, const Traits& traits)
{
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return setError(errorMessage,
                    QString("Unable to open BSA file: %1").arg(filePath));
  }

  if (file.size() < 2) {
    return setError(errorMessage, "BSA file is too small to contain a header");
  }

  QDataStream stream(&file);
  stream.setByteOrder(QDataStream::LittleEndian);

  quint16 recordCount = 0;
  quint16 typeRaw = 0x0100;
  qint64 dataStartOffset = 4;
  stream >> recordCount;
  if (file.size() >= 4) {
    stream >> typeRaw;
  }

  IndexType type = static_cast<IndexType>(typeRaw);
  if (type != IndexType::NameRecord && type != IndexType::NumberRecord) {
    if (!traits.allowMissingTypeHeader) {
      return setError(errorMessage,
                      QString("Unsupported XnGine BSA type: 0x%1")
                          .arg(typeRaw, 4, 16, QChar('0')));
    }
    type = IndexType::NameRecord;
    dataStartOffset = 2;
  }

  const int descriptorSize = descriptorSizeForType(type);
  const qint64 footerSize = static_cast<qint64>(recordCount) * descriptorSize;
  const qint64 footerOffset = file.size() - footerSize;
  if (footerOffset < dataStartOffset) {
    return setError(errorMessage, "Invalid BSA layout: footer overlaps header");
  }

  struct Descriptor
  {
    QString name;
    quint16 id = 0;
    qint16 compressed = 0;
    qint32 size = 0;
  };
  QVector<Descriptor> descriptors;
  descriptors.reserve(recordCount);

  if (!file.seek(footerOffset)) {
    return setError(errorMessage, "Failed to seek to BSA footer");
  }

  for (quint16 i = 0; i < recordCount; ++i) {
    Descriptor descriptor;
    if (type == IndexType::NameRecord) {
      std::array<char, 12> rawName{};
      if (file.read(rawName.data(), static_cast<qint64>(rawName.size())) !=
          static_cast<qint64>(rawName.size())) {
        return setError(errorMessage, "Failed to read name descriptor");
      }

      int nullPos = 12;
      for (int c = 0; c < 12; ++c) {
        if (rawName[c] == '\0') {
          nullPos = c;
          break;
        }
      }
      descriptor.name = QString::fromLatin1(rawName.data(), nullPos);
      stream >> descriptor.compressed;
      stream >> descriptor.size;
    } else {
      stream >> descriptor.id;
      stream >> descriptor.compressed;
      stream >> descriptor.size;
    }

    if (descriptor.size < 0) {
      return setError(errorMessage, "Encountered negative record size");
    }
    if (descriptor.compressed != 0 && !traits.allowCompressed) {
      return setError(errorMessage,
                      "Compressed records are not supported for this game");
    }
    descriptors.push_back(descriptor);
  }

  outArchive.type = type;
  outArchive.variant = ArchiveVariant::Standard;
  outArchive.entries.clear();
  outArchive.entries.reserve(recordCount);

  qint64 recordOffset = dataStartOffset;
  for (int i = 0; i < descriptors.size(); ++i) {
    const auto& descriptor = descriptors.at(i);
    const qint64 recordSize = static_cast<qint64>(descriptor.size);
    if (recordOffset + recordSize > footerOffset) {
      return setError(errorMessage, "Record data exceeds footer boundary");
    }
    if (!file.seek(recordOffset)) {
      return setError(errorMessage, "Failed to seek to record data");
    }
    const QByteArray data = file.read(recordSize);
    if (data.size() != recordSize) {
      return setError(errorMessage, "Failed to read full record payload");
    }

    Entry entry;
    entry.compressed = descriptor.compressed;
    if (descriptor.compressed != 0) {
      if (!traits.allowCompressed) {
        return setError(errorMessage,
                        "Compressed records are not supported for this game");
      }
      if (traits.compressionMode == CompressionMode::BattlespireLzss) {
        entry.data = decompressBattlespireLzss(data);
      } else {
        return setError(errorMessage,
                        "Compressed record encountered but no decompressor is configured");
      }
    } else {
      entry.data = data;
    }
    if (type == IndexType::NameRecord) {
      entry.name = traits.normalizeNameCase ? descriptor.name.toUpper() : descriptor.name;
      if (traits.enforceDos83Names && !isDos83Name(entry.name)) {
        return setError(errorMessage,
                        QString("Record name is not DOS 8.3 compatible: %1")
                            .arg(entry.name));
      }
    } else {
      entry.recordId = descriptor.id;
    }

    outArchive.entries.push_back(entry);
    recordOffset += recordSize;
  }

  if (recordOffset != footerOffset) {
    return setError(errorMessage, "Record data area does not match footer offset");
  }

  if (traits.variantHint != ArchiveVariant::Standard) {
    outArchive.variant = traits.variantHint;
  } else {
    outArchive.variant = detectArchiveVariant(filePath, outArchive);
  }

  return true;
}

bool XngineBSAFormat::writeArchive(const QString& filePath, const Archive& archive,
                                   QString* errorMessage, const Traits& traits)
{
  if (archive.entries.size() > std::numeric_limits<quint16>::max()) {
    return setError(errorMessage, "BSA entry count exceeds UInt16 limit");
  }

  const ArchiveVariant variant =
      (traits.variantHint != ArchiveVariant::Standard) ? traits.variantHint : archive.variant;
  if (variant == ArchiveVariant::DaggerfallSnd || variant == ArchiveVariant::BattlespireSnd) {
    if (archive.type != IndexType::NumberRecord) {
      return setError(errorMessage, "SND BSA variants require NumberRecord index type");
    }
  }

  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return setError(errorMessage,
                    QString("Unable to create BSA file: %1").arg(filePath));
  }

  QDataStream stream(&file);
  stream.setByteOrder(QDataStream::LittleEndian);

  const quint16 recordCount = static_cast<quint16>(archive.entries.size());
  stream << recordCount;
  if (traits.writeTypeHeader) {
    stream << static_cast<quint16>(archive.type);
  }

  QVector<qint32> recordSizes;
  recordSizes.reserve(archive.entries.size());

  for (const auto& entry : archive.entries) {
    if (entry.compressed != 0) {
      if (!traits.allowCompressed) {
        return setError(errorMessage,
                        "Compressed records are not supported for this game");
      }
      if (!traits.allowCompressedPassthroughWrite) {
        return setError(errorMessage,
                        "Writing compressed records is not implemented for this game");
      }
    }
    if (entry.data.size() > std::numeric_limits<qint32>::max()) {
      return setError(errorMessage, "Record payload exceeds Int32 size limit");
    }
    recordSizes.push_back(static_cast<qint32>(entry.data.size()));
    if (file.write(entry.data) != entry.data.size()) {
      return setError(errorMessage, "Failed writing record payload");
    }
  }

  for (int i = 0; i < archive.entries.size(); ++i) {
    const auto& entry = archive.entries.at(i);
    const qint16 compressed = entry.compressed;
    const qint32 size = recordSizes.at(i);
    if (archive.type == IndexType::NameRecord) {
      const QString name =
          traits.normalizeNameCase ? entry.name.toUpper() : entry.name;
      if (traits.enforceDos83Names && !isDos83Name(name)) {
        return setError(errorMessage,
                        QString("Record name is not DOS 8.3 compatible: %1")
                            .arg(name));
      }

      const QByteArray nameBytes = name.toLatin1();
      if (nameBytes.size() > 12) {
        return setError(errorMessage,
                        QString("Record name too long for NameRecord: %1")
                            .arg(name));
      }

      std::array<char, 12> rawName{};
      memcpy(rawName.data(), nameBytes.constData(),
             static_cast<size_t>(nameBytes.size()));
      if (file.write(rawName.data(), static_cast<qint64>(rawName.size())) !=
          static_cast<qint64>(rawName.size())) {
        return setError(errorMessage, "Failed writing name descriptor");
      }
      stream << compressed;
      stream << size;
    } else {
      stream << entry.recordId;
      stream << compressed;
      stream << size;
    }
  }

  return true;
}

XngineBSAFormat::ArchiveVariant XngineBSAFormat::detectArchiveVariant(const QString& filePath,
                                                                      const Archive& archive)
{
  const QString base = QFileInfo(filePath).fileName();
  if (!base.endsWith(".SND", Qt::CaseInsensitive)) {
    return ArchiveVariant::Standard;
  }
  if (archive.type != IndexType::NumberRecord || archive.entries.isEmpty()) {
    return ArchiveVariant::Standard;
  }

  // Battlespire SPIRE.SND stores RIFF/WAVE payloads. Daggerfall DAGGER.SND stores raw PCM payloads.
  if (startsWithRiffWave(archive.entries.first().data)) {
    return ArchiveVariant::BattlespireSnd;
  }
  return ArchiveVariant::DaggerfallSnd;
}

bool XngineBSAFormat::unpackToDirectory(const QString& filePath,
                                        const QString& outputDirectory,
                                        QString* errorMessage,
                                        const Traits& traits)
{
  Archive archive;
  if (!readArchive(filePath, archive, errorMessage, traits)) {
    return false;
  }

  QDir outDir(outputDirectory);
  if (!outDir.exists() && !QDir().mkpath(outputDirectory)) {
    return setError(errorMessage,
                    QString("Could not create output directory: %1")
                        .arg(outputDirectory));
  }

  QFile manifestFile(outDir.filePath("xngine_bsa_manifest.tsv"));
  if (!manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
    return setError(errorMessage,
                    QString("Failed writing manifest file: %1")
                        .arg(manifestFile.fileName()));
  }
  QTextStream manifest(&manifestFile);
  manifest << "ordinal\ttype\tname\trecord_id\tcompressed\tsource_file\n";

  QSet<QString> usedNames;
  for (const auto& entry : archive.entries) {
    const int ordinal = &entry - archive.entries.constData();
    QString outName;
    if (archive.type == IndexType::NameRecord) {
      const QString baseName = sanitizeOutputName(entry.name);
      if (baseName.isEmpty()) {
        return setError(errorMessage, "Encountered empty record name");
      }
      outName = baseName;
      int suffix = 1;
      while (usedNames.contains(outName.toLower())) {
        outName = QString("%1__%2").arg(baseName).arg(suffix++);
      }
      usedNames.insert(outName.toLower());
    } else {
      outName = QString("%1_%2.bin")
                    .arg(entry.recordId, 5, 10, QChar('0'))
                    .arg(ordinal, 6, 10, QChar('0'));
      if (outName.isEmpty()) {
        return setError(errorMessage, "Encountered empty record name");
      }
    }

    QFile outFile(outDir.filePath(outName));
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      return setError(errorMessage,
                      QString("Failed writing extracted record: %1")
                          .arg(outFile.fileName()));
    }
    if (outFile.write(entry.data) != entry.data.size()) {
      return setError(errorMessage,
                      QString("Failed writing extracted payload: %1")
                          .arg(outFile.fileName()));
    }

    manifest << ordinal << '\t'
             << (archive.type == IndexType::NameRecord ? "name" : "number") << '\t'
             << entry.name << '\t'
             << (archive.type == IndexType::NumberRecord ? QString::number(entry.recordId)
                                                          : QString())
             << '\t' << entry.compressed << '\t' << outName << '\n';
  }

  return true;
}

bool XngineBSAFormat::packFromDirectory(const QString& inputDirectory,
                                        const QString& filePath, IndexType type,
                                        QString* errorMessage,
                                        const Traits& traits)
{
  QDir inDir(inputDirectory);
  if (!inDir.exists()) {
    return setError(errorMessage,
                    QString("Input directory does not exist: %1")
                        .arg(inputDirectory));
  }

  const QFileInfoList entries =
      inDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

  Archive archive;
  archive.type = type;
  archive.entries.reserve(entries.size());

  for (const auto& fileInfo : entries) {
    QFile inputFile(fileInfo.filePath());
    if (!inputFile.open(QIODevice::ReadOnly)) {
      return setError(errorMessage,
                      QString("Failed reading input file: %1")
                          .arg(fileInfo.filePath()));
    }

    Entry entry;
    entry.data = inputFile.readAll();
    if (type == IndexType::NameRecord) {
      entry.name = fileInfo.fileName();
      if (traits.normalizeNameCase) {
        entry.name = entry.name.toUpper();
      }
    } else {
      bool ok = false;
      const quint16 id =
          fileInfo.completeBaseName().toUShort(&ok, 10);
      if (!ok) {
        return setError(errorMessage,
                        QString("NumberRecord input filename must be a numeric ID: %1")
                            .arg(fileInfo.fileName()));
      }
      entry.recordId = id;
    }

    archive.entries.push_back(entry);
  }

  return writeArchive(filePath, archive, errorMessage, traits);
}

bool XngineBSAFormat::packFromManifestFile(const QString& inputDirectory,
                                           const QString& manifestFilePath,
                                           const QString& filePath, IndexType type,
                                           QString* errorMessage,
                                           const Traits& traits)
{
  QDir inDir(inputDirectory);
  if (!inDir.exists()) {
    return setError(errorMessage,
                    QString("Input directory does not exist: %1")
                        .arg(inputDirectory));
  }

  QFile manifestFile(manifestFilePath);
  if (!manifestFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return setError(errorMessage,
                    QString("Failed reading manifest file: %1")
                        .arg(manifestFilePath));
  }

  struct ManifestRow
  {
    QString name;
    quint16 recordId = 0;
    qint16 compressed = 0;
    QString sourceFile;
  };

  QVector<ManifestRow> rows;
  QTextStream in(&manifestFile);
  int lineNo = 0;
  while (!in.atEnd()) {
    const QString line = in.readLine();
    ++lineNo;
    if (line.trimmed().isEmpty() || line.startsWith('#')) {
      continue;
    }
    if (lineNo == 1 && line.startsWith("ordinal\t")) {
      continue;
    }

    const QStringList cols = line.split('\t');
    if (cols.size() < 6) {
      return setError(errorMessage,
                      QString("Invalid manifest format at line %1").arg(lineNo));
    }

    ManifestRow row;
    row.name = cols.at(2);
    if (type == IndexType::NumberRecord) {
      bool ok = false;
      const ushort id = cols.at(3).toUShort(&ok, 10);
      if (!ok) {
        return setError(errorMessage,
                        QString("Invalid record_id in manifest at line %1")
                            .arg(lineNo));
      }
      row.recordId = id;
    }

    bool compressedOk = false;
    const int compressed = cols.at(4).toInt(&compressedOk);
    if (!compressedOk || compressed < std::numeric_limits<qint16>::min() ||
        compressed > std::numeric_limits<qint16>::max()) {
      return setError(errorMessage,
                      QString("Invalid compressed field in manifest at line %1")
                          .arg(lineNo));
    }
    row.compressed = static_cast<qint16>(compressed);
    row.sourceFile = cols.at(5);
    if (row.sourceFile.isEmpty()) {
      return setError(errorMessage,
                      QString("Missing source_file in manifest at line %1")
                          .arg(lineNo));
    }
    rows.push_back(row);
  }

  Archive archive;
  archive.type = type;
  archive.entries.reserve(rows.size());

  for (const auto& row : rows) {
    QFile inputFile(inDir.filePath(row.sourceFile));
    if (!inputFile.open(QIODevice::ReadOnly)) {
      return setError(errorMessage,
                      QString("Failed reading input file from manifest: %1")
                          .arg(inputFile.fileName()));
    }

    Entry entry;
    entry.data = inputFile.readAll();
    entry.compressed = row.compressed;
    if (type == IndexType::NameRecord) {
      entry.name = row.name;
    } else {
      entry.recordId = row.recordId;
    }
    archive.entries.push_back(entry);
  }

  return writeArchive(filePath, archive, errorMessage, traits);
}
