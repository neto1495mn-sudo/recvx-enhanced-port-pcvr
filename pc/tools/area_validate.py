"""Offline geometric, graph, script and resource checks for the authored annex.

These are conservative static checks, not substitutes for the in-game acceptance run.
"""
from collections import deque
import math
import rdx
import rdxbuild as b


def require(condition,message):
    if not condition:raise ValueError(message)


def contains(r,x,z,pad=0):
    return r['px']-pad<=x<=r['px']+r['w']+pad and r['pz']-pad<=z<=r['pz']+r['d']+pad


def segment_hits_box(p,q,box):
    """Slab test: does segment p-q pass through the open box (x,y,z,w,h,d)?"""
    t0,t1=0.,1.
    for k in range(3):
        lo,hi=box[k],box[k]+box[k+3];dk=q[k]-p[k]
        if abs(dk)<1e-9:
            if not lo<p[k]<hi:return False
            continue
        a,c=sorted(((lo-p[k])/dk,(hi-p[k])/dk))
        t0,t1=max(t0,a),min(t1,c)
        if t0>=t1:return False
    return True


def room(j,compiled,packed_size):
    w,d=int(j['room']['w']),int(j['room']['d'])
    walls=compiled.records('walls'); floors=[f for f in compiled.records('floors') if f['type']==1]
    # Measured in game (RECVX_NOCLIP logs): the player stops 3.5 units from low boxes
    # (benches, desks, crates up to 10 high) but 5.5 from tall ones (shelves 17+, shell walls).
    def clear(x,z):
        return any(contains(f,x,z) for f in floors) and not any(contains(r,x,z,5.5 if r['h']>12 else 3.5) for r in walls)
    # Generated room shell must be closed both visually and physically, including the corners.
    expected=[(-2,-2,2,d+4),(w,-2,2,d+4),(0,-2,w,2),(0,d,w,2)]
    require([(r['px'],r['pz'],r['w'],r['d']) for r in walls[:4]]==expected,'wall shell not closed')
    boxes=j['_design']['boxes']
    require(boxes[0][:6]==(0,-1,0,w,1,d) or list(boxes[0][:6])==[0,-1,0,w,1,d],'visual floor gap')
    for idx,r in enumerate(walls[:4],1):
        require(list(boxes[idx][:6])==[r['px'],r['py'],r['pz'],r['w'],r['h'],r['d']],'visual wall gap')
    require(list(boxes[5][:6])==[0,30,0,w,1,d],'ceiling shell gap')
    for rec in walls:
        require(rec['type']==0 and min(rec['w'],rec['d'],rec['h'])>0,'unsupported collider')
    walkable={(x,z) for x in range(w+1) for z in range(d+1) if clear(x,z)}
    positions=compiled.records('positions')
    require(all(clear(p['px'],p['pz']) for p in positions),'entry inside wall or outside floor')
    start=min(walkable,key=lambda q:math.dist(q,(positions[0]['px'],positions[0]['pz'])))
    reached={start};queue=deque([start])
    while queue:
        x,z=queue.popleft()
        for q in ((x+1,z),(x-1,z),(x,z+1),(x,z-1)):
            if q in walkable and q not in reached:reached.add(q);queue.append(q)
    require(reached==walkable,'disconnected walkable floor (radius 3.5, unit grid)')
    # All potentially active cuts must frame feet and head at every walkable sample, from a
    # camera inside the closed shell (nothing hidden, so no view of the void outside), with
    # the head top (Claire 16.5) not behind a prop.
    cuts=compiled.cuts()
    props=[bx for bx in boxes[6:] if bx[4]>2]
    for c in cuts:
        cam=c['cam'][0]
        require(0<cam['px']<w and 0<cam['py']<30 and 0<cam['pz']<d,'camera outside the room shell')
        require(not any(cam['hidobj']),'camera hides shell geometry')
        # The game drops big ceiling triangles passing over a camera (a wedge of background in
        # game), so the top of the view must stay below the horizontal.
        require(cam['ax']*math.tau/65536>=math.atan(math.tan(cam['pers']*math.pi/65536)*.75),'camera sees the ceiling')
    for x,z in walkable:
        active=[c for c in cuts if any(a['minx']<=x<a['maxx'] and a['minz']<=z<a['maxz'] for a in c['areas'])]
        require(active,'camera coverage gap at %s'%((x,z),))
        require(any(contains(r,x,z) for r in compiled.records('routes')),'route coverage gap')
        for c in active:
            cam=c['cam'][0];pitch=cam['ax']*math.tau/65536
            require(cam['ay'] in (0,32768),'validator expects a +Z or -Z camera')
            ahead=1 if cam['ay']==32768 else -1
            half=math.tan(cam['pers']*math.pi/65536)
            require(not any(segment_hits_box((cam['px'],cam['py'],cam['pz']),(x,16.5,z),bx) for bx in props),
                    'prop hides the player at %s'%((x,z),))
            for y in (0,17.5):
                dx,dy,dz=x-cam['px'],y-cam['py'],(z-cam['pz'])*ahead
                depth=dz*math.cos(pitch)-dy*math.sin(pitch)
                up=dy*math.cos(pitch)+dz*math.sin(pitch)
                require(depth>0 and abs(dx)<depth*half and abs(up)<depth*half*.75,
                        'camera frustum misses player at %s'%((x,y,z),))
    witnesses=[]
    for i,t in enumerate(compiled.records('triggers')):
        # bhCheckExmAtari has a 4.5-unit forward probe; test four cardinal facings.
        candidates=[]
        for x,z in reached:
            for dx,dz,angle in [(0,-4.5,0),(-4.5,0,16384),(0,4.5,32768),(4.5,0,49152)]:
                if not contains(t,x,z) and contains(t,x+dx,z+dz):candidates.append((math.dist((x,z),(t['px']+t['w']/2,t['pz']+t['d']/2)),x,z,angle))
        require(candidates,'unreachable trigger %d'%i)
        _,x,z,angle=min(candidates)
        if i<len(positions):
            # Stable acceptance warp: eight units from the wall, facing the slab.
            if t['pz']==0:x,z,angle=w/2,8,0
            elif t['pz']+t['d']==d:x,z,angle=w/2,d-8,32768
            elif t['px']==0:x,z,angle=8,d/2,16384
            else:x,z,angle=w-8,d/2,49152
            require(clear(x,z),'door approach blocked')
            rad=angle*math.tau/65536
            require(contains(t,x-4.5*math.sin(rad),z-4.5*math.cos(rad)),'door probe misses trigger')
        witnesses.append({'trigger':i,'x':x,'z':z,'angle':angle})
    for kind in ('items','enemies'):
        for rec in compiled.records(kind):
            require(clear(rec['px'],rec['pz']),kind+' inside collision or outside floor')
            if kind=='enemies':
                require(all(math.dist((rec['px'],rec['pz']),(p['px'],p['pz']))>=16 for p in positions),'enemy too close to entry')
    b.validate_authored(compiled);b.validate_enemy_mix(compiled.records('enemies'))
    # The scene uses no object slots beyond the four reserved player entries.
    require(len(compiled.records('objects'))<=32 and len(compiled.records('items'))<=32,'object/item slots')
    ts=[t for _,lst in rdx.texture_blocks(compiled) for t in lst]
    texture_bytes=sum(t['image_size']+t['clut_size'] for t in ts)
    require(texture_bytes+4*1024*1024<10*1024*1024,'texture heap reserve')
    require(len(ts)+64<256,'texture entry reserve')
    # Leave 2 MiB persistent + 1 MiB vertex/AI/workspace beyond packed and expanded data.
    require(len(compiled.d)+packed_size+3*1024*1024<12845056,'load-time arena reserve')
    mo=rdx.model_objects(compiled);triangles=0
    require(len(mo['objects'])==len(boxes),'visual model group mismatch')
    for ob in mo['objects']:
        vl,pl,_,_=rdx.cnk_model(compiled.d,mo['top'],ob['model'])
        polys,_=rdx.parse_plist(compiled.d,mo['top']+pl)
        triangles+=sum(len(strip)-2 for tid,strips in polys for _,strip in strips)
    require(triangles<5000,'room triangle budget')
    return {'walkable_samples':len(walkable),'triangles':triangles,'model_nodes':len(boxes),
            'textures':len(ts),'texture_bytes':texture_bytes,'expanded_bytes':len(compiled.d),
            'packed_bytes':packed_size,'conservative_enemy_slots':sum(11 if e['id']==1 else 4 for e in compiled.records('enemies')),
            'trigger_witnesses':witnesses}


