#ifndef DAGGERFALL_FALLEXEHACKS_H
#define DAGGERFALL_FALLEXEHACKS_H

#include <QByteArray>
#include <QString>
#include <QVector>
#include <QtGlobal>

class DaggerfallFallExeHacks
{
public:
  struct BytePatch
  {
    qsizetype offset = -1;
    QByteArray replacement;  // bytes to write
    QByteArray expected;     // optional; empty = no pre-check
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

  // Deprecated: built-in hardcoded sets are intentionally empty.
  static QVector<PatchSet> documentedPatchSets();
  // Deprecated: use findPatchSetById() with user-loaded patch sets.
  static PatchSet patchSetById(const QString& id);
  static bool loadPatchSetsFromJsonFile(const QString& jsonPath, QVector<PatchSet>& outSets,
                                        QString* errorMessage = nullptr);
  static bool loadPatchSetsFromJsonBytes(const QByteArray& jsonData, QVector<PatchSet>& outSets,
                                         QString* errorMessage = nullptr);
  static bool validatePatchJsonFile(const QString& jsonPath, QString* errorMessage = nullptr);
  static bool validatePatchJsonBytes(const QByteArray& jsonData, QString* errorMessage = nullptr);
  static bool validatePatchSets(const QVector<PatchSet>& sets, QString* errorMessage = nullptr);
  static bool findPatchSetById(const QVector<PatchSet>& sets, const QString& id,
                               PatchSet& outSet);

  static bool verifyPatchSet(const QByteArray& exeData, const PatchSet& set,
                             QString* errorMessage = nullptr);
  static bool applyPatchSet(QByteArray& exeData, const PatchSet& set,
                            QString* errorMessage = nullptr);
};

#endif  // DAGGERFALL_FALLEXEHACKS_H
