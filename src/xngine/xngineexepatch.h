#ifndef XNGINE_EXE_PATCH_H
#define XNGINE_EXE_PATCH_H

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

namespace XngineExePatch
{
struct BytePatch
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
bool applyPatchSets(QByteArray& exeData, const QVector<PatchSet>& sets,
                    QString* errorMessage = nullptr);

QString findFirstExistingFile(const QString& rootPath, const QStringList& relativeCandidates);
QString findFirstMatchingFileRecursive(const QString& rootPath, const QStringList& fileNames);

QString findFirstTool(const QStringList& candidates);
QString findXdeltaTool(const QString& modRootPath, const QString& gameRootPath,
                       const QString& configuredPath = {});
bool applyXdeltaPatch(const QString& xdeltaTool, const QString& sourceExe,
                      const QString& patchFile, const QString& outputExe,
                      QString* errorMessage = nullptr);

bool applyXdeltaPatchToAnyFileInTree(const QString& xdeltaTool, const QString& patchFile,
                                     const QString& baseRoot, const QString& stagedRoot,
                                     QString* matchedRelativePath = nullptr,
                                     QString* errorMessage = nullptr);

bool applyIpsPatch(const QString& sourceFile, const QString& ipsPatchFile, const QString& outputFile,
                   QString* errorMessage = nullptr);
bool applyIpsPatchToAnyFileInTree(const QString& patchFile, const QString& baseRoot,
                                  const QString& stagedRoot,
                                  QString* matchedRelativePath = nullptr,
                                  QString* errorMessage = nullptr);

bool applyBpsPatch(const QString& sourceFile, const QString& bpsPatchFile, const QString& outputFile,
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
