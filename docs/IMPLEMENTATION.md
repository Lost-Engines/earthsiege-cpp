# Implementation record

The native game is being built from the supplied Earthsiege 1.0 data and original-code comparisons in `../earthsiege-reverse`. Original x86 execution is confined to the research tools; the game and C++ tests use native implementations and recorded fixtures.

Current integration: single missions and careers can launch, fly and return with
saved results. Arming, crew assignment, repair, salvage, construction and launch
warnings are connected to the original shell controls and static artwork.
Opening, travel and ending movies, original environment layers and workshop
dialogue, menu doors, credits and cockpit impact effects are connected. All four original recordings now agree on gameplay, complete visual RNG and
final return files. Full image and audio-stream equality remain under comparison. Later entries supersede the older
integration boundaries in this cumulative record.

Working components:

- Original player weapon-list owner matches 34 loadouts / 11,594 sequential updates, including 609 discharges and paired weapons, ELF spin-up, grenade trigger release, editable groups, charge controls and automatic selection. The device-independent player input router matches 4,096 original calls, retaining mouse origins, nonlinear response, target tracking, centering, camera/flyer routing and Enter edges. Connecting these owners to the live session/cockpit follows these independent comparisons.

- An owned mission session joins the original terrain, full HERC roster, animated bodies, mounted weapons and model-attached cameras. `--inspect-demo 1–4` exposes these starting states in the native window with held movement/aiming and free/orbit/attached camera controls. Its fixed 25 ms inspection updates are separate from full frame scheduling; it does not substitute inactive scenery or player-roster bodies for enemy activation, combat or AI.

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
- Original moving-body occupancy matches 5,600 scene queries across every height field; radius and altitude remain unused in that original consumer. Actor contact and ramming match 1,400 cases, including 91 damaging impacts and local articulated hit heights. HERC collision response matches 1,920 sequential updates across eight chassis, preserving saved skeletal state, reverse/clamped speed, second probes, unchanged Euler angles during rollback and direct stop reset during transitions. The inspection session now blocks on scenery and active HERCs; component damage consumes the retained contact dispatch separately.
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

Remaining integration: player target selection and weapon-panel controls, outcome presentation, attached smoke and ejected-weapon motion, cockpit widgets, shell/campaign flow, audio and presentations. Live autonomous HERC/flyer controllers and impact reactions are connected and independently checked below. Complete TAP replay remains a later end-to-end check. At that early checkpoint the native application was a scene inspector; the later shell integration supersedes that boundary.

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

## Authored HERC starting conditions

The mission session now constructs every referenced HERC in original source
order, then the player's roster, and applies final group placement after starting
damage. It retains group ownership, side and activation gates; waiting groups do
not appear, collide or receive shots. Player-roster-only rendering remains an
explicit comparison view. All 56 HERC starts in the four recorded missions match
original placement, damage arrays/cells, group state and final shared randomness.

Starting condition restoration matches 384 original shell-roster cases across
the eight playable chassis. Armour, internal and equipment percentage arrays now
have explicit names. Restoration preserves the original untouched equipment
internal values and does not prematurely derive damage cells or impairment flags.
The original writes a tenth equipment result beyond its allocated arrays; native
code preserves the saved field but operates on the nine valid slots.

Another 1,792 original NPC initial-condition calls match across all sixteen
chassis, including threshold boundaries, detached sections, ordered effects,
weapon disabling, salvage exclusion, collapse latch and shared randomness.
Effects execute during initialization, before formation placement. The Spider's
missing torso/elevation channels are optional, as in the original; its live
internal-damage storage still has 21 entries despite its twelve-entry capacity
resource. Shell restoration of a Spider is unsupported by the original shell and
would read beyond that capacity table; native code does not reproduce that read.
AI decisions and flyer ownership remain integration work.

## Fixed mission actors

The session owns all referenced bases, buildings, radar dishes and gun turrets,
with separate friendly/enemy static and animated banks and a shared wreck bank.
They render in the mission, participate in their original heightfield or circular
contact tests, and receive projectile damage. Background cell/radar animations and
the eight-stage destruction sequence run with their distinct original clocks.
Weak points triple body damage; damage equal to capacity does not destroy a base.
The collapsing intact geometry remains hittable until the final wreck transition.

Seventy-two original construction cases and 288 untouched original VGA images
match, as do 1,332 lifecycle operations and 1,728 rays through intact, collapsing
and wrecked bases. Ordered damage effects and primary RNG are compared with
explicit effect/mission-notification boundaries. Difficulty scaling is shared
with HERC impacts. Active turret target acquisition/firing, kill credit, mission
counter notifications and attached smoke are subsequent consumers.

Both animated radar exports contain seven invalid BSP transform indices in their
three distant detail levels: indices 2–4 with only two poses available. A native
probe confirms DOS uses the following animation-channel allocation as matrices.
The ground catalogue repairs only those invalid radar references to local-space
planes, leaving raw assets untouched. Sixteen further VGA views execute the same
explicit repair in the original renderer and compare the resulting pixels. This
is a defined repair of an out-of-bounds read, not a claim of matching its unstable
allocator-dependent result.

## Gun turret tracking and fire control

Gun-turret tracking matches 1,536 original updates, including local barrel-space
errors, signed squared input response, rate acceleration, pitch stops, wrapping
yaw and articulated channel phases. Another 3,072 original control ticks match
target reacquisition, heading-only target lead, alternating burst windows,
refire/cell clocks and both ordered muzzle requests. Target selection and target
state are explicit inputs to this owner; projectile factories remain the shared
world's responsibility. The live scheduler still needs to supply perception and
target context before enabling active turrets in missions.

Target ranking now also covers the ordinary actor virtual callback used by flyers
and bases: it always reports comparable strength, without consuming random draws.
This differs from the HERC's randomized comparison. The fixture retains the first
3,000 HERC-callback cases and adds 1,500 ordinary-actor cases, with complete RNG.

## Flyer actors and flight controls

Mission sessions now own every referenced flyer alongside their HERCs and bases.
Skimmers use their original model, ENEMY texture bank, single-section damage and
immediate removal on destruction. The shared projectile path includes flyers and
resolves owner groups independently of actor kind. Thirty-two complete original
VGA frames, 768 rays, ten cumulative damage steps and all 18 demo-world flyer starts
verify appearance, damage, effect ordering, random state and initial placement.

Flyer pitch/bank/turn/altitude controls and movement are reconstructed separately
from controller policy: 2,304 original steps and 4,608 obstacle queries agree.
Motion is per dispatch; only the avoidance timer scales with elapsed time. The
original zero-initialized obstacle-distance comparison is preserved and documented
in the research. Route/attack policy and active mission scheduling are now connected as described
in the mission integration section below.

Flyer route and formation navigation now matches 1,200 original cases, including
closed-route advancement, circling the final waypoint, full leader-matrix
horizontal formation offsets and speed requests. A further 3,072 leader attack
updates match all four attack phases, lead, steering, missile/bullet selection,
refire gates and RNG; 78 recorded projectile requests match their muzzle positions
and angles. Six actual original rocket launches verify inherited forward speed,
including signed-word wrapping. HERC launches retain their existing zero inherited
speed. These consumers are now connected to the mission controller/perception scheduler.

## Complete perception pass

The world perception pass now agrees with 2,304 original updates and 14,824
ordered visibility, orientation, view-cone and notification callbacks. Captures
include waiting/dead/disabled actors, the player-last scan, asymmetric visibility
clocks, shared sightings, contact expiry, trigger actions and full RNG state.
Line-of-sight results are controlled at the collision boundary; this establishes
the world policy, not yet its integration with all live geometry.

The ordinary HERC/flyer view cone preserves signed angle subtraction without
wrapping; static bases have an unrestricted view. Detection range comes from
the mission environment setting (30,000 or 60,000), independently of the
100,000 sighting and 120,000 retained-contact ranges.

## Mission options and starting counters

The third compiled-header word is campaign mode, not difficulty. The source
compiler and reader now name campaign mode, objective fields, unlimited weapons,
player immunity and actual difficulty separately. Existing compiler fingerprints
verify the unchanged binary layout. The inspection session now respects the
recorded weapon-power/ammunition and player-immunity settings and supplies the
correct difficulty to all three actor impact paths. Campaign mode suppresses
the optional cheats and selects the original campaign damage multiplier.

The shell's 100 signed mission counters are retained from the recorded starting
files, with counter 7 cleared on simulation entry as in DBSIM. This supplies the
state needed by spatial triggers and later objective reporting.

## Mission activity ownership

The mission activity owner now preserves original actor ordinals, group member
order, objective links, counters, activation gates, controller entry state,
contact knowledge and target references. All 128 group/trigger/perception passes
from the four recorded worlds agree with original execution, including callback
order and full random state. Controller bodies and sight-line results are
explicit boundaries in these integration fixtures.

Runtime combat categories are HERC/base/flyer, while the mission format uses
HERC/flyer/base; the two now have separate enums and an explicit conversion.
Controller wrappers also cross their callback slots: advance runs first, then
decision, using the previous update's controls for movement. Native callback
labels now express that order. The independent 1,200 dispatch fixtures continue
to pass with unchanged behavior.

The live session now dispatches flyer controllers through group activation,
route/formation navigation, avoidance, patrol/search acquisition and attack;
active turrets use their existing original tracking/firing loop. World perception
and power allocation retain the original phase order. Flyer/base destruction
notifies group and actor counters, fires the destruction trigger and stops dead
flyer movement. Hostile hits activate their contact triggers.

The original-world integration fixture covers 960 frames (8,640 complete flyer
states) across two recorded missions and three frame ratios. Groups are activated
on the same side to isolate navigation; HERC controller bodies and audio are
explicit boundaries. Original bases, existing effects, scenery queries, group
dispatch, area scans, perception and shared randomness execute. These are not
full combat replay comparisons. The existing firing/effect smoke check explicitly
disables unlimited ammunition and puts groups on the same side, so enemy fire
cannot invalidate its final projectile-retirement check.

Missiles retain the selected HERC component at launch and resolve its current
articulated position every update. Targets without a component use the original
HERC camera node, base target-height offset or flyer origin. All 1,536 original
target choices and 9,180 component positions agree. The selector's pitch/roll
ranking (without yaw) is preserved. These checks also exposed and fixed missing
time-zero animation evaluation during HERC equipment setup; some enemy chassis
have an initial aiming pose that differs from their raw model defaults.

Group/actor destruction accounting matches 2,160 original notifications, including
live/dead/disabled group members, repeated notifications, shared counter slots
and signed overflow. Group accounting precedes individual accounting. Counter
notifications are repeatable; spatial/contact trigger latches remain one-shot.


## HERC tactical decisions

The attack, flank, face-off, base-attack and flyer-attack policies match 3,200
original decisions, including scratch timers, movement/aim ordering and command
reset. Their obstruction and firing consumers remain explicit boundaries. The
base attack preserves two locomotion calls and the original negative far-range
constant rather than silently correcting it.

