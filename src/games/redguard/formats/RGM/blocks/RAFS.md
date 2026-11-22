# RAFS

## Status
Documented (commonly empty).

## Purpose
FSphere-related data (still unclear).

## Layout
- Standard section framing: `RAFS` tag + `be u32 length` + raw payload.
- Common emitted form in known toolchains is `length = 1`, payload `00`.

## Observed Behavior
- Often empty/effectively empty in sampled maps.
- Java writer path emits length `1` with a single zero byte.
- Workspace parser treats it as opaque and does not currently derive any record model from the payload.

## Relationships
- Appears immediately after `RAHD` and before the script/string system (`RAST`, `RASB`, `RAVA`, `RASC`).
- `RAHD` contains unresolved fields that may historically have pointed at RAFS-related data, but that coupling is not confirmed.

## Unknowns
- Canonical semantic meaning and non-empty field model.
- Whether non-trivial RAFS payloads exist in retail corpus variants.
- Whether "FSphere" corresponds to collision, visibility, fog, or some other runtime system.
