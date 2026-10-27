"""Offline village checks: geometry budgets, navigation, pickups, cameras and reciprocal doors."""
from collections import deque
import math
import rdx
import rdxbuild as b
from area_validate import segment_hits_box


def require(value,message):
    if not value:raise ValueError(message)


def walkable(walls,w,d):
    """1-unit grid of player positions, with the clearances measured in game (docs/modding/BUILDING_ROOMS.md "Validation")."""
    def clear(x,z):
        return 5.5<=x<=w-5.5 and 5.5<=z<=d-5.5 and not any(
            a['px']-(5.5 if a['h']>12 else 3.5)<x<a['px']+a['w']+(5.5 if a['h']>12 else 3.5)
            and a['pz']-(5.5 if a['h']>12 else 3.5)<z<a['pz']+a['d']+(5.5 if a['h']>12 else 3.5) for a in walls)
    return {(x,z) for x in range(w+1) for z in range(d+1) if clear(x,z)}


def axes(cam):
    """Forward, right, up; ay 0 looks -Z and 16384 +X, ax>0 looks down (retail cut cameras)."""
    ay,ax=cam['ay']*math.tau/65536,cam['ax']*math.tau/65536
    f=(math.sin(ay)*math.cos(ax),-math.sin(ax),-math.cos(ay)*math.cos(ax));r=(math.cos(ay),0.,math.sin(ay))
    return f,r,(r[1]*f[2]-r[2]*f[1],r[2]*f[0]-r[0]*f[2],r[0]*f[1]-r[1]*f[0])


def in_view(cam,p,margin=.95):
    """Inside the 4:3 view (pers = horizontal angle; widescreen only adds to the sides)."""
    f,r,u=axes(cam);rel=[p[k]-cam[('px','py','pz')[k]] for k in range(3)]
    depth=sum(a*b for a,b in zip(rel,f));t=math.tan(cam['pers']*math.pi/65536)*margin
    return depth>0 and abs(sum(a*b for a,b in zip(rel,r)))<depth*t and abs(sum(a*b for a,b in zip(rel,u)))<depth*t*.75


def visual_boxes(design):
    """Occluders as (x,y,z,w,h,d): placed props and support boxes except the floor slab."""
    out=[(*p['bounds'][0],*(p['bounds'][1][k]-p['bounds'][0][k] for k in range(3))) for p in design['placements']]
    return out+[tuple(bx[:6]) for bx in design['boxes'][1:]]+[tuple(f) for f in design.get('flames',[])]


