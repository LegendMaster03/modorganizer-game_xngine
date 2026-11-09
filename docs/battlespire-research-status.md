# Battlespire–Daggerfall lineage research

## Purpose and status

This branch is an **experimental research branch**, not a set of verified fixes.
Its initial motivation is the hypothesis that Battlespire's original development
as a Daggerfall expansion may expose useful connections between their file
formats, record layouts, or engine behavior. It also contains other research
implementations that have not yet been validated against the original game.

Shared ancestry is a reason to investigate a correspondence, **not evidence
that the two games use identical structures**. An implementation can correctly
follow its own assumptions without those assumptions being historically or
technically accurate for Battlespire.

## Areas under investigation

- Archive and model layout variations, including candidate Battlespire BSA
  descriptors and ten-byte v2.7 face headers.
- BS6 level chunks and BSI image/palette structures.
- SAVETREE and SAVEVARS offsets, record relationships, and cross-file joins.
- Name hashing and interpretation of records whose meaning is incomplete.
- Explicitly isolated Daggerfall-derived TEXT.RSC and WATER.TBL adapters whose
  applicability to Battlespire has not been established.

This list describes **what the code investigates**; it does not certify that
each interpretation matches data produced by the original game. A specific
research provenance should be recorded when the source material becomes
available, rather than inferred from the mere presence of an implementation.

## Evidence levels

1. **Implemented hypothesis:** The code models a proposed interpretation.
   Until tested, its format or semantic assumptions remain provisional.
2. **Synthetic fixture result:** A unit test can show that the implementation
   handles bytes constructed according to its proposed interpretation. This
   is useful regression protection, not independent evidence for the format.
3. **Original-game validation:** Representative real assets or saves, with
   documented origins, support or falsify the interpretation. Record observed
   exceptions and version differences instead of generalizing prematurely.
4. **Runtime validation:** When necessary, test the resulting behavior in the
   game or Mod Organizer 2 to confirm that the integration works beyond parsing.

Do not promote a hypothesis to confirmed behavior simply because a parser
builds, a synthetic test passes, or a Daggerfall structure appears analogous.

## Documentation conventions

- Explain *why* a field, offset, layout, or fallback is being tested.
- Mark speculative fields, inferred semantics, and research-only adapters
  explicitly. Preserve undecoded bytes where practical.
- Keep implementation guarantees separate from claims about original-game
  behavior. For example, `XngineRecordGraph` can guarantee cycle detection
  for supplied links without proving that the links were decoded correctly.
- When evidence is added, identify its provenance, relevant game/version,
  observed bytes or behavior, and whether it supports or contradicts the
  hypothesis.
- Do not convert research code into production-facing assumptions solely
  because a local test succeeds.

The detailed reasoning and evidence for individual hypotheses have not yet
been attached to this branch-level note. Do not fill those gaps with invented
citations or assertions.
