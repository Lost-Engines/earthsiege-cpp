#!/usr/bin/env python3
"""Import original executable radio captures; no expected values are synthesized."""
from pathlib import Path
import sys
root=Path(__file__).resolve().parents[1]
source=Path(sys.argv[1])/'cockpits'
for name,magic in [('queue',b'ESRQUE01'),('descriptors',b'ESRDES01'),('state',b'ESRSTA01')]:
 data=(source/('radio-'+name+'.bin')).read_bytes()
 assert data[:8]==magic
 (root/'tests/fixtures'/('radio_'+name+'.bin')).write_bytes(data)
 print(name,len(data),'bytes imported')
