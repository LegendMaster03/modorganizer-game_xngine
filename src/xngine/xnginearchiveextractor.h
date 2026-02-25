#ifndef XNGINEARCHIVEEXTRACTOR_H
#define XNGINEARCHIVEEXTRACTOR_H

#include <QString>
#include <QStringList>

#include <game_feature.h>

class XngineBSAFormat;

class XngineArchiveExtractor : public MOBase::details::GameFeatureCRTP<XngineArchiveExtractor>
{
public:
  virtual ~XngineArchiveExtractor() = default;

  // Glob-style filters (for example "*.bsa") that a tool can use when scanning mods.
  virtual QStringList supportedArchiveNameFilters() const = 0;

  // Canonical archive file names the current game knows about (for example "ARCH3D.BSA").
  virtual QStringList knownArchiveNames() const = 0;

  // Optional usage hint for UI display.
  virtual QString describeArchive(const QString& archiveName) const = 0;

  // Validate whether the file looks like an extractable XnGine archive for this game.
  virtual bool canExtractArchive(const QString& archivePath,
                                 QString* errorMessage = nullptr) const = 0;

  // Extract archive contents to a directory. Implementations should create directories as needed.
  virtual bool extractArchive(const QString& archivePath, const QString& outputDirectory,
                              QString* errorMessage = nullptr) const = 0;
};

#endif  // XNGINEARCHIVEEXTRACTOR_H
