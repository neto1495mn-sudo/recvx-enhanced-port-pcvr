"""Contact sheet of RM_1260: one in-game frame per cut (from a room_test run), the software
preview of the same cut, and the top-down overview. Output stays in ignored scratch/re4/contact/.
  python3 pc/tools/re4/contact_square.py scratch/re4/play/cams 510 600 660 750 840 960"""
import sys
from pathlib import Path
from PIL import Image,ImageDraw
run,frames=Path(sys.argv[1]),sys.argv[2:];sq=Path('scratch/re4/square');out=Path('scratch/re4/contact');out.mkdir(parents=True,exist_ok=True)
W,H=480,360;sheet=Image.new('RGB',(2*W,len(frames)*(H+20)),'#202020');d=ImageDraw.Draw(sheet)
for i,f in enumerate(frames):
 game=Image.open(run/'frames'/('frame_%05d.bmp'%int(f))).convert('RGB');game.save(out/('cut%d-game.png'%i))
 y=i*(H+20);sheet.paste(game.resize((W,H)),(0,y+20));sheet.paste(Image.open(sq/('camera%d.png'%i)).convert('RGB').resize((W,H)),(W,y+20))
 d.text((4,y+4),'cut %d  in game (%s frame %s)'%(i,run.name,f),fill='yellow');d.text((W+4,y+4),'cut %d  software preview'%i,fill='yellow')
sheet.save(out/'cameras.png');Image.open(sq/'overview.png').save(out/'overview.png');print(out/'cameras.png',out/'overview.png')