def validate(compiled,sources,specs):
    report={}
    rooms={rid:rdx.Room(data) for rid,data in compiled.items()}
    for spec in specs:
        rid=spec['id'];room=rooms[rid];w,d=spec['size'];walls=room.records('walls')
        walk=walkable(walls,w,d);clear=lambda x,z:(round(x),round(z)) in walk
        positions=room.records('positions')
        require(all(clear(p['px'],p['pz']) for p in positions),str(rid)+' entry blocked')
        start=min(walk,key=lambda q:math.dist(q,(positions[0]['px'],positions[0]['pz'])))
        reached={start};queue=deque([start])
        while queue:
            x,z=queue.popleft()
            for q in [(x-1,z),(x+1,z),(x,z-1),(x,z+1)]:
                if q in walk and q not in reached:reached.add(q);queue.append(q)
        require(walk==reached,str(rid)+' disconnected walking region')
        for kind in ('items','enemies'):
            require(all(clear(r['px'],r['pz']) for r in room.records(kind)),str(rid)+' blocked '+kind)
        witnesses=[]
        for i,t in enumerate(room.records('triggers')):
            candidates=[]
            for x,z in reached:
                for dx,dz,angle in [(0,-4.5,0),(-4.5,0,16384),(0,4.5,32768),(4.5,0,49152)]:
                    if t['px']<=x+dx<=t['px']+t['w'] and t['pz']<=z+dz<=t['pz']+t['d']:
                        candidates.append((math.dist((x,z),(t['px']+t['w']/2,t['pz']+t['d']/2)),x,z,angle))
            require(candidates,str(rid)+' unreachable trigger '+str(i))
            _,x,z,angle=min(candidates);witnesses.append({'trigger':i,'x':x,'z':z,'angle':angle})
        cuts=room.cuts();misses=[];occluded=[];design=sources[rid]['_design']
        for c in cuts:
            cam=c['cam'][0];t=math.tan(cam['pers']*math.pi/65536)
            # No shot reaches above the shell: camera below its top, view pitched down enough.
            require(cam['py']<spec['wall'] and cam['ax']*math.tau/65536>=math.atan(.75*t),str(rid)+' camera can see past the shell')
            require(2<=cam['px']<=w-2 and 2<=cam['pz']<=d-2 and not any(
                bx[0]<cam['px']<bx[0]+bx[3] and bx[1]<cam['py']<bx[1]+bx[4] and bx[2]<cam['pz']<bx[2]+bx[5]
                for bx in visual_boxes(design)),str(rid)+' camera outside the shell or inside a prop')
            # Flames are billboards: near a camera they fill the shot.
            require(all(math.hypot(cam['px']-f[0]-f[3]/2,cam['pz']-f[2]-f[5]/2)>=35 for f in design.get('flames',[])),
                    str(rid)+' camera too close to the flames')
        for x,z in reached:
            active=[c for c in cuts if any(a['minx']<=x<=a['maxx'] and a['minz']<=z<=a['maxz'] for a in c['areas'])]
            require(active,str(rid)+' camera coverage gap at '+str((x,z)))
            for c in active:
                cam=c['cam'][0]
                for y in (0,17.5):
                    if not in_view(cam,(x,y,z),1.):misses.append((x,y,z))
                hidden={n for n in range(512) if cam['hidobj'][n//32] & (0x80000000>>(n%32))}
                boxes=[bx for p,bx in zip(design['placements'],visual_boxes(design)) if not set(p['render_nodes'])<=hidden]
                boxes+=visual_boxes(design)[len(design['placements']):]
                if any(segment_hits_box((cam['px'],cam['py'],cam['pz']),(x,16.5,z),bx) for bx in boxes):occluded.append((x,z))
        require(not misses,str(rid)+' camera misses '+str(sorted(set(misses))[:3]))
        require(not occluded,str(rid)+' foreground props obscure player '+str(sorted(set(occluded))[:3]))
        ts=[t for _,lst in rdx.texture_blocks(room) for t in lst]
        b.validate_authored(room)
        report[str(rid)]={'name':spec['name'],'expanded_bytes':len(room.d),'textures':len(ts),
                          'texture_bytes':sum(t['size'] for t in ts),'walkable_samples':len(walk),
                          'enemy_slot_reserve':len(spec['enemies'])*11,'interaction_witnesses':witnesses,
                          'camera_frustum_failures':len(misses),'conservative_prop_occlusion_samples':len(set(occluded))}
        for i,(_,_,_,target,entry) in enumerate(spec['doors']):
            other=rooms[target*10]
            require(entry<len(other.records('positions')),'missing target landing')
            require(any(t['type']==0 and t['prm0']*100+t['prm1']==rid//10 and t['prm2']==i
                        for t in other.records('triggers')),'nonreciprocal door')
    report['20']={'name':'Graveyard append-only patch','expanded_bytes':len(rooms[20].d),
                  'new_trigger':len(rooms[20].records('triggers'))-1,'new_entry':len(rooms[20].records('positions'))-1,
                  'retail_records_scripts_preserved':True}
    return report


def main():
    import argparse
    import json
    from pathlib import Path
    import build_village as v
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--iso')
    p.add_argument('--mods',type=Path,default=v.ROOT/'mods')
    p.add_argument('--source',type=Path,default=v.ROOT/'scratch/village')
    args=p.parse_args()
    compiled={rid:rdx.expand((args.mods/('RM_%04d.RDX'%rid)).read_bytes()) for rid in [20]+[s['id'] for s in v.SPECS]}
    sources={s['id']:json.loads((args.source/str(s['id'])/'room.json').read_text()) for s in v.SPECS}
    expected=v.patch_graveyard(rdx.load_room(args,'0020'),args)
    require(compiled[20]==expected,'graveyard patch differs from verified append-only recipe')
    print(json.dumps(validate(compiled,sources,v.SPECS),indent=2))

if __name__=='__main__':main()
