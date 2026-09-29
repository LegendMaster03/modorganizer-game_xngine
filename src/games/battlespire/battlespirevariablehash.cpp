#include "battlespirevariablehash.h"

#include <QByteArray>

quint32 BattlespireVariableHash::hashName(QStringView name)
{
  const QByteArray bytes = name.toString().toLatin1();
  return hashAscii(std::string_view(bytes.constData(), static_cast<size_t>(bytes.size())));
}
