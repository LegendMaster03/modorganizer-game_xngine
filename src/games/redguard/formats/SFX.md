# SFX (MAIN.SFX)

Last updated: 2026-03-11

## Scope
All Redguard sound effects are contained in `SOUND/MAIN.SFX` (footsteps, combat sounds, fire, water, ambient machinery, etc.).

This does **not** include voice clips, which are stored in `ENGLISH.RTX`.

## Format Overview
`MAIN.SFX` is a sequential container with:
1. `FXHD`
2. `FXDT`
3. terminal `END ` marker (includes trailing space)

Effects are referenced by 0-based index in file order. There is no per-effect offset table.

## FXHD
`FXHD` is a 44-byte section that stores metadata, including effect count.

| Offset | Type | Name | Description |
|---:|---|---|---|
| 0 | `be u32` | `SectionSize` | Size of section payload, excluding this field |
| 4 | `char[32]` | `Description` | Misspelled/legacy SoupFX-related descriptor |
| 36 | `u32` | `EffectCount` | Number of effects (retail corpus: 118) |

## FXDT
`FXDT` starts with a `be u32` section-size value (excluding itself), then effect records are read sequentially.

Each effect is:
- 27-byte header
- raw PCM payload (`DataLength` bytes)

### Effect Header
| Offset | Type | Name | Description |
|---:|---|---|---|
| 0 | `u32` | `TypeID` | Audio layout enum: `0=8-bit mono`, `1=16-bit mono`, `2=8-bit stereo*`, `3=16-bit stereo` |
| 4 | `u32` | `BitDepth` | Bit depth enum: `0=8-bit`, `1=16-bit` |
| 8 | `u32` | `SampleRate` | Observed values: `11025` or `22050` |
| 12 | `u8` | `Unknown_0C` | Observed `64` on all effects |
| 13 | `s8` | `LoopFlag` | Observed `0`, `-1 (0xFF)`, or `-31 (0xE1)` |
| 14 | `u32` | `LoopOffset` | Byte offset (from effect-data start) used for loop restart |
| 18 | `u32` | `LoopEnd` | Sample position where playback loops back to `LoopOffset` |
| 22 | `u32` | `DataLength` | Length of raw PCM payload in bytes |
| 26 | `u8` | `Unknown_1A` | Observed `0`, likely reserved/padding |
| 27 | `struct[DataLength]` | `PCMData` | Raw PCM (`u8` if `BitDepth=0`, `s16` if `BitDepth=1`) |

\* `TypeID=2` is not observed in known retail corpus and meaning remains inferred.

## Looping Semantics (External Notes)
- `LoopFlag = 0`: nominally non-looping. External notes indicate forcing this on loop-capable effects may add a short restart gap (~0.02s).
- `LoopFlag = -1`: used on looped ambient/effect clips and restarts immediately.
- `LoopFlag = -31`: observed on effect `117` (Snake Charmer tune), behavior not yet fully characterized.

External corpus notes indicate:
- `LoopOffset` is `0` across listed effects.
- `LoopEnd` is `0xFFFFFFFF` across listed effects.

These fields appear provisioned for a fuller loop-window feature not actively authored in retail assets.

## END Marker
File terminates with four-byte marker `END `.

## Sound Effects (External Full Listing)
External table listing all 118 effect entries with index, offset, data length, type, sample rate, duration, and descriptive usage:

