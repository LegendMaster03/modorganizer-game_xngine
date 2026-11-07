#ifndef XNGINE_EXE_PATCH_H
#define XNGINE_EXE_PATCH_H

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

namespace XngineExePatch
{
// Offsets address the current in-memory executable image. Expected bytes
// are optional; omitting them removes the source-byte compatibility check.
struct BytePatch
{struct BytePatch
{
  qsizetype offset = -1;
  QByteArray replacement;
  QByteArray expected;
  QString description;
};

struct PatchSet
{
  QString id;
  QString name;
  QString targetVersion;
  QVector<BytePatch> patches;
  QString notes;
};

bool loadPatchSetsFromJsonFile(const QString& jsonPath, QVector<PatchSet>& outSets,
                               QString* errorMessage = nullptr);
bool validatePatchSets(const QVector<PatchSet>& sets, QString* errorMessage = nullptr);
// Verifies each set before applying that set; earlier sets are not rolled
// back if a later set fails. The caller owns persistence of the result.
bool applyPatchSets(QByteArray& exeData,bool applyPatchSets(QByteArray& exeData, const QVector<PatchSet>& sets,
                    QString* errorMessage = nullptr);

QString findFirstExistingFile(const QString& rootPath, const QStringList& relativeCandidates);
QString findFirstMatchingFileRecursive(const QString& rootPath, const QStringList& fileNames);

QString findFirstTool(const QStringList& candidates);
// Resolves a configured path or searches mod/game/MO2/tool locations and
// environment/PATH fallbacks. Presence does not prove tool compatibility.
QString findXdeltaTool(const QString& modRootPath,QString findXdeltaTool(const QString& modRootPath, const QString& gameRootPath,
                       const QString& configuredPath = {});
bool applyXdeltaPatch(const QString& xdeltaTool, const QString& sourceExe,
                      const QString& patchFile, const QString& outputExe,
                      QString* errorMessage = nullptr);

// Binary patch tree helpers prefer already staged output over original
// game files. They emit the result under stagedRoot, not in baseRoot.
bool applyXdeltaPatchToAnyFileInTree(const QString& xdeltaTool,bool applyXdeltaPatchToAnyFileInTree(const QString& xdeltaTool, const QString& patchFile,
                                     const QString& baseRoot, const QString& stagedRoot,
                                     QString* matchedRelativePath = nullptr,
                                     QString* errorMessage = nullptr);

// IPS and PPF do not verify a mandatory source checksum in these helpers;
// an apparently applicable patch may target the wrong source file.
bool applyIpsPatch(const QString& sourceFile,bool applyIpsPatch(const QString& sourceFile, const QString& ipsPatchFile, const QString& outputFile,
                   QString* errorMessage = nullptr);
bool applyIpsPatchToAnyFileInTree(const QString& patchFile, const QString& baseRoot,
                                  const QString& stagedRoot,
                                  QString* matchedRelativePath = nullptr,
                                  QString* errorMessage = nullptr);

// BPS and UPS verify source, patch, and output CRC32 before committing an
// output file. The tree helpers also use filename heuristics for ordering.
bool applyBpsPatch(const QString& sourceFile,bool applyBpsPatch(const QString& sourceFile, const QString& bpsPatchFile, const QString& outputFile,
                   QString* errorMessage = nullptr);
bool applyBpsPatchToAnyFileInTree(const QString& patchFile, const QString& baseRoot,
                                  const QString& stagedRoot,
                                  QString* matchedRelativePath = nullptr,
                                  QString* errorMessage = nullptr);

bool applyUpsPatch(const QString& sourceFile, const QString& upsPatchFile, const QString& outputFile,
                   QString* errorMessage = nullptr);
bool applyUpsPatchToAnyFileInTree(const QString& patchFile, const QString& baseRoot,
                                  const QString& stagedRoot,
                                  QString* matchedRelativePath = nullptr,
                                  QString* errorMessage = nullptr);

bool applyPpfPatch(const QString& sourceFile, const QString& ppfPatchFile, const QString& outputFile,
                   QString* errorMessage = nullptr);
bool applyPpfPatchToAnyFileInTree(const QString& patchFile, const QString& baseRoot,
                                  const QString& stagedRoot,
                                  QString* matchedRelativePath = nullptr,
                                  QString* errorMessage = nullptr);
}  // namespace XngineExePatch

#endif  // XNGINE_EXE_PATCH_H
