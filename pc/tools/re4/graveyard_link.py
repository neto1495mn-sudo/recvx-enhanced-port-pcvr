"""Append a second graveyard door while preserving every existing record and script."""
import copy
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import assets
import rdx
import rdxbuild as b
from build_area import atr
from village_validate import walkable


def load(spec):
    room=rdx.Room(rdx.expand(Path(spec['base']).read_bytes()),'RM_0020.RDX')
    if not any(t['type']==0 and (t['prm0'],t['prm1'])==(1,22) for t in room.records('triggers')):
        raise ValueError('base must contain the existing CVX village door')
    if any(t['type']==0 and (t['prm0'],t['prm1'])==(1,26) for t in room.records('triggers')):
        raise ValueError('base already contains an RE4 door')
    return room


def patch(spec, target_entry, args):
    room=load(spec);j=room.to_json();models=b.split_table(room,'model');textures=b.split_table(room,'texture')
    _,parts=assets.door_pack(rdx.open_disc(args).iso,22)
    mi=1+sum(bool(e['flg']&1) for e in j['enemies'])+max(0,len(j['objects'])-4)
    deps=assets.dependencies(room)
    item_tex=[ti for (kind,_),(model,ti) in deps.items() if kind=='items' and ti is not None]
    if not item_tex:raise ValueError('expected retail item texture table')
    models.insert(mi,parts[0])
    # room.c loads one texture list per object id: a second door of id 900 shares the first one's
    if not assets.owns_texture(room,'objects',900):textures.insert(min(item_tex),parts[1])
    x,z,angle=spec['model'];j['objects'].append(b.zero(rdx.ETTY,flg=1,id=900,px=x,pz=z,ay=angle,hide=[-1]*4))
    x,z,angle=spec['landing'];j['positions'].append(b.zero(rdx.POS,px=x,pz=z,ay=angle))
    # Type 7 records clamp height; they are not rectangular horizontal blockers.
    walk=walkable([a for a in j['walls'] if a['type']!=7],220,200)
    if (round(x),round(z)) not in walk:raise ValueError('blocked graveyard return')
    x,z,w,d=spec['trigger'];j['triggers'].append(atr(type=0,px=x,pz=z,w=w,d=d,prm0=1,prm1=26,prm2=target_entry,prm3=22))
    if not any(x-5<=a<=x+w+5 and z-5<=c<=z+d+5 for a,c in walk):raise ValueError('unreachable graveyard door')
    raw=b.relayout(room,j,{1:b.table_blob(models),4:b.table_blob(textures,True)})
    out=rdx.Room(raw,'RM_0020.RDX')
    assets.dependencies(out)   # raises unless the game reads exactly the lists the table holds
    for kind,pi,ci,st in rdx.SECTIONS:
        n=room.rom['cnt'][ci];a=room.rom['ptr'][pi];z=out.rom['ptr'][pi]
        if room.d[a:a+n*st[0].size]!=out.d[z:z+n*st[0].size]:raise ValueError(kind+' original records changed')
    if b.split_table(room,'motion')!=b.split_table(out,'motion'):raise ValueError('motions changed')
    script=room.d[room.hdr['hed'][3]:room.hdr['hed'][4]]
    if script!=out.d[out.hdr['hed'][3]:out.hdr['hed'][3]+len(script)]:raise ValueError('scripts changed')
    if len(raw)-len(room.d)>128*1024:raise ValueError('graveyard incremental memory budget')
    return raw,{'existing_records_preserved':True,'scripts_preserved':True,'cvx_door_preserved':True,'return_entry':len(j['positions'])-1,'target_entry':target_entry,'added_bytes':len(raw)-len(room.d),'runtime_tested':False}
