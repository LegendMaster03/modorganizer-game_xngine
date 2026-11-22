# WDNM

## Status
Documented, under-modeled.

## Purpose
Walk-node map graph (`WalkMap -> WalkNode -> NodeRoute`).

## WalkMap Layout
| Offset | Type | Field |
|---|---|---|
| `+0x00` | `u32` | `MapLength` |
| `+0x04` | `u32` | `NodeCount1` |
| `+0x08` | `u32` | `NodeCount2` |
| `+0x0C` | `s24 slot` | `MapPosX` |
| `+0x10` | `s24 slot` | `MapPosY` |
| `+0x14` | `s24 slot` | `MapPosZ` |
| `+0x18` | `u32` | `Radius` |
| `+0x1C` | variable | `WalkNodes` |

## WalkNode / Route Layout
| Structure | Fields |
|---|---|
| `WalkNode` | `NodeLength (u32)`, `NodePosX (u16)`, `NodePosY (s16)`, `NodePosZ (u16)`, `Unknown06 (u8)`, `RouteCount (u8)` |
| `NodeRoute` | `TargetNodeId (s16)`, `Cost (s16)` |

Field-reference typing note:
- aggregate field map also records `NodeRoute` as `TargetNodeID (u16)`, `Cost (u16)`;
- runtime/parser docs commonly treat route values as signed (`s16`) in decode paths.

External invariant notes:
- `NodeCount2` is commonly equal to `NodeCount1` in sampled maps.
- `Unknown06` in `WalkNode` is usually zero, with sparse non-zero map/node-specific exceptions.

## Parser Notes
- Section starts with item-count field (`u32`) and iterates using `NodeCount1`.
- Each route list contains `RouteCount` pairs.
- External behavior notes emphasize explicit hierarchy (`WalkMap -> WalkNode -> NodeRoute`) with route `Cost` used for traversal weighting.

## Writer Notes
- Raw payload preserved during rewrite copy-window stage.

## Unknowns
- Canonical semantics of `NodeCount1` vs `NodeCount2`.
- Position/cost unit meaning.

## Related Docs
- `../RGM_BEHAVIOR_NOTES.md`
