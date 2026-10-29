#!/usr/bin/env python3
"""Import original executable radio captures; no expected values are synthesized."""
from pathlib import Path
import sys
root=Path(__file__).resolve().parents[1]
source=Path(sys.argv[1])/'cockpits'
for source_name,target_name,magic in [
 ('radio-queue','radio_queue',b'ESRQUE01'),('radio-descriptors','radio_descriptors',b'ESRDES01'),
 ('radio-state','radio_state',b'ESRSTA01'),('portrait-timing','portrait_timing',b'ESSNC001'),
 ('portrait-state','portrait_state',b'ESPSTA01'),('radio-banner','radio_banner',b'ESRBAN01'),
 ('radio-portraits','radio_portraits',b'ESRPRT01'),('wingmate-portraits','wingmate_portraits',b'ESWPRT01')]:
 data=(source/(source_name+'.bin')).read_bytes()
 assert data[:8]==magic,(source_name,data[:8])
 (root/'tests/fixtures'/(target_name+'.bin')).write_bytes(data)
 print(source_name,len(data),'bytes imported')
