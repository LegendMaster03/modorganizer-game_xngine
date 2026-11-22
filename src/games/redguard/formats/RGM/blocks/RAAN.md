# RAAN

## Status
Documented, partially modeled.

## Purpose
Model-file references and frame-count metadata.

## Record Model
- Variable-size entries in a raw payload stream.
- Common fields: `FaceCount (u32)`, `FrameCount (u8)`, `Unknown_05 (u8)`, `ModelFile (null-terminated text)`.
- `ModelFile` is typically authored as a path under `3DART`; external notes report runtime path swap to `FXART` in Glide mode.

## Parser/Runtime Notes
- Interpreted via RAHD window (`RAANOffset`, `RAANCount`, related lengths).
- Unity animation store normalizes model keys by splitting on `\\` and `.` then uppercasing base token.
- External behavior notes describe concatenation model:
	- frames from listed model files are concatenated into one object frame array,
	- total frame model should align with RAHD `FrameCount`.
- Path note from external docs:
	- records reference `3DART` paths, with reported runtime swap to `FXART` in some Glide mode paths.
- External value note for `Unknown_05`:
	- often `0x63` for non-Cyrus files,
	- often `0x73` for subset of Cyrus animation files,
	- exact semantic role remains unresolved.

## Writer Notes
- Raw payload preserved during rewrite copy-window stage.

## Related Docs
- `../RGM_BEHAVIOR_NOTES.md`
