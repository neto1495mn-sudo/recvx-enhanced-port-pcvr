from PIL import Image, ImageDraw
from pathlib import Path
import sys
for room in sys.argv[1:]:
 files=sorted(Path('scratch/re4/donors/RM_'+room).glob('tex0_*.png'),key=lambda p:int(p.stem.split('_')[1]))
 out=Image.new('RGB',(800,120*((len(files)+7)//8)), '#777777');d=ImageDraw.Draw(out)
 for i,p in enumerate(files):
  im=Image.open(p).convert('RGBA');im.thumbnail((96,96));x=(i%8)*100;y=(i//8)*120
  out.paste(im,(x,y),im);d.text((x,y+97),p.stem,fill='white')
 out.save('scratch/re4/'+room+'.jpg')
