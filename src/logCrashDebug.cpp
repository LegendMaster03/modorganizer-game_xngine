// Redguard MO2 diagnostic logger: append a timestamped message to a text file.
// The output path is currently a fixed development-machine path. A directory
// creation or file-open failure silently skips logging without affecting callers.

#include "RGMODFrameworkWrapper.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDateTime>

void RGMODFrameworkWrapper::logCrashDebug(const QString& message) const {
  QString logPath = QDir::toNativeSeparators("D:/Projects/modorganizer-game_xngine/redguard_crash_debug.log");
  QFileInfo logInfo(logPath);
  QDir logDir = logInfo.dir();
  if (!logDir.exists()) {
    logDir.mkpath(".");
  }
  QFile logFile(logPath);
  if (logFile.open(QIODevice::Append | QIODevice::Text)) {
    QTextStream out(&logFile);
    out << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz ") << message << "\n";
    out.flush();
    logFile.close();
  }
}
