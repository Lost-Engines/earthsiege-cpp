These mission handoffs were produced by the original Earthsiege 1.0 GO.EXE reader (flat 0x3276c) and writer (0x3445b), under four explicit fixtures: zero flags/zero RNG, zero flags/nonzero RNG, all-one flags/nonzero RNG and patterned flags/nonzero RNG. They are reference fixtures, not an assumption about the game's initial campaign state.

`script_checks.tsv` records the final RNG state and hashes individual tables captured from the original reader's in-memory records, independently of the writer's file layout. `condition_checks.tsv` holds original active condition records. `random_checks.tsv` captures 1,000 original generator outputs for each of nine seed states. `mission_text_checks.tsv` holds 2,223 checksums from the original text interpreter (GO 0x19453) under three counter states. Artwork and archive checks come from independent Python extraction of the supplied media.

The separate `earthsiege-reverse` tree contains the original-execution harnesses and their provenance. No emulator is linked into this program or its tests.

`model_math.bin` records 2,141 original `DBSIM` Euler matrix, short-point,
long-vector and inverse-plane calculations (0x5f0ac, 0x5e858, 0x5ef74,
0x733c0), captured by `earthsiege-reverse/tools/check_model_math.py`.
It includes exact quadrant/rounding boundaries and deterministic mixed angles.
The cosine constants retain all original entries, including four one-unit
rounding differences from mathematical cosine. Matrix storage is converted
explicitly from the original planar-first arrangement to column-major `mat3`.

`model_traversal_checks.tsv` contains 28,632 original child-render traces from
`earthsiege-reverse/tools/check_model_traversal.py`. It covers every supplied
BSP part, detail selector and cell-animation selector, with varied view points,
articulated matrices, frame counters, projection shifts and detail preferences.
The host replaces only child render calls with trace collection; all selection
arithmetic executes in the original methods (0x53240, 0x530b0, 0x52fd0).

`model_poses.bin` contains 116 default animated instances and 5,120 blended
poses/hierarchies from the original constructor and channel application
(0x61224, 0x62dfc, 0x61810). Cell states are zero, independently of stored cycle
lengths. The test excludes matrix padding and slots absent from the hierarchy:
BULLDOG root 5, for example, leaves nodes 7, 8, 11 and 12 unwritten. Filling
allocations with 0x55 and 0xaa confirmed those slots retain the supplied bytes.
The native implementation gives unused slots safe identity transforms.
Regenerate with `python3 tools/export_animation_fixtures.py ../earthsiege-reverse/analysis`.

`animation_channels.bin` imports 8,955 original channel traces, covering 98,505
post-request states plus each initial state. Original calls are 0x619f0, 0x6200c,
0x6265c, 0x626c8 and 0x62718, with accumulated forward/reverse motion retained.
The fixture also includes 5,120 original interpolated root transforms (0x61da8).
Destination-frame selection is -1 (any frame), the ordinary HERC request path.
Only unwritten matrix padding is excluded. Both animation fixture files are
produced by `export_animation_fixtures.py`.

`model_lighting.bin` contains 16,896 original intensities for model, terrain and
unoriented surfaces, plus 512 transformed lights from `check_lighting.py` in the
analysis tree. The reference retains the native incidence, attenuation, signed
wrapping and final byte conversion. Regenerate with
`python3 tools/export_lighting_fixtures.py ../earthsiege-reverse/analysis`.

`palette_shading.bin` contains all 144 texture translation tables from original
DBSIM 0x28b28, plus 32,256 ramp results independently verified against original
0x68be0 by `check_materials.py`. It covers all 18 supplied ramp-bearing palettes.
Regenerate with `python3 tools/export_palette_fixtures.py ../earthsiege-reverse`.

`projection.bin` records 3,200 model/terrain polygons through original DBSIM
0x5c5d4/0x75a28 and 0x5c9bc, followed by flat, texture-coordinate and shade
clippers 0x5cc4c/0x5ce10/0x5d06c. Original geometry and arithmetic execute without
drawing substitutions; captures stop before rasterization. Both parallel and
perspective paths are covered. Model equality at the near plane clips, whereas
terrain equality is retained. Nonstandard near/focal pairs deliberately retain
the original clipper's implicit unit projection at the intersection.
Regenerate with `python3 tools/export_projection_fixtures.py ../earthsiege-reverse/analysis`.

