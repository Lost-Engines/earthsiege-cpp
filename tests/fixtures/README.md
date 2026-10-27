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
