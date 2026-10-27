#!/usr/bin/env python3
"""Generate original service-corridor source assets, no disc data. Then use rdxbuild build."""
import copy
import json
from pathlib import Path
import sys
import rdx
from rdxbuild import zero


def box_obj(path, boxes):
    lines=['# Original procedural geometry; OBJ axes = game axes (y up)','mtllib room.mtl']
    n=0
    # Each quad includes both windings for this blockout (no hidden-face assumptions).
    for i,(x,y,z,w,h,d,tid) in enumerate(boxes):
        lines += ['o part%d'%i,'usemtl tex%d'%tid]
        points=[(x,y,z),(x+w,y,z),(x+w,y+h,z),(x,y+h,z),
                (x,y,z+d),(x+w,y,z+d),(x+w,y+h,z+d),(x,y+h,z+d)]
        lines += ['v %g %g %g'%p for p in points]
        lines += ['vt 0 0','vt 1 0','vt 1 1','vt 0 1']
        for a,b,c,e in [(0,1,2,3),(5,4,7,6),(4,0,3,7),(1,5,6,2),(3,2,6,7),(4,5,1,0)]:
            for face in [(a,b,c),(a,c,e),(c,b,a),(e,c,a)]:
                lines.append('f '+' '.join('%d/%d'%(n+j+1,4*i+(a,b,c,e).index(j)+1) for j in face))
        n+=8
    path.write_text('\n'.join(lines)+'\n')


def generate(dest):
    dest.mkdir(parents=True,exist_ok=True)
    j={'format':'recvx-room-1','mesh':'room.obj','textures':['floor.png','wall.png','accent.png'],
       'gindex':0x4D000000,'objects':[],'items':[],'enemies':[], 'cuts':[],
       'room':{'w':40.,'h':30.,'d':100.,'amb_r':[.8]*4,'amb_g':[.8]*4,'amb_b':[.8]*4},
       'enemy_assets':[{'room':'0060','record':0}],
       'object_meshes':[{'mesh':'prop.obj','textures':['wall.png'],'gindex':0x4D000020}],
       'item_meshes':[{'mesh':'item.obj','textures':['accent.png'],'gindex':0x4D000010}],
       'scripts':['2300fa0101002200f4010000','0e00fa0101000d00f4010000']}
    j['enemies']=[zero(rdx.ETTY,flg=1,id=1,type=0,mdlver=1,px=28.,pz=78.,ay=0)]
    j['items']=[zero(rdx.ETTY,flg=1,id=12,px=12.,py=1.,pz=26.,hide=[-1]*4)]
    j['objects']=[zero(rdx.ETTY) for _ in range(4)] + [zero(rdx.ETTY,flg=1,id=500,px=8.,pz=65.,hide=[-1]*4)]
    atr=lambda **kw:zero(rdx.ATR,flg=1,**kw)
    j['walls']=[atr(px=-2.,pz=-2.,w=2.,h=30.,d=104.),atr(px=40.,pz=-2.,w=2.,h=30.,d=104.),
                atr(px=0.,pz=-2.,w=40.,h=30.,d=2.),atr(px=0.,pz=100.,w=40.,h=30.,d=2.),
                atr(px=22.,pz=45.,w=18.,h=12.,d=8.),atr(px=5.,pz=62.,w=6.,h=8.,d=6.)]
    j['floors']=[atr(type=1,px=0.,pz=0.,w=40.,d=100.,prm0=0),
                 atr(type=2,px=0.,pz=0.,w=40.,d=100.)]
    j['routes']=[atr(px=0.,pz=0.,w=40.,d=100.)];j['route_next']=[[0]]
    j['triggers']=[atr(type=0,px=14.,pz=0.,w=12.,d=4.,prm0=1,prm1=11,prm2=0,prm3=22),
                   atr(type=4,px=8.,pz=23.,w=8.,d=6.,prm0=0)]
    j['positions']=[zero(rdx.POS,px=20.,pz=10.,ay=32768)]
    for z0,z1,pz in [(0.,52.,-12.),(48.,101.,38.)]:
        # Camera looks along +Z, pitched down. 16-bit angles; view direction is 180deg - ay
        # (0 = +z, 90 = +x), so ay=32768 looks along +z, like the player's ay.
        cam=zero(rdx.CAM,flg=257,px=20.,py=38.,pz=pz,ax=6000,ay=32768,pers=11000,
                 h=12.,am_spd=.3,aa_spd=30,fog_nr=0.,fog_fr=-1000.)
        if z0==0:   # the first shot looks over the door slab (room node 5): hide it, MSB first
            cam['hidobj'][0]=0x80000000>>5
        j['cuts'].append(dict(zero(rdx.CUT,flg=1,cx=0.,cz=z0,cw=40.,cd=z1-z0),cam=[cam,copy.deepcopy(cam)],
                             areas=[zero(rdx.CTAB,minx=-1.,minz=z0,maxx=41.,maxz=z1)],exd='00'*256))
    # Side walls omitted from the foreground where cameras look through; collision remains.
    box_obj(dest/'room.obj',[(0,-1,0,40,1,100,0),(-1,0,0,1,24,100,1),(40,0,0,1,24,100,1),
                              (0,0,100,40,24,1,1),(22,0,45,18,12,8,2),
                              (14,0,0,12,22,1,2)])
    box_obj(dest/'prop.obj',[(-3,0,-3,6,8,6,0)])
    box_obj(dest/'item.obj',[(-1,0,-1,2,2,2,0)])
    colors=[((40,65,80),(150,170,160)),((70,80,90),(100,110,120)),((180,115,30),(60,55,35))]
    for name,(a,b) in zip(j['textures'],colors):
        pix=bytes(v for y in range(64) for x in range(64) for v in (*((a,b)[((x//8)+(y//8))%2]),255))
        rdx.write_png(str(dest/name),64,64,pix)
    (dest/'room.mtl').write_text(''.join('newmtl tex%d\nmap_Kd %s\n'%(i,t) for i,t in enumerate(j['textures'])))
    (dest/'room.json').write_text(json.dumps(j,indent=2)+'\n')

if __name__=='__main__': generate(Path(sys.argv[1]))
