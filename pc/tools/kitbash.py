"""Bake retail MLB nodes with their UVs into authored room geometry.

No artwork is generated. Materials are original TIM2 references, deduplicated by payload.
AABB collision is explicit and conservative; decorative meshes need not block walking.
"""
import math
import struct
import hashlib
import rdx
import rdxbuild as b


def mesh_nodes(blob):
    class Model:
        d=blob
        def model_table(self): return [0]
    mo=rdx.model_objects(Model())
    top=mo['top'];stack=[];cur=([[1,0,0],[0,1,0],[0,0,1]],(0,0,0))
    result=[]
    for ob in mo['objects']:
        if ob['sibling']!=-1:stack.append(cur)
        rot,tr=cur
        R=rdx.euler_matrix(ob['ang'])
        R=[[R[i][j]*ob['scl'][j] for j in range(3)] for i in range(3)]
        wr=[[sum(rot[i][k]*R[k][j] for k in range(3)) for j in range(3)] for i in range(3)]
        wt=[tr[i]+sum(rot[i][k]*ob['pos'][k] for k in range(3)) for i in range(3)]
        cur=wr,wt; faces=[]
        if ob['model']!=-1:
            vl,pl,_,_=rdx.cnk_model(blob,top,ob['model'])
            vs,types=rdx.parse_vlist(blob,top+vl) if vl!=-1 else ({},[])
            ps,_=rdx.parse_plist(blob,top+pl) if pl!=-1 else ([],[])
            if set(types)-{41,51}:raise ValueError('unsupported retail vertex encoding')
            for tid,strips in ps:
                for reverse,strip in strips:
                    for i in range(len(strip)-2):
                        tri=list(strip[i:i+3])
                        if bool(i%2)^reverse:tri[0],tri[1]=tri[1],tri[0]
                        if len({v[0] for v in tri})<3:continue
                        if any(ix not in vs for ix,u,v in tri):raise ValueError('retail mesh references missing vertex')
                        face=[]
                        for ix,u,v in tri:
                            p=vs[ix];face.append((tuple(wt[r]+sum(wr[r][k]*p[k] for k in range(3)) for r in range(3)),(max(0.,min(1.,u)),max(0.,min(1.,v)))))
                        faces.append((tid,face))
        result.append(faces)
        if ob['child']==-1:cur=stack.pop() if stack else cur
    return result


def bounds(groups):
    points=[p for group in groups for _,face in group for p,_ in face]
    if not points:raise ValueError('empty selected mesh')
    return [min(p[k] for p in points) for k in range(3)],[max(p[k] for p in points) for k in range(3)]


class Kit:
    def __init__(self,args):
        self.args=args;self.rooms={};self.refs=[];self.hashes={};self.groups=[];self.placements=[]
    def room(self,name):
        if name not in self.rooms:self.rooms[name]=rdx.load_room(self.args,name)
        return self.rooms[name]
    def texture(self,name,index,li=0):
        room=self.room(name);t=rdx.texture_blocks(room)[li][1][index]
        payload=bytes(room.d[t['ofs']:t['ofs']+t['size']])
        digest=hashlib.sha256(payload).digest()
        if digest not in self.hashes:
            self.hashes[digest]=len(self.refs);self.refs.append({'room':name,'list':li,'index':index})
        return self.hashes[digest]
    def place(self,name,nodes,at,size=None,angle=0,model=0,texture_list=0,label='retail prop'):
        groups=mesh_nodes(b.split_table(self.room(name),'model')[model])
        selected=[groups[i] for i in nodes] if nodes is not None else groups
        lo,hi=bounds(selected)
        scale=[size[k]/max(hi[k]-lo[k],.001) for k in range(3)] if size else [1]*3
        c,s=math.cos(math.radians(angle)),math.sin(math.radians(angle))
        out=[]
        for group in selected:
            faces=[]
            for tid,face in group:
                mapped=[]
                for p,uv in face:
                    x,y,z=[(p[k]-lo[k])*scale[k] for k in range(3)]
                    mapped.append(((at[0]+c*x+s*z,at[1]+y,at[2]-s*x+c*z),uv))
                faces.append((self.texture(name,tid,texture_list),mapped))
            out.append(faces)
        first=sum(math.ceil(len(g)/600) for g in self.groups)
        count=sum(math.ceil(len(g)/600) for g in out if g)
        self.groups.extend(g for g in out if g)
        a,z=bounds(out)
        self.placements.append({'room':name,'model':model,'nodes':nodes,'label':label,'bounds':[a,z],'render_nodes':list(range(first,first+count))})
        return a,z
    def write(self,path):
        lines=['# Retail kitbash; regenerated from the ISO.'];n=0
        for gi,g in enumerate(self.groups):
            for start in range(0,len(g),600):
                lines.append('o mesh%d_%d'%(gi,start))
                for tid,face in g[start:start+600]:
                    lines.append('usemtl tex%d'%tid)
                    for p,uv in face:lines.extend(['v %.6f %.6f %.6f'%p,'vt %.6f %.6f'%(uv[0],1-uv[1])])
                    lines.append('f '+' '.join('%d/%d'%(n+k,n+k) for k in (1,2,3)));n+=3
        path.write_text('\n'.join(lines)+'\n')