`polygon_raster.bin` records 5,999 edge tables from 0x76f54 and 6,000 viewport
clips from 0x788e4. Another 360 cases run the complete original flat-polygon path
0x68fdc/0x68be0/0x8aef8 against the research VGA aperture. Their whole-frame CRCs
check all 64,000 indexed pixels, including untouched background. Captures cover
both windings, screen crossings, degenerate edges and inclusive endpoints.
Regenerate with `python3 tools/export_raster_fixtures.py ../earthsiege-reverse/analysis`.

`texture_raster.bin` contains a test bitmap and translation table plus 600
complete original 0x534ac polygon framebuffer checksums. Both column (0x7394a)
and row (0x735a4) writers execute through the VGA adapter, including clipping,
Q16 rounding, transparent zero and translated zero. A single rectangle is used
for row-mode clipping; arbitrary cockpit clip masks remain integration work.
Regenerate with `python3 tools/export_texture_fixtures.py ../earthsiege-reverse/analysis`.

`gouraud_raster.bin` contains 6,000 emitted-band traces from original 0x78ccc
and 300 complete original VGA raster checksums. The original shades triangles
by splitting them into solid palette-index bands, including its tie rules and
integer rounding. Degenerate bands retain authored winding instead of being
automatically reversed.
Regenerate with `python3 tools/export_gouraud_fixtures.py ../earthsiege-reverse/analysis`.

`line_raster.bin` records 6,000 original 0x7d4a4 line clips and complete VGA
rasters through 0x6a064/0x8cb90. `bitmap_raster.bin` records 500 original 0x8c744
transparent bitmap scalings, including simultaneous top/bottom clipping.
Regenerate with `export_line_fixtures.py` / `export_bitmap_fixtures.py`, passing
`../earthsiege-reverse/analysis`.

`model_render.bin` records eight complete original frames for each of all 401
supplied model roots (3,208 frames), plus 512 retained animated channel poses.
Original resource readers, instance constructors, transforms, light calculation,
part/mesh BSPs, detail/cell selection, projection and pixel writers all execute
unchanged. Only file transport/allocation and VGA memory/ports are host
boundaries; isolated whole-object bounding rejection is disabled. Fixtures
include original data names, orientations, positions, pose inputs and whole
64,000-pixel checksums. No model data is replaced with test-only geometry.
Regenerate with `python3 tools/export_model_render_fixtures.py ../earthsiege-reverse/analysis`.

`model_bindings.bin` records all 16 HERC cross-detail binding sets, with 7,227
part and BSP transform fields checked after original 0x19ba0/0x19ac4 execution.
The original 50-byte row indexing, static -1 part behavior and restricted BSP
remapping are retained. Regenerate with
`python3 tools/export_binding_fixtures.py ../earthsiege-reverse/analysis`.

`animation_rigs.bin` contains 800 three-channel walking/torso/elevation cases
from complete original player actors. It retains their runtime sequence
priorities, all channel state fields, stable channel order, poses and matrices.
The captures execute 0x620c4, 0x614d4, 0x6168c and 0x61810. Regenerate with
`python3 tools/export_rig_fixtures.py ../earthsiege-reverse/analysis`.

`aim_control.bin` retains 13,440 original torso/elevation control cases across
all sixteen chassis, with multiple frame ratios, demands, previous rates and
limit angles. Original 0x149ac/0x14ba0 execute without channels/HUD side effects;
the full articulated side is checked separately by `animation_rigs.bin`.
Regenerate with `python3 tools/export_aim_fixtures.py ../earthsiege-reverse/analysis`.

`locomotion.bin` retains 11,520 original DBSIM `0x11afc`/`0x12038`/`0x14148` updates: eight playable chassis, four control traces and three frame ratios. Tests compare world position/orientation, throttle, gait/steering/playback rates, all fifteen channel words and flags. `direction_angles.bin` retains 4,104 original `0x63ca0`/`0x734a0` direction results. Refresh both with `tools/export_locomotion_fixtures.py ../earthsiege-reverse/analysis`. These are unobstructed normal-player fixtures; they do not claim complete collision response or damaged movement.

`scenery_collision.bin` captures original DBSIM `0x1fa9c`/`0x1fc5c` height queries, `0x1fccc` stepped segments and `0x20894` outer ray selection. All 140 stored collision fields are covered, with translated/rotated four-object scenes, strict broad-phase bounds, shortening after a hit and the original early exit below distance 500. Refresh with `tools/export_collision_fixtures.py ../earthsiege-reverse/analysis`.

