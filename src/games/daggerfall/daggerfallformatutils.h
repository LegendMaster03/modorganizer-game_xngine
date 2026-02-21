#ifndef DAGGERFALL_FORMATUTILS_H
#define DAGGERFALL_FORMATUTILS_H

#include <QByteArray>
#include <QString>
#include <QtGlobal>

namespace Daggerfall
{
namespace FormatUtil
{
bool setError(QString* errorMessage, const QString& text);
void appendWarning(QString& warning, const QString& text);

bool readU8(const QByteArray& data, qsizetype off, quint8& value);
bool readLE16(const QByteArray& data, qsizetype off, qint16& value);
bool readLE16U(const QByteArray& data, qsizetype off, quint16& value);
bool readLE32(const QByteArray& data, qsizetype off, qint32& value);
bool readLE32U(const QByteArray& data, qsizetype off, quint32& value);

QString readFixedString(const QByteArray& data, qsizetype off, qsizetype len, bool trim = true);
}  // namespace FormatUtil
}  // namespace Daggerfall

#endif  // DAGGERFALL_FORMATUTILS_H
