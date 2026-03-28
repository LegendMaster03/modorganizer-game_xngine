#ifndef DAGGERFALL_COMMON_H
#define DAGGERFALL_COMMON_H

#include <QString>
#include <QtGlobal>

namespace Daggerfall
{
namespace Angle
{
constexpr double Pi = 3.14159265358979323846;
constexpr double DaPerPi = 1024.0;
constexpr double DaPerRadian = DaPerPi / Pi;
constexpr double RadianPerDa = Pi / DaPerPi;
constexpr double DaPerDegree = 2048.0 / 360.0;

double daToRadians(qint16 da);
double daToDegrees(qint16 da);
qint16 radiansToDa(double radians);
qint16 degreesToDa(double degrees);
}  // namespace Angle

namespace Data
{
QString regionName(int regionIndex);
QString locationTypeName(int locationType);
int locationTypePaletteIndex(int locationType);
QString buildingTypeName(int buildingType);
QString townBuildingTypeName(int buildingTypeCode);
QString rulerTitleName(int rulerCode);
bool isTownBuildingTypeShownOnAutomap(int buildingTypeCode);
bool isLocationTypeKnownOnWorldMap(int locationType);
bool isLocationTypeUnknownOnWorldMap(int locationType);
bool isMapsaveArchiveName(const QString& archiveName);
bool isMapsaveRecordName(const QString& recordName);

QString rmbPrefixForBlockIndex(quint8 blockIndex);
QString rdbPrefixForBlockIndex(quint8 blockIndex);
}  // namespace Data
}  // namespace Daggerfall

#endif  // DAGGERFALL_COMMON_H