def graph(spec):
    rooms={int(r['id'][:3]):r for r in spec['rooms']}
    edges={k:set() for k in rooms}
    for rid,r in rooms.items():
        for entry,(side,target,pos) in enumerate(r['doors']):
            if target not in rooms:continue
            back=rooms[target]['doors']
            require(0<=pos<len(back),'destination entry outside room')
            require(back[pos][1:]==[rid,entry],'door is not reciprocal')
            if target!=120:edges[rid].add(target)
    reached={116};todo=[116]
    while todo:
        for n in edges[todo.pop()]-reached:reached.add(n);todo.append(n)
    require(118 in reached and 120 not in reached,'key inaccessible before lock or reward bypass')
    require(reached==set(rooms)-{120},'main loop disconnected')
    require(rooms[120]['doors']==[['W',119,1]],'reward must have unconditional return')


def allocations(spec,used):
    for bank,col in [('it_flg','items'),('ed_flg','enemies')]:
        flags=[rec[-1] for r in spec['rooms'] for rec in r[col]]
        require(len(flags)==len(set(flags)),'duplicate '+bank)
        require(all(0<=f<512 for f in flags),'9-bit flag overflow')
        require(not set(flags)&set(used[bank]),'retail flag collision: '+str(set(flags)&set(used[bank])))
        require(not set(flags)&({506} if bank=='it_flg' else {500}),'Service Corridor flag collision')
