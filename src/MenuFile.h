#ifndef MENUFILE_H
#define MENUFILE_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <memory>

/**
 * Represents the MENU.INI file and handles applying changes to it.
 * Changes are stored as key=value pairs and applied to matching lines in the file.
 */
class MenuFile
{
public:
  /**
   * Creates an empty MenuFile.
   */
  MenuFile();

  /**
   * Creates a copy of another MenuFile.
   */
  MenuFile(const MenuFile& other);

  /**
   * Reads the menu file from disk.
   * @param filePath Path to the file to read
   * @return true if successful, false otherwise
   */
  bool readFile(const QString& filePath);

  /**
   * Writes the menu file to disk.
   * @param filePath Path to write to
   * @return true if successful, false otherwise
   */
  bool writeFile(const QString& filePath) const;

  /**
   * Reads changes from a changes file and applies them to this menu.
   * Changes file format: key=newvalue (one per line)
   * @param changesFilePath Path to the changes file
   * @return true if successful, false otherwise
   */
  bool readChanges(const QString& changesFilePath);

  /**
   * Applies changes from a changes map.
   * @param changes Map of key -> new value
   */
  void applyChanges(const QMap<QString, QString>& changes);

private:
  QStringList mLines;  ///< Lines of the file
};

#endif // MENUFILE_H