`world.bin` combines original zone constructor `0x20238` results (106 files, 5,129 placements and metadata bindings) with all 24 independently decoded WLD descriptors consumed by `0x251c4`. Tests also construct each of the 98 unshadowed scenery worlds and check owned shape/metadata/collision references. The actual DOSBox mounted-volume list and original lookup `0x566b8` establish SIMVOL0-before-ZONES precedence, including distinct ZONE6/ZONE8 duplicates. Refresh with `tools/export_world_fixtures.py ../earthsiege-reverse/analysis`.

`scene_order.bin` retains 2,000 original `0x21bec`/`0x21c60` painter trees. The independent research reconstruction and C++ implementation preserve special-scenery-first insertion, endpoint comparisons, cached observer planes, integer vector scaling and in-order traversal. Refresh with `tools/export_scene_order_fixtures.py ../earthsiege-reverse/analysis`.

`distance_shading.bin` retains 4,800 original `0x25cb4`/`0x25f38`/`0x25d44` results across all 24 environments, using the original range initialization at `0x289e0`. Cases cover distance boundaries, all ten class slots, small/large model offsets and both ordinary/split-highlight palette ramps. Refresh with `tools/export_distance_shading_fixtures.py ../earthsiege-reverse/analysis`.

`scenery_render.bin` retains 288 complete original `0x21008` static-world frames across all 24 environments and mission-selected terrain/zone combinations. Both the `0x20f40` draw order and whole-frame CRCs agree. Captures execute original world/model loading and all scene draw instructions, with whole-object bounds rejection disabled as in the isolated-model fixtures; sky, ground plane, cockpit and moving actors are deliberately outside this capture. Refresh with `tools/export_scenery_render_fixtures.py ../earthsiege-reverse/analysis`.

`background_bands.bin` retains 2,400 original `0x26264` sky/ground cases across 24 environments. All 2,111 defined cases match complete horizon geometry and emitted rectangles/polygons; 503 also match every VGA pixel. The remaining 289 deliberately exercise the original zero-cosine divisor near vertical pitch, detected as a domain error in C++. Panorama and textured ground are disabled at their existing original switches. Refresh with `tools/export_background_fixtures.py ../earthsiege-reverse/analysis`.

`actor_placement.bin` retains all 300 compiled mission constructor captures plus four recorded demo starts, each with its exact SCRIPT.DAT and PLAYER.MEC inputs. It compares 8,176 actors and 4,368 groups against original `0x1e638`/`0x1cff4` results, including compact pool order, mount equipment mapping, authored member ordinals and last-group ownership. The 89 negative-formation positions are marked undefined rather than equated to captured allocator contents. Refresh with `tools/export_placement_fixtures.py ../earthsiege-reverse/analysis`.

`combat.bin` imports the 36 definition resources independently compared with original `0xbb8c`, `0x9930` and `0x94f0` by the research tools. Tests compare every typed component, dependency, sphere, weapon and projectile field, including the original clearing of weapon-tail word 19. The malformed HOVTANK.DMG is rejected, as by the original bounded reader. Refresh with `tools/export_combat_fixtures.py ../earthsiege-reverse/analysis`.

`weapon_assembly.bin` captures 32 original external weapons from all eight starting loadouts through `0xc31c`, `0xa308`, `0x12b58` and `0x2654`. It checks 808 nodes and all 10,513 stored mesh points, including invisible helper geometry and unchanged normals. The weapon wrapper receives the replaced body part's ID/transform; its owned shape remains distinct. Refresh with `tools/export_weapon_assembly_fixtures.py ../earthsiege-reverse/analysis`.

`armed_render.bin` records 384 complete frames from original weapon-wrapper and HERC model rendering: eight chassis, starting and empty loadouts, eight default views and sixteen recorded animated poses per loadout. It exercises body/weapon texture switching and shared skeleton transforms without world culling, firing or damage. Refresh with `tools/export_armed_render_fixtures.py ../earthsiege-reverse/analysis`.

`actor_detail.bin` retains 768 original `0x2b30` detail selections and full armed HERC frames across eight chassis, four minimum-detail biases, three quality scales and eight distances. Actual original lower-detail node binding and weapon placeholder replacement execute before drawing. Refresh with `tools/export_actor_detail_fixtures.py ../earthsiege-reverse/analysis`.

