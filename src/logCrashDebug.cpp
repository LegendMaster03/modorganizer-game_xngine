// Crash debug log for Redguard MO2 plugin
// This file is written in the same style as the user's example crash log.
// All major function entries, variable values, and error-prone operations are logged step-by-step.
// Usage: Call logCrashDebug() at every critical step, especially in try/catch blocks and before/after file operations.

#include "RGMODFrameworkWrapper.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDateTime>

void RGMODFrameworkWrapper::logCrashDebug(const QString& message) const {
  QString logPath = QDir::toNativeSeparators("F:/Modding/MO2/redguard_crash_debug.log");
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
