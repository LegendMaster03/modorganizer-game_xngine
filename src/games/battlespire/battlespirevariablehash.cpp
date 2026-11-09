#include "battlespirevariablehash.h"

#include <QByteArray>

// Convert to the byte-oriented representation used by the hash routine.
// Non-Latin-1 characters are not represented as Unicode code points.
quint32 BattlespireVariableHash::hashName(QStringView name)
{
  const QByteArray bytes = name.toString().toLatin1();
  return hashAscii(std::string_view(bytes.constData(), static_cast<size_t>(bytes.size())));
}
