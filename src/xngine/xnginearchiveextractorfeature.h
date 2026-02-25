#ifndef XNGINEARCHIVEEXTRACTORFEATURE_H
#define XNGINEARCHIVEEXTRACTORFEATURE_H

#include "xnginearchiveextractor.h"

class GameXngine;

class XngineArchiveExtractorFeature : public XngineArchiveExtractor
{
public:
  explicit XngineArchiveExtractorFeature(const GameXngine* game);

  virtual QStringList supportedArchiveNameFilters() const override;
  virtual QStringList knownArchiveNames() const override;
  virtual QString describeArchive(const QString& archiveName) const override;
  virtual bool canExtractArchive(const QString& archivePath,
                                 QString* errorMessage = nullptr) const override;
  virtual bool extractArchive(const QString& archivePath, const QString& outputDirectory,
                              QString* errorMessage = nullptr) const override;

private:
  const GameXngine* m_Game;
};

#endif  // XNGINEARCHIVEEXTRACTORFEATURE_H
