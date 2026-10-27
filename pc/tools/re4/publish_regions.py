#!/usr/bin/env python3
"""Validate and publish generated bounded rooms; archive previous generated revisions."""
import argparse
import collections
import hashlib
import json
import math
from pathlib import Path
import sys
import time
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import rdx
import rdxbuild as b
from village_validate import walkable,in_view


def validate(directory):
    raw=(directory/'room.rdx.raw').read_bytes();room=rdx.Room(raw)
    source=json.loads((directory/'room.json').read_text())
    build=json.loads((directory/'build-report.json').read_text())
    rays=json.loads((directory/'ray-check.json').read_text())
    b.validate_authored(room)
    walk=walkable(room.records('walls'),math.ceil(source['room']['w']),math.ceil(source['room']['d']))
    positions=room.records('positions')
    assert walk and positions
    assert all((round(p['px']),round(p['pz'])) in walk for p in positions),'blocked entry'
    reached={(round(positions[0]['px']),round(positions[0]['pz']))};queue=collections.deque(reached)
    while queue:
        x,z=queue.popleft()
        for q in ((x-1,z),(x+1,z),(x,z-1),(x,z+1)):
            if q in walk and q not in reached:
                reached.add(q);queue.append(q)
    assert reached==walk,'disconnected walking area'
    for x,z in walk:
        active=[c for c in room.cuts() if any(a['minx']<=x<=a['maxx'] and a['minz']<=z<=a['maxz'] for a in c['areas'])]
        assert active,'camera gap'
        assert all(in_view(c['cam'][0],(x,y,z),1) for c in active for y in (0,17.5)),'frustum failure'
    assert rays['head_occlusions']==0,rays
    assert build['source_ground_missing_samples']==0
    nv=nt=0;mo=rdx.model_objects(room)
    for ob in mo['objects']:
        vl,pl,*_=rdx.cnk_model(room.d,mo['top'],ob['model'])
        nv+=len(rdx.parse_vlist(room.d,mo['top']+vl)[0])
        for _,strips in rdx.parse_plist(room.d,mo['top']+pl)[0]:
            nt+=sum(max(0,len(s)-2) for _,s in strips)
    assert nt<=13639 and len(mo['objects'])<=120
    assert (nt,nv)==(build['triangles'],build['vertices'])
    packed=rdx.compress(raw);assert rdx.expand(packed)==raw
    assert len(raw)+len(packed)+3*1024*1024<12845056
    report={'id':build['id'],'name':build['name'],'teleport':f"1:{build['id']//1000}:{build['id']//10%100}:0",
            'expanded_bytes':len(raw),'packed_bytes':len(packed),'triangles':nt,'vertices':nv,'nodes':len(mo['objects']),
            'textures':sum(len(ts) for _,ts in rdx.texture_blocks(room)),'walkable_samples':len(walk),'cuts':len(room.cuts()),
            'source_ground_missing_samples':0,'disconnected_samples':0,'camera_frustum_failures':0,'mesh_rays':rays,
            'compression_roundtrip':True,'runtime_tested':False,'sha256':hashlib.sha256(packed).hexdigest()}
    return packed,report


def install(rid,packed):
    mods=Path('mods');mods.mkdir(exist_ok=True);dest=mods/f'RM_{rid:04d}.RDX'
    if dest.exists():
        archive=Path('scratch/re4/archive');archive.mkdir(parents=True,exist_ok=True)
        (archive/f'RM_{rid:04d}.{time.time_ns()}.RDX').write_bytes(dest.read_bytes())
    tmp=dest.with_suffix('.tmp');tmp.write_bytes(packed);tmp.replace(dest)
    if rid!=20:
        (mods/f'RM_{rid:04d}.SPQ.key').write_text('60\n')


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('recipe',type=Path);args=p.parse_args()
    recipe=json.loads(args.recipe.read_text());reports={};staged={}
    for spec in recipe['rooms']:
        rid=spec['id'];assert f'RM_{rid:04d}.RDX' not in rdx.room_names(),'retail ID collision'
        staged[rid],reports[str(rid)]=validate(Path(recipe['output'])/str(rid))
    for rid,packed in staged.items():
        install(rid,packed)
        (Path(recipe['output'])/str(rid)/'final-validation.json').write_text(json.dumps(reports[str(rid)],indent=2)+'\n')
    Path('mods/re4-regions-validation.json').write_text(json.dumps(reports,indent=2)+'\n')
    print(json.dumps(reports,indent=2))


if __name__=='__main__':
    main()
