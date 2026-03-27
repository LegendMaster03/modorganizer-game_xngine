#ifndef REDGUARDSPATCHUTILS_H
#define REDGUARDSPATCHUTILS_H

#include <QMap>
#include <QString>

class RedguardsMapChanges;

bool ensureDir(const QString& path);
bool removeDirRecursive(const QString& path);
bool copyDirectoryContents(const QString& sourceDir, const QString& destDir);

bool resolveBaseFilePath(const QString& tempModPath, const QString& gameDir,
                         const QString& fileName, QString& basePath,
                         QString& relativeSubdir);

QMap<QString, QMap<QString, QMap<QString, QString>>>
parseIniChanges(const QString& changesFilePath);

bool applyIniChangesToFile(const QString& iniFileName,
                           const QMap<QString, QMap<QString, QString>>& sectionChanges,
                           const QString& tempModPath,
                           const QString& gameDir);

bool applyIniChanges(const QString& modPath, const QString& tempModPath,
                     const QString& gameDir);

bool applyRtxChanges(const QString& modPath, const QString& tempModPath,
                     const QString& gameDir);

QString findSoupPath(const QString& gameDir);
QString findMapsRoot(const QString& gameDir);

bool applyMapChanges(const RedguardsMapChanges& mapChanges, const QString& tempModPath,
                     const QString& gameDir);

#endif  // REDGUARDSPATCHUTILS_H