`herc_body.bin` retains 1,280 sequential original combined-control states across all eight playable chassis, using changing frame ratios and concurrent throttle, steering, torso and elevation inputs. Original actors are fully constructed; the player flag is cleared only to exclude HUD-following side effects. Physical controls, runtime priorities, authored root motion and all poses/matrices execute unchanged. This does not include collision, damage, AI or firing. Refresh with `tools/export_herc_body_fixtures.py ../earthsiege-reverse/analysis`.

`mixed_scene.bin` retains 64 complete original `0x21008` frames with scenery and armed player-roster HERCs from all four demo starts. It checks painter order and every pixel via whole-frame CRCs, including per-object palette binding and actor-level model detail. The harness installs the full DPL ramp table before per-object distance ramps, matching display setup; missing ramp setup must not be emulated as a null-pointer colour read. These captures explicitly select roster visibility and do not simulate activation or AI. Refresh with `tools/export_mixed_scene_fixtures.py ../earthsiege-reverse/analysis`.

`input.bin` combines 4,096 original `0x4c818` held-key accumulations, 3,000 `0x4c6ec` press/release events and 361 original `0x63f04`/`0x63fa4` time-helper results. It retains every held/released/consumed flag and all returned axes/buttons. Capture with `check_input.py` and `check_clock.py`; refresh with `tools/export_input_fixtures.py ../earthsiege-reverse/analysis`.

`camera.bin` retains 960 original `0x4b8` camera updates and 1,200 `0xa04` control updates over free/orbit/attached modes. Targets use actual constructed HERC/ground-object virtual methods. It covers pitch and distance limits, ground-clearance rollback/correction, attached-node composition and model-to-Euler conversion. `arcsine_table.h` retains original `0x5f54c` lookup samples after four-unit input rounding. Refresh with `tools/export_camera_fixtures.py ../earthsiege-reverse/analysis`.

`contact_motion.bin` records 1,920 sequential original `0x14844` collision responses across eight playable HERCs, including candidate positions/angles at each contact probe, resulting control state, all poses and matrices. Contact Boolean results, foot effects and collision sound are explicit fixture boundaries; native physical controls, save/restore and animation execute unchanged. `actor_contact.bin` retains 1,400 original `0x14174` actor contacts and 91 paired ramming dispatches (including native local part heights). `scenery_contact.bin` records 5,600 original `0x2075c` queries over all 140 fields, varying object transforms, input altitude and the original unused body radius. Export with `tools/export_contact_motion_fixtures.py` and `tools/export_contact_fixtures.py` from the sibling analysis.

`triggers.bin` retains all 1,841 original spatial-boundary, one-shot counter, repeated-entry and audience results from `check_triggers.py` (`0x1cb48`, `0x1cb94`, `0x1cc5c`, `0x1ccb4`, `0x1cd30`, `0x1f87c`). Refresh with `tools/export_trigger_fixtures.py`. Mission parsing and compiler fixtures also guard the counter-field rename.

`navigation.bin` retains 7,200 original decisions from `0x19604`, `0x1965d` and `0x11c80`: complete route/formation/actor inputs, resulting control requests and cursors, plus all generated avoidance probes. Final walking and scenery intersections are explicit boundaries, separately verified elsewhere. Refresh with `tools/export_navigation_fixtures.py` after `check_navigation.py` in the analysis project.

`herc_request.bin` adds 1,280 complete original normal-health body states using direct `0x12038` speed/steering requests, followed by the original aiming and root-motion calls. These are the prepared controls used by AI, distinct from player throttle accumulation. Export with `tools/export_herc_body_fixtures.py --requested` after the analogous `check_herc_body.py --requested` capture.

`damage.bin` records 5,106 original per-loadout damage updates and 180 accumulator boundaries from `check_damage.py`. Each scenario carries the original assembled definition, initial and final arrays/cells/consequences, disabled weapons and the exact component-choice draws. Effects execute in the original captures; the C++ component core receives those draws explicitly, so this is not a claim about full effect/RNG replay. Refresh with `tools/export_damage_fixtures.py`.

