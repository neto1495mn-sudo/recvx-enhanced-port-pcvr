#!/usr/bin/env python3
"""Build the village from retail references only; run with RECVX_ISO set.

All generated geometry, palettes, models and room files stay in ignored scratch/ and mods/.
No hooks or changes to retail events. See docs/modding/BUILDING_ROOMS.md "Example area: village from retail assets".
"""
import argparse
import copy
import json
import math
from pathlib import Path
import struct
import rdx
import rdxbuild as b
import assets
import kitbash
from build_area import atr, census, room_obj

ROOT=Path(__file__).resolve().parents[2]
GROUND,PLANKS,FIELDSTONE=('5810',20),('0120',26),('7150',29)   # tiling retail materials
FOG=0xff393038
PERS=13000   # horizontal view at 4:3 (retail rooms use about 10000-15000)


def camera(x,y,z,tx,tz):
    """Camera at x,y,z looking at ground point tx,tz (ay 0 looks -Z, 16384 +X; ax>0 looks down)."""
    dx,dz=tx-x,tz-z
    return b.zero(rdx.CAM,flg=257,px=x,py=y,pz=z,ay=round(math.atan2(dx,-dz)*65536/math.tau)%65536,
                  ax=round(math.atan2(y,math.hypot(dx,dz))*65536/math.tau),pers=PERS,
                  h=12.,am_spd=.3,aa_spd=30,fog_col=FOG,fog_nr=-90.,fog_fr=-320.)
# Room numbers are decimal. Every authored room uses case zero. Cuts: ([areas x0,z0,x1,z1],
# camera x,y,z and the ground point it looks at), chosen by coverage search; village_validate checks them.
SPECS=[
 {'id':1220,'name':'Village square','size':[160,180],'wall':50,'cuts':[([(56,0,120,60)],(123,46,87,86,43)),([(56,52,120,82)],(75,46,27,89,77)),([(96,54,160,112)],(75,46,51,110,79)),
            ([(0,70,40,116)],(3,46,117,26,91)),([(34,70,66,116)],(21,46,93,51,93)),
            ([(40,108,160,180)],(39,46,81,74,131))],
  'doors':[(80,0,'S',2,2),(32,66,'S',123,0),(128,108,'N',124,0),(80,180,'N',125,0)],
  'items':[(12,110,45,480),(21,55,133,481)],'enemies':[(56,95,480),(100,145,481),(112,75,482)]},
 {'id':1230,'name':'Two-storey house, ground floor','size':[64,80],'wall':30,'cuts':[([(0,0,64,44)],(57,28,75,38,35)),([(0,36,64,80)],(57,28,3,32,42))],
  'doors':[(32,0,'S',122,1)],'items':[(12,18,43,482),(21,44,57,483)],'enemies':[(40,35,483)]},
 {'id':1240,'name':'Barn','size':[72,90],'wall':32,'cuts':[([(0,0,72,49)],(69,30,75,47,38)),([(0,45,72,90)],(63,30,21,37,64))],
  'doors':[(36,0,'S',122,2)],'items':[(12,22,52,484),(21,50,64,485)],'enemies':[(42,40,484),(25,68,485)]},
 {'id':1250,'name':'Church approach','size':[64,140],'wall':50,'cuts':[([(0,0,64,50)],(3,45,69,31,26)),([(0,42,64,92)],(3,45,21,31,66)),([(0,84,64,140)],(3,45,69,30,101))],
  'doors':[(32,0,'S',122,3)],'items':[(21,40,80,486)],'enemies':[(30,55,486),(32,100,487)]},
]


