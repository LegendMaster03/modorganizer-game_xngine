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
  descriptors and ten-byte face headers in supported v2.x records.
- BS6 level chunks and BSI image/palette structures.
- SAVETREE and SAVEVARS offsets, record relationships, and cross-file joins.
- Name hashing and interpretation of records whose meaning is incomplete.
- Explicitly isolated Daggerfall-derived TEXT.RSC and WATER.TBL adapters whose
  applicability to Battlespire has not been established.

This list describes **what the code investigates**; it does not certify that
each interpretation matches data produced by the original game. A specific
research provenance should be recorded when the source material becomes
available, rather than inferred from the mere presence of an implementation.

## Evidence ledger

The branch combines several kinds of evidence. The table records the basis for
each implementation so that a Daggerfall analogy is not mistaken for a
Battlespire observation, and a reverse-engineered description is not mistaken
for validation performed in this repository.

| Area | Current basis | Repository validation | Remaining requirement |
|---|---|---|---|
| SAVETREE/SAVEVARS offsets and record IDs | The UESP Battlespire save-format notes; Daggerfall Unity save structures are used only to identify candidate inherited relationships | Offset arithmetic, record matching, graph behavior, and joins have synthetic coverage | Compare against representative original saves and controlled save-state changes |
| BSA descriptors and LZSS | `battlespire-tools` BSA format notes and decoder behavior | Synthetic descriptor and decode/rewrite fixtures | Read representative archives from supported releases and compare extracted bytes |
| Ten-byte 3D face headers | `battlespire-tools` 3D reader plus the existing shared XnGine mesh parser | A synthetic v2.7 fixture checks selection, normalization, and preservation of unknown bytes | Validate multiple original models and determine the remaining unknown fields' meanings |
| BS6 level chunks | `battlespire-tools` BS6 reader and tag names | Synthetic nested chunks, typed projections, and malformed-length handling | Validate tag nesting, units, object projection, textures, and water data against original levels |
| BSI images | `battlespire-tools` BSI format notes and reader | Synthetic palettes, raw/RLE scanlines, and malformed chunks | Compare decoded images, palettes, and animation frames with original assets |
| Variable-name hash | The rotate-left-by-four algorithm reproduces the published name/hash pairs | Compile-time and runtime known-answer tests | Confirm how names are sourced and used by real scripts and SAVEVARS records |
| TEXT.RSC and WATER.TBL adapters | Daggerfall compatibility analogy only | Shared-parser behavior can be tested, but no Battlespire resource use is established | Locate original Battlespire files or callers before treating either adapter as supported |

These references are reverse-engineering inputs, not vendor specifications:

- [UESP Battlespire save format](https://en.uesp.net/wiki/Mod:Battlespire_Save_Game_Format)
- [Daggerfall Unity save implementation](https://github.com/Interkarma/daggerfall-unity/tree/81e89e90c27bc3c1a7a61871e545fad129174dec/Assets/Scripts/API/Save)
- [`battlespire-tools`](https://github.com/ariscop/battlespire-tools/tree/c2f2bb24d42a71024ab53eabf42eca5b6ffcd8c9)

The branch does not currently contain provenance-tagged original-game fixtures.
Until those are added, the final column remains part of the acceptance work for
each hypothesis.

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
been captured for every individual field. Do not fill those gaps with invented
citations or assertions; extend the ledger when new evidence is introduced.