`weapon_controls.bin` combines 9,397 original weapon control cases and 5,186 shield/allocation cases and 2,048 reactor sequences. `check_weapon_controls.py` now additionally captures 1,200 common-update flag/cell/cooldown states and twenty setting presets. All callbacks execute original instructions; these fixtures do not claim firing or hit dispatch. Refresh with `tools/export_weapon_control_fixtures.py`.

`projectile_flight.bin` records 216 original bullet/rocket trajectories (14,895 updates) from `capture_projectile_motion.py`. It compares motion-table records, every intermediate position/angle/speed/age/cell, target retention, manual-input consumption and AI guidance flags. The target position accessor is an explicit boundary and the fixtures disable actor/scenery hits. Refresh with `tools/export_projectile_fixtures.py`. The independent JavaScript analysis check covers the same enlarged captures.

`hit_volumes.bin` retains 6,608 original `0x89d4`, `0x8ab8` and `0xfca8` cases from `check_collision.py`: individual volume boundaries, all stored collision sphere lists, and directional shield absorption. `tools/export_hit_volume_fixtures.py` imports the captures. Returned native depth-plus-one values are compared to the C++ optional distances.

`herc_damaged.bin` retains 2,880 original combined body states with light/heavy damage, forced collision recovery and disabled collapse. Native `0x12038`, aiming and root-motion methods execute unchanged; `0x13078` collapse damage and `0x4deb8` sound calls are explicit recorded events. All eight chassis reach their final collapsed pose, with exact event timing and complete hierarchical transforms. Refresh with `check_herc_body.py --damaged` and `export_herc_body_fixtures.py --damaged`.

`component_hits.bin` captures 8,192 complete original `0x9064` queries against eight starting armed HERCs in sixteen moving/aiming poses each. It retains assembled collision definitions and their native bounding spheres, randomized section presence, three-axis ray transforms and exact selected section/depth. The C++ test advances its own native bodies to obtain the hierarchy transforms. These are geometry queries, without damage/effect dispatch. Refresh with `check_component_hits.py` and `export_component_hit_fixtures.py`.

The component-hit and damage tests now construct their own loadout damage/collision definitions from the original resources and GL equipment mapping. `damage.bin` version `ESDMG002` includes the raw starting equipment selections; the assembled definition and starting arrays are expectations, not inputs to the damage simulation. `component_hits.bin` similarly checks the assembled collision list before querying the C++-constructed list.

The reactor cases in `weapon_controls.bin` run original `0x14ee0` through its reserve clamp, with weapon allocation supplied and recorded at its virtual-call boundary. They check signed word budgets, generation scaling, reactor damage priority, final shield charge and the reserve clamp. Capture with `check_reactor.py`.

`damaged_render.bin` retains 370 complete original post-impact HERC frames. Original damage results supply the reference renderer with cell indices and disabled weapon pointers; the C++ actor instead computes those states from the recorded impact and component draws before drawing. It covers intact, penetrated and detached states on all eight starting chassis from two angles. Capture with `check_damaged_model_render.py`, export with `tools/export_damaged_render_fixtures.py`. Debris and continuing smoke are outside these body-frame comparisons.

`weapon_banks.bin` (`ESWBANK2`) captures 34 original mixed-loadout constructors, 2,720 allocation passes and 1,360 complete reactor/weapon/shield sequences. All seventeen weapon IDs, empty and internal GL mounts, missile subtype selection, readiness, charge controls and disabled retention are covered. C++ uses the owned HERC actor and its actual bank. Capture with `check_weapon_bank.py`; export with `tools/export_weapon_bank_fixtures.py`.

`weapon_firing.bin` captures 1,160 original discharge calls and 1,272 projectile requests across every weapon ID. Original constructors, muzzle transforms, discharge methods and resource consumption execute; projectile creation is the explicit recording boundary. It compares all world muzzle matrices/angles/positions, barrel offsets, power, ammunition, cooldown and HUD/firing latches. Cases deliberately include values below readiness to check the discharge boundary independently of its caller. Capture with `check_weapon_firing.py`, export with `tools/export_weapon_firing_fixtures.py`.

`projectile_packets.bin` retains 270 valid bullet/rocket updates and 576 instantaneous beams from `check_projectile_packets.py`. The native constructors and discharge consumers run, recording the world-hit and beam-visual boundaries. Tests compare transforms, ranges, collision radii, scaled damage, effect tables, missile signatures, movement results and beam endpoints. Export with `tools/export_projectile_packet_fixtures.py ../earthsiege-reverse/analysis`. Motion and packet captures derive valid projectile subtypes from the loaded original definition table; unused bullet model slots 3–5 have no bullet definition and are excluded.

