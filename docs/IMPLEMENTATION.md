# Implementation record

The native game is being built from the supplied Earthsiege 1.0 data and original-code comparisons in `../earthsiege-reverse`. Original x86 execution is confined to the research tools; the game and C++ tests use native implementations and recorded fixtures.

Working components:

- An owned mission session joins the original terrain, player roster, animated bodies, mounted weapons and model-attached cameras. `--inspect-demo 1–4` exposes these starting states in the native window with held movement/aiming and free/orbit/attached camera controls. Its fixed 25 ms inspection updates are separate from full frame scheduling; it does not substitute inactive scenery or player-roster bodies for enemy activation, combat or AI.

- All seven original VOL archives, bounded resource lookup and independent payload checks.
- Indexed artwork, VGA palettes/shade ramps and DFN fonts; native 320×200 software framebuffer and square-pixel SDL2 window.
- Original main-menu artwork, dialog text/hitboxes, three-state button styles and caption plaque. Actions other than exit are still being integrated.
- Original free, orbit and attached cameras retain pitch/range limits, ground-clearance rollback, model-node attachment and inverse-table Euler extraction. All 960 original placements and 1,200 control-acceleration updates agree, including target-local focus offsets and distinct free/orbit steering signs.
- Native held-key latches preserve short taps, diagonal movement precedence, first-key aiming and Alt/alternate-mode scaling. All 4,096 original direction combinations and 3,000 press/release events agree. Shared Q8 frame-time helpers match 71 original period ratios and 290 signed time values; SDL routing and command precedence remain integration work.
- Original 56-word additive random generator, including signed bounds and wrapping seed warm-up.
- Source-mission compilation: conditions, late variant binding, coordinate sums, partial merges and reference remapping. All nine output tables and final RNG state agree with 300 original compiler cases under four explicit campaign/RNG states.
- HERC navigation requests now match 1,800 original route decisions, 2,400 formation-following decisions and 3,000 obstacle-avoidance decisions, including all four generated scenery/ground probes. Route closure uses point identity; moving-leader thresholds and equal-clearance steering retain their original asymmetries. Controller scheduling and firing decisions remain integration work.
- Spatial mission triggers preserve strict rectangle/octagonal boundaries, permanent latches, one-shot wrapping counter actions and all six audience modes. All 1,841 original cases agree. These wait for group/frame integration; the resource fields previously named delay/argument are now counter/counter_operation.
- Native mission-start construction decodes player rosters, compacts referenced actors, maps equipment through GL slots and applies group/route/formation placement. All 304 original starts agree on 8,176 actors and 4,368 groups; 89 original out-of-bounds formation positions remain explicitly undefined. This is pre-tick placement, separate from activation and controller simulation.
- Typed compiled mission handoff: points, headings, routes, areas, triggers, HERCs, flyers, fixed ground objects and groups. All nine tables match captured native in-memory records in 300 cases.
- Complete mission briefing/objective/debrief decoding, with counter-selected phrases matching 2,223 original interpreter results.
- Original BSP painter traversal, projected-size detail selection and cell-animation selection; 28,632 original child-render traces agree.
- Fixed-point Euler matrices and point/vector/plane transforms, with 2,141 original-code comparisons. Original cosine-table rounding is preserved.
- Typed 3Space meshes, materials, BSP trees, detail levels, cell animations, articulated poses and collision heightfields. All 47,575 objects in 44 resources agree with original-reader fingerprints across 23 field groups.
- Top-level HERC distance-detail selection preserves the original descending thresholds, quality scale and minimum-detail bias. All 768 original selections and complete armed frames agree across eight chassis and their full detail sets.
- Runtime HERC detail binding remaps lower-detail geometry/BSP nodes to the full-detail animated skeleton. All 16 original sets and 7,227 part/node transforms agree.
- Independent model instances with zeroed cell state, default poses and incremental hierarchical transforms. All 5,236 original default/blended pose references agree; stored cell cycle counts are distinct from live frame indices.
- Queued animation channels with signed playback rates, authored transition search, stop-on-arrival, reverse stepping and accumulated/interpolated root motion. 8,955 original traces (98,505 transition states) and 5,120 root-motion samples agree. Requests currently select any destination frame, matching ordinary HERC consumers.
- Owned multi-channel animation rigs preserve stable priority order, angle-driven phase selection and overlapping-node precedence. All 800 original walking/torso/elevation compositions agree on channel states, complete poses and hierarchical matrices.
- Owned HERC weapon assemblies preserve equipment-slot selection, mount offsets, helper geometry, body-node binding and independent weapon texture/cell state. All 32 starting external weapons match 808 nodes and 10,513 stored points. Complete rendering matches 384 original armed/empty-mount frames, including 256 animated poses.
- An owned HERC actor joins its body, loadout, damage, reactor, shields and live weapon bank. Damage updates model cells, weapon visibility/readiness and movement consequences together. All 5,106 damage updates run through this owner, and 370 complete post-damage frames match every original VGA pixel.
- Reactor generation preserves frame-ratio scaling, damage penalties, a 1,500-unit reserve and weapon-before-shield allocation. Its 2,048 original comparisons extend the weapon/power tests to 16,631 cases.
- Mixed weapon banks preserve all seventeen mechanisms, original mount order, empty/internal slots, missile subtype selection and initial energy charge. All 34 mixed loadouts, 2,720 allocation passes and 1,360 combined reactor/weapon/shield updates agree. Visible weapon cells follow the live firing state.
- Weapon discharge preserves articulated muzzle placement, independent pitch/yaw fallback, barrel offsets, ammunition/capacitor consumption and the original no-op grenade spawn. All 1,160 original firing calls and 1,272 projectile requests agree. Readiness and group/trigger policy belong to the controller.
- Spawned bullets/rockets own flight and visible cells; instant beams share their impact-payload interface. All 270 valid projectile updates and 576 beam calls match original hit packets and endpoints. Invalid bullet slots 3–5 are excluded: they have models but no bullet definition, and their weapon definitions use beams.
- Explosion/debris definitions and creation preserve original resource selection, launch vectors, light/sound requests and shared RNG. All 1,152 explosion constructors, 256 debris bursts and 3,046 combined primary/secondary/light animation updates match. Definitions 16/17 preserve the original final-frame light lookup into the adjacent range field. Owned lifetimes, particle motion and complete effect drawing are verified below; mission/audio integration remains.
- World impact dispatch preserves ground/scenery/actor order, immediate damage calls, triggers, post-hit reactions, nearest companion warnings and obstruction reports. All 1,200 actor cases, 200 ground impacts and 3,360 combined calls across 140 scenery fields match, including exact effect RNG. Actor damage/reaction and effect construction remain explicit boundaries in these combined fixtures.
- Owned HERC interception now connects difficulty scaling, front/rear shield absorption, missile detection/ECM effects, accumulated received damage and articulated body-hit selection. All 1,152 original cases agree, including 445 hits and 178 direct/radial damage requests. Complete damage, effects and salvage consumption has its separate combined comparisons below.
- Per-HERC combat definitions are now assembled directly from original chassis, mount and weapon resources. Weapon damage dependencies/capacities, inherited section parents, collision offsets and initial presence arrays match all eight original loadouts. The 8,192 component-ray and 5,106 damage comparisons use these C++-constructed definitions, rather than adopting captured assembled definitions.
- Articulated component selection now matches 8,192 original queries against all eight armed starting loadouts, including 728 hits. Native bounding volumes, animated node transforms, absent-section filtering and stored traversal order agree. Collision records are now named sections, reflecting their damage-array role rather than animation states.
- Projectile hit volumes and directional shield absorption match all 6,608 original cases, covering every stored sphere list. Tests retain strict octagonal bounds, overflowing word radii, stored-order shortening, front/rear choice and exhausted shields that still intercept a ray.
- Bullet/rocket motion, original cell clocks, update-count lifetimes, acceleration and guidance now match 14,895 sequential original updates across 216 trajectories. This includes moving targets, target changes, sensor/ECM eligibility, interference, held/released manual steering and AI-owned missiles. Flight exposes the original pre-movement collision boundary; world hit dispatch is implemented below; missile-camera handoff remains integration work.
- Weapon readiness, strict range bounds, capacitor priorities, ELF spin-up, signed cooldowns, firing cells, HUD latches and shield recharge/balance match 14,583 original cases. Charging stays per update while refire timers use the frame ratio; disabled retention, missing-slot early exit and fast-charge-induced discharge are preserved. Owned weapon banks, reactor allocation and projectile creation are implemented below; session scheduling remains integration work.
- Dynamic light ownership matches 456 isolated selections and 3,250 live operations, including source reuse, zero-intensity retention, saturation, registration order and both constructor/game attenuation settings. Complete effect drawing matches 72 original VGA frames across all eighteen explosion definitions, with debris, original texture banks, the separate ground-flash pass and lighted scenery.
- Owned live effects match 1,620 complete original mixed frames, including model cells, debris movement, retirement, original slot reuse and complete shared randomness. The three-effect-light controller limit is separate from the twenty-slot lighting manager. Complete world drawing is now verified below; mission audio and persistent smoke remain integration boundaries.
- Debris movement agrees on 960 original trajectories / 41,576 updates, including drag, spin, bounce, retirement and independence from the frame ratio. Fixed object ownership preserves newest-first traversal, capacity, slot reuse and deferred reclamation across 2,400 original pool operations. These components precede live effect/world integration; they do not yet imply persistent smoke or complete visual effect drawing.
- Complete HERC impact consumption matches 2,304 original incoming rays, including 1,597 interceptions, component/armour state, radial damage, detachment cascades, 80 weapon ejections, 48 salvage records, player invulnerability and all ordered effect requests. Original explosion/debris creation runs synchronously between component draws; the complete shared random state agrees after every impact. Attached smoke, physical ejected-weapon motion and group/UI notifications remain explicit world boundaries. These are controlled starting-state impacts, not a full mission replay.
- Component damage matches 5,106 complete original updates and 180 signed accumulator cases: armour overflow, weighted internal choices, last-in-first-out detachment cascades, damage cells, disabled weapons and sticky HERC consequences. The isolated component API retains explicit component-choice inputs; the owned actor now couples these to synchronous explosion/debris creation in the full incoming-impact comparisons.
- Typed weapon, projectile, component-damage and collision definitions match 36 original-reader-checked resources, including 17 weapons, 19 projectiles, 17 damage files and 18 collision files. The original malformed HOVTANK damage layout remains an explicit rejection.
- Damaged HERC movement now matches 2,880 additional complete original body states across all eight playable chassis. Light/heavy speed penalties, signed collision-recovery countdowns, disabled animation requests, collapse timing and event order are retained; collapse damage/audio run at an explicit callback before root movement.
- Named HERC movement/aiming profiles and native torso/elevation acceleration, averaged angular integration and angle limits. All 13,440 physical aiming cases across sixteen chassis agree.
- Original moving-body occupancy matches 5,600 scene queries across every height field; radius and altitude remain unused in that original consumer. Actor contact and ramming match 1,400 cases, including 91 damaging impacts and local articulated hit heights. HERC collision response matches 1,920 sequential updates across eight chassis, preserving saved skeletal state, reverse/clamped speed, second probes, unchanged Euler angles during rollback and direct stop reset during transitions. The inspection session now blocks on scenery and roster members; component damage consumes the retained contact dispatch separately.
- Prepared AI speed/steering requests share the native gait and aiming implementation with player controls while retaining the distinct absolute-speed semantics. Another 1,280 complete original body states agree across all eight playable chassis. Avoidance and damaged/disabled modifiers are applied by their separate consumers.
- Owned HERC bodies combine equipment geometry, runtime animation priorities, movement, torso/elevation controls and shared skeletal poses. All 1,280 original combined updates across eight playable chassis agree on position, angles, control state and every pose/matrix; the owned actor connects these to damage and weapon subsystems.
- Original normal-player locomotion couples throttle, gait transitions, stationary/moving steering and authored root movement. All 11,520 original states across eight playable HERCs and three frame ratios agree; another 4,104 original direction-angle cases agree, including low-word rounding without carry. Collision rollback and disabled/damaged movement now have separate original-backed consumers.
- Distance shading reads SHADES.DAT bindings and interpolates the authored WLD palette ramps, including split highlights, terrain/ground-target/effect distance rules and size-dependent offsets. All 4,800 original palette and cutoff cases across 24 environments agree.
- Original low-detail sky/ground gradients preserve horizon projection, viewport offsets, palette bands and rolled polygon/level rectangle paths. All 2,111 defined command sequences and 503 complete VGA frames agree across 24 environments; 289 captured original zero-cosine division faults are explicitly detected. Panorama, textured ground and clouds remain separate integration work.
- Complete static scenery drawing combines original placements, model transforms, distance culling/palette ramps and painter ordering. All 288 complete frames across 24 environments match every pixel and object order. The gradient background is integrated; cockpit masks remain separate work.
- Mixed scenery and armed HERC drawing shares the same painter tree, distance shading and original actor-detail selection. All 64 complete frames from the four demo starting rosters agree pixel for pixel. These explicitly visible roster actors do not substitute for mission activation or AI.
- Whole-scene painter ordering preserves the original two-pass insertion tree, authored scenery segments and cached separating planes. All 2,000 original mixed point/segment scenes agree, including equal/overlapping extents.
- Owned mission scenery loads the original terrain model set, per-shape metadata, palette, textures and environment descriptor. All 106 zone files and 5,129 original constructor placements agree; all 98 unshadowed zones construct successfully, with SIMVOL0 preceding ZONES as in the original mounted search list. All 24 WLD descriptors retain their complete fields and resource names.
- Scenery collision uses the original authored height fields, radius-dependent sampling and ordered ray candidates. All 38,080 height queries, 8,960 stepped segments and 3,360 complete scene rays agree across 140 fields (1,218 scene hits).
- Native ambient, directional and positional lighting, object-space light transforms and the original ten-light list. All 16,896 original intensity calculations and 512 transformed lights agree, including different model/terrain incidence rules and fixed-point wrapping.
- Original palette shade indexing and all eight texture-remap levels, generated from retained six-bit VGA colours. All 144 original remap tables and 32,256 independently verified shade lookups agree.
- Native model/terrain projection and near-plane polygon clipping, including texture coordinates and Gouraud shade values. All 3,200 original polygon references agree, including the original strict/inclusive near-plane difference and wrapping arithmetic.
- Original two-pass viewport clipping, side-dependent integer polygon edges and solid fills. Tests match 6,000 clip results, 5,999 edge tables and 360 complete original VGA polygon rasters.
- Native affine textured polygons in both column and row scan directions, including top clipping, signed Q16 rounding, inclusive spans and the differing transparency rules with/without a lighting remap. All 600 complete original VGA raster references agree.
- Native Gouraud shading as integer palette bands, preserving the original stepped colours and degenerate-band winding. All 6,000 original band-emission traces and 300 complete VGA rasters agree.
- Complete isolated model rendering with articulated transforms, part/mesh BSP painter order, detail/cell selection, side-specific materials, palette lighting, textures and billboards. All 401 supplied roots match eight original views each (3,208 complete frames); another 512 animated frames match, including integer-projection folds. Scenery and armed actor assemblies are verified separately; cockpit clipping masks remain integration work.
- Original point/line and transparent bitmap drawing: 6,000 clipped line rasters and 500 scaled bitmap rasters agree, including the original sampling and cropping quirks.
- Four bundled original demo starting states and their 9,300 input frames. Replay simulation is not implemented yet.

