#include "daggerfallworldlayers.h"
#include "daggerfallformatutils.h"

#include <QDir>

namespace {

using Daggerfall::FormatUtil::appendWarning;
using Daggerfall::FormatUtil::setError;

}  // namespace

bool DaggerfallWorldLayers::loadFromArena2(const QString& arena2Path,
                                           Data& outData,
                                           QString* errorMessage)
{
  outData = {};

  const QDir arena2(arena2Path);
  const QString woodsPath = arena2.filePath("WOODS.WLD");
  const QString climatePath = arena2.filePath("CLIMATE.PAK");
  const QString politicPath = arena2.filePath("POLITIC.PAK");

  QString err;
  if (!DaggerfallWoodsWld::load(woodsPath, outData.woods, &err)) {
    return setError(errorMessage, QString("WOODS.WLD: %1").arg(err));
  }
  if (!DaggerfallClimatePak::load(climatePath, outData.climate, &err)) {
    return setError(errorMessage, QString("CLIMATE.PAK: %1").arg(err));
  }
  if (!DaggerfallPoliticPak::load(politicPath, outData.politic, &err)) {
    return setError(errorMessage, QString("POLITIC.PAK: %1").arg(err));
  }

  if (outData.woods.header.width != outData.climate.width ||
      outData.woods.header.height != outData.climate.height) {
    appendWarning(outData.warning,
                  QString("WOODS (%1x%2) vs CLIMATE (%3x%4) size mismatch")
                      .arg(outData.woods.header.width)
                      .arg(outData.woods.header.height)
                      .arg(outData.climate.width)
                      .arg(outData.climate.height));
  }
  if (outData.woods.header.width != outData.politic.width ||
      outData.woods.header.height != outData.politic.height) {
    appendWarning(outData.warning,
                  QString("WOODS (%1x%2) vs POLITIC (%3x%4) size mismatch")
                      .arg(outData.woods.header.width)
                      .arg(outData.woods.header.height)
                      .arg(outData.politic.width)
                      .arg(outData.politic.height));
  }
  appendWarning(outData.warning, outData.woods.warning);
  appendWarning(outData.warning, outData.climate.warning);
  appendWarning(outData.warning, outData.politic.warning);

  return true;
}

DaggerfallWorldLayers::PixelInfo DaggerfallWorldLayers::pixelInfoAt(const Data& data,
                                                                    int x, int y)
{
  PixelInfo out;
  out.x = x;
  out.y = y;

  const int idx = DaggerfallWoodsWld::mapIndex(data.woods, x, y);
  if (idx < 0) {
    return out;
  }

  out.woodsIndex = idx;
  if (idx < data.woods.elevationMap.size()) {
    out.elevation = data.woods.elevationMap.at(idx);
  }
  if (idx < data.woods.pixelData.size()) {
    out.woodsPixelData = data.woods.pixelData.at(idx);
  }

  out.climateValue = DaggerfallClimatePak::valueAt(data.climate, x, y);
  out.climateMapping = DaggerfallClimatePak::mappingForValue(out.climateValue);

  out.politicValue = DaggerfallPoliticPak::valueAt(data.politic, x, y);
  out.regionIndex = DaggerfallPoliticPak::regionFromValue(out.politicValue);
  out.valid = true;
  return out;
}
