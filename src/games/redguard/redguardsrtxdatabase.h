#ifndef REDGUARDSRTXDATABASE_H
#define REDGUARDSRTXDATABASE_H

#include <QString>
#include <QByteArray>
#include <QMap>
#include <memory>

/**
 * Represents an entry in the RTX database (dialogue/texture with optional audio).
 */
struct RedguardsRtxEntry
{
  QString label;
  QString subtitle;
  QByteArray audioBytes;
  int sampleRate = 11025;
  bool doubleSize = false;

  /**
   * Calculate the total length of this entry in the RTX file.
   */
  int length() const
  {
    int size = 6;  // label (4) + hasAudio flag (2)
    size += subtitle.length();
    if (!audioBytes.isEmpty()) {
      size += 27 + audioBytes.length();  // audio metadata + audio data
    }
    return size;
  }
};

/**
 * Represents the ENGLISH.RTX database file for Redguard.
 * Handles reading, modifying, and writing texture/dialogue data.
 * File format: entries with label, subtitle, optional audio, then reverse index.
 * Based on the Java RtxDatabase implementation from Redguard Mod Manager.
 */
class RedguardsRtxDatabase
{
public:
  /**
   * Creates an empty RedguardsRtxDatabase.
   */
  RedguardsRtxDatabase();

  /**
   * Creates a copy of another RedguardsRtxDatabase.
   */
  RedguardsRtxDatabase(const RedguardsRtxDatabase& other);

  /**
   * Reads the RTX database from disk.
   * @param filePath Path to the RTX file
   * @return true if successful, false otherwise
   */
  bool readFile(const QString& filePath);

  /**
   * Writes the RTX database to disk.
   * @param filePath Path to write to
   * @return true if successful, false otherwise
   */
  bool writeFile(const QString& filePath) const;

  /**
   * Reads and applies changes from a changes file.
   * Format: index\tlabel\tsubtitle (one per line)
   * @param changesFilePath Path to the changes file
   * @return true if successful, false otherwise
   */
  bool applyChanges(const QString& changesFilePath);

  /**
   * Gets an entry by label.
   */
  RedguardsRtxEntry* getEntry(const QString& label);

  /**
   * Gets an entry by index.
   */
  RedguardsRtxEntry* getEntry(int index);

  /**
   * Checks if an entry with the given label exists.
   */
  bool hasLabel(const QString& label) const { return mEntries.contains(label); }

  /**
   * Gets the number of entries.
   */
  int size() const { return mEntries.size(); }

private:
  QMap<QString, RedguardsRtxEntry> mEntries;  ///< Map of label -> entry

  // Utility functions for binary format parsing
  int readLittleEndianInt(const QByteArray& data, int offset) const;
  int readLittleEndianShort(const QByteArray& data, int offset) const;
  static void writeLittleEndianInt(QByteArray& data, int offset, int value);
  static void writeLittleEndianShort(QByteArray& data, int offset, short value);
  QString readString(const QByteArray& data, int offset, int length) const;
  static QByteArray writeString(const QString& str);
};

#endif  // REDGUARDSRTXDATABASE_H
