#ifndef DAGGERFALL_WORLDLAYERS_H
#define DAGGERFALL_WORLDLAYERS_H

#include "daggerfallclimatepak.h"
#include "daggerfallpoliticpak.h"
#include "daggerfallwoodswld.h"

#include <QString>

class DaggerfallWorldLayers
{
public:
  struct PixelInfo
  {
    int x = -1;
    int y = -1;
    bool valid = false;

    int woodsIndex = -1;
    qint8 elevation = 0;
    DaggerfallWoodsWld::PixelData woodsPixelData;

    int climateValue = -1;
    DaggerfallClimatePak::TextureMapping climateMapping;

    int politicValue = -1;
    int regionIndex = -2;  // -1 water, -2 unknown/invalid
  };

  struct Data
  {
    DaggerfallWoodsWld::Data woods;
    DaggerfallClimatePak::Data climate;
    DaggerfallPoliticPak::Data politic;
    QString warning;
  };

  static bool loadFromArena2(const QString& arena2Path, Data& outData,
                             QString* errorMessage = nullptr);

  static PixelInfo pixelInfoAt(const Data& data, int x, int y);
};

#endif  // DAGGERFALL_WORLDLAYERS_H
