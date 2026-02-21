#ifndef DAGGERFALL_FLATSCFG_H
#define DAGGERFALL_FLATSCFG_H

#include <QString>
#include <QVector>
#include <QtGlobal>

class DaggerfallFlatsCfg
{
public:
  static constexpr int FlatFieldCount = 6;

  enum class Gender : quint8
  {
    Unknown = 0,
    Male = 1,
    Female = 2
  };

  struct Flat
  {
    int recordIndex = -1;
    int textureFile = -1;         // TEXTURE.### extension
    int textureRecordIndex = -1;  // record index inside texture file
    QString description;
    Gender gender = Gender::Unknown;
    bool isObscene = false;  // '?' prefix on gender line
    quint32 unknown1 = 0;
    quint32 unknown2 = 0;
    qint32 faceIndex = 0;  // index in TFAC00I0.RCI
    QString warning;
  };

  struct File
  {
    QVector<Flat> records;
    QString warning;
  };

  static bool loadFile(const QString& filePath, File& outFile, QString* errorMessage = nullptr);
  static QString genderName(Gender gender);
};

#endif  // DAGGERFALL_FLATSCFG_H
