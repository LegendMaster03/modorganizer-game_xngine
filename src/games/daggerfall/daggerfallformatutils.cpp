#include "daggerfallformatutils.h"

#include <QtEndian>

#include <cstring>

namespace Daggerfall
{
namespace FormatUtil
{
bool setError(QString* errorMessage, const QString& text)
{
  if (errorMessage != nullptr) {
    *errorMessage = text;
  }
  return false;
}

void appendWarning(QString& warning, const QString& text)
{
  if (text.isEmpty()) {
    return;
  }
  if (!warning.isEmpty()) {
    warning.append("; ");
  }
  warning.append(text);
}

bool readU8(const QByteArray& data, qsizetype off, quint8& value)
{
  if (off < 0 || off >= data.size()) {
    return false;
  }
  value = static_cast<quint8>(data.at(off));
  return true;
}

bool readLE16(const QByteArray& data, qsizetype off, qint16& value)
{
  if (off < 0 || off + 2 > data.size()) {
    return false;
  }
  qint16 raw = 0;
  std::memcpy(&raw, data.constData() + off, sizeof(raw));
  value = qFromLittleEndian(raw);
  return true;
}

bool readLE16U(const QByteArray& data, qsizetype off, quint16& value)
{
  if (off < 0 || off + 2 > data.size()) {
    return false;
  }
  quint16 raw = 0;
  std::memcpy(&raw, data.constData() + off, sizeof(raw));
  value = qFromLittleEndian(raw);
  return true;
}

bool readLE32(const QByteArray& data, qsizetype off, qint32& value)
{
  if (off < 0 || off + 4 > data.size()) {
    return false;
  }
  qint32 raw = 0;
  std::memcpy(&raw, data.constData() + off, sizeof(raw));
  value = qFromLittleEndian(raw);
  return true;
}

bool readLE32U(const QByteArray& data, qsizetype off, quint32& value)
{
  if (off < 0 || off + 4 > data.size()) {
    return false;
  }
  quint32 raw = 0;
  std::memcpy(&raw, data.constData() + off, sizeof(raw));
  value = qFromLittleEndian(raw);
  return true;
}

QString readFixedString(const QByteArray& data, qsizetype off, qsizetype len, bool trim)
{
  if (off < 0 || len <= 0 || off + len > data.size()) {
    return {};
  }

  QByteArray raw = data.mid(off, len);
  const int nullPos = raw.indexOf('\0');
  if (nullPos >= 0) {
    raw.truncate(nullPos);
  }

  QString out = QString::fromLatin1(raw);
  if (trim) {
    out = out.trimmed();
  }
  return out;
}
}  // namespace FormatUtil
}  // namespace Daggerfall