Combat assessment matches 1,848 original live-loadout ratings and aimed-component
choices, and 2,048 retreat decisions. Chassis base rating and internal-damage
penalties have named profile fields. Cached ratings now feed world observations
and refresh in the original post-perception phase after hit invalidation.
Ammunition availability, damage thresholds, attacker counts and shared randomness
are retained. The original component selector's use of the observer's damage is
intentional here. The unused tenth metric output beyond the original nine-slot
allocation is excluded from the research capture and never accessed by C++.

## HERC weapon selection

AI weapon selection matches 2,448 original decisions across 34 mixed banks,
including readiness, strict range limits, seeker locks, shield-aware scoring,
radar activation, the unarmed event and complete random state. The unused
grenade definition is deliberately outside this claim: its selector addresses
past the original projectile array and its original spawn factory does nothing.
It is not assigned a fabricated projectile definition. Target leading, convergence and fire-policy consumers are covered below.

## Articulated aim and fire control

Torso-cone and component-range decisions match 3,072 original calls. World-point
aiming/recentering matches 1,792 updates and 6,368 per-weapon convergence angles.
Original enemy accuracy, signed-word lead arithmetic, ELF retention and firing
gates match 3,072 decisions and 620 ordered firing requests with complete RNG.

HERC ownership now separates control preparation from physical advancement. Each
weapon owns its convergence correction; projectile creation consumes it with the
current articulated pose. A composed eight-chassis capture verifies 512 frames,
all hierarchy matrices and 453 projectile launches. Contacts, audio and actual
projectile construction are explicit boundaries in this fixture. The existing
91 checks pass after this split; the composed fixture is an additional check.
Full HERC mission dispatch and player target-lock/HUD integration continue.

## HERC strategy and auxiliary controllers

The strategy selector matches 2,560 original transitions, including target
reference counts, follower alerts, repeated retreat assessment, order resumption
and randomized HERC strength comparison. The shared ranking regression retains
all 4,500 existing original selection cases.

Travel, fleeing, skirting and ramming match 3,072 original decisions, preserving
ordered consumers, scratch state and RNG. Fleeing's original direction word
partially overwrites its timer; the port expresses that operation explicitly.
Ramming burst substeps re-read position and heading after each physical advance.
Skirting preserves the same-call scratch reset on a behavior transition.

Guard and drive-off policies match 2,048 original decisions, including candidate
priority, assigned-attacker weighting, anchor-relative movement, target clearing
and the completion check after firing. These verified policies precede full live
HERC dispatch; they do not yet constitute a completed playable mission loop.

The post-frame sensor owner matches 4,096 original ECM/interference and missile
acquisition updates, including distance-derived timers, channel-specific gates,
HUD lock state, delayed target-change consumption, damage-window reset and full
RNG. Power, equipment queries and HUD drawing are explicit boundaries in this
fixture; mission integration supplies them separately.

The mission session now dispatches live HERC navigation and combat controllers,
including route/formation following, guard, travel, retreat, skirting and ramming.
Movement consumes the previous controls; player preparation follows group
dispatch, and power/ECM/seeker updates follow perception. The composed navigation
path matches 960 original frames / 17,760 HERC and flyer observations, with exact
random state. These replace the earlier flyer-only frame fixture, whose omitted
HERC dispatch also omitted the shared controller-entry staggering. Player cockpit and outcome presentation remain unfinished; this is still not a complete playable game.


## Live hostile frames and retaliation

An additional 640 original frames from two hostile demo starts match 12,160
HERC/flyer states, including controls, articulated aim, controllers, movement,
power, damage, and shared random state. The navigation and hostile fixtures use
the research harness's initial RNG state, not a complete original demo replay;
standalone decision captures separately exercise varied RNG values.

HERC retaliation and its long timers match 4,096 original cases. Player wingmates
wait until accumulated damage exceeds 7,999 within a 30,000-unit window; other
groups can react immediately. The 10,000-unit retaliation cooldown and 40,000-unit
friendly-fire warning cooldown remain distinct. The HUD-target suppression flag
is separate from the contact-alert latch. Hit dispatch now supplies live target
references, group contact sharing, and the actual blocking object for skirting.

The hostile comparison also verifies that base-attack facing includes a gun
turret's rotating yaw, as the original virtual direction query does. Using only
the base orientation incorrectly changes the HERC's maneuver near the flank
threshold. Audio messages remain recorded events pending playback integration.


## Mission objectives and debrief decisions

The live session now advances player route events, reconnaissance arrival and
rescue dialogue, then evaluates mission outcomes after power/sensor updates. The
outcome owner retains polling and confirmation clocks; the shell still needs to
present the resulting prompts. Status checks match 5,122 original cases, debrief
grades 16,000 cases, and player objective updates 4,096 cases. A further 2,048
worlds verify group targets, escort safety, recognition, target routes and mixed
member damage, including original signed-word accumulation.

The original player stops before a repeated first point at the end of a closed
route; autonomous followers retain their separate route behavior. Rescue timing
uses the original signed longs 5,000, 5,000, -25,536, 0: the third delay expires
on the next update. The alert flag used in debrief grading is shared with contact
and hostile-hit notification, rather than being a separate kill counter.

A stronger composed fixture matches 880 hostile frames / 16,720 HERC and flyer
states using the original initialized, warmed random stream. It includes moving
projectile updates and reclamation, and checks outcome status, debrief grades,
prompts, both countdowns, and player objective progress. This exposed an omitted
projectile phase in the earlier research harness; the omission affected longer
runs once the first projectile arrived, and is now corrected in the capture tool.
Modal presentation and recorded player input remain outside this fixture.


## Integrated player controls and target handoff

The live inspection view now consumes the original player routing and weapon
controller. W/Alt+W cycles weapons, Enter/Tab cycles forward targets, R toggles
radar, and right mouse applies the original positional aiming response. Weapon
groups and capacitor settings use the original commands. Focus loss clears held
controls. External camera control continues the mission while suppressing fire.

The original cockpit target predicate matches 4,096 captured cycles. Three
composed 720-frame fixtures additionally match 41,040 HERC/flyer states, power,
weapon resources, convergence, target references, lock states and random state.
They include explicit cockpit target requests, paired fire, automatic tracking,
and a controlled manual-missile ammunition variant. Camera application, audio,
widget painting and selected UI callbacks remain declared capture boundaries.
These are controlled mission comparisons, not recorded-demo replay claims.

The HUD exchanges the selected target after the simulation and power phases.
Manual missiles share the player's control-axis pair, so consumption by a missile
also clears the corresponding movement/aim inputs. Releasing guidance retains
its one-frame activity indication before the weapon list advances. The original
actor-to-cockpit throttle exchange and complete instrument widgets remain active
integration work.


## Cockpit resources and clipping

All eight cockpit layouts retain their 425 original GAU words, with the same
selective parent-rectangle rebasing. All 32 view selections match the original
bitmap loader and selector, including the shared mirrored left panel. Chassis
palette banks and their repeated highlight colour match the native palette-copy
helpers. ED mask tails are retained, including the unused COLOSSUS bytes.

A further 96 complete original model rasters match through all 32 cockpit masks.
The native renderer uses horizontal texture spans in this mode. For solid
polygons, rectangular regions clip vertices before generating the edge table;
curved regions intersect the completed edge spans. Combining those operations
into a final pixel test changes edge rounding and Gouraud colour bands. Scene
clipping is scoped, so cockpit artwork and instruments can draw independently.
These captures isolate artwork and masked models; live instruments and complete
cockpit frames are the next integration layer.


## Live cockpit view

The inspection scene now starts inside the player's cockpit. Attached views
exclude the player's own exterior model and use the VUE projection and ED mask;
external cameras retain the inspection viewport. Forward, lower, left and right
artwork is available, with the original chassis palette. Instruments and view
transitions remain subsequent integration work.

All 32 projection centres match the original camera installer `0x5bf68`.
Side panels extend the forward image plane, with centres at x = -160 and 480;
they do not rotate the camera. Another 64 original full scenery frames verify
all 32 masks and viewport projections, including distance shading and scene
order. This capture isolates scenery from background and instrument painting.

## Cockpit instruments and presentations

Live throttle, reactor and shield gauges, weapon rows/groups, HUD targeting and
waypoints now use the original update rules and indexed artwork. Clock scheduling
matches 4,250 original frames; TTM playback matches 2,560 frames and all 123 decoded
programs. The inspection view runs the original decorative lamp scripts.

The shared STATUS/TARGET damage display now matches 1,200 original indexed frames,
including all eight player chassis, per-section damage, health-colour boundaries,
shield/death precedence, friendly/enemy/self labels and real ground/flyer targets.
The PDG reader retains all three views and mount records; diagram palette remaps
and the stateful target-name font follow the original. The parent MFD controls,
remaining pages and their live integration are still in progress. Full shell,
campaign presentation and audio remain required before this is a playable game.

MFD control artwork, all thirteen button bounds, page-dependent availability and
range cycling now match 472 original button frames and 72 range transitions across
all eight cockpits. This also establishes the original initial STATUS page and
30,000/80,000/120,000 scanner ranges; remaining page consumers are still being
connected.

Scanner rendering matches 480 original frames containing 25 contacts, including
strict range cutoffs, fixed-point bearing projection, friend/foe and actor-kind
colours, selection brackets, active/passive labels, disabled/dead/waiting filters
and blinking last-known enemy positions. Distance readouts preserve the original
integer conversion and four-digit padding.

FLASH COMM text and emitted command indices match 240 original frames covering
all six choices, selection highlights, disabled entries, and the EMCON/HOLD YOUR
FIRE alternatives. Shortcut-letter colours and row geometry follow the original
DFN labels. Command delivery to wingmates is a separate remaining UI consumer.

NAV MAP matches 288 original frames across all 24 environment variants. It uses
the original fixed overhead camera, map palette ramps, stored static-zone order
and five-pixel centre cross. The original map can be empty where the zone has no
terrain inside the viewport; it does not show dynamic radar contacts.

MISSILE CAM matches 94 original indexed frames and tracking transitions: readiness
and lock labels, live static scenery, explosion blinking, projectile retirement
and last-position impact detection. All player rockets update the watched slot;
only EO selection controls automatic page switching. The shared map shading keeps
original texture translations. These captures explicitly disable panorama detail;
shared high-detail backgrounds and live MFD/controller integration remain work.

The healthy parent MFD controller matches 600 original state and child-dispatch
transitions. This covers page changes, the two-stage repaint, strict 30-tick
STATUS/NAV/TARGET refresh, immediate scanner/camera updates, EO automatic page
selection and manual overrides, hidden/external views, range cycling and
communications scrolling past disabled choices. Damage-induced flicker and the
live mission bindings remain separate work.

Live mission bindings for the damage and scanner pages now match all 128 actors
in four original demo starts. This verifies actor identity, chassis/structure
labels, armour and internal fractions, side, position, activity, detection and
radar state. The dormant original last-position flag is preserved. Player rocket
launches expose their actual pool slot for the camera tracker; page rendering and
wingmate delivery still need to be connected to the live cockpit.

The live cockpit now renders all six healthy MFD pages through their original
mouse controls, refresh schedule and F1–F6 shortcuts. Navigation/scanner/camera
pages consume the actual mission, and STATUS/TARGET keep separate stateful name
fonts. The missile camera follows actual player rocket slots and checks effects
in original oldest-first order. Captures of all six pages across the four demo
starts can be reproduced with `capture_cockpit assets OUTPUT_DIRECTORY`.

