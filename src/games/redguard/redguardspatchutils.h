#ifndef REDGUARDSPATCHUTILS_H
#define REDGUARDSPATCHUTILS_H

#include <QMap>
#include <QString>

class RedguardsMapChanges;

bool ensureDir(const QString& path);
bool removeDirRecursive(const QString& path);
bool copyDirectoryContents(const QString& sourceDir, const QString& destDir);

// Resolve already staged output before the installed original so later
// patch mods build on earlier results without modifying the base game.
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

// Apply all collected map changes to generated output. Earlier staging
// is not undone if one of the affected maps fails to write.
bool applyMapChanges(const RedguardsMapChanges& mapChanges, const QString& tempModPath,
                     const QString& gameDir);

#endif  // REDGUARDSPATCHUTILS_H
