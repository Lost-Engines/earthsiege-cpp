# Implementation record

The native game is being built from the supplied Earthsiege 1.0 data and original-code comparisons in `../earthsiege-reverse`. Original x86 execution is confined to the research tools; the game and C++ tests use native implementations and recorded fixtures.

Current integration: single missions and careers can launch, fly and return with
saved results. Arming controls and the static bay layers are connected; crew,
repairs/construction controls, cinematics, full environment presentation and
end-to-end replay verification remain. Later entries supersede the older
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
