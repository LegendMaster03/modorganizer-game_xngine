#ifndef XNGINEARCHIVEEXTRACTORFEATURE_H
#define XNGINEARCHIVEEXTRACTORFEATURE_H

#include "xnginearchiveextractor.h"

class GameXngine;

// Delegates MO2 archive requests to GameXngine's per-game BSA traits and
// filename specs. canCreateArchive is not proof the index type is known.
class XngineArchiveExtractorFeature : public XngineArchiveExtractor
{
public:
  explicit XngineArchiveExtractorFeature(const GameXngine* game);

  virtual QStringList supportedArchiveNameFilters() const override;
  virtual bool supportsArchive(const QString& archivePath) const override;
  virtual bool extractArchive(const QString& archivePath, const QString& outputDirectory,
                              const ProgressCallback& progress = {},
                              QString* errorMessage            = nullptr) const override;
  virtual bool canCreateArchive(const QString& archivePath) const override;
  virtual bool createArchive(const QString& sourceDirectory, const QString& archivePath,
                             const ProgressCallback& progress = {},
                             QString* errorMessage            = nullptr) const override;

private:
  const GameXngine* m_Game;
};

#endif  // XNGINEARCHIVEEXTRACTORFEATURE_H