FLASH COMM delivery is integrated into live HERC controllers. Verification covers
1,536 original receiver branches, 256 group-dispatch sequences, and 48 sequential
orders across four complete original starting worlds, including target ownership,
strategy, controller clocks, radio response IDs and gameplay RNG. Original
Alt+A/G/H/O/C/E/F shortcuts route through this path too. Radio responses are
retained for the pending text/portrait/speech consumer. The original group
dispatcher's uninitialized priority for neutral orders is explicitly represented
in the fixture; the application uses zero, as documented in README.md. Original
aimed-component scratch is compared only once an original write initializes it.

Validation at this checkpoint: full build and all 133 tests pass. Damage flicker,
radio presentation, complete input/view transitions and the wider application
remain integration work; the inspection mode is not a completed game.


The live MFD also now includes original instrument failure scheduling, OFFLINE
labels and the combined repaint/child-update behavior. Band and timing fixtures
cover 257 health values, 14,400 failure steps and 1,200 combined parent updates.
Radio queue/descriptor/consumer verification covers 1,152 queue operations,
880 resolutions (776 messages) and 1,920 consumer transitions. Documented original
short-shift and double-index queue defects are preserved; the commander's
out-of-bounds descriptor writes are represented safely, as explained in README.

Live acknowledgements now use original pilot identities, banner text, animated
portraits and opening/closing static, including hidden-panel progression. Exact
comparisons cover 37,280 SNC states from all 556 simulation timelines, 2,400
portrait phases, 600 talking/death rasters, 1,620 idle/static rasters and 32
banners. An integration check exercises all four demo rosters with the panel
visible or hidden, resolving actual speech resources and completing each reply.
The original unloaded death-frame offset is explicitly zero. Hardware audio is
not yet connected; lower-panel tactical controls, complete view/input transitions
and the wider application remain required. `capture_cockpit` also renders the
lower wingmate panel and a live form-up reply at several stages.

Validation after radio integration: all 144 tests pass (18.58 seconds).

## PCM control and original mixer

The native PCM owner now matches the original object, HMI allocator, stop,
volume, finite/infinite loop and completion rules in 2,560 stateful control
captures (594 sample submissions and 273 voice replacements). Positional gain
and pan match 6,144 cases; source coalescing matches 6,000 submissions.

The archived `tr8m.com` and `tr8st.com` modules execute their own generated mix
loops in the analysis fixture. Native signed-byte gain quantization, overflow,
saturating accumulation and stereo balance match 1,024 kernels / 262,144 source
samples. Another 9,600 comparisons match full DMA ring CRCs, loop counters,
source positions and delayed completion. The native mixer retains the original
128-byte stereo / 256-byte mono initial lead, guarded ring refill and fewer-than-
four-byte restart threshold. Descriptor pitch values are retained but ignored
without the pitch flag, exactly as in ordinary SoundManager playback.

Only ordinary unsigned 8-bit mono source data is needed by shipped PCM objects.
Manually supplied subrange loop descriptors have an uninitialized tail-length
word in the original: native subranges are bounded instead. A positional gain
query with master volume zero is defined as zero rather than dividing by zero.
Driver-specific ISA timing and analogue circuitry remain outside these checks.
Fixtures can be refreshed with `tools/import_audio_fixtures.py ../earthsiege-reverse/analysis`.

### Live PCM audio and world triggers

`mission_audio` loads the counted SOUNDS.STR catalogue, retains sample storage in the resource cache and drives the original unsigned 8-bit stereo ring mixer at 11,025 Hz from a 120 Hz clock. The underlying PCM stream remains available for comparison; live output now combines it with FM synthesis at 44.1 kHz. All 37 effects and four live wingmate acknowledgements produce non-silent output in the resource integration check. Radio's initial mouth-frame gate still determines whether a transmission requests speech.

World sound requests preserve launch/impact order. Beam impacts precede their sound even if the visual pool is full; ordinary projectile allocation failure remains the documented safe omission. Flyer actors share sound 16 as in the original, including a dead flyer's stop request. Collapse transitions request 30/41 and player contacts request 41. Footsteps use authored attachment local Y thresholds with retained latches; collapse uses world Z and its one-shot latch. The pure controller matches 1,800 original states, and complete articulated updates for all eight player chassis match 2,560 states with 279 sound requests. The original skips latch-pointer increments for absent attachment objects; the native controller retains that behavior. Live attachment allocation limits still belong to the pending smoke-controller integration.

Full regression after this integration: 150 tests passed. A 120-frame SDL mission smoke test passed with dummy video/audio devices. No listening comparison or full-game audio equivalence is implied by those checks.

The native mission also consumes original yaw/pitch servo requests at each aim update, target acquisition/loss/pulse requests at the sensor callback, and the engine loop gate before pause/physics dispatch. The two servo axes retain independent latches while sharing sample 33. The target pulse uses bit 6 of the 60 Hz clock; losing a target entirely retains the original no-callback path. Cockpit view 4 (external) stops engine sample 45, while cockpit views wait for startup sample 19 before starting it. All 6,000 original control states and emitted actions agree.

### Native HMP scheduler

The resource reader and timer scheduler match all 103,958 original event deltas, all 80 device-routing fixtures (1,020 track routes), and all 169,947 timed MIDI packets across 20 songs. The comparisons include source offsets and generated branch-restoration packets. Per-track elapsed clocks reset after each event; global loops rebase active tracks in the original order, restoring their programs/controllers. All supplied songs use explicit status bytes and only end-of-track meta events. Unused system/meta forms and zero-time infinite loops fail explicitly; the HMI library's separate fade API is unused by the game. SoundObject's loop filter is installed by normal MIDI playback and is now implemented; it is distinct from the uninstalled optional callback driver and the controller-119 application callback.

The HMI OPL2/OPL3 MIDI drivers are now native register-level implementations,
verified against 16,276 original commands and 223,115 register writes. This
includes the nine paired OPL3 voices, original bank conversion, sustain queue,
allocation, controller and pitch-bend differences. The original release-cache
overlap with frequency registers is preserved. Chip synthesis and the mission music owner are now connected.

The sampled-percussion MIDI driver and HMI gain stage also pass original-code
comparisons (10,000 percussion operations and 4,000 gain updates). Physical ring
slot removal, per-device rather than per-channel percussion gain, untracked notes
when the eight-entry bookkeeping budget is full, and direct-volume rounding are
retained. These drivers now share the effects mixer’s 32 digital voices.


### Live music integration

The complete scheduler/gain/FM/PCM pipeline matches 40 original song/device
cases: 257,910 register writes and 3,092 digital events. Combat selection matches
1,200 original states including RNG, fades, distance/recognition and the unusual
distant-flyer rule. The original SoundObject repeat filter matches 3,072 cases;
six complete-song tests cover zero, finite and infinite repeats over 23,000 ticks.
The game’s 255-repeat count applies to high-bit loop markers; unmarked branches
remain unaffected. Natural completion and explicit stop retain original cleanup.

`mission_audio` joins those pieces to the live encounter state and the existing
120 Hz audio clock. PCM music uses the same voice allocator and completion
notifications as effects. Unmodified Nuked OPL3 supplies 44.1 kHz chip synthesis;
the original 11.025 kHz unsigned DAC stream is held for four output samples, with
one source frame of transport delay, then combined at equal headroom with FM.
The analogue mixer balance and bus timing are not claimed to match physical ISA
hardware. The live default is OPL3; the independently verified OPL2 path remains
available to the music owner. The future settings interface must expose hardware
selection and volume preferences. The resource integration test exercises a
15-second calm/combat/calm transition with audible FM and shared percussion.

Validation after music integration: all 159 CTest checks pass (19.08 seconds),
and a 120-frame mission run passes with SDL dummy video/audio. The native music
checks include 44,444 timed packets through the real original repeat filter.
No hardware listening comparison is claimed.


### Persistent smoke and detached weapons

Live mission damage now creates the original 20-slot attached-smoke controllers
and 200-slot puff pool. Emitter attachments follow current owner/part transforms;
existing puffs update before new emissions. Family counts, nine-frame cells,
overshoot discard, deferred retirement and retained slot velocity agree across
12,906 original frames and 113,400 puff states. The cockpit, music and smoke
share the secondary random generator; combat retains its separate stream.

Physical weapon detachment uses the articulated mount, the MECHWPN2 template,
WPNTEX textures and original outward launch velocity. All 28 external-mount
cases / 808 subsequent updates agree with original captures. These bodies share
the existing 150-slot debris pool. The supplied smoke families are 0 and 1;
unbacked family 2 is rejected. Exhausted visual pools discard new objects safely
instead of following the original null-dereference paths.

### Lower-panel tactical map raster

The separate HDD map renderer matches 160 complete original VGA frames. It
retains north-up projection, point-table mission bounds, resource-order schematic
terrain cells, grid rounding, directional icons, building scaling and contact
visibility. Native ICONS.DBA's padded width is distinct from the height used for
its scaling threshold. Lower-panel control/lifecycle integration follows this
renderer; the forward navigation MFD remains a distinct implementation.

### Lower-panel controls and full damage pages

The tactical controller matches 908 original sequential states: command and
wingmate changes, strict five-pixel actor picking, pending/committed destinations,
zoom, pan and retained outgoing packet fields. Individual wingmate orders share
the verified group receiver. Original actor-order map contacts and route points
are exposed by the mission adapter.

The full structural/internal/weapon damage renderer matches 592 complete original
VGA frames across all eight playable chassis. PDG/GL/WEAPONS resources determine
weapon placement, padded mirrored bitmaps, component lists and colour remapping.
The fixture covers health thresholds, mixed damage, pagination and both missing-
target messages. Incremental transport and live lower-panel ownership remain the
next integration step. All 163 CTest checks pass after these components.

Arming now uses CFG compatibility/artwork tables and HOTSPOT.DAT directly.
Native comparisons cover all 45 mounts, availability from technology/inventory,
the retained current weapon, ordered equipment exchange and missile selection.
Clicking a mount selects it; arrow/list controls change equipment immediately.
Single missions load S_ARMORY.DAT and update the current pilot when changing
HERCs; careers keep unassigned HERCs selectable without assigning them implicitly.
The original base positions, mount paint order and static weapon sprites are
connected. Resident TTM bay animation and full condition-overlay painting are
still separate integration work. Missing Samson GL images are bounded safely.

The arming increment passes all 177 CTest checks. The paired POINTS/HERCS
buttons use the original shared sprite and label anchor.

### Crew assignment integration

The original 1610 event handler supplies 96 independent sequences / 3,360
transitions. C++ agrees on all four assignments, selected HERC, crew cursor,
pending swap source and displaced pilot, including repeated selection, NONE,
cycling through missing/construction slots, Accept and Cancel. The handler's
presentation controls and save/animation calls are explicit capture boundaries.
The UI test additionally exercises mouse/keyboard actions and persistence.
The original REGN1610 palette, PILOTS/INSIGNIA/MECHS artwork and native text
placement are connected. Arming's backward cycling from -1 shares the correction.

