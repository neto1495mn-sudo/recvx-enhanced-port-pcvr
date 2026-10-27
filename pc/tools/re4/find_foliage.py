"""Make a deduplicated retail green-cutout contact sheet from an extracted catalogue."""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('catalogue',type=Path)
    p.add_argument('--output',type=Path,default=Path('scratch/re4/foliage-review'))
    args=p.parse_args()
    if not args.output.resolve().is_relative_to((Path.cwd()/'scratch').resolve()):
        p.error('output must be inside scratch/')
    seen=set();candidates=[]
    for room in json.loads(args.catalogue.read_text())['rooms']:
        for t in room['textures']:
            if t['list']!=0:continue
            path=(args.catalogue.parent/t['path']).with_suffix('.png')
            if not path.exists():continue
            im=Image.open(path).convert('RGBA');data=np.array(im)
            digest=hashlib.sha256(data.tobytes()).hexdigest()
            if digest in seen:continue
            seen.add(digest)
            alpha=data[:,:,3];opaque=alpha>127
            transparent=float(np.mean(~opaque))
            if not .03<transparent<.9:continue
            rgb=data[:,:,:3].astype(float)
            green=(rgb[:,:,1]>rgb[:,:,0]*1.05)&(rgb[:,:,1]>rgb[:,:,2]*1.1)&opaque
            score=float(green.sum()/max(1,opaque.sum()))
            if score<.25:continue
            candidates.append(dict(room=room['room'],list=0,texture=t['index'],path=str(path),green=score,transparent=transparent))
    candidates.sort(key=lambda v:-v['green']);args.output.mkdir(parents=True,exist_ok=True)
    for page in range((len(candidates)+63)//64):
        subset=candidates[page*64:page*64+64];sheet=Image.new('RGB',(960,120*((len(subset)+7)//8)),'#777777');draw=ImageDraw.Draw(sheet)
        for i,c in enumerate(subset):
            im=Image.open(c['path']).convert('RGBA');im.thumbnail((115,95));x=i%8*120;y=i//8*120
            sheet.paste(im,(x,y),im);draw.text((x,y+96),c['room'][3:7]+':'+str(c['texture']),fill='white')
        sheet.save(args.output/f'candidates-{page}.jpg')
    (args.output/'candidates.json').write_text(json.dumps(candidates,indent=2)+'\n')
    print(len(candidates),'unique green cutout candidates')


if __name__=='__main__':main()
