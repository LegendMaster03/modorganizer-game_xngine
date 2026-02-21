#ifndef DAGGERFALL_FALLEXEITEMS_H
#define DAGGERFALL_FALLEXEITEMS_H

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QVector>
#include <QtGlobal>

class DaggerfallFallExeItems
{
public:
  static constexpr qsizetype RecordSize = 0x30;
  static constexpr qsizetype NameSize = 24;

  struct ItemRecord
  {
    qsizetype offset = -1;
    QString name;
    qint32 weightQuarterKg = 0;
    qint16 hitPoints = 0;
    qint32 unknown1 = 0;
    qint32 value = 0;
    qint16 enchantmentPoints = 0;
    quint8 unknown28 = 0;
    quint8 unknown29 = 0;
    quint8 unknown2A = 0;
    quint8 unknown2B = 0;
    quint8 pictureLo = 0;
    quint8 pictureHi = 0;
    quint8 unknown2E = 0;
    quint8 unknown2F = 0;

    quint16 pictureCode() const { return static_cast<quint16>(pictureLo) |
                                         (static_cast<quint16>(pictureHi) << 8); }
  };

  struct ItemTable
  {
    qsizetype startOffset = -1;  // location of the first record ("Ruby")
    QVector<ItemRecord> records;
    QString warning;
  };

  static bool loadFromExe(const QString& exePath, ItemTable& outTable,
                          QString* errorMessage = nullptr);
  static bool decodeFromBytes(const QByteArray& exeData, ItemTable& outTable,
                              QString* errorMessage = nullptr);

  static QHash<quint16, QString> knownPictureCodes();
  static QString pictureDescription(quint16 code);
};

#endif  // DAGGERFALL_FALLEXEITEMS_H
