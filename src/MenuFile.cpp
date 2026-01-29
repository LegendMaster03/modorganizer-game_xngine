#include "MenuFile.h"

#include <QFile>
#include <QMap>
#include <QTextStream>

MenuFile::MenuFile() = default;

MenuFile::MenuFile(const MenuFile& other) : mLines(other.mLines)
{
}

bool MenuFile::readFile(const QString& filePath)
{
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return false;
  }

  mLines.clear();
  QTextStream in(&file);
  while (!in.atEnd()) {
    mLines.append(in.readLine());
  }
  file.close();
  return true;
}

bool MenuFile::writeFile(const QString& filePath) const
{
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    return false;
  }

  QTextStream out(&file);
  for (const QString& line : mLines) {
    out << line << "\n";
  }
  file.close();
  return true;
}

bool MenuFile::readChanges(const QString& changesFilePath)
{
  QFile file(changesFilePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return false;
  }

  QMap<QString, QString> lineChanges;
  QTextStream in(&file);
  while (!in.atEnd()) {
    QString line = in.readLine();
    int eqPos = line.indexOf('=');
    if (eqPos != -1) {
      QString key = line.left(eqPos);
      QString value = line.mid(eqPos + 1);
      lineChanges[key] = value;
    }
  }
  file.close();

  applyChanges(lineChanges);
  return true;
}

void MenuFile::applyChanges(const QMap<QString, QString>& changes)
{
  for (int i = 0; i < mLines.size(); ++i) {
    const QString& line = mLines[i];
    int eqPos = line.indexOf('=');
    if (eqPos != -1) {
      QString key = line.left(eqPos);
      if (changes.contains(key)) {
        mLines[i] = key + "=" + changes[key];
      }
    }
  }
}
