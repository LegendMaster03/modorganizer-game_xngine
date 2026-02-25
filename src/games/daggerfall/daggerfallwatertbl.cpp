#include "daggerfallwatertbl.h"

#include <xnginetblformat.h>

bool DaggerfallWaterTbl::load(const QString& filePath, Data& outData,
                              QString* errorMessage)
{
  outData = {};

  XngineTblFormat::Document doc;
  XngineTblFormat::Traits traits;
  traits.variant = XngineTblFormat::Variant::ByteLut256;
  if (!XngineTblFormat::readFile(filePath, doc, errorMessage, traits)) {
    return false;
  }

  outData.values = doc.byteLut256.values;
  return true;
}

bool DaggerfallWaterTbl::save(const QString& filePath, const Data& data,
                              QString* errorMessage)
{
  XngineTblFormat::Document doc;
  doc.variant = XngineTblFormat::Variant::ByteLut256;
  doc.byteLut256.values = data.values;

  XngineTblFormat::Traits traits;
  traits.variant = XngineTblFormat::Variant::ByteLut256;
  return XngineTblFormat::writeFile(filePath, doc, errorMessage, traits);
}
