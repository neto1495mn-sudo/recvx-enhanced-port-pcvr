"""Render isolated external OBJ material uses with group context into scratch only."""
import argparse
import collections
import json
from pathlib import Path
import sys
import numpy as np
from PIL import Image, ImageDraw
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from analyze_obj_surfaces import read_obj


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('obj',type=Path)
    p.add_argument('--materials',nargs='+',required=True)
    p.add_argument('--output',type=Path,default=Path('scratch/re4/material-review'))
    args=p.parse_args()
    if not args.output.resolve().is_relative_to((Path.cwd()/'scratch').resolve()):
        p.error('output must be inside scratch/')
    groups=collections.defaultdict(list)
    for group,material,ps in read_obj(args.obj):
        groups[group].append((material,np.array(ps)))
    args.output.mkdir(parents=True,exist_ok=True)
    index=[]
    for material in args.materials:
        uses=[(g,fs) for g,fs in groups.items() if any(m==material for m,ps in fs)]
        for i,(group,faces) in enumerate(uses):
            im=Image.new('RGB',(1200,420),'#24272c');draw=ImageDraw.Draw(im)
            for view,(right,up) in enumerate((((1,0,0),(0,1,0)),((0,0,1),(0,1,0)),((.707,0,-.707),(.35,.866,.35)))):
                right=np.array(right);up=np.array(up);forward=np.cross(right,up)
                points=np.concatenate([ps for m,ps in faces]);u=points@right;v=points@up
                low=np.array([u.min(),v.min()]);high=np.array([u.max(),v.max()]);scale=350/max(high-low)
                center=(low+high)/2
                for m,ps in sorted(faces,key=lambda f:float(f[1].mean(0)@forward)):
                    coords=np.column_stack((ps@right,ps@up))-center
                    coords=coords*np.array([scale,-scale])+[view*400+200,210]
                    n=np.cross(ps[1]-ps[0],ps[2]-ps[0]);n=n/max(np.linalg.norm(n),1e-9)
                    shade=.35+.65*abs(n@np.array([.3,.8,.5]))
                    base=np.array([255,180,75]) if m==material else np.array([120,130,140])
                    draw.polygon([tuple(v) for v in coords],fill=tuple(int(x) for x in base*shade))
            draw.text((5,5),material+' '+group,fill='white')
            name=f'{material}-{i:02}.png';im.save(args.output/name)
            index.append({'material':material,'group':group,'preview':name,
                          'triangles':sum(m==material for m,ps in faces)})
    (args.output/'index.json').write_text(json.dumps(index,indent=2)+'\n')
    print('Rendered',len(index),'group/material reviews')


if __name__=='__main__':
    main()