### Repair and salvage integration

MENU1900/1910/1920 and scrap confirmation 4444 now use the original records,
material accounting, condition-band rules and ordered equipment recovery.
Component edits match all 5,323 original events in 128 sequences; construction
records match all eight chassis and 128 original scrap/cancel outcomes.
The UI test covers damaged mission return, allocation/cancel/accept, policy,
scrap cancellation, Repair All and persistence. All 180 tests pass.
Native equipment quotes use mount indices, and Proceed can grant the selected
component another band without charge. Both quirks are retained. Negative pilot
indices are bounded instead of overwriting the original crew header.
Construction ownership, presentation animations and pixel-level shell widget
coverage still require integration; state parity is not full-screen parity.

### Construction screen

MENU2000 now uses original BLUEHERX artwork, specification text, required and
allocated material, completion percentage and button transitions. All 96
original event sequences / 1,424 transitions agree, including technology gating,
partial-build cancellation and the original failure to disable unaffordable
rebuilding immediately after cancellation. Eight constructor and 128 salvage
comparisons continue to pass, as do the 6,400 quote / 1,280 whole-repair / 25
construction-progress checks. The full shell test exercises build/cancel and
save/reload. Resident bay animation and the complete original launch-screen
sequence remain to be integrated.

### Launch eligibility and full bays

The bay and briefing map now show the original missing-HERC and unarmed-squad
warnings. Next Mission blocks a disabled player and permits confirmation for
disabled wingmates; construction checks all eight bays before reserving one.
All 2,048 captured original handler dispatches agree, including the cursor and
selected-HERC side effects. The exact disabled rule is either leg or the engine
below 50%; installed weapon condition does not affect the unarmed test.

Warning dialogs retain their owning screen, artwork and selections. The UI test
loads original-format saves into all five warning paths, exercises Cancel and
Proceed, and checks that blocked actions do not launch, advance or reserve a bay.
A career with no assigned player HERC can load into the bay for reassignment.

Validation: all 182 CTest checks pass after launch guards and shared panel/font corrections.

### Original shell animation interpreter

GO has a distinct TTM player from DBSIM's cockpit interpreter. The native shell
scheduler retains first-execution latches, inclusive random waits, the shared
additive RNG, primary/weighted branch priority, nested label offsets and repeat
cleanup. All 84 currently covered programs agree across 12,064 updates and
19,820 ordered drawing requests, with every RNG word compared.

The compositor loads original brush banks, preserves transparent/opaque and
mirroring flags, and commits FIX_RECT updates to a retained background. Another
4,662 original VGA captures match both complete pages (visible and background).
Those probes supply the frame restore boundary and omit only dirty-region
bookkeeping; original decoding, brush drawing and rectangle copies run unchanged.
This increment covers the brush/label/wait subset, not cinematic palette/screen
operations or the live shell owners. R4000A08 references absent ROSEY5 artwork
and is explicitly excluded rather than replaced with invented imagery.

### Ambient animation sound and body damage artwork

The GO sound-script handler now retains sound-enable/load-success gates, its
shared latch and randomized 10..34 volume. Authored VOLUME_SX is ignored in GO.
All 2,048 original updates agree, including 573 load attempts, 328 play requests
and complete shared RNG state. Playback is an explicit sink until connected to
the shell audio owner; the HMI mixer itself is already implemented.

Service, arming and repair views now draw original chassis damage layers below
mounted equipment. The six armour groups average their constituent percentages;
only severity bands 1..3 draw overlays, so exactly zero draws none. All 384
original visible/background frame pairs agree across the eight chassis and
condition boundaries. Overlay coordinates are signed 32-bit values after a
signed 16-bit sprite index, a correction also made in the analysis catalogue.
The five affected shell/animation regression checks pass.


### Resident service-bay scene

The live menus now run the 25 programs named by CFG/TM_FILES, retaining them
while moving between bay screens. Worker assignments, elevator positions and
construction changes match 128 original initializations and 512 subsequent
changes, including every shared RNG word. Animations consume the same random
stream as the original shell's later mission choices.

The complete compositor matches 1,984 original visible/background frame pairs
and all 25 interpreter states per frame. These include the eight completed HERCs
and all 17 partial-construction stages, original body/equipment ordering,
elevators, bay numbers, workers and welding. Static coordinate construction
0x25090 is part of the reference setup. The capture supplies page restoration
and excludes dialogs/sound; it is not a claim of whole-shell replay equivalence.

GO 0x45d24 creates a PIT clock with divisor 0x26d3; the TTM callback is registered
every two interrupts. Its approximately 60 Hz rate is confirmed in the original
DOSBox snapshot, where all clock pointers name the same initialized object.
The host supplies nominal 120 Hz timer ticks and advances bay programs every
second tick. The seven affected native checks pass, including live menu motion,
construction, save/reload and launch guards. Ambient playback, weapon preview
rotation and the remaining presentation owners continue separately.

Full rebuild after the bay increment: all 188 CTest checks pass (41.44 seconds).


### Shell music, workshop sound and weapon previews

The shell now supplies original region music and resident TTM sample requests
to the native HMI/FM/PCM pipeline. All 16 original music replacement cases /
848 ordered events agree, including enabled/disabled playback, the 100..1 fade,
period-two waits and 65535 loop count. Menu music and all six workshop samples
produce stereo output; preference changes and mission audio handoff are connected.

The capture executes GO 0x20a70 and its fade loop. SoundManager allocation,
playback calls and waits are recording boundaries. The current host advances
that fade over timer ticks while menus remain responsive; original blocking
input/presentation timing still needs the full presentation owner. These checks
do not imply end-to-end sound-card waveform equivalence.

Arming's W1800W00..13 and W1800R00..03 programs now run their original one-turn
animation and retain the final frame. Existing interpreter/raster fixtures
cover those programs, and the live shell test checks visible movement as well
as audible output. The normal control loads these eighteen programs; retained
G00/G01 resources are not substituted for the original selection behavior.

Full rebuild: all 189 CTest checks pass (20.38 seconds).


### Briefing maps and guided tour

The live briefing screens now use GO's terrain models, palette, icons and font,
with the original player formation, fitted views, opening reveal, objective/
waypoint tour, zoom, pan and latched object callouts. Returning from arming
restores the fitted camera while retaining pan. The separate shell cosine table,
bearing approximation and bitmap scaler preserve GO/DBSIM version differences.

Independent captures match 90 complete map frames across seven single-mission
categories and all eight campaign chapters; 3,069 tour updates and 62 sampled
frames; 39 opening frames/deadlines; 79 callout frames; and 500 scaler cases.
All 674 coherent launch fixtures also construct and draw their briefing maps.

Negative base point indices and B0_C6_7A's player formation -1 expose original
unchecked memory. GO's latter path reads uninitialized stack offsets, independently
demonstrated with three prior stack patterns. Undefined markers are omitted,
not replaced by guessed coordinates; these cases are outside pixel equivalence.
The opening reveal uses nominal 120 Hz ticks. Blocking original input/fade
boundaries remain presentation-owner work; the current host remains responsive.

Full rebuild: all 194 CTest checks pass (20.27 seconds).

### Horizon panorama

Native missions now draw the WLD-selected horizon strip, preserving heading
wrap, pitch/roll and original affine bitmap rotation. The shell horizon setting
is passed into flight. Low-visibility worlds have no strip.

All 576 original full background frames and bitmap placement sequences match
across 24 worlds. Another 96 complete frames match through the 32 cockpit
windows/masks. Clouds, ground marks and textured ground remain separate work.
Nonpositive horizontal panorama strides are detected rather than entering the
original unbounded loop.

Full rebuild: all 196 CTest checks pass (20.84 seconds). The live cockpit
smoke capture also renders the horizon through its original aperture.

Full rebuild: all 198 CTest checks pass (20.74 seconds).

### Cloud and ground-mark integration

Native missions draw clouds and procedural marks between the horizon and
scenery, using the original detail preference bytes and shared visual RNG.
Cloud initialization consumes eight inherited ring offsets even though its
derived distance callback is empty. Wind moves active clouds once per draw;
all slots recycle with hysteresis. Only active ground marks recycle, and detail
>= 240 retains their prior anchor/count. Both fields initialize at zero detail.

All 224 ground-mark frames match every pixel, point, anchor, count and RNG word.
All 144 cloud frames / 501 draws also match complete fields and pixels, including
both cloud faces and cockpit masks. All 198 tests pass. High-detail ground and
combined-layer comparisons continue; original frame scheduling remains separate.

### High-detail ground texture

The native ground sampler reproduces the original 24-bit wrapping coordinates,
heading/pitch projection, fifteen distance palettes, temporary scanline bitmap,
rolled bitmap placement and retained projection cache. It uses TRENCHES.64K
and the selected TEX/TEX_LO thresholds. All 480 original complete frames and
their scratch bitmaps match across the 24 worlds, including cockpit masks,
banks, projection shifts and viewport changes. The established background-band
check also passes. Flight integration and combined-layer checks follow.

### Complete environment composition

Flight now uses the original environment layer order and persistent fields:
sky bands, high-detail projected ground, panorama, clouds, ground marks and
pitch-dependent reflection colours. Shell detail settings reach that owner.
The reflection cache initializes after the cockpit palette is installed.

All 288 complete original frames across 24 worlds match indexed pixels,
complete visual RNG state and the 64..79 reflection palette. A separate
144-case palette capture covers pitch wrap, one-shot overrides and impact
colours; flight impact flicker and camera disturbance are not connected yet.
Original frame scheduling also remains separate.

Full rebuild: all 201 CTest checks pass (30.02 seconds). The live eight-frame
headless cockpit screenshot was inspected. The external bulk resets removed
some tracked integrations during this work; reflog inspection and revalidation
confirmed their recovery, now retained in commits.


### Shell dialogue and synchronized portraits

Commander briefings, the general's chapter introductions, launch remarks and
single-mission debriefs now run as modal presentations with original voices,
caption wrapping and synchronized mouth frames. Text-only, voice-only and both
preferences remain independent of music and ambient effects. Skipping consumes
the input; departure releases launch only after speech finishes or is skipped.

Independent original-code comparisons cover all 209 speech-list entries and
45,319 mouth states; 164 captions and 621 complete frames; 629 speech selections
with full RNG state; and 159 complete portrait/caption compositions including
resident commander animation and both retained pages. The live shell test covers
natural completion, multi-clip introductions, skipping, voice output with music
and effects disabled, and mission launch/return. Three native screens were saved
for inspection. The shell SNC format differs from the simulator's radio format.

Reference capture supplies clock ticks, page restoration and sound-device sinks;
it executes original selection, mouth, caption and TTM code. These are bounded
comparisons, not a claim of complete blocking-owner or sound-card equivalence.
Chapter travel movies, campaign mechanic response chains and remaining cinematic
owners continue next. Bay resident programs are currently paused during speech.

Full rebuild: all 206 CTest checks pass (51.90 seconds). Commander and general screenshots were inspected.


### Cinematic interpreter and palette transitions

