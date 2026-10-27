import os,sys
sys.path.insert(0,'pc/tools')
import rdx
from pathlib import Path
from types import SimpleNamespace
args=SimpleNamespace(iso=os.environ['RECVX_ISO'])
a=rdx.open_disc(args)
for rid in sys.argv[1:]:
 room=rdx.Room(rdx.expand(a.get(rdx.room_names().index('RM_'+rid+'.RDX'))),rid)
 p=Path('scratch/re4/donors')/('RM_'+rid);p.mkdir(parents=True,exist_ok=True)
 for li,ts in rdx.texture_blocks(room):
  if li:continue
  for i,t in enumerate(ts):rdx.write_png(str(p/('tex0_%d.png'%i)),t['w'],t['h'],rdx.tim2_rgba(room.d,t))
