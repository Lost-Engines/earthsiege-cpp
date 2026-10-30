#!/usr/bin/env python3
"""Refresh only display RGB hashes from independent extracted VGA DAC resources.

The other artwork expectations and decoded native six-bit colours are unchanged.
This expands six-bit components by bit replication, as the DOSBox VGA reference
does, instead of the old floor(component * 255 / 63) preview conversion.
"""
import argparse, struct, zlib
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('resources',type=Path)
a=p.parse_args()
root=Path(__file__).resolve().parents[1]
lookup={p.relative_to(a.resources).as_posix().lower():p for p in a.resources.rglob('*')if p.is_file()}
manifest=root/'tests/artwork_checks.tsv'
rows=manifest.read_text().splitlines();changed=0
for i,line in enumerate(rows):
    parts=line.split()
    if parts[0]!='P':continue
    data=lookup[parts[1]+'/'+parts[2]].read_bytes()
    tag,size,count=struct.unpack_from('<III',data)
    assert tag in (0x28000f,0x280002) and count<=256
    rgba=[0]*256
    for index in range(count):
        c=data[12+index*4:15+index*4]
        assert len(c)==3 and max(c)<=63
        red,green,blue=[(v<<2)|(v>>4)for v in c]
        rgba[index]=0xff000000|(red<<16)|(green<<8)|blue
    parts[-1]=format(zlib.crc32(struct.pack('<256I',*rgba)),'08x')
    rows[i]='\t'.join(parts);changed+=1
manifest.write_text('\n'.join(rows)+'\n')
print(changed,'independent VGA palette display fixtures refreshed')
