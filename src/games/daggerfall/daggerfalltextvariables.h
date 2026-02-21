#ifndef DAGGERFALL_TEXTVARIABLES_H
#define DAGGERFALL_TEXTVARIABLES_H

#include <QByteArrayView>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

class DaggerfallTextVariables
{
public:
  struct Entry
  {
    quint32 hash = 0;
    QString name;
  };

  // Daggerfall text-variable hash:
  // val = (val << 1) + byte
  static quint32 hashVariableName(QByteArrayView name);
  static quint32 hashVariableName(const QString& name);
  static quint32 hashVariableToken(const QString& token);  // accepts optional leading '%'

  static QVector<Entry> knownEntries();
  static QStringList namesForHash(quint32 hash);

  // Extracts %name style variables from rendered text.
  static QStringList extractVariables(const QString& text);
};

#endif  // DAGGERFALL_TEXTVARIABLES_H
