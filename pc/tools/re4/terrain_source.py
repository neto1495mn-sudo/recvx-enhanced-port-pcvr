"""Spatially indexed, exact vertical queries of reviewed source terrain triangles."""
import collections
import math
import numpy as np
from navigation import read_obj


class TerrainSource:
    def __init__(self, faces, mapping, transform, bin_size=32.):
        triangles=[]
        for group,material,vertices in faces:
            info=dict(mapping['materials'].get(material,{}));overrides=mapping.get('group_overrides',{}).get(group,{})
            for key in ('*',material):info.update(overrides.get(key,{}))
            if info.get('exclude') or material in mapping.get('navigation_excluded_materials',[]) or info.get('class')!='ground':continue
            ps=(np.asarray(vertices)-transform['source_origin'])*transform['scale']
            if transform.get('reflect_z'):ps[:,2]*=-1
            n=np.cross(ps[1]-ps[0],ps[2]-ps[0]);length=np.linalg.norm(n)
            if length<1e-9 or abs(n[1])/length<=.2:continue
            triangles.append(ps)
        self.triangles=np.array(triangles);self.bin_size=bin_size;self.bins=collections.defaultdict(list)
        for i,tri in enumerate(self.triangles):
            low=np.floor(tri[:,[0,2]].min(axis=0)/bin_size).astype(int);high=np.floor(tri[:,[0,2]].max(axis=0)/bin_size).astype(int)
            for x in range(low[0],high[0]+1):
                for z in range(low[1],high[1]+1):self.bins[x,z].append(i)

    def heights(self, points):
        points=np.asarray(points,float);result=np.full(len(points),np.nan)
        buckets=collections.defaultdict(list)
        for i,key in enumerate(np.floor(points/self.bin_size).astype(int)):buckets[tuple(key)].append(i)
        for key,indices in buckets.items():
            ids=self.bins.get(key,[])
            if not ids:continue
            tris=self.triangles[ids];a=tris[:,0];u=tris[:,1]-a;v=tris[:,2]-a;det=u[:,0]*v[:,2]-u[:,2]*v[:,0]
            for start in range(0,len(indices),2048):
                ix=np.array(indices[start:start+2048]);p=points[ix];dx=p[:,None,0]-a[None,:,0];dz=p[:,None,1]-a[None,:,2]
                s=(dx*v[None,:,2]-dz*v[None,:,0])/det;t=(u[None,:,0]*dz-u[None,:,2]*dx)/det
                good=(s>=-1e-8)&(t>=-1e-8)&(s+t<=1+1e-8)
                y=a[None,:,1]+s*u[None,:,1]+t*v[None,:,1]
                best=np.argmin(np.where(good,abs(y),np.inf),axis=1);valid=good[np.arange(len(ix)),best]
                result[ix[valid]]=y[np.arange(len(ix))[valid],best[valid]]
        return result

    def grid(self,bounds,step):
        x,z,r,t=bounds;xx,zz=np.meshgrid(np.linspace(x,r,round((r-x)/step)+1),np.linspace(z,t,round((t-z)/step)+1),indexing='ij')
        return self.heights(np.column_stack((xx.ravel(),zz.ravel()))).reshape(xx.shape)
