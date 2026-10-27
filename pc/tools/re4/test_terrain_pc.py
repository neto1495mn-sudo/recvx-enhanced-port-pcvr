"""PC terrain hazards: half-open boxes, nearest surface, downhill band and flats."""
import unittest
import numpy as np
from terrain_pc import Ground,record,movement
import rdxbuild as b


class TerrainPCTests(unittest.TestCase):
    def test_flat_is_explicit_and_positive(self):
        patch=dict(bounds=[0,0,10,4],fit=dict(axis='x',gradient=0,coordinate_origin=5,height_at_origin=7))
        r=record(patch)
        self.assertEqual((r['flg'],r['type'],r['attr'],r['flr_no']),(1,6,0,0))
        self.assertEqual(r['h'],.001)
        self.assertAlmostEqual(Ground([r]).height(5,2,7),7.0005)

    def test_rise_directions_match_heightfield(self):
        for axis in ('x','z'):
            for g in (-.4,.4):
                p=dict(bounds=[0,0,4,4],fit=dict(axis=axis,gradient=g,coordinate_origin=2,height_at_origin=3))
                ground=Ground([record(p)])
                for x,z in ((1,1),(3,1),(1,3),(3,3)):
                    self.assertAlmostEqual(ground.height(x,z),3+g*((x if axis=='x' else z)-2))

    def test_half_open_and_nearest_eligible_layer(self):
        a=b.slope_wall(0,0,4,4,0,4,'+x');c=b.slope_wall(0,0,4,4,10,14,'+x')
        ground=Ground([a,c])
        self.assertIsNone(ground.height(4,2))
        self.assertEqual(ground.height(2,2,11),12)
        self.assertEqual(ground.height(2,2,1),2)
        self.assertIsNone(ground.height(2,2,40))

    def test_downhill_seam_stays_eligible_with_actor_height_band(self):
        ground=Ground([b.slope_wall(0,0,1,1,10,11,'-x'),b.slope_wall(1,0,1,1,9,10,'-x')])
        self.assertEqual(ground.height(1,0.5),10)
        # Previous-frame y, before crossing the seam, is above the next top.
        self.assertAlmostEqual(ground.height(1.1,.5,10.1),9.9)
        mask=np.ones((2,1),bool);h=np.array([[10.5],[9.5]])
        report,failures=movement(ground,mask,h,1,-2)
        self.assertEqual(report['failed_trajectories'],0)
        self.assertFalse(failures)
        self.assertTrue(report['engine']['compiled_function'])

    def test_flat_grid_has_no_missing_or_ineligible_surface(self):
        rs=b.heightfield_slopes([[0]*3 for _ in range(3)],0,0,1)
        report,_=movement(Ground(rs),np.ones((2,2),bool),np.zeros((2,2)),1,-2)
        self.assertEqual(report['failed_trajectories'],0)
        self.assertEqual(report['uncovered_centers'],0)

if __name__=='__main__':unittest.main()