`hit_dispatch.bin` retains 1,200 outer actor-hit calls and 200 ground intersections from `check_hit_dispatch.py`. `world_impacts.bin` adds 3,360 combined original calls across all 140 scenery heightfields from `check_world_impacts.py`. Scenery geometry, original traversal, nearest companion selection and target-blocking decisions execute unchanged. Actor hit/damage, reactions, trigger activation and visual-effect construction are recorded boundaries. C++ checks exact event order, narrowed depths, enemy-hit latches and RNG, including a victim becoming dead during its hit callback. Refresh with `tools/export_hit_dispatch_fixtures.py` and `tools/export_world_impact_fixtures.py`, passing `../earthsiege-reverse/analysis`.

`herc_impacts.bin` retains the interception boundary from 1,152 complete original HERC impact calls in `capture_herc_impacts.py`, across all eight starting chassis. The capture runs original damage and visual effects; speech/cockpit notifications are recorded UI boundaries. The C++ test checks initial geometry, shield absorption, difficulty scaling, missile sensor state, received-damage accounting, warning latches and the resulting 178 direct/radial body-damage requests. It does not yet compare the complete later damage/effect sequence, which remains retained in the analysis JSON. Refresh with `tools/export_herc_impact_fixtures.py ../earthsiege-reverse/analysis`.

`effects.bin` retains 1,152 explosion constructors and 256 debris bursts (all definitions, 64 seeds) from `check_effect_creation.py`, plus 126 complete explosion animation traces / 3,046 updates from `capture_effect_updates.py`. Original factories, pools, model creation, light setup, sound requests and particle launch execute. Tests compare creation and final RNG, selected debris models, launch vectors, all primary/secondary cells, light intensities and expiry. Original definitions 16/17 read final-frame intensity 32 from the low byte of their adjacent 20,000-unit light-range field; C++ reproduces the alias explicitly. Export with `tools/export_effect_fixtures.py ../earthsiege-reverse/analysis`.

`herc_damage.bin` retains 2,304 full original `0x13b44` incoming-ray executions
from `capture_herc_impacts.py`, refreshed with `export_herc_damage_fixtures.py`.
The original body/shield/component/effect factories run unchanged. Only speech and
cockpit UI calls are host boundaries. The first 144 cases per chassis remain the
separate interception fixtures; 128 more aim at a weapon across random seeds and
both sides, followed by 16 player invulnerability/override cases. The C++ test
compares complete component state, shield/sensor flags, damage cells, ordered
damage/effect/UI/salvage requests and all 56 shared random words plus both indices.
The independent explosion/debris factories execute in the callbacks, so no random
draws are injected from expected results. Isolated bursts fit the original pools;
this does not test pool exhaustion, persistent smoke controllers or a full replay.

`debris_motion.bin` contains 960 original `0x64b8` trajectories / 41,576 updates,
from `check_debris_motion.py`. Native launches and signed-word/long boundary cases
run with four frame ratios, including zero; movement is deliberately unscaled.
`object_pools.bin` retains 2,400 original operations at capacities 1, 3, 40 and 150,
including allocation failure, active-list traversal, direct release and queued
collection. `check_object_pool.py` runs `0x638c8`, `0x6398c` and `0x63784`; object
destructors are recorded callback boundaries. Duplicate queued releases and the
original twenty-entry queue overflow are excluded. Refresh both with
`tools/export_effect_lifetime_fixtures.py`.

`effect_world.bin` contains 1,620 complete original mixed effect frames from
`capture_effect_world.py`. Original explosion/debris factories, geometry instance
creation/destruction, light controllers and pool reclamation all execute. Nine
schedules combine three ratios and three seeds. C++ compares update and retirement
order, every live state and primary/secondary cell, final shared RNG and reused
slot IDs. The three-light controller limit is exercised. Persistent smoke, actor
combat and effect rasterization are outside these explicit schedules. Refresh
using `tools/export_effect_world_fixtures.py`.