def source(spec,args,directory):
    directory.mkdir(parents=True,exist_ok=True)
    w,d=spec['size'];rid=spec['id'];kit=kitbash.Kit(args)
    j={'format':'recvx-room-1','mesh':'room.obj','room':{'w':w,'d':d,'h':120.,
       'amb_r':[.70,.80,.75,.90],'amb_g':[.59,.69,.65,.80],'amb_b':[.52,.61,.58,.72],
       'amb_chr':1,'amb_obj':2,'amb_itm':3,'bak_col':0xff393038,'fog_col':0xff393038,'fog_nr':-90.,'fog_fr':-320.},
       'cuts':[],'walls':[],'floors':[atr(type=1,w=w,d=d),atr(type=2,w=w,d=d,prm0=1)],
       'routes':[atr(w=w,d=d)],'route_next':[[0]],'positions':[],
       'objects':[b.zero(rdx.ETTY) for _ in range(4)],'object_assets':[],
       'items':[],'item_assets':[],'enemies':[],'enemy_assets':[],'triggers':[]}
    def box_collision(lo,hi):
        j['walls'].append(atr(px=lo[0],py=lo[1],pz=lo[2],w=max(.2,hi[0]-lo[0]),h=max(.2,hi[1]-lo[1]),d=max(.2,hi[2]-lo[2])))
    def place(room,nodes,at,size=None,label='prop',solid=True):
        lo,hi=kit.place(room,nodes,at,size,label=label)
        if solid:box_collision(lo,hi)
    # Seamless retail materials: CVX's own tiling ground, plank and fieldstone textures.
    # room_obj tiles texture 2 every 16 units, the others every 32: planks get the finer grain.
    stone=kit.texture(*FIELDSTONE);dirt=kit.texture(*GROUND);wood=kit.texture(*PLANKS)
    hw=spec['wall'];interior=rid in (1230,1240)
    boxes=[(-100,-1,-100,w+200,1,d+200,wood if interior else dirt)]
    # Closed shell: walls reach the ceiling indoors; outdoors they rise above every camera.
    for x,z,ww,dd in [(-2,-2,2,d+4),(w,-2,2,d+4),(0,-2,w,2),(0,d,w,2)]:
        box_collision((x,0,z),(x+ww,hw,z+dd))
        boxes.append((x,0,z,ww,hw,dd,wood if rid==1240 else stone))
    if rid==1220:
        # The south entry opens toward the pyre; paired houses form the west/east lanes.
        place('0030',[23],(8,0,8),(44,48,58),'west two-storey house')
        place('0030',[9],(112,0,108),(42,34,66),'east barn')
        place('0050',[38],(4,0,116),(32,34,50),'northwest cottage')
        place('0030',[22],(120,0,6),(38,32,46),'southeast cottage')
        place('0020',[55],(1,0,70),(1,18,35),'west iron fence')
        place('0020',[55],(158,0,60),(1,18,44),'east iron fence')
        place('0050',[27],(142,0,90),label='supply crate')
        place('0050',[27],(7,0,88),label='supply crate')
        place('1000',[115],(105,0,112),(3,24,3),'street lamp')
        place('1000',[82],(28,0,163),(14,25,14),'tree')
        place('1000',[79],(6,0,166),(14,25,14),'tree')
        # Effect 9 type 0 loops the game's common flame atlas (effsub1.c bhEff009).
        j['effects']=[b.zero(rdx.EFF,flg=1,id=9,type=0,px=x,py=15.,pz=z,
                             sx=3.5,sy=6.,sz=3.5) for x,z in [(78,87),(85,88),(81,95)]]
        # Pyre of crossed split timbers on a stone ring.
        for x,z in [(72,83),(84,83),(72,97),(84,97)]:
            place('0030',[3],(x,0,z),(4,2,4),'pyre stones',False)
        boxes += [(73,1,86,18,2,3,wood),(75,3,82,3,2,19,wood),(84,3,82,3,2,19,wood),
                  (76,5,86,12,2,3,wood),(79,0,90,2,22,2,wood)]
        box_collision((72,0,81),(92,22,102))
    elif rid==1230:
        boxes += [(0,hw,0,w,1,d,wood)]
        place('0050',[27],(3,0,62),label='store crate')
        place('0050',[9],(48,0,60),(12,8,12),'timber bench')
        # Retail planking on stairs to a blocked loft: decorative, no fake traversable floor.
        for n in range(6):boxes.append((3,n*2,6+n*5,12,2,5,wood))
        box_collision((3,0,6),(15,12,36))
    elif rid==1240:
        boxes += [(0,hw,0,w,1,d,wood)]
        for x,z in [(3,10),(3,22),(60,10),(60,24),(3,76),(60,76)]:
            place('0050',[27],(x,0,z),label='barn crate')
        place('0050',[34],(4,0,45),(10,2,4),'loose planks')
        for x in (2,68):boxes += [(x,0,2,2,hw,2,wood),(x,0,86,2,hw,2,wood)]
    else:
        for z in (22,60,98):
            place('0020',[3],(2,0,z),(9,12,14),'wayside grave')
            place('1000',[82],(53,0,z),(10,25,12),'tree')
        # The church front closes the path: a retail masonry facade inside the shell.
        place('0030',[87,88,90],(0,0,128),(64,hw,12),'church front')
    room_obj(directory/'support.obj',boxes)
    kit.groups.extend(b.read_obj(directory/'support.obj'))
    entry=bytearray([55,0,182,0]);frame=bytearray()
    for slot,(x,z,side,target,landing) in enumerate(spec['doors']):
        facing=32768 if side=='S' else 0
        j['positions'].append(b.zero(rdx.POS,px=x,pz=z+10 if side=='S' else z-10,ay=facing))
        j['triggers'].append(atr(type=0,px=x-6,pz=z if side=='S' else z-4,w=12,d=4,
                                prm0=target//100,prm1=target%100,prm2=landing,prm3=22))
        j['objects'].append(b.zero(rdx.ETTY,flg=1,id=900,px=x,pz=z,ay=-32768 if facing==32768 else facing,hide=[-1]*4))
        j['object_assets'].append({'door':22})
    for slot,(item,x,z,bit) in enumerate(spec['items']):
        donor_record=0 if item==12 else 2
        rec=copy.deepcopy(kit.room('0060').records('items')[donor_record])
        rec.update(px=x,py=0.,pz=z,ay=0,hide=[-1]*4)
        j['items'].append(rec);j['item_assets'].append({'room':'0060','record':donor_record})
        trigger=len(j['triggers']);j['triggers'].append(atr(type=4,px=x-3,pz=z-3,w=6,d=6,prm0=slot))
        entry.extend(struct.pack('<BBHBB',35,slot,bit,trigger,0));frame.extend(struct.pack('<BBHBB',14,0,bit,trigger,0))
    for slot,(x,z,bit) in enumerate(spec['enemies']):
        j['enemies'].append(b.zero(rdx.ETTY,flg=1,id=1,mdlver=1,wrk_no=slot,px=x,pz=z))
        j['enemy_assets'].append({'room':'0060','record':0})
        entry.extend(struct.pack('<BBH',34,slot,bit));frame.extend(struct.pack('<BBH',13,slot,bit))
    j['scripts']=[(entry+bytes(2)).hex(),(frame+bytes(2)).hex()]
    # Fixed cameras inside the shell, below the outdoor walls / indoor ceiling and pitched at
    # least half the vertical view down, so no shot reaches above the shell (no void).
    from village_validate import in_view, walkable, visual_boxes
    from area_validate import segment_hits_box
    # The pyre flames count as occluders too: a camera behind them shows mostly fire.
    design={'boxes':boxes,'placements':kit.placements,
            'flames':[(e['px']-4,0,e['pz']-4,8,26,8) for e in j.get('effects',[])]}
    for areas,(x,y,z,tx,tz) in spec['cuts']:
        cam=camera(x,y,z,tx,tz)
        # Retail-style cut masks hide a prop only when it would still hide the player.
        samples=[(px,16.5,pz) for px,pz in walkable(j['walls'],w,d) if any(a<=px<=c and e<=pz<=f for a,e,c,f in areas)]
        for placement in kit.placements:
            lo,hi=placement['bounds'];box=(*lo,*(hi[k]-lo[k] for k in range(3)))
            if any(in_view(cam,q) and segment_hits_box((x,y,z),q,box) for q in samples):
                for node in placement['render_nodes']:
                    cam['hidobj'][node//32] |= 0x80000000>>(node%32)
        x0,z0=min(a[0] for a in areas),min(a[1] for a in areas);x1,z1=max(a[2] for a in areas),max(a[3] for a in areas)
        j['cuts'].append(dict(b.zero(rdx.CUT,flg=1,cx=x0,cz=z0,cw=x1-x0,cd=z1-z0),cam=[cam,copy.deepcopy(cam)],
                              areas=[b.zero(rdx.CTAB,minx=a,minz=e,maxx=c,maxz=f) for a,e,c,f in areas],exd='00'*256))
    kit.write(directory/'room.obj');j['retail_textures']=kit.refs
    j['_design']={'name':spec['name'],**design}
    (directory/'room.json').write_text(json.dumps(j,indent=2)+'\n')
    return j


def patch_graveyard(room,args):
    """Append records. Never renumber original arrays or rewrite retail bytecode."""
    j=room.to_json();models=b.split_table(room,'model');textures=b.split_table(room,'texture')
    _,parts=assets.door_pack(rdx.open_disc(args).iso,22)
    mi=1+sum(bool(e['flg']&1) for e in j['enemies'])+max(0,len(j['objects'])-4)
    # New object's upload is immediately before the first item (or following effects).
    deps=assets.dependencies(room)
    item_tex=[ti for (kind,_),(model,ti) in deps.items() if kind=='items' and ti is not None]
    if not item_tex:raise ValueError('graveyard expected a textured item')
    models.insert(mi,parts[0])
    # room.c loads one texture list per object id: a second door of id 900 shares the first one's
    if not assets.owns_texture(room,'objects',900):textures.insert(min(item_tex),parts[1])
    j['objects'].append(b.zero(rdx.ETTY,flg=1,id=900,px=178.,py=0.,pz=88.,ay=16384,hide=[-1]*4))
    # Retail collision keeps the player at z>=92.5 next to the east wall (an obstacle covers
    # x>=164, z<=89): land in the free strip in front of the door, facing west.
    j['positions'].append(b.zero(rdx.POS,px=168.,py=0.,pz=93.,ay=16384))
    j['triggers'].append(atr(type=0,px=174.,pz=82.,w=4.,d=12.,prm0=1,prm1=22,prm2=0,prm3=22))
    out=rdx.Room(b.relayout(room,j,{1:b.table_blob(models),4:b.table_blob(textures,True)}),room.name)
    assets.dependencies(out)   # raises unless the game reads exactly the lists the table holds
    for kind,pi,ci,st in rdx.SECTIONS:
        n=room.rom['cnt'][ci];a=room.rom['ptr'][pi];z=out.rom['ptr'][pi]
        assert room.d[a:a+n*st[0].size]==out.d[z:z+n*st[0].size],kind+' original records changed'
    assert b.split_table(room,'motion')==b.split_table(out,'motion')
    script=room.d[room.hdr['hed'][3]:room.hdr['hed'][4]]
    assert script==out.d[out.hdr['hed'][3]:out.hdr['hed'][3]+len(script)]
    return bytes(out.d)


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--iso');args=p.parse_args()
    scratch=assets.output_dir(ROOT/'scratch/village');mods=ROOT/'mods';mods.mkdir(exist_ok=True)
    names,used,_=census(args)
    for spec in SPECS:
        if 'RM_%04d.RDX'%spec['id'] in names:raise ValueError('room id already exists')
        for key,rows in [('it_flg',spec['items']),('ed_flg',spec['enemies'])]:
            if any(row[-1] in used[key] or not 472<=row[-1]<500 for row in rows):raise ValueError('flag allocation collision')
    compiled={};sources={}
    for spec in SPECS:
        print('Building',spec['name'],flush=True)
        dest=scratch/str(spec['id']);sources[spec['id']]=source(spec,args,dest)
        compiled[spec['id']]=b.compile_source(dest/'room.json',args)
    compiled[20]=patch_graveyard(rdx.load_room(args,'0020'),args)
    from village_validate import validate
    report=validate(compiled,sources,SPECS)
    staged={}
    for rid,data in compiled.items():
        packed=rdx.compress(data)
        if rdx.expand(packed)!=data:raise ValueError('compression verification failed')
        if rid!=20 and len(packed)+len(data)+3*1024*1024>12845056:
            raise ValueError('arena estimate exceeded')
        if rid==20:
            retail=rdx.load_room(args,'0020')
            delta=len(data)-len(retail.d)
            if delta>128*1024:raise ValueError('graveyard addition exceeds 128 KiB incremental allowance')
            report[str(rid)]['added_expanded_bytes']=delta
            report[str(rid)]['budget_note']='Retail room exceeds authored packed+expanded+3MiB heuristic already; incremental check only.'
        staged[rid]=packed
        report[str(rid)]['packed_bytes']=len(packed)
    # Publish only after every room has passed. Output files are individually atomic.
    for rid,data in staged.items():
        path=mods/('RM_%04d.RDX'%rid);tmp=path.with_suffix('.tmp');tmp.write_bytes(data);tmp.replace(path)
        if rid!=20:(mods/('RM_%04d.SPQ.key'%rid)).write_text('60\n')
    (mods/'village-validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))

if __name__=='__main__':main()
