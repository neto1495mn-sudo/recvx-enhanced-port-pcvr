"""Draw source triangle sections for reviewing room openings; output only in scratch."""
import argparse
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from analyze_obj_surfaces import read_obj
from PIL import Image, ImageDraw


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('obj'); p.add_argument('--bounds', nargs=4, type=float, required=True)
    p.add_argument('--height', type=float, default=10)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    args.output.resolve().relative_to(Path('scratch').resolve())
    x0,z0,x1,z1=args.bounds; scale=min(1400/(x1-x0),1000/(z1-z0))
    im=Image.new('RGB',(int((x1-x0)*scale)+1,int((z1-z0)*scale)+1),'#202020'); d=ImageDraw.Draw(im)
    def pt(v):return ((v[0]-x0)*scale,(v[2]-z0)*scale)
    for g,m,ps in read_obj(args.obj):
        cross=[]
        for a,b in zip(ps,ps[1:]+ps[:1]):
            if (a[1]<=args.height<b[1]) or (b[1]<=args.height<a[1]):
                t=(args.height-a[1])/(b[1]-a[1]); cross.append(tuple(a[k]+t*(b[k]-a[k]) for k in range(3)))
        if len(cross)==2:d.line([pt(v) for v in cross],fill='#cccccc',width=2)
    for x in range(int(x0),int(x1)+1,10):
        for z in range(int(z0),int(z1)+1,10):
            u,v=pt((x,0,z));d.ellipse((u-1,v-1,u+1,v+1),fill='cyan');d.text((u+2,v+2),f'{x},{z}',fill='#549292')
    args.output.parent.mkdir(parents=True,exist_ok=True); im.save(args.output)

if __name__=='__main__':main()
