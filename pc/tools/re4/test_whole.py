"""Regression tests for PC partitioning and terrain evidence hazards."""
import tempfile
import unittest
from pathlib import Path
import numpy as np
from audit_terrain_seams import project_slope, patch_height
from build_whole import write_mesh, translated_enclosure, limit_sorted_nodes
from survey_whole import terrain_flood, fit_patches
from joint_terrain import joint_fit, shared_edges
import rdxbuild as b


class WholeTests(unittest.TestCase):
    def test_sorted_budget_preserves_existing_hides_and_opaque_nodes(self):
        nodes=[dict(index=i,triangles=10000,alpha_triangles=10000,
                    bounds=[[i*100,0,0],[i*100+10,20,10]]) for i in range(4)]
        nodes.append(dict(index=4,triangles=10000,alpha_triangles=0,bounds=[[900,0,0],[910,20,10]]))
        cam=dict(px=0,pz=0,hidobj=[0x80000000]+[0]*15)
        self.assertLessEqual(limit_sorted_nodes(cam,nodes),28672)
        self.assertTrue(cam['hidobj'][0]&0x80000000)
        self.assertFalse(cam['hidobj'][0]&0x08000000)

    def test_engine_hide_word_is_msb_first(self):
        nodes=[dict(index=i,triangles=1,alpha_triangles=1,bounds=[[1000,0,0],[1010,20,10]]) for i in (0,31,32,511)]
        cam=dict(px=0,pz=0)
        limit_sorted_nodes(cam,nodes)
        self.assertEqual(cam['hidobj'][0],0x80000001)
        self.assertEqual(cam['hidobj'][1],0x80000000)
        self.assertEqual(cam['hidobj'][15],1)

    def test_unhideable_alpha_overflow_rejected(self):
        node=dict(index=512,triangles=30000,alpha_triangles=30000,bounds=[[0,0,0],[1,1,1]])
        with self.assertRaises(ValueError):limit_sorted_nodes(dict(px=0,pz=0),[node])

    def test_joint_fit_recovers_continuous_ramp(self):
        h=np.repeat((np.arange(8)+.5)[:,None]*.1,4,axis=1)
        patches=[dict(bounds=[x,0,x+4,4],fit=dict(axis='x')) for x in (0,4)]
        result,report=joint_fit(patches,h,np.ones(h.shape,bool),1)
        self.assertLess(report['maximum_seam_gap'],1e-6)
        self.assertLess(report['maximum_center_error'],1e-6)
        self.assertAlmostEqual(patch_height(result[0],4,2),.4)

    def test_joint_fit_reports_incompatible_axis_constraints(self):
        h=np.zeros((8,4));h[4:]=.6
        patches=[dict(bounds=[x,0,x+4,4],fit=dict(axis='z')) for x in (0,4)]
        _,report=joint_fit(patches,h,np.ones(h.shape,bool),1)
        self.assertGreater(report['maximum_center_error'],.25)
        self.assertGreater(report['center_failures'],0)

    def test_tolerance_solver_keeps_source_and_shared_edges(self):
        h=np.repeat((np.arange(8)+.5)[:,None]*.1,4,axis=1)
        patches=[dict(bounds=[x,0,x+4,4],fit=dict(axis='x')) for x in (0,4)]
        _,report=joint_fit(patches,h,np.ones(h.shape,bool),1,tolerance=.1)
        self.assertLessEqual(report['maximum_center_error'],.10002)
        self.assertLessEqual(report['maximum_seam_gap'],.10002)
        self.assertEqual(report['patches_passing_local_checks'],2)

    def test_steep_adjacency_requires_transition_not_smoothing(self):
        h=np.zeros((8,4));h[4:]=10
        patches=[dict(bounds=[x,0,x+4,4],fit=dict(axis='z')) for x in (0,4)]
        self.assertEqual(shared_edges(patches,np.ones(h.shape,bool),1,h),[])
        self.assertEqual(len(shared_edges(patches,np.ones(h.shape,bool),1)),8)

    def test_pc_node_boundary(self):
        face=(0,[((0.,0.,0.),(0.,0.)),((1.,0.,0.),(1.,0.)),((0.,0.,1.),(0.,1.))])
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'mesh.obj';nodes=write_mesh(path,[(('shell','detail'),[face]*2730)])
            self.assertEqual([n['triangles'] for n in nodes],[2729,1])
            self.assertEqual([len(g) for g in b.read_obj(path)],[2729,1])
            b.mesh_mlb(b.read_obj(path),1,'pc')

    def test_flood_rejects_cliff_and_void(self):
        h=np.zeros((4,3));h[2:]=10;allowed=np.ones((4,3),bool);allowed[1,1]=False
        reached,_=terrain_flood(allowed,h,(0,0),1,.65)
        self.assertEqual(int(reached.sum()),5)

    def test_missing_support_never_filled(self):
        h=np.zeros((4,4));h[1,1]=np.nan;mask=np.isfinite(h)
        patches,_=fit_patches(h,mask,1,.25)
        for p in patches:
            x,z,r,t=map(int,p['bounds']);self.assertTrue(np.isfinite(h[x:r,z:t]).all())

    def test_slope_projects_horizontal_and_vertical(self):
        rec=dict(px=0,py=0,pz=0,w=10,d=10,h=10,id=0,flr_no=0);grand=[0.]*32
        xyz,floor,hit=project_slope([5,0,5],rec,grand,0)
        self.assertTrue(hit);self.assertEqual(floor,0);self.assertEqual(xyz,[2.5,2.5,5.])

    def test_zero_height_is_not_flat(self):
        rec=dict(px=0,py=0,pz=0,w=10,d=10,h=0,id=0,flr_no=0)
        xyz,_,hit=project_slope([5,0,5],rec,[0.]*32,0)
        self.assertTrue(hit);self.assertGreater(xyz[1],0);self.assertLess(xyz[0],.02)

    def test_wrong_floor_clamps_to_grand(self):
        rec=dict(px=0,py=0,pz=0,w=10,d=10,h=10,id=0,flr_no=0);grand=[0.]*32;grand[3]=2
        xyz,floor,_=project_slope([5,0,5],rec,grand,0)
        self.assertEqual(floor,1);self.assertEqual(xyz,[5.,2.,5.])

if __name__=='__main__':unittest.main()
