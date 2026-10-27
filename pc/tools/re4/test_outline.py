import unittest
import numpy as np
from outline import rectangles, expand, candidate, reduce_rectangles, connect_regions
from validate_outline import blocked_boxes, flood, audit


class OutlineTests(unittest.TestCase):
    def test_exact_cover_preserves_holes_and_passages(self):
        rng=np.random.default_rng(5)
        for mask in (rng.random((30,40))>.6, np.pad(np.zeros((5,5),bool),2,constant_values=True)):
            cover=np.zeros_like(mask,dtype=int)
            for x0,z0,x1,z1 in rectangles(mask):
                cover[x0:x1,z0:z1]+=1
            np.testing.assert_array_equal(cover,mask.astype(int))

    def test_square_clearance(self):
        mask=np.zeros((9,9),bool);mask[4,4]=True
        grown=expand(mask,2)
        self.assertEqual(grown.sum(),25)
        self.assertTrue(grown[2,2])

    def test_open_ground_creates_no_invisible_wall(self):
        solid=np.zeros((10,10),bool);ground=np.ones_like(solid)
        report={'cell_size':1,'bounds':[0,0,10,10],'actor_radius':0}
        result=candidate(report,solid,ground,ground)
        self.assertEqual(result['boxes'],[])
        self.assertFalse(result['publishable'])
        self.assertEqual(result['open_boundaries'][0]['kind'],'survey_edge')

    def test_merge_keeps_protected_courtyard_and_passage(self):
        protected=np.zeros((20,20),bool)
        protected[8:12,:]=True
        protected[2:5,6:9]=True
        boxes=[(0,0,6,3),(0,3,2,10),(5,3,6,10),(14,0,20,10)]
        result=reduce_rectangles(boxes,protected)
        cover=np.zeros_like(protected)
        for x0,z0,x1,z1 in result:cover[x0:x1,z0:z1]=True
        self.assertFalse(np.any(cover & protected))
        for x0,z0,x1,z1 in boxes:self.assertTrue(cover[x0:x1,z0:z1].all())

    def test_collinear_runs_merge_across_inaccessible_gap(self):
        boxes=[(2,2,4,6),(2,8,4,12),(2,12,4,18)]
        self.assertEqual(reduce_rectangles(boxes,np.zeros((20,20),bool)),[(2,2,4,18)])

    def test_narrow_bent_passage_stays_connected(self):
        allowed=np.zeros((30,30),bool)
        allowed[2:10,2:10]=True;allowed[20:28,20:28]=True
        allowed[8:23,8]=True;allowed[22,8:23]=True
        core=np.zeros_like(allowed);core[5,5]=True;core[25,25]=True
        protected=connect_regions(core,allowed)
        self.assertFalse(np.any(protected & ~allowed))
        self.assertTrue(flood(protected,(5,5))[25,25])

    def test_box_clearance_expanded_once_and_strict(self):
        blocked=blocked_boxes([[10,0,10,2,18,2]],[0,0,30,30],1,(30,30),5.5)
        self.assertFalse(blocked[4,10])  # center 4.5 equals lower clearance edge
        self.assertTrue(blocked[5,10])
        self.assertTrue(blocked[16,10])
        self.assertFalse(blocked[17,10]) # center 17.5 equals upper edge

    def test_audit_detects_disconnected_supported_region(self):
        solid=np.zeros((20,20),bool);ground=np.ones_like(solid)
        report={'cell_size':1,'bounds':[0,0,20,20],'actor_radius':0,'entry':[2,2]}
        result,*_=audit(report,solid,ground,{'boxes':[[10,0,0,1,18,20]]})
        self.assertEqual(result['baseline_cells_blocked'],20)
        self.assertEqual(result['surviving_baseline_cells_disconnected'],180)
        self.assertFalse(result['publishable'])

    def test_merges_do_not_close_a_floor_transition(self):
        solid=np.zeros((20,20),bool);solid[7:10,3:6]=True;solid[7:10,16:19]=True
        reached=np.ones_like(solid);terrain=np.zeros_like(solid);terrain[:,6:16]=True
        report={'cell_size':1,'bounds':[0,0,20,20],'actor_radius':0}
        result=candidate(report,solid,np.zeros_like(solid),reached,True,3,terrain)
        self.assertEqual(result['wall_records'],2)


if __name__=='__main__':
    unittest.main()
