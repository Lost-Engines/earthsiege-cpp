#!/usr/bin/env python3
import json,struct,sys
from pathlib import Path
source=Path(sys.argv[1]);destination=Path(__file__).resolve().parents[1]/'tests/fixtures'
data=json.loads((source/'cockpits/tactical-map.json').read_text())
p=bytearray(b'ESTMAP03')
def pack(fmt,*v):p.extend(struct.pack('<'+fmt,*v))
pack('10h',*data['header']);pack('4i',*data['bounds']);pack('4i',*data['limits']);pack('iI',data['initial_zoom'],len(data['cases']))
for row in data['cases']:
 pack('3iIBBH',*row['camera'],row['ticks'],row['failed'],row['blink_phase'],len(row['contacts']))
 for c in row['contacts']:
  pack('3h6BH3i2iB',c['kind'],c['base'],c['size'],c['side'],c['dead'],c['hidden'],c['detected'],c['last_missing'],c['group_active'],c['heading'],*c['position'],*c['last'],c['selected']);pack('B',c['blink'])
 pack('I',row['crc32'])
(destination/'tactical_map.bin').write_bytes(p)

data=json.loads((source/'cockpits/tactical-controls.json').read_text())
p=bytearray(b'ESTCTL01')
pack('4i',*data['bounds']);pack('iiI',data['player'],data['range'],len(data['actors']))
for a in data['actors']:pack('3iB',*a['position'],a['side'])
ops=['wingmate','command','click','commit','packet','cancel','left','right','up','down','out','in']
def state(s):
 pack('3i4B3i',s['command'],s['previous'],s['wingmate'],s['requires'],s['ready'],s['cursor'],s['blink'],*s['pan'],s['height'])
 for o in s['orders']:
  pack('8i2B2i',*o['committed'],*o['pending'],o['committed_actor'],o['pending_actor'],o['committed_active'],o['pending_active'],o['committed_kind'],o['pending_kind'])
 pack('4i',*s['packet_position'],s['packet_actor'])
state(data['initial']);pack('I',len(data['cases']))
for row in data['cases']:
 pack('B2i',ops.index(row['op']),*(list(row['args'])+[0,0])[:2]);state(row['state'])
(destination/'tactical_controls.bin').write_bytes(p)
data=json.loads((source/'cockpits/full-damage.json').read_text())
p=bytearray(b'ESFDAM01');pack('I',len(data['chassis']))
for i,c in enumerate(data['chassis']):
 pack('h4i4iH',i,*c['bounds'],*c['listing'],len(c['equipment']));pack('h'*len(c['equipment']),*c['equipment']);pack('h'*len(c['weapon_values']),*c['weapon_values'])
 pack('H',len(c['mounts']))
 for m in c['mounts']:pack('9i',m['frame'],*m['anchor'],*m['bounds'],m['flags'],m['slot'])
 pack('I',len(c['cases']))
 for r in c['cases']:
  pack('4hB',r['model'],r['mode'],r['scroll'],r['subject'],r.get('missing',0));pack('40h',*r['armour'],*r['internal'],*r['weapon_damage']);pack('I',r['crc32'])
(destination/'full_damage.bin').write_bytes(p)
