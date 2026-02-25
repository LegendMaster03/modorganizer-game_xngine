#include "xnginetblformat.h"

#include <QByteArray>
#include <QFile>
#include <QFileInfo>

namespace {

bool setError(QString* errorMessage, const QString& error)
{
  if (errorMessage != nullptr) {
    *errorMessage = error;
  }
  return false;
}

XngineTblFormat::Variant detectVariant(const QByteArray& bytes)
{
  if (bytes.size() == 256) {
    return XngineTblFormat::Variant::ByteLut256;
  }
  return XngineTblFormat::Variant::Auto;
}

}  // namespace

bool XngineTblFormat::parseByteLut256(const QByteArray& bytes, ByteLut256& outTable,
                                      QString* errorMessage)
{
  outTable = {};
  if (bytes.size() != 256) {
    return setError(errorMessage, QString("ByteLut256 requires exactly 256 bytes, got %1")
                                      .arg(bytes.size()));
  }

  outTable.values.resize(256);
  for (int i = 0; i < 256; ++i) {
    outTable.values[i] = static_cast<quint8>(bytes.at(i));
  }

  return true;
}

bool XngineTblFormat::writeByteLut256(const ByteLut256& table, QByteArray& outBytes,
                                      QString* errorMessage)
{
  if (table.values.size() != 256) {
    return setError(errorMessage, QString("ByteLut256 write requires 256 values, got %1")
                                      .arg(table.values.size()));
  }

  outBytes.clear();
  outBytes.reserve(256);
  for (quint8 v : table.values) {
    outBytes.append(static_cast<char>(v));
  }
  return true;
}

bool XngineTblFormat::readFile(const QString& filePath, Document& outDocument,
                               QString* errorMessage, const Traits& traits)
{
  outDocument = {};

  QFile f(filePath);
  if (!f.open(QIODevice::ReadOnly)) {
    return setError(errorMessage, QString("Unable to open TBL file: %1").arg(filePath));
  }
  const QByteArray bytes = f.readAll();

  Variant variant = traits.variant;
  if (variant == Variant::Auto) {
    variant = detectVariant(bytes);
    if (variant == Variant::Auto) {
      return setError(errorMessage, QString("Unable to detect TBL variant for %1")
                                        .arg(QFileInfo(filePath).fileName()));
    }
  }

  switch (variant) {
    case Variant::ByteLut256:
      outDocument.variant = variant;
      return parseByteLut256(bytes, outDocument.byteLut256, errorMessage);
    case Variant::Auto:
    default:
      return setError(errorMessage, "Unsupported/unknown TBL variant");
  }
}

bool XngineTblFormat::writeFile(const QString& filePath, const Document& document,
                                QString* errorMessage, const Traits& traits)
{
  Variant variant = (traits.variant == Variant::Auto) ? document.variant : traits.variant;

  QByteArray outBytes;
  switch (variant) {
    case Variant::ByteLut256:
      if (!writeByteLut256(document.byteLut256, outBytes, errorMessage)) {
        return false;
      }
      break;
    case Variant::Auto:
    default:
      return setError(errorMessage, "TBL write: unspecified variant");
  }

  QFile f(filePath);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return setError(errorMessage, QString("Unable to write TBL file: %1").arg(filePath));
  }
  if (f.write(outBytes) != outBytes.size()) {
    return setError(errorMessage, "Failed writing TBL bytes");
  }

  return true;
}
