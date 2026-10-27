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
