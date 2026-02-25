#include "xnginearchiveextractorfeature.h"

#include "gamexngine.h"

#include <QFileInfo>
#include <QSet>

namespace
{

QString normalizeArchiveName(const QString& pathOrName)
{
  return QFileInfo(pathOrName).fileName().toUpper();
}

QString suffixFilterForName(const QString& archiveName)
{
  const QString suffix = QFileInfo(archiveName).suffix().toLower();
  if (suffix.isEmpty()) {
    return {};
  }
  return QStringLiteral("*.") + suffix;
}

}  // namespace

XngineArchiveExtractorFeature::XngineArchiveExtractorFeature(const GameXngine* game)
  : m_Game(game)
{}

QStringList XngineArchiveExtractorFeature::supportedArchiveNameFilters() const
{
  QStringList filters;
  QSet<QString> dedupe;

  if (!m_Game) {
    return filters;
  }

  for (const auto& spec : m_Game->bsaFileSpecs()) {
    const QString filter = suffixFilterForName(spec.archiveName);
    if (!filter.isEmpty() && !dedupe.contains(filter)) {
      dedupe.insert(filter);
      filters.push_back(filter);
    }
  }

  return filters;
}

QStringList XngineArchiveExtractorFeature::knownArchiveNames() const
{
  QStringList names;

  if (!m_Game) {
    return names;
  }

  for (const auto& spec : m_Game->bsaFileSpecs()) {
    names.push_back(spec.archiveName);
  }

  return names;
}

QString XngineArchiveExtractorFeature::describeArchive(const QString& archiveName) const
{
  if (!m_Game) {
    return {};
  }

  const auto spec = m_Game->bsaFileSpecForArchiveName(normalizeArchiveName(archiveName));
  if (!spec.has_value()) {
    return {};
  }

  return spec->usage;
}

bool XngineArchiveExtractorFeature::canExtractArchive(const QString& archivePath,
                                                      QString* errorMessage) const
{
  if (!m_Game) {
    if (errorMessage != nullptr) {
      *errorMessage = QStringLiteral("XnGine archive extractor is not attached to a game");
    }
    return false;
  }

  const QFileInfo archiveInfo(archivePath);
  if (!archiveInfo.exists() || !archiveInfo.isFile()) {
    if (errorMessage != nullptr) {
      *errorMessage = QStringLiteral("Archive file does not exist: %1").arg(archivePath);
    }
    return false;
  }

  const QString fileName = archiveInfo.fileName();
  const auto spec = m_Game->bsaFileSpecForArchiveName(fileName);
  XngineBSAFormat::Archive archive;

  if (spec.has_value()) {
    auto traits = m_Game->bsaTraits();
    traits.variantHint = spec->archiveVariant;
    return XngineBSAFormat::readArchive(archivePath, archive, errorMessage, traits);
  }

  return XngineBSAFormat::readArchive(archivePath, archive, errorMessage, m_Game->bsaTraits());
}

bool XngineArchiveExtractorFeature::extractArchive(const QString& archivePath,
                                                   const QString& outputDirectory,
                                                   QString* errorMessage) const
{
  if (!m_Game) {
    if (errorMessage != nullptr) {
      *errorMessage = QStringLiteral("XnGine archive extractor is not attached to a game");
    }
    return false;
  }

  if (m_Game->bsaFileSpecForArchiveName(QFileInfo(archivePath).fileName()).has_value()) {
    return m_Game->unpackKnownXngineBsaArchive(archivePath, outputDirectory, errorMessage);
  }

  return m_Game->unpackXngineBsaArchive(archivePath, outputDirectory, errorMessage);
}
