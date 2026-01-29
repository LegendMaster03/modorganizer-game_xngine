#include "RtxDatabase.h"

#include <QFile>
#include <QTextStream>
#include <QDataStream>
#include <QByteArray>
#include <QString>

RtxDatabase::RtxDatabase() = default;

RtxDatabase::RtxDatabase(const RtxDatabase& other) : mEntries(other.mEntries)
{
}

bool RtxDatabase::readFile(const QString& filePath)
{
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return false;
  }

  mEntries.clear();

  while (!file.atEnd()) {
    // Read 4-byte label
    QByteArray labelBytes(4, 0);
    if (file.read(labelBytes.data(), 4) != 4) {
      break;
    }
    QString label = QString::fromLatin1(labelBytes);

    if (label == "END ") {
      break;
    }

    // Read total length (4 bytes, BIG ENDIAN) - not needed, but must read to advance position
    QByteArray totalLengthBytes(4, 0);
    if (file.read(totalLengthBytes.data(), 4) != 4) {
      break;
    }

    // Read hasAudio flag (2 bytes, BIG ENDIAN)
    QByteArray hasAudioBytes(2, 0);
    if (file.read(hasAudioBytes.data(), 2) != 2) {
      break;
    }
    int hasAudioFlag = ((unsigned char)hasAudioBytes[0] << 8) | (unsigned char)hasAudioBytes[1];
    bool hasAudio = (hasAudioFlag == 1);

    // Read subtitle length (4 bytes) - little endian
    QByteArray subtitleLenBytes(4, 0);
    if (file.read(subtitleLenBytes.data(), 4) != 4) {
      break;
    }
    int subtitleLength = 0;
    for (int i = 0; i < 4; ++i) {
      subtitleLength |= ((unsigned char)subtitleLenBytes[i] << (8 * i));
    }

    // Sanity check on subtitle length
    if (subtitleLength < 0 || subtitleLength > 1000000) {
      // Invalid length, skip this entry
      break;
    }

    // Read subtitle
    QByteArray subtitleBytes(subtitleLength, 0);
    if (file.read(subtitleBytes.data(), subtitleLength) != subtitleLength) {
      break;
    }
    QString subtitle = QString::fromLatin1(subtitleBytes);

    RtxEntry entry;
    entry.label = label;
    entry.subtitle = subtitle;

    // Read audio if present
    if (hasAudio) {
      // Read doubleSize (4 bytes, little endian)
      QByteArray doubleSizeBytes(4, 0);
      if (file.read(doubleSizeBytes.data(), 4) != 4) break;
      int doubleSize = 0;
      for (int i = 0; i < 4; ++i) {
        doubleSize |= ((unsigned char)doubleSizeBytes[i] << (8 * i));
      }

      // Skip unused1 (4 bytes)
      QByteArray unused1(4, 0);
      if (file.read(unused1.data(), 4) != 4) break;

      // Read sampleRate (4 bytes, little endian)
      QByteArray sampleRateBytes(4, 0);
      if (file.read(sampleRateBytes.data(), 4) != 4) break;
      int sampleRate = 0;
      for (int i = 0; i < 4; ++i) {
        sampleRate |= ((unsigned char)sampleRateBytes[i] << (8 * i));
      }
      entry.sampleRate = sampleRate;

      // Skip unused2 (4 bytes)
      QByteArray unused2(4, 0);
      if (file.read(unused2.data(), 4) != 4) break;

      // Skip unused3 (2 bytes)
      QByteArray unused3(2, 0);
      if (file.read(unused3.data(), 2) != 2) break;

      // Skip unused4 (4 bytes)
      QByteArray unused4(4, 0);
      if (file.read(unused4.data(), 4) != 4) break;

      // Read audioLength (4 bytes, little endian)
      QByteArray audioLenBytes(4, 0);
      if (file.read(audioLenBytes.data(), 4) != 4) break;
      int audioLength = 0;
      for (int i = 0; i < 4; ++i) {
        audioLength |= ((unsigned char)audioLenBytes[i] << (8 * i));
      }

      // Sanity check on audio length
      if (audioLength < 0 || audioLength > 100000000) {
        break;
      }

      // Skip unused5 (1 byte)
      QByteArray unused5(1, 0);
      if (file.read(unused5.data(), 1) != 1) break;

      // Read audio data
      entry.audioBytes.resize(audioLength);
      if (file.read(entry.audioBytes.data(), audioLength) != audioLength) {
        break;
      }

      entry.doubleSize = (doubleSize == 1);
    }

    mEntries[label] = entry;
  }

  file.close();
  return !mEntries.isEmpty();
}

