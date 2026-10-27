"""Connect generated rooms using an ignored manifest; stage and validate before install.

Coordinates and placements belong in scratch recipes. A passage uses animation 255
and no door asset; architectural doors use the retail type-22 object and animation.
"""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import struct
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import rdx
import rdxbuild as b
from build_area import atr
from village_validate import walkable
from build_rooms import flood, output_path
from publish_regions import install


def connect(manifest, args):
    out=output_path(manifest['output'])
    sources={int(k):Path(v).resolve() for k,v in manifest['sources'].items()}
    rooms={rid:json.loads(path.read_text()) for rid,path in sources.items()}
    for rid,j in rooms.items():
        j['mesh']=str((sources[rid].parent/j['mesh']).resolve())
        j.setdefault('triggers',[])
        j.setdefault('object_assets',[])
    edges=[]
    for link in manifest['links']:
        ends=link['ends'];positions=[]
        for end in ends:
            j=rooms[end['room']];positions.append(len(j['positions']))
            x,z,angle=end['landing']
            j['positions'].append(b.zero(rdx.POS,px=x,pz=z,ay=angle))
        for i,end in enumerate(ends):
            other=ends[1-i];j=rooms[end['room']]
            x,z,w,d=end['trigger'];target=other['room']
            animation=22 if link['kind']=='door' else 255
            j['triggers'].append(atr(type=0,px=x,pz=z,w=w,d=d,prm0=target//1000,prm1=target//10%100,prm2=positions[1-i],prm3=animation))
            if animation==22:
                x,z,angle=end['model']
                j['objects'].append(b.zero(rdx.ETTY,flg=1,id=900,px=x,pz=z,ay=angle,hide=[-1]*4))
                j['object_assets'].append({'door':22})
            edges.append(dict(source=end['room'],target=target,entry=positions[1-i],kind=link['kind']))
    used={'items':set(),'enemies':set()}
    unavailable={'items':set(), 'enemies':set()}
    if manifest.get('features'):
        retail=json.loads(Path(manifest['retail_flags']).read_text())
        unavailable={'items':set(retail['it_flg']), 'enemies':set(retail['ed_flg'])}
        # Also inspect installed custom rooms so the CVX village keeps its bits.
        for directory in manifest.get('flag_avoid_mods', ['mods']):
            for path in Path(directory).glob('RM_*.RDX'):
                if int(path.stem[3:]) in rooms:continue
                room=rdx.Room(rdx.expand(path.read_bytes()));seen=[]
                if not rdx.disasm(room,seen)[1]:raise ValueError(f'unclean script: {path}')
                for off in set(seen):
                    op=room.d[off]
                    if rdx.insn_len(room.d,off)<4:continue
                    bit=struct.unpack_from('<H',room.d,off+2)[0]&1023
                    if op in (14,35):unavailable['items'].add(bit)
                    if op in (13,34):unavailable['enemies'].add(bit)
                    if op in (4,5) and room.d[off+1] in (3,7):unavailable['enemies' if room.d[off+1]==3 else 'items'].add(bit)
                    if op==33:unavailable['enemies'].update(range(bit,bit+room.d[off+1]))
    donor=rdx.load_room(args,'0060')
    for rid,features in manifest.get('features',{}).items():
        j=rooms[int(rid)];entry=bytearray.fromhex(j['scripts'][0])[:-2];frame=bytearray.fromhex(j['scripts'][1])[:-2]
        for kind in ('items','enemies'):
            j.setdefault(kind,[]);j.setdefault('item_assets' if kind=='items' else 'enemy_assets',[])
            for feature in features.get(kind,[]):
                x,z,bit=feature['x'],feature['z'],feature['flag']
                if not 0<=bit<512 or bit in used[kind] or bit in unavailable[kind]:raise ValueError('invalid, occupied or duplicate persistence bit')
                used[kind].add(bit);slot=len(j[kind])
                if kind=='items':
                    record=feature['donor_record'];rec=copy.deepcopy(donor.records('items')[record]);rec.update(px=x,py=0,pz=z,ay=0,hide=[-1]*4)
                    j[kind].append(rec);j['item_assets'].append({'room':'0060','record':record})
                    trigger=len(j['triggers']);j['triggers'].append(atr(type=4,px=x-3,pz=z-3,w=6,d=6,prm0=slot))
                    entry.extend(struct.pack('<BBHBB',35,slot,bit,trigger,0));frame.extend(struct.pack('<BBHBB',14,0,bit,trigger,0))
                else:
                    j[kind].append(b.zero(rdx.ETTY,flg=1,id=1,mdlver=1,wrk_no=slot,px=x,pz=z))
                    j['enemy_assets'].append({'room':'0060','record':0})
                    entry.extend(struct.pack('<BBH',34,slot,bit));frame.extend(struct.pack('<BBH',13,slot,bit))
        j['scripts']=[(entry+bytes(2)).hex(),(frame+bytes(2)).hex()]
    staged={};reports={}
    grave_report=None
    if manifest.get('graveyard'):
        import graveyard_link
        spec=manifest['graveyard'];base=graveyard_link.load(spec)
        j=rooms[1260];target_entry=len(j['positions'])
        x,z,angle=spec['square_landing']
        j['positions'].append(b.zero(rdx.POS,px=x,pz=z,ay=angle))
        x,z,w,d=spec['square_trigger']
        j['triggers'].append(atr(type=0,px=x,pz=z,w=w,d=d,prm0=0,prm1=2,prm2=len(base.records('positions')),prm3=255))
        raw,grave_report=graveyard_link.patch(spec,target_entry,args)
        packed=rdx.compress(raw)
        if rdx.expand(packed)!=raw:raise ValueError('graveyard compression roundtrip')
        grave_report.update(expanded_bytes=len(raw),packed_bytes=len(packed),sha256=hashlib.sha256(packed).hexdigest())
        directory=output_path(out/'0020');(directory/'room.rdx.raw').write_bytes(raw)
        staged[20]=packed
    for rid,j in rooms.items():
        directory=output_path(out/str(rid));path=directory/'room.json';path.write_text(json.dumps(j,indent=2)+'\n')
        raw=b.compile_source(path,args);room=rdx.Room(raw);b.validate_authored(room)
        walk=walkable(room.records('walls'),math.ceil(j['room']['w']),math.ceil(j['room']['d']))
        starts=[(round(p['px']),round(p['pz'])) for p in room.records('positions')]
        if any(p not in walk for p in starts):raise ValueError(f'{rid}: blocked landing {starts}')
        if flood(walk,starts[0])!=walk:raise ValueError(f'{rid}: disconnected walk region')
        for kind in ('items','enemies'):
            for rec in room.records(kind):
                if (round(rec['px']),round(rec['pz'])) not in walk:raise ValueError(f'{rid}: blocked {kind}')
        for trigger in room.records('triggers'):
            if trigger['type']!=0:continue
            # Trigger must touch standing space or be within the engine's short
            # forward interaction probe. This is necessary, not runtime proof.
            if not any(trigger['px']-5<=x<=trigger['px']+trigger['w']+5 and trigger['pz']-5<=z<=trigger['pz']+trigger['d']+5 for x,z in walk):
                raise ValueError(f'{rid}: unreachable trigger {trigger}')
            target=trigger['prm0']*1000+trigger['prm1']*10
            if target in rooms and trigger['prm2']>=len(rooms[target]['positions']):raise ValueError('invalid target entry')
        packed=rdx.compress(raw)
        if rdx.expand(packed)!=raw:raise ValueError('compression roundtrip')
        if len(raw)+len(packed)+3*1024*1024>=12845056:raise ValueError('arena budget')
        if f'RM_{rid:04d}.RDX' in rdx.room_names():raise ValueError('retail room collision')
        (directory/'room.rdx.raw').write_bytes(raw);staged[rid]=packed
        reports[str(rid)]={'teleport':f'1:{rid//1000}:{rid//10%100}:0','walk_samples':len(walk),'entries':len(starts),'doors':sum(t['type']==0 and t['prm3']==22 for t in j['triggers']),'passages':sum(t['type']==0 and t['prm3']==255 for t in j['triggers']),'items':len(j.get('items',[])),'zombies':len(j.get('enemies',[])),'expanded_bytes':len(raw),'packed_bytes':len(packed),'sha256':hashlib.sha256(packed).hexdigest(),'runtime_tested':False}
    report={'rooms':reports,'edges':edges,'flags':{k:sorted(v) for k,v in used.items()},'graveyard':grave_report}
    (out/'validation.json').write_text(json.dumps(report,indent=2)+'\n')
    if args.publish:
        # Preserve the existing CVX village chain in this isolated mods folder.
        for source in manifest.get('preserve_files',[]):
            source=Path(source);dest=output_path('mods')/source.name
            if dest.exists() and dest.read_bytes()!=source.read_bytes():raise ValueError(f'refusing to replace unrelated mod {dest}')
        for source in manifest.get('preserve_files',[]):
            source=Path(source);(output_path('mods')/source.name).write_bytes(source.read_bytes())
        for rid,packed in staged.items():install(rid,packed)
        (output_path('mods')/'re4-connected-validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('manifest',type=Path);p.add_argument('--publish',action='store_true');args=p.parse_args()
    args.manifest.resolve().relative_to(Path('scratch').resolve())
    manifest=json.loads(args.manifest.read_text());args.iso=manifest['iso'];connect(manifest,args)

if __name__=='__main__':main()
