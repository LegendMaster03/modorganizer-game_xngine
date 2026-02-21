#include "daggerfallflatscfg.h"
#include "daggerfallformatutils.h"

#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <climits>

namespace {

bool parseUnsigned(const QString& text, quint32& outValue)
{
  bool ok = false;
  const qulonglong v = text.trimmed().toULongLong(&ok, 0);
  if (!ok || v > 0xFFFFFFFFull) {
    return false;
  }
  outValue = static_cast<quint32>(v);
  return true;
}

bool parseSigned(const QString& text, qint32& outValue)
{
  bool ok = false;
  const qlonglong v = text.trimmed().toLongLong(&ok, 0);
  if (!ok || v < static_cast<qlonglong>(INT32_MIN) ||
      v > static_cast<qlonglong>(INT32_MAX)) {
    return false;
  }
  outValue = static_cast<qint32>(v);
  return true;
}

bool parseTextureField(const QString& text, int& outFile, int& outRecordIndex)
{
  const QStringList parts = text.trimmed().split(QRegularExpression("\\s+"),
                                                 Qt::SkipEmptyParts);
  if (parts.size() < 2) {
    return false;
  }

  bool okFile = false;
  bool okIndex = false;
  const int textureFile = parts.at(0).toInt(&okFile, 0);
  const int textureIndex = parts.at(1).toInt(&okIndex, 0);
  if (!okFile || !okIndex || textureFile < 0 || textureIndex < 0) {
    return false;
  }

  outFile = textureFile;
  outRecordIndex = textureIndex;
  return true;
}

bool parseGenderField(const QString& text, DaggerfallFlatsCfg::Gender& outGender, bool& outObscene)
{
  QString value = text.trimmed();
  outObscene = false;

  if (value.startsWith('?')) {
    outObscene = true;
    value = value.mid(1).trimmed();
  }

  bool ok = false;
  const int n = value.toInt(&ok, 0);
  if (!ok) {
    return false;
  }

  switch (n) {
    case 1:
      outGender = DaggerfallFlatsCfg::Gender::Male;
      return true;
    case 2:
      outGender = DaggerfallFlatsCfg::Gender::Female;
      return true;
    default:
      outGender = DaggerfallFlatsCfg::Gender::Unknown;
      return false;
  }
}

}  // namespace

bool DaggerfallFlatsCfg::loadFile(const QString& filePath, File& outFile, QString* errorMessage)
{
  outFile = {};

  QFile f(filePath);
  if (!f.open(QIODevice::ReadOnly)) {
    return Daggerfall::FormatUtil::setError(errorMessage,
                                            QString("Unable to open FLATS.CFG: %1").arg(filePath));
  }

  const QString text = QString::fromLatin1(f.readAll());
  const QStringList allLines = text.split('\n');

  QVector<QString> currentRecordLines;
  currentRecordLines.reserve(FlatFieldCount);

  auto flushRecord = [&](bool isFinalFlush) -> bool {
    if (currentRecordLines.isEmpty()) {
      return true;
    }

    if (currentRecordLines.size() != FlatFieldCount) {
      Daggerfall::FormatUtil::appendWarning(
          outFile.warning, QString("Record %1 has %2 lines (expected %3)")
                               .arg(outFile.records.size())
                               .arg(currentRecordLines.size())
                               .arg(FlatFieldCount));
      currentRecordLines.clear();
      return true;
    }

    Flat rec;
    rec.recordIndex = outFile.records.size();
    rec.description = currentRecordLines.at(1).trimmed();

    if (!parseTextureField(currentRecordLines.at(0), rec.textureFile, rec.textureRecordIndex)) {
      return Daggerfall::FormatUtil::setError(
          errorMessage, QString("Invalid texture field in record %1: '%2'")
                            .arg(rec.recordIndex)
                            .arg(currentRecordLines.at(0).trimmed()));
    }

    if (!parseGenderField(currentRecordLines.at(2), rec.gender, rec.isObscene)) {
      Daggerfall::FormatUtil::appendWarning(
          rec.warning, QString("Unrecognized gender field '%1'").arg(currentRecordLines.at(2).trimmed()));
    }

    if (!parseUnsigned(currentRecordLines.at(3), rec.unknown1) ||
        !parseUnsigned(currentRecordLines.at(4), rec.unknown2)) {
      return Daggerfall::FormatUtil::setError(
          errorMessage, QString("Invalid unknown UInt32 fields in record %1").arg(rec.recordIndex));
    }

    if (!parseSigned(currentRecordLines.at(5), rec.faceIndex)) {
      return Daggerfall::FormatUtil::setError(
          errorMessage, QString("Invalid face index in record %1: '%2'")
                            .arg(rec.recordIndex)
                            .arg(currentRecordLines.at(5).trimmed()));
    }

    if (isFinalFlush) {
      Daggerfall::FormatUtil::appendWarning(rec.warning, "Record terminator missing at end-of-file");
    }

    if (!rec.warning.isEmpty()) {
      Daggerfall::FormatUtil::appendWarning(
          outFile.warning, QString("Record %1: %2").arg(rec.recordIndex).arg(rec.warning));
    }

    outFile.records.push_back(rec);
    currentRecordLines.clear();
    return true;
  };

  for (const QString& rawLine : allLines) {
    QString line = rawLine;
    if (line.endsWith('\r')) {
      line.chop(1);
    }

    if (line.trimmed().isEmpty()) {
      if (!flushRecord(false)) {
        return false;
      }
      continue;
    }

    currentRecordLines.push_back(line);
  }

  if (!flushRecord(true)) {
    return false;
  }

  if (outFile.records.isEmpty()) {
    return Daggerfall::FormatUtil::setError(errorMessage, "FLATS.CFG parsed but no records were decoded");
  }

  return true;
}

QString DaggerfallFlatsCfg::genderName(Gender gender)
{
  switch (gender) {
    case Gender::Male:
      return "Male";
    case Gender::Female:
      return "Female";
    default:
      return "Unknown";
  }
}
