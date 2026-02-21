#ifndef DAGGERFALL_BOOKTXT_H
#define DAGGERFALL_BOOKTXT_H

#include <QByteArray>
#include <QString>
#include <QVector>
#include <QtGlobal>

class DaggerfallBookTxt
{
public:
  struct Page
  {
    quint32 offset = 0;
    QByteArray rawBytes;
    QString text;
    QString warning;
  };

  struct Book
  {
    QString title;
    QString author;
    bool isNaughty = false;
    quint32 price = 0;
    quint16 rarity = 0;
    quint16 unknown2 = 0;
    quint16 unknown3 = 0;
    quint16 pageCount = 0;
    QVector<quint32> pageOffsets;
    QVector<Page> pages;
    QString warning;
  };

  static bool loadBook(const QString& filePath, Book& outBook,
                       QString* errorMessage = nullptr);
};

#endif  // DAGGERFALL_BOOKTXT_H
