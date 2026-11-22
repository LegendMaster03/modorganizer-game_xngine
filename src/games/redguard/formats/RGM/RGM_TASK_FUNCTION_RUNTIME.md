# RGM Task/Function Runtime Notes (Unity Importer)

Last updated: 2026-03-08

## Scope
This appendix documents observed runtime behavior of RGM script task/function dispatch in the Unity importer (`RGScriptedObject` + `RGRGMScriptStore` + `soupdeffcn_nimpl`).

It is a behavior reference for implementation and validation, not a claim of canonical original engine behavior.

## Dispatch Model
- Script VM decodes task/function IDs from RASC and calls `RGObjectStore.DoObjectTask(...)`.
- Each scripted object has a function table of size `367` (`FUNC_CNT`) with signature:
  - `Func<uint caller, bool multitask, int[] parameters, int>`
- `RGScriptedObject.SetupFunctions()` binds a subset of IDs to implemented handlers.
- Unbound IDs are filled with `soupdeffcn_nimpl.getNIMPL(id)` stubs.

## Time/Angle/Coordinate Runtime Conventions
- time unit scaling in task handlers: `TIME_VAL = 0.1f`
- DA-angle conversion in handlers: `DA2DG = -(180.0f / 1024.0f)`
- world/script position scaling: `RGM_MPOB_SCALE = 1 / 5120.0f`
- axis mapping helper (`RGAxisToVector3`):
  - `0 -> X`, `1 -> Y`, `2 -> Z` (with importer-specific sign flip on Z)

## Task Scheduler Model
Task slots in `RGScriptedObject`:
- `mainTask`: single blocking lane
- `multiTasks`: concurrent async lane(s)

Observed task states:
- `idle`, `waitingtasks`, `rotating`, `moving`, `waiting`, `syncing`, `walking`, `pathfinding`, `facing`, `animating`

Execution behavior:
- If `mainTask` is idle, object executes one script instruction (`script.tickScript()`).
- Multitasks update each frame and are removed when returning to idle.

## Implemented ID Map (RGScriptedObject)
Only these IDs are concretely implemented in `RGScriptedObject`:

| ID | Name | Observed behavior |
|---:|---|---|
| 17 | `WaitOnTasks` | blocks until multitask list is empty |
| 21 | `showObj` | camera show/offset helper |
| 39 | `RTX` | subtitle display + timed wait task |
| 44 | `RotateByAxis` | timed local-axis rotation |
| 45 | `RotateToAxis` | timed absolute/local target rotation |
| 46 | `WalkForward` | timed forward walk using attribute speed |
| 47 | `WalkBackward` | timed backward walk using attribute speed |
| 53 | `MoveByAxis` | timed movement on selected axis |
| 56 | `MoveToLocation` | timed move to `RALC`/location slot |
| 60 | `Wait` | timed wait |
| 62 | `Light` | enable/create point light, set range/intensity |
| 63 | `LightRadius` | set point light range |
| 64 | `LightIntensity` | set point light intensity |
| 65 | `LightOff` | disable light |
| 66 | `LightOffset` | local light offset from scripted params |
| 83 | `SetAttribute` | set RAAT attribute byte |
| 84 | `GetAttribute` | get RAAT attribute byte |
| 88 | `FacePlayer` | task to face player target |
| 93 | `Sound` | play SFX clip |
| 94 | `FlatSound` | play SFX clip (flat variant) |
| 95 | `AmbientSound` | play SFX + wait for clip length |
| 100 | `HideMe` | disable renderer/collider |
| 101 | `ShowMe` | enable renderer/collider |
| 107 | `LoadWorld` | world transition request |
| 127 | `ACTIVATE` | UI prompt + returns activation state |
| 136 | `PushAnimation` | starts animation task and completion callback |
| 156 | `Offset` | slaves caller transform to target+offset |
| 174 | `isFighting` | stub returns 0 |
| 224 | `PlayerStand` | returns standing flag |
| 228 | `PlayerDistance` | returns scaled distance in RG units |
| 233 | `PlayerLooking` | angle threshold test |
| 243 | `PlayerFaceObj` | rotate player-facing relation |
| 271 | `SyncWithGroup` | group syncpoint coordination |
| 292 | `MoveMarker` | pathfinding-style move to marker |
| 320 | `IsHoldingWeapon` | stub returns 0 |

## Validator Priority Tiers
Use these tiers to prioritize install-folder validation effort.

### Tier A (validator-ready)
Implementations are concrete enough to validate against retail behavior with high value:
- `17 WaitOnTasks`
- `44 RotateByAxis`
- `46 WalkForward`
- `47 WalkBackward`
- `53 MoveByAxis`
- `56 MoveToLocation`
- `60 Wait`
- `62 Light`
- `63 LightRadius`
- `64 LightIntensity`
- `65 LightOff`
- `83 SetAttribute`
- `84 GetAttribute`
- `93 Sound`
- `94 FlatSound`
- `95 AmbientSound`
- `100 HideMe`
- `101 ShowMe`
- `107 LoadWorld`
- `127 ACTIVATE`
- `136 PushAnimation`
- `228 PlayerDistance`
- `233 PlayerLooking`
- `271 SyncWithGroup`

