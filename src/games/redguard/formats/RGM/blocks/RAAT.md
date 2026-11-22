# RAAT

## Status
Documented, under-modeled semantically.

## Purpose
Attribute bytes per object/script entry.

## Record Model
- Fixed size: `256` bytes per object.
- Bytes `0..127`: attribute values.
- Bytes `128..255`: trailing padding.

## Runtime Notes
- Unity consumes as contiguous windows using `RAHD.index * 256`.

## Behavioral Highlights
- Movement/combat stats: `forward_speed`, `turn_speed`, `attack_skill`, `defense_skill`.
- Camera tuning: `camera_target_offset_*`, `camera_offset_*`, `camera_distance`.
- Combat routing IDs: `attack_*`, `defend_*`, `weapon`, `v_weapon`.
- Mode flags: `npc`, `combat`, `no_collide`, `non_moving`, `special_activate`.

## Caveat
- Map defaults are not full runtime truth; scripts/engine mutate many values.

## Full Attribute Index Map (0..127)
`u8` attributes at offsets `0..127` (remaining bytes are padding to `256`):

```text
0 active
1 collide
2 strength
3 health
4 armor
5 movement
6 weight
7 user_flag
8 forward_speed
9 backward_speed
10 side_speed
11 step_type
12 forward_walk
13 backward_walk
14 combat
15 turn_speed
16 fight_lunge_speed
17 fight_forward_speed
18 fight_backward_speed
19 fight_forward_walk
20 fight_backward_walk
21 fight_turn_speed
22 fight_turn_accel
23 weapon
24 player_align
25 master
26 slave
27 slave_type
28 rotation
29 group
30 user
31 jump_time
32 jump_height
33 no_slave_rot
34 sheath_frame
35 draw_frame
36 ignore_own_matrix
37 stop_y_rotate
38 stop_x_rotate
39 stop_z_rotate
40 stop_x_move
41 stop_y_move
42 stop_z_move
43 always_sync
44 panic_group
45 camera_move
46 camera_pan_type
47 camera_target_offset_x
48 camera_target_offset_y
49 camera_target_offset_z
50 camera_offset_x
51 camera_offset_y
52 camera_offset_z
53 camera_distance
54 dialog_y
55 no_collide
56 no_sphere_object
57 mass
58 aggression
59 combat_defense
60 attack
61 defend
62 attack_skill
63 defense_skill
64 combat_allies
65 combat_alignment
66 combat_group
67 weapon_type
68 armor_type
69 death_type
70 attack_single
71 attack_double
72 attack_triple
73 attack_thrust
74 attack_lunge
75 attack_low
76 attack_jump_up
77 attack_jump_forward
78 attack_disarm
79 defend_low
80 defend_right
81 defend_left
82 defend_high
83 fight_circle_speed
84 fight_circle_delta_angle
85 num_taunts
86 hit_sound
87 chatty
88 twitch
89 stick_to_surface
90 node_idle
91 node_climbs
92 node_jumps
93 node_run
94 node_in_use
95 node_ignore_in_sight
96 damage
97 npc
98 bounce_power
99 bounce_move
100 bounce_turn
101 boat_rock_align
102 v_weapon
103 light_frame
104 unlight_frame
105 player_type
106 special_activate
107 fall_hold
108 fall_damage
109 zap_anim
110 holding_weapon
111 reload_type
112 no_continue
113 short_mapping
114 no_view_manage
115 reserved_weight
116 no_node_combat
117 lock_pos
118 woman
119 reserved_collide
120 invulnerable
121 combat_ignore_in_sight
122 non_moving
123 combat_long_sight
124 combat_radius_weapon
125 mess_group
126 combat_radius_vhit
127 padding boundary
```

## Attribute Behavior Notes (0..127)
Observed map-start behavior/ranges from reference documentation:

