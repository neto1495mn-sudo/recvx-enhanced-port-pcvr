import copy
import unittest
from materials import resolve


class MaterialTests(unittest.TestCase):
    def setUp(self):
        self.a={'room':'0030','list':0,'texture':1}
        self.b={'room':'0030','list':0,'texture':2}
        self.mapping={'classes':{'wood':self.a,'plaster':self.b},
                      'materials':{'m':{'class':'wood','retail_texture':self.a,'review_required':True}},
                      'group_overrides':{}}

    def test_group_class_and_material_donor_precedence(self):
        self.mapping['group_overrides']={'house':{'*':{'class':'plaster'},'m':{'retail_texture':self.a}}}
        before=copy.deepcopy(self.mapping)
        mat,info=resolve(self.mapping,'house','m')
        self.assertEqual((mat.category,mat.texture),('plaster',1))
        self.assertTrue(info['review_required'])
        self.assertEqual(self.mapping,before)
        self.assertEqual(resolve(self.mapping,'other','m')[0].category,'wood')

    def test_explicit_null_cannot_silently_fallback(self):
        self.mapping['group_overrides']={'house':{'m':{'retail_texture':None}}}
        with self.assertRaisesRegex(ValueError,'unresolved donor'):
            resolve(self.mapping,'house','m','wood')
        self.mapping['materials']['m']['retail_texture']=None
        self.assertEqual(resolve(self.mapping,'other','m','plaster')[0].texture,2)

    def test_uv_atlas_region_and_validation(self):
        self.a['uv_rect']=[.1,.2,.8,.7]
        mat,_=resolve(self.mapping,'g','m')
        self.assertEqual(mat.map_uv((0,0)),(.1,.2))
        self.assertEqual(mat.map_uv((1,1)),(.8,.7))
        self.a['uv_rect']=[.8,.2,.1,.7]
        with self.assertRaises(ValueError):resolve(self.mapping,'g','m')

    def test_group_exclusion(self):
        self.mapping['group_overrides']={'house':{'m':{'exclude':True}}}
        self.assertIsNone(resolve(self.mapping,'house','m')[0])


if __name__=='__main__':unittest.main()
