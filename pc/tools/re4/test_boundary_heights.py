import unittest
import numpy as np
from boundary_heights import height_field, inspect_boundaries


class BoundaryHeightTests(unittest.TestCase):
    def field(self, faces, mapping=None, transform=None):
        return height_field(faces, mapping or {'materials': {'dirt': {'class': 'ground'},
                            'roof': {'class': 'roof'}, 'leaf': {'class': 'foliage'}}},
                            transform or {'source_origin': [0,0,0], 'scale': 1},
                            [0,0,4,4], 1, (4,4))

    def test_slope_interpolation_and_triangle_extent(self):
        opaque, terrain = self.field([('g','dirt',[(0,0,0),(4,4,0),(0,0,4)])])
        self.assertAlmostEqual(terrain[1,1],1.5)
        self.assertTrue(np.isnan(terrain[3,3]))
        np.testing.assert_equal(opaque,terrain)

    def test_roof_does_not_certify_terrain_and_foliage_is_ignored(self):
        opaque, terrain = self.field([('g','roof',[(0,8,0),(4,8,0),(0,8,4)]),
                                      ('g','leaf',[(0,0,0),(4,0,0),(0,0,4)])])
        self.assertEqual(opaque[0,0],8)
        self.assertTrue(np.isnan(terrain).all())

    def test_nearest_surface_and_group_exclusion(self):
        faces = [('g','dirt',[(0,2,0),(4,2,0),(0,2,4)]),
                 ('g','dirt',[(0,-1,0),(4,-1,0),(0,-1,4)]),
                 ('hidden','dirt',[(0,0,0),(4,0,0),(0,0,4)])]
        mapping = {'materials': {'dirt': {'class':'ground'}},
                   'group_overrides': {'hidden': {'*': {'exclude':True}}}}
        opaque, terrain = self.field(faces,mapping)
        self.assertEqual(terrain[0,0],-1)

    def test_missing_mesh_never_becomes_drop_wall(self):
        ground = np.zeros((4,4),bool)
        ground[1,1] = True
        missing = np.full((4,4),np.nan)
        result = inspect_boundaries({'open_boundaries':[{'points':[[1.5,1.5]],
                    'kind':'unsupported_ground','samples':1}]},
                    {'bounds':[0,0,4,4],'cell_size':1}, ground,
                    np.zeros_like(ground),missing,missing,2)
        self.assertEqual(result['boundaries'][0]['proposal'],'mixed_or_missing_support_review')
        self.assertFalse(result['publishable'])
        self.assertEqual(result['closed_boundaries'],0)


if __name__ == '__main__':
    unittest.main()
