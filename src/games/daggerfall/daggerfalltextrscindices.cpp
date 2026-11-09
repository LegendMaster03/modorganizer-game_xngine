#include "daggerfalltextrscindices.h"

// The category intervals are descriptive display labels for known text
// record ranges, not a schema that validates every TEXT.RSC record ID.
QVector<DaggerfallTextRscIndices::Range> DaggerfallTextRscIndices::knownRanges()
{
  return {
      {0, 7, "Attribute descriptions"},
      {100, 117, "Disease / poison status"},
      {200, 208, "Racial jokes / oaths"},
      {260, 299, "Merchant / banking dialogue"},
      {600, 612, "Guild join / eligibility"},
      {7206, 7294, "NPC dialogue core"},
      {7332, 7333, "Map/direction dialogue"},
      {8050, 8064, "Courtroom dialogue"},
      {8200, 8215, "Dungeon templates"},
      {8350, 8403, "Holiday messages"},
      {8700, 8722, "Artifact info"},
  };
}

// Unknown IDs deliberately have no category label; they are still
// legitimate candidates for raw TEXT.RSC record inspection.
QString DaggerfallTextRscIndices::labelForRecordId(quint16 id)
{
  const QVector<Range> ranges = knownRanges();
  for (const Range& r : ranges) {
    if (r.contains(id)) {
      return r.label;
    }
  }
  return {};
}
