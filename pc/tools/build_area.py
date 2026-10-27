#!/usr/bin/env python3
"""Build the original Quarantine Annex, donors resolved from RECVX_ISO into mods/ only.

Run from the repository root: python3 pc/tools/build_area.py
The committed design and procedural recipes are the complete editable source.
"""
import argparse
import copy
import json
import math
from pathlib import Path
import random
import struct
import rdx
import rdxbuild as b
from rdx_demo import box_obj

ROOT = Path(__file__).resolve().parents[2]
SPEC = Path(__file__).with_name('area') / 'annex.json'
MATERIALS = ['concrete', 'metal', 'tile', 'wood', 'rust', 'hazard', 'sign', 'supply']
# Original 5x7 stencil alphabet; 1 bits mark paint. No font assets or third-party package.
FONT = {
 'A':[14,17,17,31,17,17,17], 'B':[30,17,17,30,17,17,30],
 'C':[15,16,16,16,16,16,15], 'D':[30,17,17,17,17,17,30],
 'E':[31,16,16,30,16,16,31], 'F':[31,16,16,30,16,16,16],
 'G':[15,16,16,23,17,17,15], 'H':[17,17,17,31,17,17,17],
 'I':[31,4,4,4,4,4,31], 'K':[17,18,20,24,20,18,17],
 'L':[16,16,16,16,16,16,31], 'M':[17,27,21,21,17,17,17],
 'N':[17,25,25,21,19,19,17], 'O':[14,17,17,17,17,17,14],
 'P':[30,17,17,30,16,16,16], 'Q':[14,17,17,17,21,18,13],
 'R':[30,17,17,30,20,18,17], 'S':[15,16,16,14,1,1,30],
 'T':[31,4,4,4,4,4,4], 'U':[17,17,17,17,17,17,14],
 'V':[17,17,17,17,17,10,4], 'W':[17,17,17,21,21,21,10],
 'X':[17,17,10,4,10,17,17], 'Y':[17,17,10,4,4,4,4],
 '0':[14,17,19,21,25,17,14], '1':[4,12,4,4,4,4,14],
 '2':[14,17,1,2,4,8,31], '3':[30,1,1,14,1,1,30],
 '4':[2,6,10,18,31,2,2], '5':[31,16,16,30,1,1,30],
 '6':[14,16,16,30,17,17,14], '7':[31,1,2,4,8,8,8],
 '8':[14,17,17,14,17,17,14], '9':[14,17,17,15,1,1,14],
 ' ':[0]*7,
}