Rationale:
- direct, deterministic handlers with explicit state changes and low ambiguity.

### Tier B (partially reliable / importer-specific)
Usable but likely to diverge from original engine in edge cases or coordinate conventions:
- `21 showObj` (camera-path helper depends on Unity camera runtime)
- `45 RotateToAxis` (angle handling has sign/convention uncertainty)
- `66 LightOffset` (Z conversion and coordinate assumptions)
- `88 FacePlayer` (turn-speed depends on attribute scaling assumptions)
- `156 Offset` (continuous parent/offset behavior is importer implementation)
- `224 PlayerStand` (depends on runtime collision flag maintenance)
- `243 PlayerFaceObj` (helper implementation likely differs from engine script intent)
- `292 MoveMarker` (currently TODO/simple move, not real node-path behavior)

### Tier C (known stubs)
Currently placeholder/no gameplay-meaningful behavior:
- `174 isFighting` (returns `0`)
- `320 IsHoldingWeapon` (returns `0`)

## Suggested Validation Order
1. **Movement/Timing core**: `44,46,47,53,56,60`
2. **Visibility/Light/Audio**: `62..66,93..95,100,101`
3. **Script/UI flow**: `17,83,84,107,127,136,271`
4. **Player relation checks**: `228,233,224,243`
5. **Defer or mark provisional**: Tier B and Tier C IDs above

## SOUP Function Map Notes
- `soupdeffcn_nimpl.getNIMPLS()` builds a full indexed function-name array for IDs `0..366`.
- This map provides stable symbolic naming for unimplemented handlers and is used as fallback by `SetupFunctions()`.
- Examples from map start:
  - `0=PLACEHOLDER_ZERO`, `1=sRotate`, `17=WaitOnTasks`, `39=RTX`, ...

## Java Toolchain Cross-Check (`redguard-mod-manager-main`)
The Java parser/writer path (`MapDatabase` + `ScriptReader` + `ScriptParser`) adds a second implementation reference for ID/type handling.

SOUP ingestion behavior (`MapDatabase.readSoupFile`):
- A synthetic entry is injected at index `0`: `function NullFunction params 0`.
- Then `[functions]..[refs]` lines are parsed into `SoupFunction(type, name, paramCount)`.
- Consequence: bytecode function IDs map directly into this list with the synthetic zero slot preserved.

Opcode emission behavior (`ScriptParser.parseTask` / object-dot path):
- `@Task(...)` encodes as opcode `1` (multitask dispatch) regardless of SOUP `type`.
- Non-`@` task/function encodes as:
  - opcode `0` when `SoupFunction.type == "task"`
  - opcode `2` when `SoupFunction.type == "function"`
- Object-dot task/function calls (`obj.Name(...)`) similarly branch by SOUP type:
  - opcode `25` for task
  - opcode `26` for function

Reader coupling behavior (`ScriptReader.getTaskText`):
- Task/function name lookup is always `mapDatabase.getFunctions().get(id).getName()`.
- Parameter count uses encoded byte count for most IDs, but ID `0` is forced to `0` params in reader output.
- Practical implication: the synthetic index-0 `NullFunction` convention is baked into both writer and reader code paths.

Flag opcode behavior (`ScriptReader`/`ScriptParser`):
- Flag references use opcode `6` + `u16` flag index (`MapDatabase.getFlags()` ordering).
- In parameter mode, Java writer appends a fixed padding short (`0`), mirrored by reader consuming an extra short on flag parameters.
- Numeric parameter encoding switches from opcode `7` to opcode `22` when current task is one of:
  - `TestGlobalFlag`
  - `SetGlobalFlag`
  - `ResetGlobalFlag`
- Practical implication: global-flag calls are distinguished in bytecode by opcode (`22`) even when source text argument is numeric/string-like.

## Additional Unity `io_tst` Coverage (`SOUPDEF_TASKS.cs`)
- `io_tst/SOUPDEF_TASKS.cs` contains a full legacy-style global function catalog with:
  - explicit ID-to-name assignment (`soupdefFcn[0..366] = ...`),
  - task/function intent comments (`/*task*/` vs `/*function*/`),
  - declared parameter counts in signatures/comments.

### Concrete Parameter Notes from `SOUPDEF_TASKS.cs`
The file includes a small set of explicit parameter-meaning comments useful for validator wiring:
- `lockoutPlayer(i[0])`: `1` disables player input, `0` enables input.
- `RotateByAxis`/`MoveByAxis`: `i[0]=axis (0/1/2)`, `i[1]=amount`, `i[3]=time to complete`.
- `Wait(i[0])`: time-to-wait parameter.
- `ACTIVATE(i[0])`: string ID shown when looking at an activatable item.
- `PushAnimation`: `i[0]=animationId`, `i[1]=firstFrame`.
- `WaitAnimFrame`/`WaitPlayerAnimFrame`: `i[0]=animationId`, `i[1]=frame`.
- `SyncWithGroup(i[0])`: sync-group selector.