`effect_lighting.bin` records 456 original per-object selections and 3,250 live manager operations from `check_light_manager.py` and `capture_effect_lighting.py`. Selection uses source/cache lifetime and registration order, including zero intensity, saturation, reuse and the game initializer's 2000/10 attenuation arguments. `effect_render.bin` records 72 complete original VGA frames from `capture_effect_render.py`, with authentic effect factories/updates, terrain, textures, dynamic lighting and the independent flat-effect pass. Requests and update schedules are explicit fixture inputs, not a complete mission replay.

`beam_effects.bin` retains 52 complete original beam shapes, RNG states, lifetimes
and VGA frames from `capture_beam_effects.py`. All six types are included; ELF
cases stay within the original twenty-face index table. Refresh with
`tools/export_beam_fixtures.py`. `projectile_world.bin` retains 540 mixed original
bullet/rocket/beam frames from `capture_projectile_world.py`, using the real
constructors, virtual updates, destructors and common 160-slot pool. The world-hit
result is a controlled boundary. Every launch/retirement order, live flight/cell
state and shared RNG word is checked. Refresh with
`tools/export_projectile_world_fixtures.py`.

`mission_combat_test` additionally exercises all eligible weapons in each of the
four demo starting rosters against native scenery. It checks resource consumption,
projectile/effect creation and eventual reclamation and renders 256 combined
frames. This integration check is not an original full-mission replay.

`target_selection.bin` captures all inputs, decisions and 58-word/index RNG
states from 3,000 original `0xddf4` calls (`check_target_selection.py`). Observer
and group leader can differ. All native eligibility/scoring helpers and strength
assessment execute. `perception.bin` retains the 1,110 recognition, sharing and
visibility-cache cases from `check_perception.py`; fresh sight tests use empty
scenery, while sharing runs the real trigger/counter methods. Refresh with
`export_target_selection_fixtures.py` and `export_perception_fixtures.py`.

`controllers.bin` combines `check_controllers.py` (27 tables, 162 entries,
62 selections and 480 status results) with `capture_controller_dispatch.py`
(1,200 original outer dispatch calls). Behavior callbacks are explicit recording
thunks; state-entry calls execute original instructions. Callback translations
are invalidated whenever those thunks change. `engagement.bin` retains 3,600
patrol/search decisions and 256 flyer group entries from `check_engagement.py`.
Navigation, target scoring and HERC attack strategy are explicit boundaries;
target ownership, timers, controller entry and flyer broadcast run unchanged.
Refresh with `export_controller_fixtures.py` and `export_engagement_fixtures.py`.

`starting_damage.bin` retains 1,792 original NPC condition calls across all
sixteen HERCs and 384 original shell percentage restores across the eight playable
chassis. Effects and shared primary randomness execute unchanged in the capture;
C++ independently creates explosions/debris between damage draws. The capture
also probes unsupported enemy-roster restores for research; those are excluded
from the native shell contract (the Spider overreads its capacity table).
Refresh using `tools/export_starting_damage_fixtures.py ../earthsiege-reverse/analysis`.

`mission_hercs.bin` checks all 56 pre-tick HERCs in the four recorded worlds,
including source/group identity, side, waiting gate, pose, damage and shared RNG.
It supplies the captured initial RNG, not captured damage, to the native mission
constructor. Refresh using `tools/export_mission_herc_fixtures.py ../earthsiege-reverse/analysis`.

`ground_actors.bin` comes from `capture_ground_actors.py`: all nine base types,
both side banks and four initial conditions, with 288 complete untouched original
VGA frames. Sixteen additional distant radar views explicitly repair seven invalid
BSP transform indices in the original resource; those views verify the documented
native repair rather than allocator-dependent out-of-bounds reads. Refresh with
`export_ground_actor_fixtures.py`. `ground_combat.bin` contains 1,332 original
background/damage operations and 1,728 rays across intact, collapsing and wrecked
bases. Full animation transforms, cells, timers, damage, ordered requests and RNG
are retained. Effect factories, kill credit, mission counters and attached smoke
are explicit recording boundaries; turret target selection returns no target.
Refresh with `export_ground_combat_fixtures.py` from `capture_ground_combat.py`.