def materials(dest, sign, seed):
    """Weathering uses seeded multiscale variation, joints and directional wear."""
    rng = random.Random(seed)
    coarse = [[rng.random() for _ in range(17)] for _ in range(17)]
    def cloud(x,y):
        u,v=x/8,y/8; a,c=int(u),int(v); u-=a;v-=c
        return (coarse[c][a]*(1-u)+coarse[c][a+1]*u)*(1-v)+(coarse[c+1][a]*(1-u)+coarse[c+1][a+1]*u)*v
    for name in MATERIALS + ['token']:
        pixels=[]
        for y in range(128):
            for x in range(128):
                noise=rng.randrange(-9,10); stain=cloud(x,y)
                if name=='concrete':
                    n=85+50*stain+noise-(18 if y%64<2 or x%64<1 else 0)
                    rgb=(n,n+5,n+3)
                elif name=='metal':
                    n=90+20*math.sin(y*.5)+noise+20*stain
                    if min(x,y,127-x,127-y)<3:n=42
                    if any((x-a)**2+(y-c)**2<8 for a in [8,119] for c in [8,119]):n=185
                    rgb=(n,n+10,n+15)
                elif name=='tile':
                    n=80+25*stain+noise
                    rgb=(n*.65,n+23,n+7) if x%32>2 and y%32>2 else (38,42,37)
                    if x%32==4 or y%32==4:rgb=(115,145,128)
                elif name=='wood':
                    n=75+23*math.sin(x*.45+5*math.sin(y*.025))+25*stain+noise
                    if x%32<2:n=32
                    rgb=(n+30,n,n*.55)
                elif name=='rust':
                    n=45+100*stain+noise
                    rgb=(n+32,n*.58+12,n*.25+8) if stain>.3 else (70,80,81)
                elif name=='hazard':rgb=(192+noise,144+noise,35) if (x+y)%40<20 else (30+noise,34+noise,28)
                elif name in ('sign','token'):rgb=(27+noise//3,52+noise//3,48+noise//3)
                else:
                    rgb=(125+noise,137+noise,109+noise)
                    if abs(x-64)<9 or abs(y-64)<9:rgb=(38,99,61)
                pixels.extend(max(1,min(255,int(v))) for v in (*rgb,255))
        if name in ('sign','token'):
            lines=['QUARANTINE',sign] if name=='sign' else ['TOKEN','RECORDS']
            for row,word in enumerate(lines):
                scale=2; start=(128-len(word)*6*scale)//2
                for k,ch in enumerate(word):
                    for yy,bits in enumerate(FONT[ch]):
                        for xx in range(5):
                            if bits & (16>>xx):
                                for sy in range(scale):
                                    for sx in range(scale):
                                        at=((40+row*30+yy*scale+sy)*128+start+k*12+xx*scale+sx)*4
                                        pixels[at:at+4]=[215,204,153,255]
        rdx.write_png(str(dest/(name+'.png')),128,128,bytes(pixels))


def room_obj(path, boxes):
    """World-scaled materials, tiled by splitting faces: the game clamps room textures, so
    every UV stays in 0..1 (in-game finding: UVs beyond 1 smeared into stripes). Signs
    retain a single readable image per face."""
    lines=['# Original Quarantine Annex geometry, game axes; no disc mesh data.','mtllib room.mtl']
    nv=0
    for i,(x,y,z,w,h,d,tid) in enumerate(boxes):
        lines+=['o part%d'%i,'usemtl tex%d'%tid]
        points=[(x,y,z),(x+w,y,z),(x+w,y+h,z),(x,y+h,z),
                (x,y,z+d),(x+w,y,z+d),(x+w,y+h,z+d),(x,y+h,z+d)]
        for a,c,e,f in [(0,1,2,3),(5,4,7,6),(4,0,3,7),(1,5,6,2),(3,2,6,7),(4,5,1,0)]:
            p0,p1,p3=points[a],points[c],points[f]
            unit=16 if tid==2 else 32
            lu,lv=math.dist(p0,p1),math.dist(p0,p3)
            cols,rows=(max(1,math.ceil(lu/unit-1e-9)),max(1,math.ceil(lv/unit-1e-9))) if tid<6 else (1,1)
            def at(s,t):return tuple(p0[k]+s*(p1[k]-p0[k])+t*(p3[k]-p0[k]) for k in range(3))
            for col in range(cols):
                for row in range(rows):
                    s0,s1=col/cols,(col+1)/cols;t0,t1=row/rows,(row+1)/rows
                    lines+=['v %g %g %g'%q for q in (at(s0,t0),at(s1,t0),at(s1,t1),at(s0,t1))]
                    u,v=(lu/cols/unit,lv/rows/unit) if tid<6 else (1,1)
                    # These quads advance leftward as seen from outside each box. With
                    # +Z cameras, +X is screen-left: reverse U for legible outward labels.
                    if tid>=6:uvs=[(u,0),(0,0),(0,v),(u,v)]
                    else:uvs=[(0,0),(u,0),(u,v),(0,v)]
                    lines+=['vt %g %g'%t for t in uvs]
                    for face in [(0,1,2),(0,2,3),(2,1,0),(3,2,0)]:
                        lines.append('f '+' '.join('%d/%d'%(nv+k+1,nv+k+1) for k in face))
                    nv+=4
    path.write_text('\n'.join(lines)+'\n')


def atr(**kw):return b.zero(rdx.ATR,flg=1,**kw)


def conditional(condition, body):
    """IfelCk skip lands on Endif; no nested else stack is needed here."""
    offset=2+len(condition)+len(body)
    if offset>255:raise ValueError('short condition block overflow')
    return bytes([1,offset])+condition+body+bytes([3,0])


def flag(op,bank,bit,invert=0):return struct.pack('<BBHBB',op,bank,bit,0,invert)


def door_location(side,w,d):
    # trigger bounds, landing position, inward player yaw
    return {
        'S':((w/2-6,0,12,5),(w/2,11,32768)),
        'N':((w/2-6,d-5,12,5),(w/2,d-11,0)),
        'W':((0,d/2-6,5,12),(11,d/2,49152)),
        'E':((w-5,d/2-6,5,12),(w-11,d/2,16384)),
    }[side]


def room_source(spec,dest,key_flag):
    dest.mkdir(parents=True,exist_ok=True)
    w,d=spec['size']; rid=int(spec['id']); style=spec['style']
    materials(dest,spec['sign'],rid)
    gid=0x4E000000+(rid-1160)*4096
    j={'format':'recvx-room-1','mesh':'room.obj','textures':[t+'.png' for t in MATERIALS]+['token.png'],
       'gindex':gid,'room':{'w':w,'h':30.,'d':d,'fog_nr':0.,'fog_fr':-1000.,
       'amb_r':[.72,.88,.8,.95],'amb_g':[.77,.87,.8,.95],'amb_b':[.72,.83,.75,.85],
       'amb_chr':1,'amb_obj':2,'amb_itm':3},
       'cuts':[],'walls':[],'floors':[atr(type=1,w=w,d=d)],'positions':[],
       'objects':[b.zero(rdx.ETTY) for _ in range(4)],'items':[],'item_meshes':[],
       'enemies':[],'enemy_assets':[],'triggers':[], 'routes':[atr(w=w,d=d)],'route_next':[[0]]}
    # Closed shell, separate visual groups; the cameras sit inside it (see cuts below).
    floor_material=2 if style in ('wash','isolation','supply') else 0
    wall_material=2 if style=='wash' else 0
    boxes=[(0,-1,0,w,1,d,floor_material),(-2,0,-2,2,30,d+4,wall_material),
           (w,0,-2,2,30,d+4,wall_material),(0,0,-2,w,30,2,wall_material),
           (0,0,d,w,30,2,wall_material),(0,30,0,w,1,d,1)]
    for x,y,z,ww,h,dd,t in boxes[1:5]:j['walls'].append(atr(px=x,py=y,pz=z,w=ww,h=h,d=dd))
    def prop(x,z,ww,dd,h,material,kind):
        # A single simple collider for the composite; do not block the central spine.
        j['walls'].append(atr(px=x,pz=z,w=ww,h=h,d=dd))
        if kind=='shelf':
            boxes.extend([(x,0,z,1,h,dd,material),(x+ww-1,0,z,1,h,dd,material)])
            for y in range(0,int(h),6):boxes.append((x,y,z,ww,1,dd,material))
            for y in (2,8,14):
                if y+3<h:boxes.append((x+1,y,z+1,ww-2,3,dd-2,7))
        elif kind=='bed':
            boxes.extend([(x,0,z,ww,5,dd,1),(x,5,z,ww,2,dd,7),(x,7,z,ww,2,2,7)])
        elif kind=='basin':
            boxes.extend([(x,0,z,ww,8,dd,1),(x,8,z,ww,1,dd,2),(x+2,9,z+2,ww-4,1,dd-4,1)])
        elif kind=='cage':
            boxes.append((x,0,z,ww,1,dd,4))
            for dx in range(0,int(ww)+1,3):boxes.append((x+dx,0,z,0.5,h,dd,1))
            boxes.extend([(x,h-1,z,ww,1,dd,4),(x,0,z+dd-1,ww,h,1,4)])
        else:
            boxes.append((x,0,z,ww,h,dd,material))
            boxes.extend([(x,0,z,ww,1,dd,1),(x,h-1,z,ww,1,dd,1)])
    if style=='reception':
        prop(3,18,5,16,10,3,'desk');prop(w-8,18,5,14,5,3,'bench')
        prop(3,d-24,5,14,5,3,'bench');prop(w-8,d-24,5,14,17,1,'shelf')
    elif style=='wash':
        for x in (2,w-8):
            prop(x,18,6,16,10,2,'basin');prop(x,d-20,6,20,20,1,'locker')
        boxes.append((w/2-1,.02,4,2,.03,d-8,1))
    elif style=='records':
        prop(2,0,5,18,23,3,'shelf');prop(w-7,0,5,18,23,3,'shelf')
        prop(2,d-18,5,18,23,3,'shelf');prop(w-7,53,5,18,12,3,'desk')
        prop(10,57,9,3,9,3,'desk')
    elif style=='isolation':
        for x in (2,w-10):
            prop(x,0,8,20,24,4,'cage');prop(x,67,8,20,9,1,'bed')
    elif style=='supply':
        for x in (2,w-7):
            prop(x,0,5,18,22,1,'shelf');prop(x,d-18,5,18,22,1,'shelf')
        prop(22,d-12,12,8,10,3,'crate')
    else:
        for x in (2,w-10):
            prop(x,14,8,20,14,3,'crate');prop(x,67,8,20,9,1,'trolley')
    # Skirting, overhead utilities and readable destination panels.
    boxes.extend([(0,1,0,1,2,d,4),(w-1,1,0,1,2,d,4),
                  (4,27,0,1,1,d,4),(w-5,27,0,1,1,d,4),
                  (w/2-13,18,d-.3,26,9,.2,6)])
    entry=bytearray(bytes([55,0,182,0]));frame=bytearray()
    for i,(side,target,target_entry) in enumerate(spec['doors']):
        (x,z,ww,dd),(px,pz,ay)=door_location(side,w,d)
        j['positions'].append(b.zero(rdx.POS,px=px,pz=pz,ay=ay))
        trigger=atr(type=0,px=x,pz=z,w=ww,d=dd,prm0=target//100,prm1=target%100,prm2=target_entry,prm3=22)
        if target==102:trigger['type']=3 # explicit case-aware script, as in the retail door
        j['triggers'].append(trigger)
        if side in ('S','N'):
            zz=-.1 if side=='S' else d-.1
            slab=(w/2-6,0,zz,12,23,.2,1)
            boxes.extend([(w/2-7,0,zz-.1,1,25,.5,4),(w/2+6,0,zz-.1,1,25,.5,4),
                          (w/2-7,23,zz-.1,14,2,.5,5)])
        else:
            xx=-.1 if side=='W' else w-.1
            slab=(xx,0,d/2-6,.2,23,12,1)
            boxes.extend([(xx-.1,0,d/2-7,.5,25,1,4),(xx-.1,0,d/2+6,.5,25,1,4),
                          (xx-.1,23,d/2-7,.5,2,14,5)])
        boxes.append(slab)
        if target==120:
            # A sign provides the locked interaction's feedback; no retail message borrowed.
            boxes.append((w-.4,14,d/2-5,.2,7,10,8))
            lock=bytes([11,i,1,0])+conditional(flag(4,7,key_flag),bytes([11,i,0,0]))
            entry.extend(lock);frame.extend(lock)
        if target==102:
            # RM_1110's own hall exit: case 0 and door attr 4 while ev_flg 151 is clear; else
            # case 1 and attr 4 / 0x24 / 0x64 by ev_flg 181 and 142 (RoomCaseNo, RoomSoundCase).
            def hall(case,attr):return bytes([55,case,182,0,51,0,attr,0,1,2,3,22])
            set151,clear181,set181=flag(4,1,151),flag(4,1,181,1),flag(4,1,181)
            body=(conditional(flag(4,1,151,1),hall(0,4))+conditional(set151+clear181,hall(1,4))
                  +conditional(set151+set181+flag(4,1,142,1),hall(1,0x24))
                  +conditional(set151+set181+flag(4,1,142),hall(1,0x64)))+flag(5,10,23,1)
            frame.extend(conditional(struct.pack('<BBHBB',4,10,23,i,0),body))
    for slot,(item,x,z,bit) in enumerate(spec['items']):
        trigger=len(j['triggers'])
        j['items'].append(b.zero(rdx.ETTY,flg=1,id=item,px=x,py=1,pz=z,hide=[-1]*4))
        j['triggers'].append(atr(type=4,px=x-3,pz=z-3,w=6,d=6,prm0=slot))
        entry.extend(struct.pack('<BBHBB',35,slot,bit,trigger,0))
        frame.extend(struct.pack('<BBHBB',14,0,bit,trigger,0))
        mesh='item%d.obj'%slot
        if item==72:
            shapes=[(-1.2,0,-1.2,2.4,.6,2.4,0),(-.3,0,1,.6,.6,3,0),(.3,0,2,1,.6,.6,0)]
        elif item in (21,22):
            shapes=[(-1,0,-1,2,1.5,2,0),(-.2,1,0,.4,3,.4,0),(-1.4,2,-.2,2.8,.4,.8,0),(-.8,3,-.4,1.6,.4,.8,0)]
        else:shapes=[(-1.8,0,-1,3.6,1.7,2,0)]
        box_obj(dest/mesh,shapes)
        # Small purpose-colored pickup textures, original artwork.
        color=(190,151,53) if item==72 else (61,135,51) if item==21 else (160,44,31) if item==22 else (157,83,39)
        pix=bytes(v for yy in range(32) for xx in range(32) for v in (*tuple(min(255,c+(25 if xx%8<2 else 0)) for c in color),255))
        texture='item%d.png'%slot;rdx.write_png(str(dest/texture),32,32,pix)
        j['item_meshes'].append({'mesh':mesh,'textures':[texture],'gindex':gid+128+slot*8})
    for slot,(eid,ver,donor,record,x,z,bit) in enumerate(spec['enemies']):
        j['enemies'].append(b.zero(rdx.ETTY,flg=1,id=eid,type=0,mdlver=ver,wrk_no=slot,px=x,pz=z,ay=0))
        j['enemy_assets'].append({'room':donor,'record':record})
        entry.extend(struct.pack('<BBH',34,slot,bit));frame.extend(struct.pack('<BBH',13,slot,bit))
    for eid in sorted({e[0] for e in spec['enemies']}):j['floors'].append(atr(type=2,w=w,d=d,prm0=eid))
    j['scripts']=[(entry+bytes(2)).hex(),(frame+bytes(2)).hex()]
    # Each half is shot from inside the room, under the ceiling at the opposite end wall,
    # so no hidden wall or ceiling ever shows the void outside the shell (in-game finding:
    # the earlier cameras behind the south wall at y=65 showed black beyond the room).
    # Pitched so the top of the view stays below the ceiling: the game drops the large
    # ceiling triangles that pass over the camera, which left a wedge of background.
    for z0,z1,pz,ay in [(0,d/2+2,d-1,0),(d/2-2,d,1,32768)]:
        cam=b.zero(rdx.CAM,flg=257,px=w/2,py=29,pz=pz,ax=6000,ay=ay,pers=14000,
                   h=12.,am_spd=.3,aa_spd=30,fog_nr=0.,fog_fr=-1000.)
        j['cuts'].append(dict(b.zero(rdx.CUT,flg=1,cx=0,cz=z0,cw=w,cd=z1-z0),
                             cam=[cam,copy.deepcopy(cam)],areas=[b.zero(rdx.CTAB,minx=-1,minz=z0,maxx=w+1,maxz=z1+.01)],exd='00'*256))
    room_obj(dest/'room.obj',boxes)
    (dest/'room.mtl').write_text(''.join('newmtl tex%d\nmap_Kd %s\n'%(i,t) for i,t in enumerate(j['textures'])))
    j['_design']={'name':spec['name'],'boxes':boxes,'key_flag':key_flag}
    (dest/'room.json').write_text(json.dumps(j,indent=2)+'\n')
    return j


def census(args):
    """Every retail script, generic flags and counted death ranges included."""
    afs=rdx.open_disc(args);names=rdx.room_names()
    if len(afs.toc)!=len(names) or len(names)!=205:raise ValueError('unexpected retail AFS census')
    used={'it_flg':set(),'ed_flg':set()}; keep={}
    for i,name in enumerate(names):
        room=rdx.Room(rdx.expand(afs.get(i)),name);seen=[]
        if not rdx.disasm(room,seen)[1]:raise ValueError('unclean retail script '+name)
        for o in set(seen):
            op=room.d[o];bit=struct.unpack_from('<H',room.d,o+2)[0] if rdx.insn_len(room.d,o)>=4 else 0
            if op in (14,35):used['it_flg'].add(bit&1023)
            if op in (13,34):used['ed_flg'].add(bit&1023)
            if op in (4,5) and room.d[o+1] in (3,7):used[{3:'ed_flg',7:'it_flg'}[room.d[o+1]]].add(bit&1023)
            if op==33:used['ed_flg'].update(range(bit&1023,(bit&1023)+room.d[o+1]))
        if name[3:7] in ('1110','1020','1021'):keep[name[3:7]]=room
        if (i+1)%40==0:print('Census:',i+1,'/ 205 rooms',flush=True)
    return names,{k:sorted(v) for k,v in used.items()},keep


def patch_retail(room,trigger,previous,target,normalize_case=False):
    """Patch only this connection's target literals; retain all retail lock conditions.

    RM_102x starts with an examine record which its scripts turn into a door.
    RM_1110's sole RoomCaseNo 1 refers to its original hall destination, not the annex.
    """
    before=bytes(room.d);seen=[]
    if not rdx.disasm(room,seen)[1]:raise ValueError('unclean retail script')
    changes=[]
    t=room.records('triggers')[trigger]
    if t['type']==0:
        if [t['prm%d'%k] for k in range(3)]!=previous:raise ValueError('unexpected retail door')
        off=room.ptr['etcp']+trigger*rdx.ATR[0].size+32
        room.d[off:off+3]=bytes(target);changes.extend(range(off,off+3))
    hits=0;cases=0
    for o in set(seen):
        if ((room.d[o]==37 and room.d[o+1]==trigger and room.d[o+8]==0) or room.d[o]==51) and list(room.d[o+4:o+7])==previous:
            room.d[o+4:o+7]=bytes(target);changes.extend(range(o+4,o+7));hits+=1
        if normalize_case and room.d[o]==55:
            if room.d[o+1]!=1:raise ValueError('unexpected piano room case selection')
            room.d[o+1]=0;changes.append(o+1);cases+=1
    if hits==0 or (normalize_case and cases!=1):raise ValueError('retail door patch pattern changed')
    if any(a!=c and i not in changes for i,(a,c) in enumerate(zip(before,room.d))):raise ValueError('unrelated retail byte changed')
    return bytes(room.d),{'script_targets':hits,'case_resets':cases,'changed_bytes':sum(a!=c for a,c in zip(before,room.d))}


def main():
    import area_validate as check
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--iso');p.add_argument('--source-only',action='store_true',help='generate original OBJ/PNG/JSON without reading disc')
    args=p.parse_args()
    # Fixed destinations prevent donor data accidentally being written into source paths.
    if Path.cwd().resolve()!=ROOT:raise SystemExit('Run from repository root')
    spec=json.loads(SPEC.read_text());check.graph(spec)
    dest=ROOT/'mods'/'quarantine-source';dest.mkdir(parents=True,exist_ok=True)
    sources={}
    for r in spec['rooms']:
        sources[r['id']]=room_source(r,dest/r['id'],spec['key_flag'])
    if args.source_only:
        print('Original source assets:',dest);return
    names,used,retail=census(args);check.allocations(spec,used)
    report={'area':spec['name'],'retail_names':names,'retail_flags':used,'rooms':{},'retail_patches':{}}
    outputs={}
    for r in spec['rooms']:
        name='RM_'+r['id']+'.RDX'
        if any(n[3:6]==r['id'][:3] for n in names):raise ValueError('retail stage/room collision')
        raw=b.compile_source(dest/r['id']/'room.json',args);packed=rdx.compress(raw)
        if rdx.expand(packed)!=raw:raise ValueError('compression mismatch')
        compiled=rdx.Room(raw,name)
        report['rooms'][name]=check.room(sources[r['id']],compiled,len(packed))
        report['rooms'][name]['sound_key']=r['sound']
        outputs[name]=packed;outputs['RM_'+r['id']+'.SPQ.key']=(str(r['sound'])+'\n').encode()
        (dest/r['id']/'scripts.txt').write_text('\n'.join(rdx.disasm(compiled)[0])+'\n')
        print(name,report['rooms'][name],flush=True)
    for name,trigger,previous,target,normalize in [('1110',0,[1,2,3],[1,16,0],True),
                                                  ('1020',9,[1,11,0],[1,16,1],False),
                                                  ('1021',9,[1,11,0],[1,16,1],False)]:
        raw,stats=patch_retail(retail[name],trigger,previous,target,normalize)
        packed=rdx.compress(raw)
        if rdx.expand(packed)!=raw:raise ValueError('retail compression mismatch')
        outputs['RM_'+name+'.RDX']=packed;report['retail_patches'][name]=stats
    # Publish only after every room and patch has validated. Each replace is atomic.
    mods=ROOT/'mods'
    for name,data in outputs.items():
        temp=mods/(name+'.tmp');temp.write_bytes(data);temp.replace(mods/name)
    (mods/'quarantine-validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Imported six new rooms, three retail patches and six sound aliases into mods/.',flush=True)


if __name__=='__main__':main()