The original screen loads, retained-page copies, drawing colours, vector lines,
palette selection and fade arithmetic are now implemented. GO ignores the
retained font-load/select tokens; it does not invoke the generic font path.
All 820 original updates across 18 opening, ending, chapter-travel and monitor
scripts match interpreter/RNG state, both complete drawing pages and 2,096
intermediate palette submissions. Cinematic lines share the verified original
rasterizer rather than the generic framebuffer line helper.

The fade range uses byte arguments and an inclusive end: an authored count of
256 changes only the first colour. More significantly, GO's FADE_IN path at
0x4933b does not initialize its temporary copy-region kind. Three independent
scratch patterns (0, 0x11111111, 0x22222222) appear unchanged at the copy dispatch.
The fixture supplies a zeroed scratch stack for this undefined field, and C++
uses an initialized rectangle. Intermediate image parity is bounded to that
explicit setup; palette computation and completed pages remain independently
compared. Live cinematic owners and hardware-dependent retrace pacing follow.

Full rebuild: all 207 CTest checks pass (19.36 seconds).


### Live cinematic and campaign-return owners

Opening logos/story, chapter travel, pilot death, victory and defeat now run in
the native shell with their original scripts and music choices. Input skips a
segment; skipping Dynamix also suppresses LEAD_IN, while IINTRO and EARTH_W still
run. --skip-intro matches the original -a boundary: opening and pilot death only.
The terminal Earth loop waits for input. Next Mission uses commander briefing
within a chapter, or mechanic/general/travel at the next chapter boundary.

Campaign return selects the mechanic's chain before automatic repairs, using
HERC condition, new technology, recovery and crew deaths. All 648 original
selections match the complete RNG state, including the one-choice technology
random draw. Zero-condition primary entry -1 is safely omitted. The new live
flow check covers natural opening completion, skip boundaries, ordinary return,
death, both campaign endings and a chapter transition. Original-format saves
remain isolated in the test's temporary directory.

The current host presents intermediate palette submissions at nominal 60 Hz.
Original VGA routine 0x70130 only waits until already inside retrace, allowing
multiple submissions per refresh depending on CPU speed. No wall-clock fade
parity is claimed yet. Bay residents during speech, general monitor transitions,
main-menu ANM playback and full owner scheduling remain separate work.

Full rebuild: all 209 CTest checks pass (20.06 seconds). Opening, chapter-travel
and damaged-HERC mechanic-return screenshots were inspected.


### Workshop animation during mechanic dialogue

Mechanic portraits now share the bay's actual visible and retained pages. Only
resident programs 3 and 6 pause, as in GO; other workers, elevators and welding
continue with the same RNG stream. Portrait cleanup returns both drawing pages
to the bay before its two programs resume. No guessed overlay masks are used.

The original capture runs eight chassis/speaker/clip combinations and compares
104 complete frames, including post-dialogue cleanup, both page CRCs and every
RNG word. All match. Existing bay, portrait, live speech and cinematic-flow
checks also pass. Original sound-device timing remains an explicit boundary.


### LPF menu doors

The two ANM resources are the closing (ESIEGE2) and opening (ESIEGE3) menu-door
transitions, not a looping main-menu background. GO 0x13aa8/0x13d9e/0x13c46
loads, locates and decodes their LPF pages. Its native counterpart now matches
all 60 captured complete frames, over both resources, two background patterns
and opaque/saved-screen modes. The final ring delta is excluded. Written zero
pixels restore the saved screen in transition mode; skip spans retain the prior
frame. ESIEGE3's page directory is not in frame order.

Normal standalone playback uses the header's 15 fps. Menu calls at 0x3c2a4 and
0x3c734 instead select background restoration and omit that clock wait; their
speed depends on decoding and VGA writes. Exact transition scheduling is separate
from the verified decoder. The two call sites request digital sounds 10/11.

G0_ON/OFF/STAT are retained monitor assets, but no caller has yet been found:
they are absent from GO's literal TTM names, the resident TM_FILES table and the
general's G0_FILES speech list. Do not assume the original plays these assets
merely because they exist. G0_SYNC is the verified general portrait program.


### Live menu doors and credits

The shell now plays the original closing/opening ANMs at menu-region changes,
including their digital sounds and saved-screen zero-pixel restoration. Input
is consumed during the transition; speech waits until the doors open. Skipping
the introductory movies still retains the closing doors. Warning dialogs retain
their parent's region and cannot accidentally start a blocked transition.

Options / Credits now runs CREDIT_A and CREDIT_B and returns through the doors.
Both original scripts match 12 captured updates and 310 palette writes. Their
unused GETTEXTSTART producer metadata is ignored as in GO. A single-mission
pilot death returns directly to mission selection, without a career death movie
or spoken debrief.

The host presents each door frame at nominal 60 Hz. The original menu calls use
saved-screen mode and omit the ANM header's 15 fps wait, so their wall-clock speed
depends on CPU/VGA throughput. No original wall-clock pacing is claimed.

All 213 CTest checks pass. Closing, opening and credit screens were inspected.


### Cockpit impact presentation

Player cockpit damage now triggers the original deadline/toggle/motion owner
synchronously from component damage. It consumes the shared visual RNG, swaps
the authored IMPACT1 palette and shifts cockpit artwork, masks and instruments
while keeping the world projection fixed. Forward/lower panels retain adjacent
artwork at the exposed edge; side panels use the original index-63 strip.
Reflection and shield colours update the currently selected palette.

All 5,005 original states agree across 50 sequences: clocks, palette ownership,
page origins, clip regions, widget translations and complete visual RNG. These
include repeated damage, palette/transition gates, view changes, zero sentinels
and clock wrap. A view change restores displacement and resumes with the original
ten-pixel range, while external view cancels it. Analysis retains complete
original captures for all four views and documents device boundaries.

All 214 CTest checks pass (38.28 seconds). Full-game timer/input scheduling and
native replay equivalence remain separate work.


### Career and debrief statistics

The career/history and mission return panels now use the original two CSTATS
fonts, WATEXT labels and pixel positions. History inspection reads its selected
backup without restoring it. The original MISC career field remains zero.
Return panels retain personal and squad totals, casualties, recovered salvage
and weapon count, and the final newly unlocked technology (or NONE).

Single missions now consume their original result records too: player chassis
conditions, all participating crew health/current/lifetime kills/ranks, and
squad totals. Temporary wingmate chassis disappear when the shell returns;
there is no campaign restocking, crew replacement or technology progression.
The unchanged launch MEC is distinct from the live post-return roster.

All 96 original complete panel rasters match. Twenty-four whole original
single-mission returns match the complete crew block, live serialized roster,
mission counters, casualties and squad totals. The full suite passed 215 checks;
the initial fixture-file mistake was corrected, and all three affected checks
(including the 216th check and live menu/history assertions) subsequently pass.
Original capture scripts and browseable panels are in earthsiege-reverse.

### Controls dialog reference port

The standalone GO controls policy now matches 36 original sequences and all
1,944 option changes, including reverse cycling, remembered joystick mode,
two-stick/pedal exclusion, calibration-time persistence and Done committing
preferences. The original CTL_ALRT resources render identically in 72 complete
dialog raster comparisons. These checks isolate the dialog from physical
devices and the host pointer. Live device integration follows separately.

### Live controls and joystick calibration

Options / Controls now opens the original CTL_ALRT and CAL_ALRT interfaces.
Right-click cycles backwards; Tab and Alt+Tab move focus by warping the pointer.
Done and Escape accept the control settings. Calibration uses the original
device selector, stages, masks, prompts, rollback behavior and file layout.
The hardware values come from SDL; calibrated response, device specializations,
keyboard/joystick priority and control-mode switches run in the verified core.

The new original comparisons cover 3,840 calibrated outputs, 12 device profiles,
22 calibration sequences, 64 complete calibration rasters, 4,096 mixed input
routes, 30 KEYJOY inputs and 80 hat request combinations. The live menu test
checks right-click, keyboard focus, actual calibration samples, disk persistence
and reload. An SDL virtual-device test covers physical-device selection,
two-stick fallback, hats, sliders, buttons and unplug, excluding the real
controller attached to the development machine.

Host joystick axes are mapped to unsigned 16-bit counts, followed by the
original seven-bit calibration and nonlinear response. The first two SDL
devices supply the two gameport pairs; one four-axis device can supply both.
Pedals use the second device's first axis, or the first device's third axis.
FCS and CH Pro hats are encoded into their original gameport representations.
CH slider input uses axis four, or axis three when only three exist. WCS uses
that lever as a modern host substitute for the original keyboard throttle.
These are documented platform mappings, not claims of electrical emulation.

Host calibration lives in data/host-joy1.cal and host-joy2.cal in the separate
save directory, using the original 64-byte format. DOS gameport calibration
counts must not be reused for SDL's different range. The original unreachable
CH slider calibration entry and cancellation's unsaved partial calibration
state are retained.

The supplied DATA/KEYJOY.CFG is now loaded. The original parser requires spaces
around equals and only sets reversal flags: a subsequent DEFAULT does not
clear a preceding REVERSE. It mistakenly uses a 28-byte token-table size as a
28-entry count; unsupported tokens can read invalid pointers. The native parser
reports those tokens safely. Original fixture captures record the failing
native instruction as well as successful parsing. The historical routing
fixture label missile_default was corrected to missile_reverse; the arithmetic
and binary fixture values have not changed.

The live cockpit uses the original device routing, Insert/button-2 mode switch,
third-button target selection, fourth-button stop/centre and mouse preference.
Keyboard Enter stays in command dispatch instead of being copied to the
third-button target signal. F9/F10 and hat requests honor their original view
gates. Hardware-panned animation and complete replay/frame scheduling remain
separate integration work.

All 225 CTest checks pass after the complete rebuild (31.89 seconds). The live controls screen was inspected at native pixel scale.

### Original live frame pacing

The host now advances the original 120 Hz timer independently of simulation,
including during mission alerts. Its wrapping fast/slow clocks and saturating
countdown match 3,840 original states. Live frames wait for at least two ticks;
ratios use the actual countdown difference and the original 64–450 clamp.
The minimum two-tick wait is about 16.7 ms, while the minimum simulated period
is 25 nominal milliseconds. This original discrepancy is retained instead of
assuming a fixed 40 Hz update loop. The host uses nominal 120 Hz, without PIT
electrical emulation.

Scene drawing only runs after a new simulation frame. Host presentation reuses
the previous image rather than advancing visual consumers and random state a
second time. Audio continues while mission dialogs are open. Window focus loss
still suspends time as a host convenience. The complete rebuild passes all
226 CTest checks, and a 40-frame headless mission smoke run completes.
Recorded playback bypasses the live pacing in the original; its integration
and whole-game parity are still separate work.

### Recorded input routing

The complete original 0x4cb14 playback path now matches all 9,300 supplied
packets and 2,048 independent post-read control contexts. Axes in the tape have
already passed keyboard/device mapping: rerouting them as live joystick axes
would be wrong. Playback repeats the physical button-2 mode latch, camera-axis
suppression, WCS override and backturn policy. Direct pointer fields remain
zero; recorded mouse events go to the interface queue separately. These checks
do not substitute for modal ownership or whole-game replay comparisons.

### In-flight preferences reference components