Remaining integration: audio, attached smoke and ejected-weapon motion; connect full controller scheduling, target/weapon selection and AI; implement flyer/ground actors and full mission outcomes; connect cockpit widgets, shell/campaign flow, sound and presentations. Complete TAP replay remains a later end-to-end check. The current native application is a scene inspector, not yet a playable game.

Some retained handoff fields have provisional names (`argument`, `initial_value`, `extra`, numbered trigger links) until their original consumers establish their roles. Tests compare their exact values regardless of naming. The original handoff writer does not truncate reused files, so the parser deliberately accepts stale trailing bytes after its declared tables.

`tools/export_mission_fixtures.py` refreshes reference fixtures from the research captures. `tests/fixtures/README.md` records the source routines and controlled campaign/RNG states. Do not treat zero campaign flags or the fixtures' arbitrary RNG state as a new campaign's real initial state.

The inspection mission now owns live projectiles and effects. Space consumes the
selected weapon's ammunition/charge, launches ordinary/scaled bullets, rockets or
beams, and dispatches terrain/scenery/HERC impacts with synchronous shared RNG.
Four demo starting missions pass combined firing, consumption, impact, rendering
and reclamation checks. Enter cycles eligible ordinary weapons as an inspection
control; the original weapon-panel/group-fire policy is still separate work.

