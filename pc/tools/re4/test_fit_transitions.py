import unittest
from fit_transitions import fit_axis_slope, assess


class FitTransitionTests(unittest.TestCase):
    def test_four_engine_directions(self):
        for axis,sign,direction in [(0,1,0),(0,-1,3),(2,1,2),(2,-1,1)]:
            points = [[x, sign*(x if axis == 0 else z)*.5+2, z]
                      for x in range(4) for z in range(4)]
            fit = fit_axis_slope(points)
            self.assertEqual(fit['direction_id'],direction)
            self.assertLess(fit['maximum_error'],1e-10)

    def test_diagonal_terrain_needs_partition(self):
        samples = [dict(x=x,z=z,terrain_y=x+z) for x in range(4) for z in range(4)]
        result = assess({'boundaries':[{'index':0,'samples':samples}]},.25)
        self.assertTrue(result['boundaries'][0]['single_slope_fit_rejected'])
        self.assertFalse(result['publishable'])

    def test_incomplete_support_cannot_pass(self):
        samples = [dict(x=x,z=0,terrain_y=0 if x < 3 else None) for x in range(4)]
        result = assess({'boundaries':[{'index':0,'samples':samples}]},.25)
        self.assertFalse(result['boundaries'][0]['single_slope_height_fit'])


if __name__ == '__main__':
    unittest.main()