The original PRF_ALRT option policy matches 1,512 changes across 24 sequences,
including the three radio modes, byte-wrapping detail controls and partial
preference-file writes. Done saves slots 0–6 without committing the manager's
saved/previous arrays. Its standalone artwork matches 96 complete original
rasters, including striped detail bars, value labels and button states; the
mouse cursor is presented separately. Live F12/Alt+P integration is described below.


### Live in-flight options and detail settings

F12/Alt+P now owns the preferences modal and its nested Controls/Calibration
screens. World updates stop while host time and audio continue. Original 57820
saves the camera, selects an orbit distance of 2500 with zero initial orbit
angles, and uses the upper 320x111 viewport (projection centre 160,50).
Callback 577f8 requests a -64 turn through the original camera acceleration.
The native owner uses that camera path and restores the saved view on exit.
Preview redraws use the host's live pacer; the original hardware-panned
entry/exit transition and exact modal-loop timing remain replay integration work.

Simulator Controls and Calibration use different fonts, positions and value
backgrounds from the shell. The additional 72/64 complete simulator rasters
match, alongside the retained shell comparisons. Tab/Alt+Tab focus, reverse
mouse cycling, +/- detail changes, nested calibration and persistence are
covered by the application-level test.

Music and effects switches call the original independent gates. Disabling music
stops its player; while disabled, encounter selection consumes no music RNG.
The effects switch stops registered PCM objects, including active radio speech.
Radio mode separately gates future speech starts and text presentation. Merely
changing to text-only does not cut short an already playing phrase. Re-enabling
music invokes encounter selection again, including while options are open.

The Shapes tables match all 512 full/low-memory inputs to original 57c08.
Root-model scale/bias are independent from nested actor, HERC and weapon biases.
Weapon drawing resets the shared nested bias to zero afterward, as the original
does. Nested bias beyond a part's child count selects child zero rather than
rejecting the model. All 640 complete original armed-HERC frames match at every
pixel across both memory tables, threshold boundaries and four distances.
Live simulation uses the full-memory table.

The original has a persistence quirk: Controls Done commits the entire
preference manager but writes only control slots 7-11. Earlier unsaved flight
edits can therefore be marked saved without reaching disk; a subsequent flight
edit makes the outer Done write all slots 0-6. The native code retains this.
Twenty-eight nested original dialog transactions verify current, saved, previous
and on-disk values. The integration test also exercises real device samples,
calibration reload, independent sound switches and mission resumption.

Validation: all 234 CTest checks pass after the complete rebuild (25.37 seconds). The live preferences preview was inspected, and a 40-frame headless mission run completes.

### Original alert input

All nineteen Pause, Quit and mission-status dialogs now share the verified
original input policy. Escape selects response zero; Enter activates the focused
response (or establishes focus first). Tab and Alt+Tab move forward/backward,
and Insert advances only with WCS enabled. Combined fire has priority over next
button and keyboard input. Held physical buttons are suppressed on entry, and
focus changes warp the pointer to the original button centre.

The 1,600 cases in alert_input.bin execute the original dialog virtual handler
for both WCS settings, every focus state and simultaneous button/key cases.
All cases and the nineteen existing complete dialog rasters agree. This
establishes the dialog input rules; ownership of nested recorded-demo input
polls remains a separate whole-game integration task.

### Complete DEMO4 simulation comparison

The native mission owner now executes all 1,571 recorded DEMO4 input records,
with queued mouse events and the installed input settings (see the correction below). The original
runs one final zero-input frame at EOF using the previous outer frame ratio;
that frame is included. All 1,573 states (initial plus 1,572 updates) agree on
every actor's position and orientation, HERC steering/gait/throttle/aiming,
power, shields, damage arrays and flags, target ownership, mobile AI controller
and clock, group orders/route cursors, and all 114 bytes of gameplay RNG.

Inputs come from the original TAP, not the expected actor states. The reference
supplies only the separate observed presentation clock and EOF ratio at the
host boundary. Original captures use normal DOSBox execution at 60,000 cycles.
The fixture stores only compared fields and omits unrelated pointers/padding.

This exposed and fixed delayed TRACK exchange, external-view input routing,
wingmate command dispatch after world AI, and the HUD's separately cached target
handoff/validation. Pending cockpit changes now follow the original VGA-page
arbitration and cooldown; all 114 sequential original presentation states agree.
Held hats can no longer accidentally request an extra turn during an immediate
native view change. These are gameplay/presentation-ownership comparisons, not
pixel equality of the complete replay or its intermediate hardware-panned frames.
The remaining demos, modal tape ownership, full-frame visuals and audio timing
are still separate integration work.

Validation: complete rebuild and all 237 CTest checks passed (37.85 seconds), including the complete DEMO4 comparison.


### All supplied recorded missions: simulation state

All four unmodified DOSBox recordings now have a complete simulation-state
comparison: DEMO1 1,939 states, DEMO2 1,830, DEMO3 1,686 and DEMO4 1,573
(7,028 total, including each initial state). The expanded ESRPLY04 fixtures
also compare contact knowledge/visibility, perception timers, controller entry
and update latches, commands, attack requests and articulated aiming rates.
The aimed-component field is checked only where initialized and consumed;
original actor construction leaves that word as heap residue.

The TAP contains starting PREFS.CFG and KEYJOY.CFG, but original playback writes
those files beside the TAP path. Running -pTAPES/DEMOx still reads runtime
controls from DATA/PREFS.CFG and DATA/KEYJOY.CFG. The native owner therefore
uses installed settings. Each fixture includes the original installed 17-byte
preferences block; expected actor state is never an input to the implementation.

DEMO3 opens preferences twice. Its native modal consumes the one initialization
poll and subsequent recorded inputs until Escape closes each dialog: 54 packets
in total. The persistent playback button-suppression latches are retained.
The fixtures stop at the final complete simulation frame: the terminal Q
confirmation and its remaining polls are not yet claimed as a verified owner.
Whole replay pixels, visual RNG, synthesized output and exact audio scheduling
remain separate comparisons.

Integration findings retained in the faithful implementation:

- The ordinary mouse queue runs before world AI. MFD commands act immediately;
  HDD button callbacks only store the last request, dispatched during the later
  instrument update. Failed HDD panels retain requests other than page changes.
- Plain MFD keys select pages, contacts and orders. TRANSMIT uses the child
  panel's last painted choice. Its shared double-click latch suppresses transmit,
  including a same-row double click routed back through that callback.
- External camera control switches after player input when the view transition
  is prepared. Panel selection and control ownership are distinct states; both
  entering and leaving the external view require the original ordering.
- AI weapon selection at 18e50 passes a front/rear Boolean to angular shield
  getter 10e6c. Both 0 and 1 mean front to that getter, so the original always
  assesses the front shield. This matters when front and rear charges differ:
  at DEMO2 frame 1068 it selects the 5,000-speed projectile rather than the
  6,000-speed alternative. Damage absorption still uses the actual impact angle.
- The HERC obstruction probe runs from the actor toward its target; the original
  segment API takes destination before origin.

Validation: complete rebuild and all 240 CTest checks passed (34.71 seconds),
including all four complete simulation-state replays.


### Recorded exits and mission-return files

The ESRPLY05 fixtures extend all four comparisons to the original exit state,
including the partial final frame: 1,940 / 1,831 / 1,687 / 1,574 states
(7,032 total). All 9,300 source TAP packets are consumed by native simulation
and dialog owners, without skipping the terminal mouse interaction.

DEMO1 and DEMO3 open Q alerts 13 and 4 and choose response 1. The original
returns before player controls, perception or power updates on that frame.
DEMO2 opens automatic outcome alert 6 after those updates and also chooses
response 1. The native frame owner preserves both exit points. DEMO4 exits
through EOF. Original constructor/entry/exit observations are retained in
the analysis modal-ownership.json report.

All final RESULTS.DAT and MISSION.VAR payloads now match the original byte
for byte. This exposed a statistics mapping error: compiled actor tables use
HERC/flyer/base order, while runtime +162 and the three returned kill counters
use HERC/base/flyer. The existing combat_kind conversion now supplies that
mapping to defeat credit.

Coverage remains simulation, input ownership and return data. Complete replay
rasters, visual RNG and exact audio-event timing are separate work. The recorded
dialog callbacks run at original frame boundaries; live SDL modal scheduling
still needs the same suspended-frame integration.

Validation: complete rebuild and all 240 CTest checks passed (35.51 seconds),
including all four terminal-state and mission-return comparisons.

### Original cockpit scrolling

The original camera retains its angles for F9/F10; side panels extend the
forward image plane through their authored projection origins. A new original
24-frame camera capture confirms that the native resource reader's existing
projection rule is correct.

The retained 640x400 cockpit surface and 22904 scanout path now reproduce all
213 captured scrolling frames across six directions. The original cursor was
disabled through 66948 before each captured presentation so this comparison
covers the scrolling surface independently of its cursor overlay. Downward
scrolling advances five pixels at a time, overshoots the authored 197-pixel
offset, then corrects it; returning upward does not overshoot. Horizontal
scrolling advances ten pixels and retains its final repeated origin.

Live drawing now prepares the incoming panel before completing the transition,
retains both page images and exposes the intermediate frames to SDL. World
simulation remains suspended while they are presented; the sound clock keeps
running. The host uses 70 presentations per second for the VGA scroll steps.
The exact wall-clock cadence on different original hardware configurations,
reticle overlays and complete scene equality are not implied by the isolated
surface comparison. Existing complete simulation replays remain unchanged.

Validation: all 241 CTest checks passed (50.46 seconds), including all four
complete simulation replays and all six mission_view pan directions.

The live side-view smoke check exposed a separate environment-call error:
mission_view supplied a zero layout offset to the background owner. Original
25b68 passes afa9c/afaa0, the VUE instrument origin, to 26264 as well as using
the shifted projection centre. The native call now passes that authored origin,
fixing the uncovered sky on side panels. Existing environment component tests
already exercise the separate offset parameter; the missing link was its caller.

After the environment caller correction, all 241 tests passed again (27.04
seconds). A private X11/SDL smoke run exercised all six transitions; intermediate
and settled side/lower views were inspected, including corrected sky coverage.


### Live SDL modal suspension

The live host now opens Pause, mission status, program-exit confirmation and
flight options inside the same player/outcome callbacks verified by the full
recordings. World simulation stays suspended at that call site while SDL and
the audio clock continue. Options retains its original rotating camera preview.
Leaving a mission releases its owner only after the suspended frame unwinds;
continuing does not clear the sound queue.

Host input stops at a pending modal command until its callback opens, preserving
later queued keys for that dialog. In particular, a rapid P/Escape pair cannot
send Escape to the underlying inspector before Pause appears. Closing a modal
also ends that event poll before another event can destroy its suspended owner.

The private X11 smoke script tools/check_native_modals.py verifies a retained
pause image, options entry/return, mission-status cancellation, Ctrl+Escape
cancellation, a rapid Pause/Escape pair and confirmed return to the main menu.
Its screenshots are host integration evidence, not original pixel comparisons.

### VGA display expansion