Interpretation note:
- `SOUPDEF_TASKS.cs` begins with a "100% speculation" warning comment. Treat these as strong naming/arity/annotation hints from tooling code, not canonical original-engine truth without retail validation.

This adds concrete domain coverage beyond currently implemented handlers, including named families for:
- messaging/broadcast (`SendMessage`, `ReceiveMessage`, `SendBroadcast`, ...)
- sync/piping/group routing (`SyncWithGroup`, `PipeGroup`, `PipeUser`, ...)
- node map/pathing (`MoveNodeMarker`, `AtNodePosition`, `Wander`, ...)
- inventory/weapon/UI (`AddItem`, `UseItem`, `DisplayHandModel`, ...)
- environment/effects (`ViewGXA`, `AnimateGXA`, `DragonBreath`, `QueueMovie`, `AddLog`, ...)

Practical value:
- improves validator routing and function-ID triage,
- provides a naming/arity reference for currently stubbed IDs without requiring speculative behavior claims.

## Unity `io_tst` Script Reader Limits (`rgmtst`)
The standalone Unity test reader (`RGRGMScriptStore` path used by `io_tst/rgmtst`) explicitly marks several instruction paths as unresolved/placeholder.

Concrete opcode-switch coverage in this path (`readInstruction`):
- handled: `0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x0A,0x11,0x12,0x13,0x14,0x15,0x16,0x19,0x1A,0x1B,0x1E`
- not handled in this switch (throws `NIMPL`): includes `0x0F/0x10` object-ref unary variants and any other unknown opcode.

Additional object-selector limitation in this test reader:
- `readObject()` accepts selectors `0x00/0x01/0x02` (general) and `0x04` (string-table object name), but not selector `0x0A` (local-variable object indirection in some references).

Observed unresolved behaviors:
- Top-level opcodes `0x00/0x01/0x02` currently dispatch with hardcoded object subject `object_me` (TODO object-name resolution).
- In `readFlag` / `readVar`, LHS/RHS mode consumes one operator byte with TODO comments and no semantic use.
- `readObjectRef` LHS path similarly reads an operator byte but does not apply operation semantics.
- `ENDINT` (`0x13`), `TASKPAUSE` (`0x1B`), and `SCRIPTRV` (`0x1E`) log debug text and return placeholder behavior.

`rgmtst` SOUPDEF dispatch coverage note:
- `io_tst/rgmtst/scriptstst.cs` builds a full function table of `367` entries (`SetupFuncs`, IDs `0..366`), aligning with importer function-count conventions.
- Practical caveat: this table is primarily a logging harness (`Log(name, params)` + `return 0`) and should be interpreted as name/arity routing coverage, not gameplay-equivalent behavior coverage.

Practical implication:
- This `io_tst` script-reader path is useful for opcode visibility and triage, but should not be treated as behavior-complete runtime parity for control-flow edge cases.

### VM Execution Semantics (Unity script runner)
Observed in `RGRGMScriptStore.ScriptData`:
- `runScript()` starts execution at `scriptPC` (from RAHD) and repeatedly executes `readInstruction(main)` until explicit end sentinel.
- End handling is `readEnd()` (`opcode 0x05`): reads target `u32`, updates `scriptPC`, and returns sentinel `0xDEAD` to terminate the current run loop.
- Goto handling is `readGoto()` (`opcode 0x04`): reads target `u32` and sets instruction pointer directly.
- Gosub/return handling uses an explicit `Stack<int>` callstack:
  - `readGoSub()` (`0x11`) pushes return address and jumps,
  - `readReturn()` (`0x12`) pops and resumes at saved address.
- Safety guard: `runScript()` bails out after ~1024 instructions with debug log (`INF LOOP; BAILING`) to avoid runaway loops in test/runtime path.
- `tickScript()` executes a single top-level instruction per call (frame-stepped VM mode).

## Integration Notes
- Runtime scripts couple to RAAT attributes for movement and behavior (for example, walk speed, turn speed).
- Group sync path uses RAAT group attribute (`attributes[29]`) and object-store group coordination.
- Location-based movement uses precomputed location list (`MPOB` base + `RALC` offsets).

## Confidence
- High: function ID bindings and concrete handler behavior listed above.
- Medium: some handlers are still importer stubs and comments indicate uncertainty on exact retail semantics.
- Medium-Low (annotation-only): `SOUPDEF_TASKS.cs` parameter comments (useful for triage, pending retail confirmation).

## Sources
- `redguard-unity-master/redguard-unity-master/Assets/Scripts/RGFileImport/RGMData/RGScriptedObject.cs`
- `redguard-unity-master/redguard-unity-master/Assets/Scripts/RGFileImport/RGMData/RGRGMScriptStore.cs`
- `redguard-unity-master/redguard-unity-master/Assets/Scripts/RGFileImport/RGMData/soupdeffcn_nimpl.cs`
- `redguard-unity-master/redguard-unity-master/Assets/Scripts/RGFileImport/RGMData/RGObjectStore.cs`
- `redguard-unity-master/redguard-unity-master/io_tst/SOUPDEF_TASKS.cs`