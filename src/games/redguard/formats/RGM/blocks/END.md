# END

## Status
Documented terminator.

## Purpose
Marks end of RGM section stream.

## Layout
- Literal tag `END ` (`45 4E 44 20`).
- No payload is consumed by current parser paths after the tag is seen.

## Notes
- Some parser paths treat `END ` as tag-only terminator with no length payload.
- Parsing should stop on this sentinel even if unknown tags were previously skipped/preserved.
- `END ` is the canonical end of the top-level section stream, not a counted section like `RAHD` or `MPOB`.
- Writer paths should emit it exactly once as the final 4CC in the file.

## Writer Notes
- Workspace C++ writer emits `END ` after all preserved/rewritten sections.
- Legacy Java writer also terminates on `END ` and does not append a trailing length field.