`turret_tracking.bin` retains 1,536 original `0x3058` tracking updates, comparing
errors, rates, angles and every articulated matrix. `turret_firing.bin` retains
3,072 complete `0x3364` ticks with controlled target selection/offset/speed inputs
and recorded bullet-construction boundaries. The original tracking, clocks,
leading, firing gates and muzzle transforms execute unchanged. Refresh using
`export_turret_tracking_fixtures.py` and `export_turret_firing_fixtures.py` from the
analysis captures with the corresponding names. Target-selection fixture version
2 adds 1,500 cases using the real base/flyer strength virtual callback (constant
class 1 and no RNG), retaining the previous 3,000 HERC-callback cases.

`target_geometry.bin` comes from `capture_target_geometry.py`: 1,536 original
virtual target selectors and 9,180 articulated component positions in all four
recorded mission starts, under varied placements and launch orientations.
`destruction_counters.bin` comes from `capture_destruction_counters.py`: 2,160
original group/actor counter notifications, including repeats and overflow.
Refresh both with `export_target_geometry_fixtures.py`.

`mission_flyer_frames.bin` comes from `capture_mission_flyer_frames.py` and covers
960 complete world navigation frames / 8,640 flyer states. All groups are active
on side zero; HERC controller bodies and audio are explicit boundaries. Original
flyer controllers, base updates, existing effects, geometry, perception and shared
randomness execute. The harness initializes the original behavior tables before
constructing actors. Refresh with `export_mission_flyer_frame_fixtures.py`.

- `herc_tactics.bin`: 3,200 original HERC attack/flank/face-off/base/flyer movement
  decisions. Obstruction entry, objective completion, locomotion and final aiming
  are recorded boundaries; all intermediate geometry, scratch state and call
  order execute. Generated by `capture_herc_tactics.py` and
  `tools/export_herc_tactics_fixtures.py`.
- `herc_assessment.bin`: 1,848 live-loadout combat ratings and component choices
  with complete RNG, plus 2,048 retreat decisions. The unused tenth output of the
  original nine-slot equipment-metric allocation is skipped in the capture.
  Generated by `capture_herc_assessment.py` and
  `tools/export_herc_assessment_fixtures.py`.

- `herc_weapon_choice.bin`: 2,448 original AI weapon choices across 34 mixed
  banks. Readiness, range and seeker callbacks execute; target shielding and
  mission events are recorded boundaries. The unused grenade's invalid original
  projectile selector is excluded. Refresh with
  `tools/export_herc_weapon_choice_fixtures.py` from the analysis capture
  `capture_herc_weapon_choice.py`.

- `herc_aiming.bin`: 1,792 original world-point/recentering updates and 6,368
  weapon convergence angles. `export_herc_aiming_fixtures.py` consumes
  `missions/herc-aiming.json` from `capture_herc_aiming.py`.
- `herc_target_aim.bin`: 3,072 original torso-cone/range aim-point selections.
  Refresh with `export_herc_target_aim_fixtures.py`.
- `herc_fire_control.bin`: 3,072 lead/accuracy/retention/fire-gate decisions,
  620 spawn requests and complete RNG. Selection, target speed, aim errors and
  spawning are recorded boundaries. Refresh with `export_herc_fire_fixtures.py`.
- `herc_aimed_body.bin`: 512 composed eight-chassis advance/next-control/aim/fire
  frames, 453 muzzle requests and complete articulated state. Contact and audio
  are explicit boundaries; selected weapons are replenished to isolate geometry.
  Refresh with `export_herc_aimed_body_fixtures.py` from the analysis capture
  `capture_herc_aimed_body.py`.

- `herc_strategy.bin`: 2,560 original strategy transitions with ordered selection,
  retreat, follower and component boundaries, real behavior entry and full RNG.
  Refresh with `export_herc_strategy_fixtures.py` from `capture_herc_strategy.py`.
- `herc_auxiliary.bin`: 3,072 travel/flee/skirt/ram decisions; retains scratch
  overlap, ordered movement/aim calls, target counts and complete RNG. Refresh with
  `export_herc_auxiliary_fixtures.py` from `capture_herc_auxiliary.py`.
- `herc_guard.bin`: 2,048 guard/drive-off decisions and ordered external consumers.
  Refresh with `export_herc_guard_fixtures.py` from `capture_herc_guard.py`.
- `herc_sensors.bin`: 4,096 original post-frame ECM/interference/seeker updates,
  including target-change latches, damage-window clocks and full RNG. Power,
  equipment queries, rating refresh and HUD presentation are explicit boundaries.
  Refresh with `export_herc_sensor_fixtures.py` from `capture_herc_sensors.py`.
