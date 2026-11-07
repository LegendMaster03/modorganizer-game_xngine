#ifndef XNGINE_SAVES_H
#define XNGINE_SAVES_H

#include <QDateTime>
#include <QDir>
#include <QRegularExpression>
#include <functional>
#include <optional>
#include <vector>

// Game-specific declaration consumed by the shared MO2 save adapter.
// Relative paths are under SaveStoragePaths::gameSavesRoot. The active slot
// regex must capture the numeric slot ID in group 1. A validator is consulted
// for directory entries, not file entries.
struct SaveLayout {
  std::vector<QString> baseRelativePaths;
  QRegularExpression slotDirRegex;
  QRegularExpression slotFileRegex;
  bool slotEntriesAreFiles = false;
  int slotWidthHint = 1;
  // When set, ensureSaveDirsExist creates slot directories 0 through this
  // index (inclusive), using slotWidthHint for numeric padding.
  std::optional<int> maxSlotHint;
  std::function<bool(const QDir&)> validator;
  // True: makeSaveGame receives the matched slot path. False: the adapter
  // enumerates files within each slot, using the game-specific extension.
  bool oneSavePerSlot = true;
};

// absolutePath identifies the matched slot directory or file; its meaning
// depends on slotEntriesAreFiles and oneSavePerSlot in the declaring layout.
struct SaveSlot {
  int slotNumber;
  QString slotName;
  QString absolutePath;
  QDateTime lastModified;
};

struct SaveStoragePaths {
  QString profileRoot;
  QString savesRoot;      // <profile>/xngine/saves
  QString gameSavesRoot;  // <profile>/xngine/saves/<gameId>
};

#endif  // XNGINE_SAVES_H
