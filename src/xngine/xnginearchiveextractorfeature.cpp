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

// File-extension filters help MO2 present likely archives. They are not
// the same as verifying a file's actual BSA structure.
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

// Recognition is based on parsing, not only the table of known basenames.
// A known file also receives its game-declared archive variant hint.
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

// Forward to the game-specific archive wrapper; progress callbacks are not
// yet reported by this implementation.
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

// This checks known filenames, not whether the spec has a known index type.
// packKnownXngineBsaArchive performs that stricter check when invoked.
bool XngineArchiveExtractorFeature::canCreateArchive(const QString& archivePath) const
{
  return m_Game != nullptr &&
         m_Game->bsaFileSpecForArchiveName(QFileInfo(archivePath).fileName()).has_value();
}

bool XngineArchiveExtractorFeature::createArchive(const QString& sourceDirectory,
                                                  const QString& archivePath,
                                                  const ProgressCallback& progress,
                                                  QString* errorMessage) const
{
  Q_UNUSED(progress);

  if (!canCreateArchive(archivePath)) {
    if (errorMessage != nullptr) {
      *errorMessage = QStringLiteral("Archive packing is not supported for %1")
                          .arg(QFileInfo(archivePath).fileName());
    }
    return false;
  }

  return m_Game->packKnownXngineBsaArchive(sourceDirectory, archivePath, errorMessage);
}
