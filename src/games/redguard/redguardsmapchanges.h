#ifndef REDGUARDSMAPCHANGES_H
#define REDGUARDSMAPCHANGES_H

#include <QString>
#include <QMap>
#include <QList>
#include <memory>

/**
 * Stores script line changes for map files under their map name and position.
 * Deletions are represented by a null string at position [0].
 * Insertions are the other elements, guaranteed to not be null.
 */
class RedguardsMapChanges
{
public:
  /**
   * Creates an empty RedguardsMapChanges container.
   */
  RedguardsMapChanges();

  /**
   * Gets the list of changes at a specific position in a map.
   * @param mapName The name of the map
   * @param pos The line position
   * @return Pointer to list of changes, or nullptr if no changes at that position
   */
  QList<QString>* lineChangesAt(const QString& mapName, int pos);

  /**
   * Checks if a map has any modifications.
   * @param mapName The name of the map
   * @return true if the map has changes, false otherwise
   */
  bool hasModifiedMap(const QString& mapName) const;

  /**
   * Adds multiple changes for a map.
   * @param mapName The name of the map
   * @param mapChanges Map of position -> list of changes
   */
  void addChanges(const QString& mapName, const QMap<int, QList<QString>>& mapChanges);

  /**
   * Adds a single change at a position.
   * Pass an empty string to represent a deletion.
   * @param mapName The name of the map
   * @param pos The line position
   * @param line The change (or empty string for deletion)
   */
  void addChange(const QString& mapName, int pos, const QString& line);

  /**
   * Reads changes from a changes file.
   * Format: map names on their own line, indented positions and changes below
   * @param changesFilePath Path to the changes file
   * @return true if successful, false otherwise
   */
  bool readChanges(const QString& changesFilePath);

  /**
   * Writes changes to a file.
   * @param filePath Path to write to
   * @return true if successful, false otherwise
   */
  bool writeChanges(const QString& filePath) const;

private:
  // Map of map name -> (position -> list of changes)
  QMap<QString, QMap<int, QList<QString>>> mLineChanges;
};

#endif  // REDGUARDSMAPCHANGES_H
