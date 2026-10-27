import unittest
from build_rooms import floor_cells, tile_samples


class BoundedRoomsTest(unittest.TestCase):
    def test_support_is_clipped_to_height_tolerance(self):
        cells=set(zip(*floor_cells([(0,0,0),(10,.5,0),(0,0,10)],.15,(12,12))))
        self.assertIn((1,1),cells)
        self.assertNotIn((4,1),cells)  # Actual height .225: outside tolerance.
        self.assertNotIn((11,1),cells)  # No source geometry.

    def test_vertical_degenerate_and_raised_surfaces_are_not_floor(self):
        for ps in ([(0,0,0),(0,1,0),(0,0,10)],[(0,0,0)]*3,[(0,1,0),(10,1,0),(0,1,10)]):
            self.assertEqual(len(floor_cells(ps,.15,(12,12))[0]),0)
        with self.assertRaises(ValueError):floor_cells([(0,0,0)]*3,1,(12,12))

    def test_shared_camera_edge_is_checked_from_both_cuts(self):
        walk={(23,10),(24,10),(25,10)}
        self.assertEqual(tile_samples(walk,0,0,24),[(23,10),(24,10)])
        self.assertEqual(tile_samples(walk,1,0,24),[(24,10),(25,10)])


if __name__=='__main__':unittest.main()
