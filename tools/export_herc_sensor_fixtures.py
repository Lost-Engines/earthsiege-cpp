#!/usr/bin/env python3
"""Export original post-frame ECM and missile acquisition comparisons."""
import json, struct, sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((Path(sys.argv[1])/'missions/herc-sensors.json').read_text())
out=bytearray(b'ESHSN001')
def words(values):out.extend(struct.pack('<'+'i'*len(values),*values))
words([len(data['cases'])])
for r in data['cases']:
    words([r['selected'],r['player'],r['channel'],*r['counts'],r['ecm'],r['pulse'],r['ratio'],*r['position'],*r['target'],r['heading'],r['view'],r['kind'],*r['timers'],r['interference_timer'],r['ecm_timer'],r['damage_timer'],r['damage'],r['changed'],r['interference'],r['detectable'],r['target_ecm'],r['target_detectable'],r['alert'],*r['random']])
    f=r['final'];words([*f['timers'],f['interference_timer'],f['ecm_timer'],f['damage_timer'],f['damage'],*f['flags'],f['target_alert'],*f['locks'],*f['random'],r['hud']])
(root/'tests/fixtures/herc_sensors.bin').write_bytes(out)
print(len(data['cases']),'original sensor frames exported')
