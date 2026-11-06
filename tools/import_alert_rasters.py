#!/usr/bin/env python3
import json,struct
from pathlib import Path
root=Path(__file__).resolve().parents[1];source=root.parent/'earthsiege-reverse/work/research/cockpits/alert-rasters'
cases=json.loads((source/'index.json').read_text())['cases'];out=bytearray(b'ESALT002'+struct.pack('<I',len(cases)))
for c in cases:
 out+=struct.pack('<h',c['message']);out+=(source/f"{c['message']:02d}-before.raw").read_bytes();out+=(source/f"{c['message']:02d}.raw").read_bytes()
(root/'tests/fixtures/alert_rasters.bin').write_bytes(out)
