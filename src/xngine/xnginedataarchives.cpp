#include "xnginedataarchives.h"

#include <Windows.h>

#include <registry.h>
#include <utility.h>

#include "gamexngine.h"

XngineDataArchives::XngineDataArchives(const GameXngine* game) : m_Game{game} {}

QDir XngineDataArchives::gameDirectory() const
{
  return QDir(m_Game->gameDirectory()).absolutePath();
}

QDir XngineDataArchives::localGameDirectory() const
{
  return QDir(m_Game->myGamesPath()).absolutePath();
}

// Windows INI APIs return copied-character counts. A missing/empty key
// yields an empty list here; failures are not independently distinguished.
QStringList XngineDataArchives::getArchivesFromKey(const QString& iniFile,
                                                     const QString& key,
                                                     const int size) const
{
  wchar_t* buffer = new wchar_t[size];
  QStringList result;
  std::wstring iniFileW = QDir::toNativeSeparators(iniFile).toStdWString();

  // Historical errno reset is retained. The return value below reports
  // copied characters; this code does not inspect errno after the call.
  errno = 0;

  if (::GetPrivateProfileStringW(L"Archive", key.toStdWString().c_str(), L"", buffer,
                                 size, iniFileW.c_str()) != 0) {
    result.append(QString::fromStdWString(buffer).split(','));
  }

  for (int i = 0; i < result.count(); ++i) {
    result[i] = result[i].trimmed();
  }
  delete[] buffer;
  return result;
}

void XngineDataArchives::setArchivesToKey(const QString& iniFile, const QString& key,
                                            const QString& value)
{
  if (!MOBase::WriteRegistryValue(L"Archive", key.toStdWString().c_str(),
                                  value.toStdWString().c_str(),
                                  iniFile.toStdWString().c_str())) {
    qWarning("failed to set archives in \"%s\"", qUtf8Printable(iniFile));
  }
}

// Avoid case-insensitive duplicates while preserving the caller's chosen
// insertion position in the profile archive list.
void XngineDataArchives::addArchive(MOBase::IProfile* profile, int index,
                                      const QString& archiveName)
{
  QStringList current = archives(profile);
  if (current.contains(archiveName, Qt::CaseInsensitive)) {
    return;
  }

  current.insert(index != INT_MAX ? index : current.size(), archiveName);

  writeArchiveList(profile, current);
}

// Existing behavior is inconsistent: contains() is case-insensitive, but
// removeAll() below uses exact-case QString equality. Differing case can
// therefore survive this removal attempt.
void XngineDataArchives::removeArchive(MOBase::IProfile* profile,
                                         const QString& archiveName)
{
  QStringList current = archives(profile);
  if (!current.contains(archiveName, Qt::CaseInsensitive)) {
    return;
  }
  current.removeAll(archiveName);

  writeArchiveList(profile, current);
}
