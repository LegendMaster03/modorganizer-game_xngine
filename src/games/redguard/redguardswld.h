#ifndef REDGUARDSWLD_H
#define REDGUARDSWLD_H

#include <QString>
#include <QVector>
#include <QtGlobal>

class XngineWldFormat;

class RedguardsWld
{
public:
  struct Data
  {
    QVector<quint32> headerDwords;
    QVector<quint32> footerDwords;

    struct Section
    {
      QVector<quint16> headerWords;
      QVector<quint8> map1;  // height/flags
      QVector<quint8> map2;  // unknown (usually zero)
      QVector<quint8> map3;  // texture index + rotation
      QVector<quint8> map4;  // unknown (usually zero)
    };

    QVector<Section> sections;
    QString warning;
  };

  static bool load(const QString& filePath, Data& outData,
                   QString* errorMessage = nullptr);

  static bool save(const QString& filePath, const Data& data,
                   QString* errorMessage = nullptr);

  // Combines one map plane from the 4 WLD sections into a single 256x256 map.
  // planeIndex: 0=Map1, 1=Map2, 2=Map3, 3=Map4
  static QVector<quint8> combinedMap(const Data& data, int planeIndex);

  static bool looksLikeRedguardWldName(const QString& fileName);
};

#endif  // REDGUARDSWLD_H
