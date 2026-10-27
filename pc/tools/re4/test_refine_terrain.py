"""Synthetic source support and adaptive seam regression tests."""
import unittest
from refine_terrain import TerrainSampler, refine, edge_audit


class RefinementTests(unittest.TestCase):
    def test_triangle_boundary_and_nearest_layer(self):
        sampler=TerrainSampler([[(0,0,0),(2,2,0),(0,0,2)],[(0,10,0),(2,10,0),(0,10,2)]])
        self.assertAlmostEqual(sampler(1,1),1)
        self.assertEqual(sampler(2,0),2)
        self.assertIsNone(sampler(2,2))

    def test_diagonal_plane_refines_and_seams_pass(self):
        patches=refine([0,0,2,2],lambda x,z: .3*x+.3*z)
        self.assertGreater(len(patches),1)
        self.assertTrue(all(p['source_check']=='pass' for p in patches))
        self.assertEqual(edge_audit(patches)['failed_edges'],0)
        self.assertEqual(edge_audit(patches)['passing_components'],[len(patches)])

    def test_missing_corner_is_not_flat_floor(self):
        patches=refine([0,0,2,2],lambda x,z: None if x==0 and z==0 else 0,minimum=.5)
        self.assertTrue(any(p['source_check']=='missing_support' for p in patches))

    def test_steep_plane_requires_steps(self):
        patches=refine([0,0,2,2],lambda x,z: x)
        self.assertEqual(len(patches),1)
        self.assertEqual(patches[0]['source_check'],'steep')

    def test_t_junction_detects_gap_on_both_subedges(self):
        a=refine([0,0,2,2],lambda x,z:0)
        b=refine([2,0,3,1],lambda x,z:1)
        c=refine([2,1,3,2],lambda x,z:1)
        report=edge_audit(a+b+c)
        self.assertEqual(report['tested_edges'],3)
        self.assertEqual(report['failed_edges'],2)
        self.assertEqual(report['passing_components'],[2,1])


class CollisionCandidateTests(unittest.TestCase):
    def test_exact_outline_keeps_hole_and_strict_clearance(self):
        import numpy as np
        from collision_whole import build_candidate
        solid=np.zeros((8,8),bool);solid[1:7,1:7]=True;solid[3:5,3:5]=False
        records,blocked=build_candidate(solid,np.zeros_like(solid,dtype=float),1,0)
        self.assertTrue(np.array_equal(solid,blocked))
        self.assertTrue(all(r['type']==0 and r['attr']==0 and r['h']>0 for r in records))

    def test_boundary_equality_not_overexpanded(self):
        import numpy as np
        from collision_whole import build_candidate
        solid=np.zeros((5,5),bool);solid[2,2]=True
        _,blocked=build_candidate(solid,np.zeros_like(solid,dtype=float),1,.5)
        self.assertEqual(int(blocked.sum()),1)


class SupportBoundaryTests(unittest.TestCase):
    def test_all_directions_close_without_adding_floors(self):
        from collision_whole import close_boundaries,boundary_audit
        edges=[dict(inside=[10,10],outside=[10+dx,10+dz]) for dx,dz in ((1,0),(-1,0),(0,1),(0,-1))]
        walls=close_boundaries(edges,2,5.5,-2,45)
        self.assertEqual(boundary_audit(edges,walls,2,5.5)['closed_edges'],4)
        for wall in walls:
            self.assertEqual(wall['type'],0)
            self.assertEqual(wall['attr'],0)
            self.assertLessEqual(min(wall['w'],wall['d']),.125)
        # Moving the east wall outward opens unsupported space past the edge.
        walls[0]['px']+=.125
        self.assertEqual(boundary_audit(edges,walls,2,5.5)['failed_edges'],[0])

    def test_wall_clearance_stops_before_half_open_floor_end(self):
        from collision_whole import close_boundaries
        r=close_boundaries([dict(inside=[0,0],outside=[1,0])],2,5.5,-2,45)[0]
        stop=r['px']-5.5
        self.assertLess(stop,2)
        self.assertGreater(stop,1.9)
        self.assertTrue(r['pz']-5.5<1<r['pz']+r['d']+5.5)


if __name__=='__main__': unittest.main()
