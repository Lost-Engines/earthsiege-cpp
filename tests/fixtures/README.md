These mission handoffs were produced by the original Earthsiege 1.0 GO.EXE reader (flat 0x3276c) and writer (0x3445b), with zero campaign flags and two explicitly specified RNG states. They are reference fixtures, not an assumption about the game's initial campaign state.

`script_checks.tsv` hashes individual tables captured from the original reader's in-memory records, independently of the writer's file layout. `condition_checks.tsv` holds original active condition records. `random_checks.tsv` captures 1,000 original generator outputs for each of nine seed states. Artwork and archive checks come from independent Python extraction of the supplied media.

The separate `earthsiege-reverse` tree contains the original-execution harnesses and their provenance. No emulator is linked into this program or its tests.
