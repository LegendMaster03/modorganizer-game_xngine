#include "daggerfallwoodswld.h"
#include <xnginewldformat.h>

#include <cstring>

bool DaggerfallWoodsWld::load(const QString& filePath, Data& outData,
                              QString* errorMessage)
{
  outData = {};
  XngineWldFormat::Document doc;
  XngineWldFormat::Traits traits;
  traits.variant = XngineWldFormat::Variant::DaggerfallWoods;
  if (!XngineWldFormat::readFile(filePath, doc, errorMessage, traits)) {
    return false;
  }
  if (doc.variant != XngineWldFormat::Variant::DaggerfallWoods) {
    if (errorMessage != nullptr) {
      *errorMessage = "Unexpected WLD variant parsed for WOODS.WLD";
    }
    return false;
  }

  const auto& src = doc.daggerfallWoods;
  auto& h = outData.header;
  h.offsetSize = src.offsetSize;
  h.width = src.width;
  h.height = src.height;
  h.nullValue1 = src.nullValue1;
  h.terrainTypesOffset = src.terrainTypesOffset;
  h.constant1 = src.constant1;
  h.constant2 = src.constant2;
  h.elevationMapOffset = src.elevationMapOffset;
  h.nullValue2 = src.nullValue2;

  outData.pixelOffsets = src.pixelOffsets;
  outData.elevationMap = src.elevationMap;
  outData.warning = doc.warning;

  outData.terrainTypes.reserve(src.terrainTypes.size());
  for (const auto& t : src.terrainTypes) {
    TerrainParameter dst;
    dst.clampHeight = t.clampHeight;
    dst.clampSensitivity = t.clampSensitivity;
    dst.unknown2 = t.unknown2;
    dst.unknown3 = t.unknown3;
    outData.terrainTypes.push_back(dst);
  }

  outData.pixelData.reserve(src.pixelData.size());
  for (const auto& sp : src.pixelData) {
    PixelData dp{};
    dp.unknown1 = sp.unknown1;
    dp.nullValue1 = sp.nullValue1;
    dp.fileIndex = sp.fileIndex;
    dp.terrainType = sp.terrainType;
    dp.terrainNoise = sp.terrainNoise;
    dp.nullValue2a = sp.nullValue2a;
    dp.nullValue2b = sp.nullValue2b;
    dp.nullValue2c = sp.nullValue2c;
    std::memcpy(dp.elevationNoise, sp.elevationNoise, sizeof(dp.elevationNoise));
    outData.pixelData.push_back(dp);
  }

  return true;
}

int DaggerfallWoodsWld::mapIndex(const Data& data, int x, int y)
{
  XngineWldFormat::DaggerfallWoodsData tmp;
  tmp.width = data.header.width;
  tmp.height = data.header.height;
  return XngineWldFormat::daggerfallMapIndex(tmp, x, y);
}