```text
Index	Offset	DataLength	Type	SampleRate	Duration	Description
0	53	18432	16-bit Mono	22050	0.418	Boat/Rope creaking
1	18512	19946	16-bit Mono	22050	0.452	Canopy/Wagon noise
2	38485	201274	16-bit Mono	22050	4.564	Water
3	239786	453212	16-bit Stereo	22050	5.138	Rain
4	693025	9268	16-bit Mono	22050	0.21	Footstep
5	702320	8274	16-bit Mono	22050	0.188	Footstep
6	710621	9516	16-bit Mono	22050	0.216	Footstep
7	720164	9516	16-bit Mono	22050	0.216	Footstep
8	729707	8260	16-bit Mono	22050	0.187	Footstep
9	737994	4302	16-bit Mono	22050	0.098	Footstep
10	742323	9982	16-bit Mono	22050	0.226	Footstep
11	752332	3442	16-bit Mono	22050	0.078	Footstep
12	755801	13854	16-bit Mono	22050	0.314	Walking/idle rattle
13	769682	14236	16-bit Mono	22050	0.323	Walking/idle rattle - Menu selection change
14	783945	19222	16-bit Mono	22050	0.436	Landing
15	803194	120228	16-bit Mono	22050	2.726	Bell ring
16	923449	41084	16-bit Mono	22050	0.932	Waterfall
17	964560	57688	16-bit Mono	22050	1.308	Door (Heavy/Stone)
18	1022275	7346	8-bit Mono	11025	0.666	Ledge hand plant/pull up
19	1029648	7711	8-bit Mono	11025	0.699	HMMMMM…
20	1037386	31648	16-bit Mono	22050	0.718	Sliding
21	1069061	17822	16-bit Mono	22050	0.404	Sheath sword
22	1086910	22460	16-bit Mono	22050	0.509	Draw sword
23	1109397	4750	16-bit Mono	22050	0.108	Attack
24	1114174	3776	16-bit Mono	22050	0.086	Attack/Defend
25	1117977	94248	16-bit Mono	22050	2.137	Fire
26	1212252	13810	16-bit Mono	22050	0.313	Hit sound (Bot)
27	1226089	70338	16-bit Mono	22050	1.595	Seagull x5
28	1296454	21560	16-bit Mono	22050	0.489	Seagull
29	1318041	30946	16-bit Mono	22050	0.702	Seagull x2
30	1349014	10667	8-bit Mono	11025	0.968	Laser
31	1359708	102282	16-bit Mono	22050	2.319	Machinery noise
32	1462017	47774	16-bit Mono	22050	1.083	Golem attack (steam noise)
33	1509818	42392	8-bit Mono	22050	1.923	Gear sound
34	1552237	67558	16-bit Mono	22050	1.532	Golem walking
35	1619822	40804	16-bit Mono	22050	0.925	Chest/Door open
36	1660653	58942	16-bit Mono	22050	1.337	Cage open
37	1719622	18238	16-bit Mono	22050	0.414	Pressure plate stand on / lever pull
38	1737887	19252	16-bit Mono	22050	0.437	Pressure plate step off / lever reset
39	1757166	132644	16-bit Mono	22050	3.008	Wind
40	1889837	44952	16-bit Mono	22050	1.019	Dwarven Ruins "flowers"
41	1934816	42704	16-bit Mono	22050	0.968	Steam noise
42	1977547	34356	16-bit Mono	22050	0.779	Golem walking / Bot activating
43	2011930	18284	8-bit Mono	11025	1.658	Whoosh noise - teleport / Dram knockout arrow
44	2030241	8370	8-bit Mono	11025	0.759	Transform to/from Gremlin
45	2038638	31162	16-bit Mono	22050	0.707	Bubbling (lava)
46	2069827	66444	16-bit Mono	22050	1.507	Bubbling (deeper sounding)
47	2136298	18818	16-bit Mono	22050	0.427	Golem attack/turn/walk
48	2155143	45298	16-bit Mono	22050	1.027	Bot activating
49	2200468	13504	8-bit Mono	11025	1.225	Landing / grab ledge
50	2213999	11704	8-bit Mono	11025	1.062	Cyrus fall death / vomit?
51	2225730	34548	16-bit Mono	22050	0.783	Cyrus fall death / Skeleton death / Ogre/Troll fight move
52	2260305	23120	16-bit Mono	22050	0.524	CYRUS - GROUP_LAND_DEATH / LAKENE - GROUP_MISC4
53	2283452	18284	8-bit Mono	11025	1.658	CYRUS - GROUP_MISC[17] - Whoosh noise
54	2301763	20496	16-bit Mono	22050	0.465	Skeleton walk/fight forward
55	2322286	18662	16-bit Mono	22050	0.423	Skeleton GROUP_MISC[13] / run forward
56	2340975	14994	16-bit Mono	22050	0.34	Skeleton GROUP_MISC1
57	2355996	8540	16-bit Mono	22050	0.194	Sword clank 1
58	2364563	10350	16-bit Mono	22050	0.235	Sword clank 2
59	2374940	9068	16-bit Mono	22050	0.206	Hit sound (Normal)
60	2384035	20972	16-bit Mono	22050	0.476	Whoosh noise - Think this may be an arrow passing by from when crossbows where supposed to be a thing
61	2405034	20972	16-bit Mono	22050	0.476	Whoosh noise - And this may be an arrow hitting you or impacting close
62	2426033	38762	16-bit Mono	22050	0.879	Wood creaking - May be drawing the bow back
63	2464822	41576	16-bit Mono	22050	0.943	Whoosh + wood creaking - Maybe firing (whoosh of arrow, bow tension released)
64	2506425	81570	16-bit Mono	22050	1.85	Menu open (Start)
65	2588022	14390	16-bit Mono	22050	0.326	Menu close (Quit)
66	2602439	41140	16-bit Mono	22050	0.933	Menu change multiple pages (option open/close)
67	2643606	26006	16-bit Mono	22050	0.59	Book page change sound (not sure where used?)
68	2669639	28156	16-bit Mono	22050	0.638	Menu change page
69	2697822	37772	16-bit Mono	22050	0.857	Cyrus swim / Footstep - SCAPEWATER/WATER (4,5,6,7,8,9,10,11)
70	2735621	35364	16-bit Mono	22050	0.802	Cyrus swim / Landing - DEEPWATER/SCAPEDEEPWATER/SCAPEWATER/WATER (14)
71	2771012	38820	16-bit Mono	22050	0.88	Dolphin/Tentacle splash noise - wave hitting planks
72	2809859	94588	16-bit Mono	22050	2.145	Fly buzzing
73	2904474	22926	16-bit Mono	22050	0.52	Golem attacks (all)
74	2927427	26354	16-bit Mono	22050	0.598	Goblin Cavern trap
75	2953808	7546	8-bit Mono	11025	0.684	Hurt
76	2961381	9087	8-bit Mono	11025	0.824	Hurt
77	2970495	15901	8-bit Mono	11025	1.442	Hurt (probably killed?)
78	2986423	7137	8-bit Mono	11025	0.647	Hurt
79	2993587	37876	16-bit Mono	22050	0.859	Bot deaths / Glass break
80	3031490	12272	16-bit Mono	22050	0.278	Gremlin (unknown)
81	3043789	12274	16-bit Mono	22050	0.278	Gremlin (unknown)
82	3056090	25364	16-bit Mono	22050	0.575	Gremlin jump start
83	3081481	14728	16-bit Mono	22050	0.334	Gremlin land
84	3096236	20454	16-bit Mono	22050	0.464	Hit sound (Gremlin)
85	3116717	18244	16-bit Mono	22050	0.414	Cyrus drink flask
86	3134988	23938	16-bit Mono	22050	0.543	Dwarven Ruins walkways
87	3158953	12660	16-bit Mono	22050	0.287	Mushroom bounce
88	3171640	192468	16-bit Mono	22050	4.364	Stone sliding/rumbling
89	3364135	8250	16-bit Mono	22050	0.187	Footstep - WOOD (4)
90	3372412	10756	16-bit Mono	22050	0.244	Footstep - WOOD (5)
91	3383195	8042	16-bit Mono	22050	0.182	Footstep - WOOD (6)
92	3391264	7728	16-bit Mono	22050	0.175	Footstep - WOOD (7)
93	3399019	7408	16-bit Mono	22050	0.168	Footstep - WOOD (8)
94	3406454	8754	16-bit Mono	22050	0.199	Footstep - WOOD (9)
95	3415235	6870	16-bit Mono	22050	0.156	Footstep - WOOD (10)
96	3422132	7408	16-bit Mono	22050	0.168	Footstep - WOOD (11)
97	3429567	9966	16-bit Mono	22050	0.226	Footstep - WOOD (14)
98	3439560	21732	16-bit Mono	22050	0.493	Bot fight forward/circle
99	3461319	11482	16-bit Mono	22050	0.26	Bot attack [12] / defend [left|right]
100	3472828	10098	16-bit Mono	22050	0.229	Bot attack [3|lunge] / defend [high|low]
101	3482953	17708	16-bit Mono	22050	0.402	Bot fight forward/circle
102	3500688	16038	16-bit Mono	22050	0.364	Footstep - ROCK (4,6,8,10)
103	3516753	13210	16-bit Mono	22050	0.3	Footstep - ROCK (5,7,9,11)
104	3529990	18404	16-bit Mono	22050	0.417	Footstep - Doesn't appear to be used?
105	3548421	15130	16-bit Mono	22050	0.343	Footstep - Doesn't appear to be used?
106	3563578	26704	16-bit Mono	22050	0.606	Footstep - SAND (8,10)
107	3590309	27138	16-bit Mono	22050	0.615	Footstep - SAND (9,11)
108	3617474	21290	16-bit Mono	22050	0.483	Footstep - SAND (4,6)
109	3638791	21702	16-bit Mono	22050	0.492	Footstep - SAND (5,7)
110	3660520	17032	16-bit Mono	22050	0.386	Footstep - TILE (8,10)
111	3677579	17032	16-bit Mono	22050	0.386	Footstep - TILE (9,11)
112	3694638	9424	16-bit Mono	22050	0.214	Footstep - TILE (4)
113	3704089	8216	16-bit Mono	22050	0.186	Footstep - TILE (5)
114	3712332	9424	16-bit Mono	22050	0.214	Footstep - TILE (6)
115	3721783	9664	16-bit Mono	22050	0.219	Footstep - TILE (7)
116	3731474	15060	16-bit Mono	22050	0.341	Landing - TILE (14)
117	3746561	768180	16-bit Mono	22050	17.419	AVIK - Snake charmer tune
```