The shared 160-slot projectile pool matches 540 original mixed frames, including
oldest-first updates, deferred reclamation, launch slots, impact call counts,
beam random consumption and all flight/model states. Missile animation clocks
are explicitly retained across slot reuse: the original missile constructor
leaves those bytes untouched, bullets leave them alone, and beams reset them.
Fresh slots start at zero in the controlled original allocation. Standalone
projectiles accept an explicit initial clock; no uninitialized C++ storage is used.
A full pool safely drops additional bullets/rockets where the original wrappers
would dereference a null allocation.

All six beam variants have original geometry and palette colours. Fifty-two
complete beam drawings match every VGA pixel, including near-plane clipping and
the original handling of projected zero-area ribbons. ELF consumes the shared
random stream for each interior/end pair; its original twenty-face index limit
is checked. Projectiles, HERCs and effects share scenery painter ordering.

Native target eligibility/ranking now matches 3,000 original selections and
complete RNG states, including player-led wingmate restrictions, objective
priority, retaliation, controller state, attacker sharing and stable tie order.
Recognition, contact sharing, trigger activation and the asymmetric visibility
cache match another 1,110 original cases. They are reusable simulation consumers;
the mission scheduler still needs to populate and advance their actor context.

Controller definitions and dispatch now match all 27 original records, 162 entry
traces, 62 order selections, 480 displayed statuses and 1,200 complete outer
dispatches. The dispatcher preserves reconsider/update/movement order, immediate
state changes, first-entry latches, stagger and suppression of one movement phase.
Patrol/search engagement matches 3,600 original decisions; target references and
attacker counts remain owned state. Another 256 flyer group attacks agree,
including dead-member retention, target propagation and per-member entry stagger.
Attack strategies, actual navigation callbacks and full world scheduling are
subsequent integration boundaries, not substituted by generic pursuit behavior.