Resource palettes, animated shields, environment reflection and shell fades now
share six-bit component expansion (v << 2) | (v >> 4), matching the retained
DOSBox VGA reference. The former v * 255 / 63 preview conversion produced
one-level RGB errors. This does not alter the original six-bit colours, palette
indices, lighting decisions or shade lookup tables.

The independent Python palette exports and original-reader cockpit references
were regenerated with the same hardware display convention. All 241 tests pass.
Whole-frame comparison also identified a separate startup fade/animation omission;
that is not concealed by this display conversion correction.


## Original cockpit launch sequence

The nearby-friendly-base branch at DBSIM 5b396/57ffc is now connected to native
mission launch. It uses the original INTRO.DBA lift door, lowers the view by
4,000 units, then advances 35 units per rendered frame (115 frames). The player
and world simulation remain stationary. A side-zero ground actor within 40,000
units selects this branch; other starts perform the original initial draw.

The 57cf4 dim palette and 287a8 fade preserve the original channel mixing,
fixed-point increments and final rounding remainder. Reflection is suspended
during startup. The 120-tick fade and 30-tick landing shake use the external
60 Hz clock; the lift itself is frame-driven. Their appearance therefore depends
on rendering cadence, as in the original. The live host uses its existing frame
pacer; it does not hard-code a recorded shake count.

During the landing shake the original keeps scanning out the last lift page
while drawing into the other page. Native presentation retains that behaviour,
including page origin displacement and restoration. Sound 33 accompanies the
lift, sound 41 starts on landing, and startup sound 19 begins after the sequence.
The scene inspector continues to offer immediate entry without the lift.

The unchanged DEMO1 original supplies 115 complete palettes and 34 landing
random/displacement states. Both the isolated controller and integrated
mission_view match them, including all 114 visual-RNG bytes and frozen gameplay
state. Three additional complete original captures have 35, 18 and 34 shake
frames respectively. The original screenshots are browsable in
analysis/cockpits/startup.html in the analysis tree.

All 242 CTest checks passed after integration. Frame comparison separately found
a missing second HUD paint above the lift door; that ordering is corrected.
Remaining pixel differences include the separately drawn software cursor,
instrument clock initialization and a transient hardware-DAC versus CPU-palette
difference. The controller comparison does not imply whole-screen equality.


## Radio, complete visual RNG and frozen options previews

The four ESRPLY08 replay fixtures now extend all 7,032 gameplay/exit comparisons
to the complete 114-byte visual RNG, including both index bytes, after every
captured frame. All 9,300 source inputs and the final RESULTS.DAT/MISSION.VAR
files still agree. Sources are the unchanged DOSBox runs
analysis/tapes/replays/demo1-radio-playback through demo4-radio-playback.
The observer records inputs at their actual consumer instructions; it does not
patch guest code or inject native actor, queue, portrait or random state.

Independent external inputs are the startup draw/fade/end clocks, interface
clock, speech-animation clock, and sound driver's success/failure return when
speech starts. Both clocks are nominally 60 Hz but have different phases.
The native live game uses its timer and PCM mixer; the replay supplies the
observed device result to isolate radio/visual behavior. This is not yet an
end-to-end equality claim for the PCM stream or the causes of driver rejection.

System alerts and pilot speech use separate queues. Native mission events now
reach them at their original call sites: contact, combat, selected-target,
shield and objective notices as well as orders. First-contact notification is
the HERC virtual override at 0e760; base/flyer observers use the empty 0e75c
method and must not decrement its cooldown. Individual orders cancel that
pilot's pending status request, rather than creating another status reply.

Original 59c08 updates portraits and the system queue before world rendering,
then advances the pilot queue afterwards. Options 57820/577f8 uses the same
render/interface path while simulation is frozen, including entering/leaving
the external camera and the interface-suppressed return preparation. Both
DEMO3 options dialogs now consume every recorded observation in this order.
The lift enters with draw/other pages 0/2 and finishes at 2/0; previously the
reversed pages changed later held-view arbitration and hid cockpit clicks.

Original speech playback failure is observable in the recordings. At 3e682
the failure sets both radio readiness and cancellation during the opening
phase. The queue removes the message but retains its starting latch, so later
messages can remain blocked until expiry. Native preserves that behavior.
Disabling speech is different: the original reports success to the portrait
while making no sound. No guessed hardware success is substituted in either
path.

Additional isolated comparisons cover 60 system-banner rasters, 15 targeted
status cancellations and 2,400 portrait transitions with an explicitly rejected
PCM start, alongside the existing 2,400 ordinary portrait and 1,920 radio
controller states. Whole-frame pixels, software cursor ownership and full audio
event timing remain separate integration comparisons.


## VGA palette device and upload ownership

The display now separates the CPU palette from the hardware DAC. Original
88684 reads the DAC mode and, in write mode, emits a zero before setting its
new colour index. Since normal uploads finish on an RGB boundary, this clears
the red component of the colour immediately after the preceding range. The
next upload need not include that colour. This explains the grey-to-teal
cockpit change after shield updates without changing the stored DPL palette.

palette_device reproduces the original 27f74 palette identity, unioned dirty
ranges and idle entry-zero upload. All 500 sequential original manager/DAC
states agree, including complete palettes, overlapping edits, source changes
and the next hardware write index. The original routines run unchanged in the
reference capture; the harness supplies VGA ports and explicit resource edits.

Integration preserves ownership: reflection changes 64–79; shield charge or
balance changes and cockpit-page preparation upload 80–85; the startup fade
uploads all 256 entries, including colours whose values are unchanged. Shield
uploads happen on the first dirty repaint, not every frame. A passive DOSBox
trace in analysis/tapes/replays/demo1-dac confirms the live upload ranges and
callers. At sampled replay frame 300, the remaining 13 differing RGB pixels
are the separately drawn software cursor. Other frames still contain radio
clear-region, marker, cursor and clock differences; this is not a claim of
whole-recording pixel equality.


## Original software pointers and retained radio clearing

The native window draws DCI sprites in the indexed presentation, using each
resource's signed hotspot and transparent pixels. It hides the host pointer
inside the game window. The cockpit selects its normal cross or green aiming
diamond using the original HUD input rectangle; the lower map uses the command
cursor state. Shell screens and flight dialogs use their own arrow resources.
Movie/startup and external-view presentation suppress the pointer. Presentation
adds the cursor after composing the retained scene, so it does not contaminate
cockpit pages, pan fixtures or gameplay state.

All 110 original cursor rasters agree, covering all eleven retained DCI resources
and ten positions including every clipped corner. The original 66948 loader and
VGA show routine execute unchanged. A private X11/SDL check additionally verifies
sprite pixels for aiming, cockpit controls, the clipped corner and a modal
pointer at scale two; screenshots and checks are in build/cursor-live.

Radio system-message regions retain their cleared colour-63 background after
expiry. Cockpit full repaint 2fa00 also clears this rectangle with no active
message; its coordinates follow the cockpit origin during scrolling. The old
native redraw restored cockpit artwork over that retained blank. This accounted
for 1,479 pixels in the sampled forward view after returning from the lower
panel. The original VGA writer was traced to 2fa72 and its subsequent page copy.
Sampled forward-view frames now differ by 13–146 RGB pixels, depending on the frame,
including reference cursor layers, clock phase and isolated world edges. The
replay-state comparisons remain exact; complete image identity is a separate
claim and has not yet been established.


## Cockpit edge controls and ordered audio integration

The cockpit now responds to the original three edge controls: bottom switches
to the lower panel, its top returns forward, and side strips enter/leave the
left and right views. Original inclusive bounds, corner precedence, directional
DCI cursors and release-button handling are preserved. All 1,365 original
hover/request cases agree. These controls were missing from the integrated
input path despite the underlying pan renderer matching its isolated fixtures;
DEMO1's recorded bottom-edge click now enters the lower panel.

World sound requests dispatch at their original call sites, preserving order
relative to UI, radio and effect requests. Radio opening/closing tones respect
the original already-playing gate. MFD/HDD buttons, shield and weapon controls,
weapon selection, options and alerts produce their original feedback.
Alert-input fixtures also compare the original click count for each response.

Original 58574 selects music before gating the cockpit engine, then flushes
positioned effects against the retained camera position before simulation.
The listener refresh keeps the original ten-call cadence and identity
orientation. Previously the native path refreshed after simulation against
the HERC's ground position, shifting distant engines' audible boundaries.

The four replay tests additionally compare 4,474 ordered non-engine requests
(1,328 / 1,500 / 1,052 / 594), including their frame and call order. All requests
are retained in separate ESREQ001 fixtures generated from the unchanged DOSBox
trace; expected requests are never fed into the game. Optional
EARTHSIEGE_REPLAY_AUDIO writes the native requests, driver acceptance and active
voice counts. Audio advances against the recorded outer clocks during replay.

Engine 16/45 starts depend on whether their prior sample is still playing and
whether the shared 32-voice pool can accept it. These requests remain in the
fixtures and analysis comparison reports, but are excluded from the exact
request assertion pending complete DMA/interrupt timeline comparison. The
third recording still has substantial flyer-engine retry differences; the
fourth differs in three frames. This is not a claim of complete audio equality.

The corrected lower-panel transition also exposes retained portrait/background
and button differences in full frames (about 4,100 RGB pixels in two sampled
lower views). The earlier 13–146-pixel results refer to sampled forward views.


## Lower cockpit composition and the original temporary brush bug

The original HDD painter 3cb90 stores a pointer to its local brush. Drawing an
occupied pilot via 3df60 replaces that pointer with another stack-local brush.
Restoring the window does not restore the brush pointer. The next empty portrait
fill succeeds, but 53d9c reuses that stack space for its rectangle; further empty
slots then see an invalid brush kind and retain the underlying artwork.

With one wingmate this clears slot two to colour 63 while slot three retains
86. With no occupied slots, all three clear. The native renderer reproduces the
observed slot behavior with ordinary owned values, without an invalid pointer.
capture_empty_portraits.py executes the original full painter and transitions
under all eight explicit occupancy masks; all twelve unused-slot rasters match.
The JSON records the brush words before and after the overwrite.

GAU offset 634 selects the pilot highlight style. All eight shipped cockpits use
style 1: the small lamp rectangles at 5fc/60c/61c, coloured 59 when selected and
57 otherwise. Style 0 uses portrait outlines. Native integration had always
drawn outlines; that added 654 incorrect border pixels in the sampled lower view.
Original 3eb3c is the style consumer; the lamp rectangles are distinct from the
portrait's inset interior.

The original front artwork overwrites the overlapping top of the lower artwork
on the shared VGA surface. HERCULES's lower origin is y=197, so its first three
visible rows come from DB0. This is now retained in direct panel rendering too.
The sampled DEMO1 lower frame 420 now differs by 25 RGB pixels rather than 4,109:
13 reference-cursor pixels and twelve lamp/clock-phase pixels. Frame 480 differs
by 110. This is a sampled comparison, not whole-recording image equality.


## Flyer engine destruction state

Original 1afa8 reads the live actor's byte at 93 before stopping or positioning
engine sound 16. The integration had read a cached decision-state flag, which
can remain stale until another controller refresh. It now reads flyer_actor's
actual destruction state; the replay audit also compares that flag against the
original on every frame. Gameplay, random state and return files still agree.

