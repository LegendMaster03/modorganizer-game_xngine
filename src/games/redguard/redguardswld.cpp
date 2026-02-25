#include "redguardswld.h"

#include <xnginewldformat.h>

#include <QFileInfo>

namespace {

XngineWldFormat::Traits redguardTraits()
{
  XngineWldFormat::Traits traits;
  traits.variant = XngineWldFormat::Variant::Redguard;
  return traits;
}

}  // namespace

bool RedguardsWld::load(const QString& filePath, Data& outData,
                        QString* errorMessage)
{
  outData = {};

  XngineWldFormat::Document doc;
  if (!XngineWldFormat::readFile(filePath, doc, errorMessage, redguardTraits())) {
    return false;
  }
  if (doc.variant != XngineWldFormat::Variant::Redguard) {
    if (errorMessage != nullptr) {
      *errorMessage = "Unexpected WLD variant parsed for Redguard";
    }
    return false;
  }

  const auto& src = doc.redguard;
  outData.headerDwords = src.headerDwords;
  outData.footerDwords = src.footerDwords;
  outData.warning      = doc.warning;

  outData.sections.reserve(src.sections.size());
  for (const auto& s : src.sections) {
    RedguardsWld::Data::Section dst;
    dst.headerWords = s.headerWords;
    dst.map1        = s.map1;
    dst.map2        = s.map2;
    dst.map3        = s.map3;
    dst.map4        = s.map4;
    outData.sections.push_back(dst);
  }

  return true;
}

bool RedguardsWld::save(const QString& filePath, const Data& data,
                        QString* errorMessage)
{
  XngineWldFormat::Document doc;
  doc.variant = XngineWldFormat::Variant::Redguard;
  doc.warning = data.warning;

  auto& dst = doc.redguard;
  dst.headerDwords = data.headerDwords;
  dst.footerDwords = data.footerDwords;
  dst.sections.reserve(data.sections.size());

  for (const auto& s : data.sections) {
    XngineWldFormat::RedguardSection outSection;
    outSection.headerWords = s.headerWords;
    outSection.map1        = s.map1;
    outSection.map2        = s.map2;
    outSection.map3        = s.map3;
    outSection.map4        = s.map4;
    dst.sections.push_back(outSection);
  }

  return XngineWldFormat::writeFile(filePath, doc, errorMessage, redguardTraits());
}

QVector<quint8> RedguardsWld::combinedMap(const Data& data, int planeIndex)
{
  XngineWldFormat::RedguardData core;
  core.headerDwords = data.headerDwords;
  core.footerDwords = data.footerDwords;
  core.sections.reserve(data.sections.size());

  for (const auto& s : data.sections) {
    XngineWldFormat::RedguardSection sec;
    sec.headerWords = s.headerWords;
    sec.map1        = s.map1;
    sec.map2        = s.map2;
    sec.map3        = s.map3;
    sec.map4        = s.map4;
    core.sections.push_back(sec);
  }

  return XngineWldFormat::redguardCombinedMap(core, planeIndex);
}

bool RedguardsWld::looksLikeRedguardWldName(const QString& fileName)
{
  static const QStringList knownNames = {
      QStringLiteral("EXTPALAC.WLD"),
      QStringLiteral("HIDEOUT.WLD"),
      QStringLiteral("ISLAND.WLD"),
      QStringLiteral("NECRISLE.WLD"),
  };

  const QString base = QFileInfo(fileName).fileName().toUpper();
  if (knownNames.contains(base)) {
    return true;
  }

  return base.endsWith(QStringLiteral(".WLD"));
}