## Unity Parser Cross-Check
- Unity importer confirms section framing as `4CC + be u32 size`, with `END ` as terminal tag.
- `FXHD` payload is read as `char[32]` description plus `s32` sound count.
- `FXDT` records are loaded by iterating exactly `FXHD.numSounds` entries.
- Per-entry payload is read as `27` bytes of metadata + `DataLength` raw bytes.
- Unity importer treats `TypeID` and `BitDepth` as enums:
  - type: `0=mono8`, `1=mono16`, `2=stereo8`, `3=stereo16`
  - depth: `0=8-bit`, `1=16-bit`

Additional Unity parser behavior:
- Unknown top-level SFX sections are skipped via bounded seek (`currentPosition + sectionLength`) instead of hard failure.
- `PCMData` is read as raw bytes regardless of declared bit depth in parser struct; sample interpretation is deferred to consumer paths.

## Unity `io_tst` Decoder Parity Note
- `io_tst/sndtst/sfxtst.cs` includes a reference conversion for 8-bit PCM preview:
  - `floatSample = (byteSample / 128.0f) - 1.0f`
- This maps unsigned-byte payloads to approximately `[-1.0, +0.992]`.

## Discrepancies / Open Points
- `TypeID = 2` (`8-bit stereo`) remains inferred/unused in sampled retail corpus.
- Behavioral meaning of `LoopFlag = -31` (effect `117`) is still not fully characterized.
- Per-effect integer endianness should be revalidated in any new parser/writer implementation path.

## Confidence + Outstanding Verification
- High confidence: container structure, effect header layout, count-driven sequential parsing, and retail effect count.
- Medium confidence: loop-behavior interpretation details for special/non-default loop flags.
- Needs confirmation: exact runtime handling of edge-case loop flags and unused type modes.

## Related Format Docs
- `RGM/RGM.md` (map/script systems that reference runtime effects)
- `TEXBSI/TEXBSI.md` (parallel asset container style with chunked binary records)

## Sources
- External reverse-engineering notes (full effect catalog + field semantics)
- `redguard-unity-master/redguard-unity-master/Assets/Scripts/RGFileImport/RGGFXImport/RGSFXFile.cs`
- `redguard-unity-master/redguard-unity-master/io_tst/sndtst/sfxtst.cs`
- UESP excerpt merge (2026-03-11): `MAIN.SFX` overview, FXHD/FXDT/effect-header field table, and full 118-entry effect listing