This reduced DEMO3's flyer-engine request differences from 376 frames to six,
and DEMO4's from three to one. Remaining start retries coincide with differences
in audio channel availability/completion timing; that timeline remains under
comparison. This supersedes the earlier substantial retry-difference count.


## Retained external cameras and world draw ownership

All four complete demo comparisons now include 24 camera components on every
observed initial/frame/exit state: 7,032 states in total. ESRPLY09 stores these
expected results separately from the TAP inputs. They are checked after native
execution, never supplied to the camera. The captures are
demo1-camera-playback-v3 through demo4-camera-playback-v3, made with the unchanged
game and passive DOSBox observer, sound enabled and startup/radio clocks retained.

Original 5894c changes camera ownership after player controls and before the
camera transform. 58a44 retains an existing camera for its target. The first
orbit starts at distance zero and clamps to 2,500 on update; later cockpit/orbit
switches retain distance, orbit angles and rates. The previous integration
recreated it at distance 10,000 and switched one presentation too early.
Player control routing also finishes after a Q dialog accepts exit; the outer
frame then leaves before perception/power. This includes the final camera-rate
update even though no further camera transform or world draw occurs.

The external viewport is (0,30)-(319,170), with projection centre (160,105).
Its rectangular clip region selects row-based texture traversal, just like the
cockpit masks. Using column traversal caused visible texture and cloud
differences. Original 58b68 places GREEN6X8 labels at x=50/215, baseline 189,
using STRINGS0 groups 14 and 32. Both control-owner label rasters match.

Original 59c8f skips environment and scene rendering while lower-panel view 1
is stable, and resumes it during the prepared transition. Advancing hidden
clouds on those frames had displaced the entire cloud field by the time the
player returned to the forward view. Native camera and cloud diagnostics are
available through EARTHSIEGE_REPLAY_DIAGNOSTICS.

A projected folded quad exposed an additional original column-mapper boundary:
with a 40x40 texture it reads offsets 2,215 and 2,805, outside 1,600 bitmap bytes.
The actual mission uses the row mapper, which stays in range for this quad.
capture_folded_texture.py isolates both original modes with sentinel guard
bytes. The native safety rule leaves undefined samples untouched; all defined
pixels remain exact. Four captures test this boundary, in addition to the 600
ordinary texture rasters. This does not emulate arbitrary adjacent heap bytes.

Whole-frame comparison remains separate from these exact state/raster checks.
Across 120 sampled frames, the largest differences in demos 1–4 are now
515/627/728/564 RGB pixels, down from tens of thousands in external views.
Cursor and independent clock-phase pixels are included in these counts;
weapon selection highlights and smaller scene differences remain under review.

Vertical cockpit pans reuse their existing pages and do not invoke the full
shield painter. Horizontal preparations and a return from external view do.
The original VGA upload routine first zeroes the next DAC entry's red component;
an unnecessary shield upload of entries 80–85 therefore changed entry 86 on the
following upload. This accounted for most of DEMO4's earlier 4,552-pixel maximum.
capture_view_palette.py observes unchanged original camera/draw/present routines
through 48 controlled transition frames; view-palette.json retains every palette
upload. The native correction removes that extra upload rather than patching
the resulting colour.


## Weapon display ordering and cockpit-engine transitions

Original 59c08 calls 2cf28 to paint cockpit widgets before 155a8/cf78 exchanges
weapon state. The previous native presentation painted the new selection and
capacitor values immediately. It now retains the values for that paint while
keeping the independently verified simulation exchange in order. An original
DEMO1 frame-1320 memory capture confirms that bank selection, charges and widget
state already agree even though the displayed highlight belongs to the prior
exchange. Repeated draw-only calls advance to the already exchanged values.

The replay tests additionally compare 584 original 18x7 weapon-name rectangles
in stable forward views. Positions are taken from live original widget objects;
eight rectangles near the recorded software pointer are excluded explicitly.
These are expected RGB outputs, never inputs to the game. Complete sampled frame
maxima are now 385/382/728/444 differing pixels in demos 1–4; these totals still
include reference cursors, clock differences and remaining scene differences.

Original 58574 gates cockpit-engine sound 45 on cockpit view 4, not on camera
ownership. During a prepared transition those change on different frames.
Reading the view state restores the original stop timing, including a frozen
options preview. ESREQ002 now checks 4,474 non-engine starts and all 2,599 stops
in exact combined frame/call order (7,073 requests), including engine stops.
Engine starts/retries remain separately reported because they depend on the PCM
completion and voice-allocation timeline: current differing-frame counts for
cockpit engine 45 are 17/5/1/10, and flyer engine 16 differs in two DEMO3 frames.


## Original transform-cache lighting behaviour

The model renderer now reproduces an original multiple-light cache defect.
528ab caches local transforms by articulated transform index, with index -1
representing the object root. A newly computed transform calls 5c140 and
transforms every active light. A cached transform calls 5d470; 5298c is a no-op,
so this is not a recursive push/pop stack.

Both 5d3d0 (save) and 5d470 (restore) repeatedly access the pointer at bdba6
without advancing through the registered light array. They therefore save and
restore the first light repeatedly. Later lights retain the local coordinates
of the most recently computed transform. Null model parts do not change this
state. Weapon replacements share the enclosing model's transform cache.

This explains a 300-pixel building face discrepancy in DEMO3 frame 840:
the world-space lights agree, but the original face has intensity 98 while
independently recomputing every local light gives 160. The native renderer now
retains the original stateful result. A corrected lighting implementation would
be a deliberate change for a future enhanced version.

capture_model_light_cache.py executes the unchanged original renderer for all
401 model roots with two additional light/view arrangements each, including
directional and positional lights. All 802 complete 320x200 raster CRCs match.
Existing single-light and armed-model comparisons also remain exact.

The passive scene diagnostics expose selected world-space lights, position,
distance and detail through EARTHSIEGE_SCENE_DIAGNOSTICS, optionally limited by
EARTHSIEGE_SCENE_FRAME. They do not supply expected state to the renderer.
The independent four-replay comparisons still agree on all 7,032 captured
states, 584 weapon-label rasters and 7,073 ordered sound requests. Across 120
sampled full frames the largest RGB differences are now 385/285/408/444 pixels;
these counts include pointer, clock-phase and remaining scene differences.


## Authored HERC ground models

HERC profile bytes 112–116 select FLAT.DTS roots; bytes 117–121 bind them to
articulated model parts. These are the feet/body shadows, not just footstep
sound markers. Constructor 11330 creates the flat objects at Z=-100000 until
their first update. Routine 144dc places each at its part's world-space origin
and copies the HERC's heading. Routine 11988 sets Z=0 when drawing, after the
ground-pass 30,000-distance test. The ground pass precedes terrain and actors.

The native mission now retains and draws those objects for all HERCs, including
the player's own shadow. Destroyed legs permanently lose their corresponding
attachments; destruction of a chassis configured to disappear removes all of
them, matching 13250. Explosion ground flashes share the pass and retain their
existing update/draw behaviour.

The extended ESFOOT02 fixture checks 1,800 unchanged original attachment
placement, heading and footstep/latch cases. The four independent replays still
match all captured states, label rasters and sound requests. Across 120 sampled
full frames, largest RGB differences are now 159/285/108/444 pixels. The former
large DEMO1/3 external-view discrepancies were these missing ground models.
The full 249-test suite passes.


## Original whole-object visibility

Original 529bc rejects a model using its stored root centre/radius before
drawing children. Its integer FOV expansion, near-plane test and three-pixel
screen margin now match 4,096 independent original classifications. The native
mission enables that check. Standalone component-raster tests retain their
explicit original no-object-culling setup.

All SMOKE.DTS outer bounds have radius zero. This makes the original hide a
puff when its centre leaves the rectangle, even while its expanded bitmap would
overlap the viewport. DEMO4 frame 1260 contained nine correctly positioned
native smoke objects that the original rejected this way. Their lifetimes and
positions were already correct; the missing object test caused the difference.

Fourteen additional replay raster fixtures cover eleven complete 320x200 frames
and three scene regions (sky, an animated ground shadow, and the smoke boundary).
They are captured RGB output, never simulation input. The regions avoid the
original software cursor. These selected exact comparisons do not imply that
every frame is identical. Across all 120 sampled frames the largest remaining
RGB differences are 159/285/108/137 pixels in demos 1–4.

All 250 tests pass. A private SDL-window run also verifies pause freezing,
options, status, quit cancellation and mission return with isolated saves.


## Retained cockpit instrument painting

Original 59c08 paints the widget container (2cf28) before 155a8 exchanges
reactor, shield and speed values for the next paint. The native renderer now
retains those values separately from simulation state. A second draw without a
simulation update uses the already exchanged values. Throttle hit testing also
retains the painted handle while a programmatic adjustment changes its pending
position.

The unchanged DEMO2 snapshot after frame 1020 shows shield values 374/374/512
with the old six DAC colours. Calling only the original widget painter changes
those colours without advancing simulation. DEMO1 frame 60 similarly retains
painted speed 921 while its newly exchanged value is 1024.

Speed lamps additionally retain their bitmap contents. The original incremental
bar setters paint the interval between old and new boundaries inclusively; an
increasing and decreasing speed can therefore colour the shared boundary pixel
differently. Raptor's horizontal full painter 3bda8 resets the forward bar cache
twice instead of resetting the reverse bar. Its resulting retained pixels and
next-update behaviour are reproduced, rather than recomputing a fresh bar.

The independent ESTHUP01 sequence fixture covers 2,720 original incremental
and full repaint cases across all eight cockpit layouts. Four replay fixtures
add 579 original reactor/throttle regions and shield-colour samples, excluding
13 regions near the original software cursor. Six shield points deliberately
avoid the separately clocked red lamps. These expected pixels never feed native
simulation or rendering.


## World-selected ground-effect models

World descriptor word 18 selects FLAT.DTS when zero and FLAT2.DTS otherwise
(original loader 6cf8). The mission effect owner now uses that selector for both
HERC ground attachments and secondary explosion models. Previously it always
loaded FLAT.DTS, leaving bright orange shadows in worlds that require FLAT2's
palette index 224 instead of 240.

Three independent original DEMO2 regions cover the corrected shadow in the
centre and at both viewport edges (frames 900, 960 and 1740). They match exactly;
the largest complete sampled DEMO2 difference falls from 195 to 90 RGB pixels.
These regions join the existing original replay scene fixtures.


## Ground projection at lookup-table boundaries

Original 28fbc builds a negative-pitch rotation with 694b8 and transposes that
same quantized matrix with 63df0 for the inverse. Recomputing a positive-pitch
matrix instead changes the result at the cosine table's rounding ties. This
caused 61 different scratch-texture pixels at the DEMO3 frame-1320 camera pose.

The native ground projector now transposes the existing coefficients. The
independent capture_ground_replay_precision.py fixture checks 66 complete
original ground frames and sampler bitmaps, covering that replay pose and
neighbouring positive/negative rounding boundaries at several headings.
All agree. The complete sampled frame now differs by only three RGB pixels;
that count includes other scene elements and is not a whole-frame equality claim.