bool RtxDatabase::writeFile(const QString& filePath) const
{
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly)) {
    return false;
  }

  QByteArray output;
  int totalBytes = 0;
  QList<int> entryPositions;

  // Write all entries
  for (auto it = mEntries.begin(); it != mEntries.end(); ++it) {
    const RtxEntry& entry = it.value();

    // Write label (4 bytes)
    QByteArray labelBytes = writeString(entry.label);
    output.append(labelBytes);
    totalBytes += 4;

    // Write total length (4 bytes) - placeholder, will calculate
    int lengthPos = output.size();
    output.append(QByteArray(4, 0));
    totalBytes += 4;

    // Save position for reverse index
    entryPositions.append(totalBytes);

    // Write hasAudio flag (2 bytes)
    QByteArray audioFlag;
    if (entry.audioBytes.isEmpty()) {
      audioFlag.append((char)0);
      audioFlag.append((char)0);
    } else {
      audioFlag.append((char)0);
      audioFlag.append((char)1);
    }
    output.append(audioFlag);
    totalBytes += 2;

    // Write subtitle length (4 bytes)
    QByteArray subtitleLengthBytes(4, 0);
    int subLen = entry.subtitle.length();
    writeLittleEndianInt(subtitleLengthBytes, 0, subLen);
    output.append(subtitleLengthBytes);
    totalBytes += 4;

    // Write subtitle
    QByteArray subtitleBytes = writeString(entry.subtitle);
    output.append(subtitleBytes);
    totalBytes += subLen;

    // Write audio data if present
    if (!entry.audioBytes.isEmpty()) {
      QByteArray audioMetadata(27, 0);
      int doubleSize = entry.doubleSize ? 1 : 0;
      writeLittleEndianInt(audioMetadata, 0, doubleSize);
      writeLittleEndianInt(audioMetadata, 4, doubleSize);
      writeLittleEndianInt(audioMetadata, 8, entry.sampleRate);
      writeLittleEndianInt(audioMetadata, 12, 100);
      writeLittleEndianShort(audioMetadata, 16, 0);
      writeLittleEndianInt(audioMetadata, 18, -1);
      writeLittleEndianInt(audioMetadata, 22, entry.audioBytes.length());
      audioMetadata[26] = 0;

      output.append(audioMetadata);
      output.append(entry.audioBytes);
      totalBytes += 27 + entry.audioBytes.length();
    }

    // Update length field
    writeLittleEndianInt(output, lengthPos, entry.length());
  }

  // Write END marker
  output.append("END ");
  totalBytes += 4;

  // Write reverse index
  for (int i = mEntries.size() - 1; i >= 0; --i) {
    auto it = mEntries.begin();
    std::advance(it, i);
    const RtxEntry& entry = it.value();

    output.append(writeString(entry.label));
    QByteArray posBytes(4, 0);
    writeLittleEndianInt(posBytes, 0, entryPositions[i]);
    output.append(posBytes);

    QByteArray lenBytes(4, 0);
    writeLittleEndianInt(lenBytes, 0, entry.length());
    output.append(lenBytes);
  }

  // Write footer
  output.append("RNAV");
  QByteArray footerBytes(12, 0);
  writeLittleEndianInt(footerBytes, 0, totalBytes);
  writeLittleEndianInt(footerBytes, 4, mEntries.size());
  output.append(footerBytes);

  qint64 written = file.write(output);
  file.close();

  return written == output.size();
}

bool RtxDatabase::applyChanges(const QString& changesFilePath)
{
  QFile file(changesFilePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return false;
  }

  QTextStream in(&file);
  while (!in.atEnd()) {
    QString line = in.readLine();
    QStringList parts = line.split('\t');

    if (parts.size() >= 2) {
      bool ok = false;
      int index = parts[0].toInt(&ok);
      if (!ok) continue;

      QString label = parts[1];
      QString subtitle = parts.size() > 2 ? parts[2] : "";

      if (index >= mEntries.size()) {
        // New entry
        RtxEntry newEntry;
        newEntry.label = label;
        newEntry.subtitle = subtitle;
        mEntries[label] = newEntry;
      } else {
        // Update existing entry
        if (mEntries.contains(label)) {
          mEntries[label].subtitle = subtitle;
        }
      }
    }
  }

  file.close();
  return true;
}

RtxEntry* RtxDatabase::getEntry(const QString& label)
{
  if (mEntries.contains(label)) {
    return &mEntries[label];
  }
  return nullptr;
}

RtxEntry* RtxDatabase::getEntry(int index)
{
  if (index >= 0 && index < mEntries.size()) {
    auto it = mEntries.begin();
    std::advance(it, index);
    return &it.value();
  }
  return nullptr;
}

int RtxDatabase::readLittleEndianInt(const QByteArray& data, int offset) const
{
  if (offset + 4 > data.size()) return 0;
  int value = 0;
  for (int i = 0; i < 4; ++i) {
    value |= (static_cast<unsigned char>(data[offset + i]) << (8 * i));
  }
  return value;
}

int RtxDatabase::readLittleEndianShort(const QByteArray& data, int offset) const
{
  if (offset + 2 > data.size()) return 0;
  int value = 0;
  for (int i = 0; i < 2; ++i) {
    value |= (static_cast<unsigned char>(data[offset + i]) << (8 * i));
  }
  return value;
}

void RtxDatabase::writeLittleEndianInt(QByteArray& data, int offset, int value)
{
  if (offset + 4 > data.size()) {
    data.resize(offset + 4);
  }
  for (int i = 0; i < 4; ++i) {
    data[offset + i] = (value >> (8 * i)) & 0xFF;
  }
}

void RtxDatabase::writeLittleEndianShort(QByteArray& data, int offset, short value)
{
  if (offset + 2 > data.size()) {
    data.resize(offset + 2);
  }
  for (int i = 0; i < 2; ++i) {
    data[offset + i] = (value >> (8 * i)) & 0xFF;
  }
}

QString RtxDatabase::readString(const QByteArray& data, int offset, int length) const
{
  if (offset + length > data.size()) {
    return "";
  }
  // Use IBM437 encoding as per Java implementation
  return QString::fromLatin1(data.mid(offset, length));
}

QByteArray RtxDatabase::writeString(const QString& str)
{
  // Use IBM437 encoding as per Java implementation
  QByteArray bytes = str.toLatin1();
  // Ensure exact length, pad with spaces if needed
  if (bytes.length() < str.length()) {
    bytes.resize(str.length(), ' ');
  }
  return bytes;
}
