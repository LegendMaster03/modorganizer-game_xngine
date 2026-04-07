#ifndef XNGINEARCHIVEEXTRACTOR_H
#define XNGINEARCHIVEEXTRACTOR_H

#include <uibase/game_features/gamearchivehandler.h>

class XngineArchiveExtractor : public MOBase::GameArchiveHandler
{
public:
  virtual ~XngineArchiveExtractor() = default;

  virtual QStringList supportedArchiveNameFilters() const = 0;

  virtual bool extractArchive(const QString& archivePath, const QString& outputDirectory,
                              const ProgressCallback& progress = {},
                              QString* errorMessage            = nullptr) const = 0;

  virtual bool canCreateArchive(const QString& archivePath) const = 0;

  virtual bool createArchive(const QString& sourceDirectory, const QString& archivePath,
                             const ProgressCallback& progress = {},
                             QString* errorMessage            = nullptr) const = 0;
};

#endif  // XNGINEARCHIVEEXTRACTOR_H
