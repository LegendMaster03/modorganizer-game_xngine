# <FORMAT> File Format (Redguard)

Last updated: YYYY-MM-DD

## Scope
What this file type is used for, where it appears, and major known variants.

## Large-Doc Rule
If a format spec grows too large to stay maintainable in one file, reorganize it to:
- `/<FORMAT>/<FORMAT>.md` (overview + relationships + status)
- `/<FORMAT>/blocks/<BLOCK>.md` (canonical per-block details)
- keep top-level `/<FORMAT>.md` as a compatibility redirect stub.

## Container / File Layout
Top-level structure in on-disk order.
- Include endianness rules here.
- Include terminator behavior (`END `, null-name terminator, etc).

## Section / Block Order
Canonical order (and optional/conditional blocks clearly marked).
Example:
`BLOCK1 BLOCK2 [OPTIONAL_BLOCK] ... END`

## Common Header (if applicable)
| Offset | Type | Name | Description |
|---|---|---|---|
| ... | ... | ... | ... |

## Blocks

### <BLOCK_NAME>
Purpose and role.

#### Header / Record Layout
| Offset | Type | Name | Description |
|---|---|---|---|
| ... | ... | ... | ... |

#### Notes
- Parsing constraints
- Special cases
- Known invariants

#### Unknowns
- Explicit unknown fields/semantics still unresolved.

---

### <NEXT_BLOCK_NAME>
(same structure)

## Cross-Block Relationships
- How offsets/counts in one block index data in another.
- Any one-to-one or one-to-many mappings.

## Discrepancies / Competing Theories
- Theory A:
- Theory B:
- Current evidence:
- Parser behavior recommendation (for now):

## Corpus Validation Notes
- What was validated in retail/GOG/corpus.
- What is observed but not yet fully explained.

## Implementation Status (Plugin)
- Implemented:
- Partial:
- Not implemented:

## Open Questions / RE Backlog
1. ...
2. ...

## References
- Internal docs used
- External projects / corpus notes