| Off | Name | Reference Notes |
|---:|---|---|
| 0 | active | Set true on mixed scene-related objects; boolean style usage. |
| 1 | collide | Set on NPC-type objects; reported range `(9,15,14)`. |
| 2 | strength | Not set in map defaults; believed runtime-updated (strength potion). |
| 3 | health | Mostly NPCs, also some environment objects; reported `(1,250,48)`. |
| 4 | armor | Not set in map defaults; believed runtime-updated (ironskin potion). |
| 5 | movement | Not set. |
| 6 | weight | Set on NPC types; reported `(1,250,142)`. |
| 7 | user_flag | Set on various usable objects; boolean-like. |
| 8 | forward_speed | Mostly NPC types; reported values: `(4,14,150)`. |
| 9 | backward_speed | NPC types; reported `(4,20,7)`. |
| 10 | side_speed | NPC types; reported `(6,8,6)`. |
| 11 | step_type | NPC types; observed value `1`; may be enum or boolean-style. |
| 12 | forward_walk | NPC types; reported `(4,26,7)`. |
| 13 | backward_walk | NPC types; reported `(4,20,7)`. |
| 14 | combat | Set on combat-capable NPC types; boolean-like. |
| 15 | turn_speed | NPC types; reported `(6,64,37)`. |
| 16 | fight_lunge_speed | NPC types; reportedly always `38` when set. |
| 17 | fight_forward_speed | NPC types; reported `(4,20,16)`. |
| 18 | fight_backward_speed | NPC types; reported `(4,20,16)`. |
| 19 | fight_forward_walk | NPC types; reported `(4,20,19)`. |
| 20 | fight_backward_walk | NPC types; reported `(4,20,10)`. |
| 21 | fight_turn_speed | NPC types; reported `(6,64,62)`. |
| 22 | fight_turn_accel | NPC types; reportedly always `12`. |
| 23 | weapon | NPC types; ItemID of object weapon. |
| 24 | player_align | Set on some movable non-NPC objects; boolean-like. |
| 25 | master | Master object ID used by slave objects. |
| 26 | slave | Master ID reference for this object. |
| 27 | slave_type | Not set. |
| 28 | rotation | Not set. |
| 29 | group | Group ID for multi-object grouping. |
| 30 | user | Not set. |
| 31 | jump_time | NPC types; reportedly always `6`. |
| 32 | jump_height | NPC types; reportedly always `70`. |
| 33 | no_slave_rot | Not set. |
| 34 | sheath_frame | NPCs with sheathing; reportedly always `9`. |
| 35 | draw_frame | NPCs with sheathing; reportedly always `5`. |
| 36 | ignore_own_matrix | Not set. |
| 37 | stop_y_rotate | Not set. |
| 38 | stop_x_rotate | Not set. |
| 39 | stop_z_rotate | Not set. |
| 40 | stop_x_move | Not set. |
| 41 | stop_y_move | Not set. |
| 42 | stop_z_move | Not set. |
| 43 | always_sync | Not set. |
| 44 | panic_group | Not set. |
| 45 | camera_move | Only set on two scenes; boolean-like. |
| 46 | camera_pan_type | Not set. |
| 47 | camera_target_offset_x | Mixed NPC/object use; reported `(1,255,135)`. |
| 48 | camera_target_offset_y | Mostly NPC types; reported `(1,255,108)`. |
| 49 | camera_target_offset_z | Mixed NPC/object use; reported `(1,178,112)`. |
| 50 | camera_offset_x | Mixed NPC/object use; reported `(128,255,138)`. |
| 51 | camera_offset_y | Mostly NPC types; reported `(1,144,107)`. |
| 52 | camera_offset_z | Mixed NPC/object use; reported `(100,165,115)`. |
| 53 | camera_distance | Only CYRUS/GREMLIN; reported `(100,250,175)`. |
| 54 | dialog_y | Not set. |
| 55 | no_collide | Mostly objects and BOATMAN; boolean-like. |
| 56 | no_sphere_object | Not set. |
| 57 | mass | Not set. |
| 58 | aggression | NPC types; reported `(80,120,102)`. |
| 59 | combat_defense | Not set. |
| 60 | attack | NPC types; reportedly always `64`. |
| 61 | defend | NPC types; reportedly always `32`. |
| 62 | attack_skill | NPC types; reported `(50,127,98)`. |
| 63 | defense_skill | NPC types; reported `(100,128,118)`. |
| 64 | combat_allies | Not set. |
| 65 | combat_alignment | Not set. |
| 66 | combat_group | Not set. |
| 67 | weapon_type | Not set. |
| 68 | armor_type | Not set. |
| 69 | death_type | Not set. |
| 70 | attack_single | NPC types; attack ID in COMBAT.INI. |
| 71 | attack_double | NPC types; attack ID in COMBAT.INI. |
| 72 | attack_triple | NPC types; attack ID in COMBAT.INI. |
| 73 | attack_thrust | NPC types; attack ID in COMBAT.INI. |
| 74 | attack_lunge | NPC types; attack ID in COMBAT.INI. |
| 75 | attack_low | NPC types; attack ID in COMBAT.INI. |
| 76 | attack_jump_up | NPC types; attack ID in COMBAT.INI. |
| 77 | attack_jump_forward | NPC types; attack ID in COMBAT.INI. |
| 78 | attack_disarm | NPC types; attack ID in COMBAT.INI. |
| 79 | defend_low | NPC types; block ID in COMBAT.INI. |
| 80 | defend_right | NPC types; block ID in COMBAT.INI. |
| 81 | defend_left | NPC types; block ID in COMBAT.INI. |
| 82 | defend_high | NPC types; block ID in COMBAT.INI. |
| 83 | fight_circle_speed | NPC types; reported `(10,15,11)`. |
| 84 | fight_circle_delta_angle | NPC types; reported `(20,30,27)`. |
| 85 | num_taunts | NPC types; used with RAEX.TauntID. |
| 86 | hit_sound | NPC types; reported `(26,84,56)`. |
| 87 | chatty | NPC types; reported `(125,127,126)`. |
| 88 | twitch | NPC types; reported `(10,100,98)`. |
| 89 | stick_to_surface | NPC types; boolean-like. |
| 90 | node_idle | Not set. |
| 91 | node_climbs | Not set. |
| 92 | node_jumps | Not set. |
| 93 | node_run | Not set. |
| 94 | node_in_use | Not set. |
| 95 | node_ignore_in_sight | Not set. |
| 96 | damage | Set on damaging/projectile objects; reported `(50,255,101)`. |
| 97 | npc | NPC marker; boolean-like. |
| 98 | bounce_power | Canopies/mushrooms/wagons; reported `(20,140,84)`. |
| 99 | bounce_move | Canopies/mushrooms/wagons; reported `(16,64,25)`. |
| 100 | bounce_turn | Set on mushrooms; boolean-like. |
| 101 | boat_rock_align | Only YAELBOAT; boolean-like. |
| 102 | v_weapon | ItemID of weapon that can damage object (specific enemy/object uses reported). |
| 103 | light_frame | CYRUS; reportedly always `12`. |
| 104 | unlight_frame | CYRUS; reportedly always `9`. |
| 105 | player_type | Enum-like; reported `0=Cyrus`, `1=Gremlin`, `2=Crossbow(?)`. |
| 106 | special_activate | NPC types; boolean-like. |
| 107 | fall_hold | Not set. |
| 108 | fall_damage | Not set. |
| 109 | zap_anim | Damaging objects; reported `(13,74,19)`. |
| 110 | holding_weapon | Skeletons only; boolean-like. |
| 111 | reload_type | Enum-like: `0 None`, `1 Hide`, `2 Resurrect`, `3 Kill` (reported). |
| 112 | no_continue | Not set. |
| 113 | short_mapping | Set on specific bots/goblins/guards; boolean-like. |
| 114 | no_view_manage | Set on CYRUS and GOLEM; boolean-like. |
| 115 | reserved_weight | Not set. |
| 116 | no_node_combat | Set on large enemies (dragon/golem/ogre/serpent); boolean-like. |
| 117 | lock_pos | Not set. |
| 118 | woman | Set on WOMEN; boolean-like. |
| 119 | reserved_collide | Not set. |
| 120 | invulnerable | Not set. |
| 121 | combat_ignore_in_sight | Set on GOLEM and OGRE; boolean-like. |
| 122 | non_moving | Set on mixed non-NPC objects; boolean-like. |
| 123 | combat_long_sight | Set on DRAGON and SERPENT; boolean-like. |
| 124 | combat_radius_weapon | Not set. |
| 125 | mess_group | Not set. |
| 126 | combat_radius_vhit | Only GOLEM; boolean-like. |
| 127 | padding boundary | Remaining bytes are record padding to 256 total bytes. |

## Notes on Interpretation
- No attribute is always populated across all records.
- Phrases like "always set" mean "always non-zero in records where used".
- Many attributes are runtime-mutated; map defaults are only initial state.
