#include "xnginearchiveextractorfeature.h"

#include "gamexngine.h"

#include <QFileInfo>
#include <QSet>

namespace
{

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

bool XngineArchiveExtractorFeature::supportsArchive(const QString& archivePath) const
{
  if (!m_Game) {
    return false;
  }

  const QFileInfo archiveInfo(archivePath);
  if (!archiveInfo.exists() || !archiveInfo.isFile()) {
    return false;
  }

  const QString fileName = archiveInfo.fileName();
  const auto spec = m_Game->bsaFileSpecForArchiveName(fileName);
  XngineBSAFormat::Archive archive;
  QString errorMessage;

  if (spec.has_value()) {
    auto traits = m_Game->bsaTraits();
    traits.variantHint = spec->archiveVariant;
    return XngineBSAFormat::readArchive(archivePath, archive, &errorMessage, traits);
  }

  return XngineBSAFormat::readArchive(archivePath, archive, &errorMessage,
                                      m_Game->bsaTraits());
}

bool XngineArchiveExtractorFeature::extractArchive(const QString& archivePath,
                                                   const QString& outputDirectory,
                                                   const ProgressCallback& progress,
                                                   QString* errorMessage) const
{
  Q_UNUSED(progress);

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

bool XngineArchiveExtractorFeature::canCreateArchive(const QString& archivePath) const
{
  return m_Game != nullptr &&
         m_Game->bsaFileSpecForArchiveName(QFileInfo(archivePath).fileName()).has_value();
}

bool XngineArchiveExtractorFeature::createArchive(const QString& sourceDirectory,
                                                  const QString& archivePath,
                                                  QString* errorMessage) const
{
  if (!canCreateArchive(archivePath)) {
    if (errorMessage != nullptr) {
      *errorMessage = QStringLiteral("Archive packing is not supported for %1")
                          .arg(QFileInfo(archivePath).fileName());
    }
    return false;
  }

  return m_Game->packKnownXngineBsaArchive(sourceDirectory, archivePath, errorMessage);
}
