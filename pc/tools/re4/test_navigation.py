import unittest
import numpy as np
from navigation import clip_height, raster, survey


class NavigationTests(unittest.TestCase):
    def test_height_clip_keeps_wall_crossing_band(self):
        poly = clip_height(clip_height([(2,-2,0),(2,30,0),(2,0,8)], .5, True),17.5,False)
        self.assertTrue(poly)
        self.assertTrue(all(.5-1e-8 <= p[1] <= 17.5+1e-8 for p in poly))
        x,z = raster(poly, [0,0,10,10], 1, (10,10))
        self.assertGreater(len(x), 0)

    def test_triangle_not_bounding_box(self):
        cells = raster([(0,0,0),(8,0,0),(0,0,8)], [0,0,10,10],1,(10,10),centers=True)
        self.assertNotIn((7,7), set(zip(*cells)))
        self.assertIn((1,1), set(zip(*cells)))

    def test_open_ground_is_not_ready(self):
        faces = [('g','ground',[(0,0,0),(10,0,0),(0,0,10)]),
                 ('g','ground',[(10,0,0),(10,0,10),(0,0,10)])]
        report,solid,ground,reached = survey(faces, {'materials':{'ground':{'class':'ground'}}},
            {'source_origin':[0,0,0],'scale':1}, [0,0,10,10],1,[5,5],0)
        self.assertEqual(report['reachable_cells'],100)
        self.assertFalse(report['ready_for_colliders'])
        self.assertEqual(report['boundary_edges']['survey_edge'],40)

    def test_sloped_terrain_is_transition_but_retaining_wall_is_solid(self):
        faces=[('g','ground',[(0,0,0),(10,0,0),(0,0,10)]),
               ('g','ground',[(10,0,0),(10,0,10),(0,0,10)]),
               ('g','ground',[(0,1,0),(4,1,0),(0,4,4)]),
               ('g','stone',[(9,0,0),(9,20,0),(9,0,10)])]
        transitions=np.zeros((10,10),bool)
        report,solid,ground,reached=survey(faces,
            {'materials':{'ground':{'class':'ground'},'stone':{'class':'stone'}}},
            {'source_origin':[0,0,0],'scale':1},[0,0,10,10],1,[5,5],0,transitions)
        self.assertTrue(transitions[1,1])
        self.assertFalse(solid[1,1])
        self.assertTrue(solid[9,1])
        self.assertEqual(report['terrain_transition_triangles'],{'ground':1})


if __name__ == '__main__':
    unittest.main()
