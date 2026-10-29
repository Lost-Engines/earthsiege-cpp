#!/usr/bin/env python3
"""Import original executable/archived mixer captures without synthesizing outputs."""
from pathlib import Path
import sys
root=Path(__file__).resolve().parents[1]
source=Path(sys.argv[1])/'music'
for name,magic in [('herc-steps',b'ESHRST01'),('footsteps',b'ESFOOT01'),('spatial-audio',b'ESSPAT01'),('pcm-sound',b'ESPCMS01'),('pcm-mixer',b'ESPCM801'),('pcm-dma',b'ESPCMD01')]:
 data=(source/(name+'.bin')).read_bytes()
 assert data[:8]==magic,(name,data[:8])
 (root/'tests/fixtures'/(name.replace('-','_')+'.bin')).write_bytes(data)
 print(name,len(data),'bytes imported')
