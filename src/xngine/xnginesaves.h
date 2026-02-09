#ifndef XNGINE_SAVES_H
#define XNGINE_SAVES_H

#include <QDateTime>
#include <QDir>
#include <QRegularExpression>
#include <functional>
#include <optional>
#include <vector>

struct SaveLayout {
  std::vector<QString> baseRelativePaths;
  QRegularExpression slotDirRegex;
  int slotWidthHint = 1;
  std::optional<int> maxSlotHint;
  std::function<bool(const QDir&)> validator;
};

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
