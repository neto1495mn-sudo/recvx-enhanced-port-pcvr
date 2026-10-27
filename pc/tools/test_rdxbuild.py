#!/usr/bin/env python3
"""Disc-free compiler checks: python3 -m unittest discover -s pc/tools -p test_rdxbuild.py."""
import copy
import json
import math
from pathlib import Path
import struct
import tempfile
import unittest
import rdx
import rdxbuild as b
import rdx_demo


class CompilerTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(dir=Path.cwd())
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        rdx_demo.generate(self.root)
        self.path = self.root/'room.json'
        self.j = json.loads(self.path.read_text())
        self.j['enemies'] = []
        self.j['enemy_assets'] = []
        self.j['scripts'] = ['2300fa0101000000', '0e00fa0101000000']

    def compile(self):
        self.path.write_text(json.dumps(self.j))
        return rdx.Room(b.compile_source(self.path))

    def test_mesh_texture_and_script_layout(self):
        r = self.compile()
        mo = rdx.model_objects(r)
        self.assertEqual(len(mo['objects']), 6)
        self.assertEqual(b.split_table(r,'model')[1][:3],b'MDL')  # object loader tests this magic
        triangles = 0
        for ob in mo['objects']:
            vl, pl, center, radius = rdx.cnk_model(r.d,mo['top'],ob['model'])
            verts, types = rdx.parse_vlist(r.d,mo['top']+vl)
            self.assertEqual(types, [51])
            self.assertTrue(all(math.dist(p,center)<=-radius+1e-4 for p in verts.values()))
            polys, types = rdx.parse_plist(r.d,mo['top']+pl)
            self.assertEqual(set(types), {8,19,66,255})
            for tid,strips in polys:
                self.assertLess(tid,3)
                for _,strip in strips:
                    triangles += len(strip)-2
                    self.assertTrue(all(v in verts for v,u,w in strip))
        self.assertEqual(triangles,144)
        ts = rdx.texture_blocks(r)
        self.assertEqual([len(t) for _,t in ts],[3,1,1])
        w,h,pixels = rdx.read_png(str(self.root/'floor.png'))
        tim = ts[0][1][0]['ofs']
        self.assertEqual(r.d[tim+5], 1)  # FormatId 1: the game reads the picture header at 0x80
        decoded = rdx.tim2_rgba(r.d,ts[0][1][0])
        clut = tim + 256 + ts[0][1][0]['image_size']
        self.assertFalse(any(r.d[p:p+3] == b'\0\0\0' for p in range(clut, clut+1024, 4)))  # black = transparent
        for i,(a,c) in enumerate(zip(pixels,decoded)):
            self.assertLessEqual(abs(a-c), (18,18,43,0)[i%4])
        self.assertEqual(len(r.script_header()[1]),2)
        self.assertTrue(rdx.disasm(r)[1])
        self.assertEqual(rdx.expand(rdx.compress(bytes(r.d))),r.d)

    def test_growing_arrays_and_textures_relocates_assets(self):
        old = self.compile()
        self.j['walls'].append(copy.deepcopy(self.j['walls'][0]))
        self.j['positions'].append(b.zero(rdx.POS,px=10.,pz=70.))
        self.j['cuts'].append(copy.deepcopy(self.j['cuts'][1]))
        self.j['cuts'][-1]['areas'].append(copy.deepcopy(self.j['cuts'][-1]['areas'][0]))
        rdx.write_png(str(self.root/'big.png'),128,32,bytes([12,90,120,255])*4096)
        self.j['textures'].append('big.png')
        new = self.compile()
        self.assertEqual(new.cnt['cut'],3)
        self.assertEqual(new.cnt['wal'],old.cnt['wal']+1)
        self.assertEqual(new.cnt['pos'],2)
        self.assertNotEqual(old.hdr['hed'][1],new.hdr['hed'][1])
        self.assertEqual(len(rdx.texture_blocks(new)[0][1]),4)
        tex = rdx.texture_blocks(new)[0][1][-1]
        self.assertEqual((tex['w'],tex['h']),(128,32))
        self.assertEqual(b.split_table(old,'model')[1].rstrip(b'\0'),
                         b.split_table(new,'model')[1].rstrip(b'\0'))

    def test_preserved_source_roundtrip_and_nan_payload(self):
        self.j['lights']=[b.zero(rdx.LGT)]
        r=self.compile()
        off=r.ptr['lgtp']+28
        struct.pack_into('<I',r.d,off,0x7f800001)  # signaling NaN, must not be quieted on re-pack
        b.decompile(r,self.root/'export')
        rebuilt=rdx.Room(b.compile_source(self.root/'export'/'room.json'))
        self.assertEqual(struct.unpack_from('<I',rebuilt.d,rebuilt.ptr['lgtp']+28)[0],0x7f800001)
        self.assertEqual(json.dumps(b.semantic(r),sort_keys=True),json.dumps(b.semantic(rebuilt),sort_keys=True))
        for kind in ('model','motion','texture'):
            for a,c in zip(b.split_table(r,kind),b.split_table(rebuilt,kind)):
                self.assertTrue(c.startswith(a))
        # Exported source is strict JSON even with legacy sentinel values.
        json.loads((self.root/'export'/'room.json').read_text(),parse_constant=lambda x:self.fail(x))

    def test_rejects_invalid_inputs(self):
        good=copy.deepcopy(self.j)
        self.j['scripts']=['2300fa0101000000','0e005a0301000000']   # it_flg 858: ItmSetCk would test 346
        with self.assertRaisesRegex(ValueError,'below 512'): self.compile()
        self.j=good
        self.j['route_next']=[]
        with self.assertRaisesRegex(ValueError,'square'): self.compile()
        with self.assertRaisesRegex(ValueError,'opaque'):
            b.texture_list([(8,8,bytes(256))])
        with self.assertRaisesRegex(ValueError,'192'):
            b.texture_list([(8,8,bytes([255])*256)]*193)
        (self.root/'bad.obj').write_text('v 0 0 0\nf 1 1 1 1\n')
        with self.assertRaisesRegex(ValueError,'triangulated'): b.read_obj(self.root/'bad.obj')

    def test_pc_budget(self):
        # The PC budget admits what the retail one refuses; engine record limits hold for both.
        b.texture_list([(8,8,bytes([255])*256)]*193,budget='pc')
        tri=[(0,[((0.,0.,0.),(0.,0.)),((1.,0.,0.),(0.,0.)),((0.,0.,1.),(0.,0.))])]
        with self.assertRaisesRegex(ValueError,'666'): b.mesh_mlb([tri*667],1)
        b.mesh_mlb([tri*2729],1,'pc')
        with self.assertRaisesRegex(ValueError,'2729'): b.mesh_mlb([tri*2730],1,'pc')
        self.j['budget']='pc'
        self.j['positions']=self.j['positions']*129
        with self.assertRaisesRegex(ValueError,'positions exceeds 128'): self.compile()
        self.j['budget']='ps3'
        with self.assertRaisesRegex(ValueError,'budget must be'): self.compile()

    def test_slopes(self):
        # Only "pc" rooms get the flag that lets the player stand on slopes (pc_prelude.h PC_ROOM_SLOPES).
        self.assertFalse(self.compile().rom['flg'] & b.ROOM_SLOPES)
        self.j['budget']='pc'
        ramp=b.slope_wall(0,10,20,10,2,8,'+x')
        self.j['walls'].append(ramp)
        r=self.compile()
        self.assertEqual(r.rom['flg'] & b.ROOM_SLOPES, b.ROOM_SLOPES)
        self.assertEqual((r.records('walls')[-1]['type'],r.records('walls')[-1]['id']),(6,0))
        # Surface heights as the engine reads them: low edge, high edge, outside the box.
        for rise,lo_pt,hi_pt in [('+x',(0,15),(19.99,15)),('-x',(19.99,15),(0,15)),('+z',(5,10),(5,19.99)),('-z',(5,19.99),(5,10))]:
            s=b.slope_wall(0,10,20,10,2,8,rise)
            self.assertAlmostEqual(b.slope_height(s,*lo_pt),2,delta=.01)
            self.assertAlmostEqual(b.slope_height(s,*hi_pt),8,delta=.01)
            self.assertIsNone(b.slope_height(s,20,15))
        self.assertAlmostEqual(b.slope_wall(0,0,5,5,3,3,'+x')['h'],b.FLAT)   # flat patch: h 0 would be the room height
        # A heightfield: cells meet without a step along each cell's rising axis.
        cells=b.heightfield_slopes([[0,2,4],[0,2,4]],0,0,10)
        self.assertAlmostEqual(b.slope_height(cells[0],9.999,5),b.slope_height(cells[1],10,5),delta=.01)
        self.j['walls'][-1]['h']=0
        with self.assertRaisesRegex(ValueError,'slope walls'): self.compile()

    def test_obj_axes_and_outward_normal(self):
        path=self.root/'triangle.obj'
        path.write_text('v 0 0 0\nv 1 0 0\nv 0 0 -1\nusemtl tex0\nf 1 2 3\n')
        data=b.mesh_mlb(b.read_obj(path),1)
        model=struct.unpack_from('<i',data,20)[0]
        vl=struct.unpack_from('<i',data,16+model)[0]
        self.assertEqual(struct.unpack_from('<3f',data,16+vl+80),(0.,1.,0.))
        # No axis reflection: the game renders right-handed like OBJ (+x is screen-left facing +z).
        self.assertEqual(struct.unpack_from('<3f',data,16+vl+128),(0.,0.,-1.))

    def test_shared_vertices(self):
        # A quad's two triangles share the two vertices with the same position and normal.
        path=self.root/'quad.obj'
        path.write_text('v 0 0 0\nv 1 0 0\nv 1 0 -1\nv 0 0 -1\nusemtl tex0\nf 1 2 3\nf 1 3 4\n')
        data=b.mesh_mlb(b.read_obj(path),1)
        model=struct.unpack_from('<i',data,20)[0]
        vl,pl=struct.unpack_from('<ii',data,16+model)
        verts,_=rdx.parse_vlist(data[16:],vl)
        chunks,_=rdx.parse_plist(data[16:],pl)
        self.assertEqual(len(verts),4)
        self.assertEqual([[i for i,u,v in st] for _,st in chunks[0][1]],[[0,1,2],[0,2,3]])

    def test_one_strip_chunk_per_texture(self):
        path=self.root/'mixed.obj'
        path.write_text('v 0 0 0\nv 1 0 0\nv 0 0 -1\nvt 0 0\n'
                        'usemtl tex1\nf 1/1 2/1 3/1\nusemtl tex0\nf 1/1 3/1 2/1\nusemtl tex1\nf 2/1 3/1 1/1\n')
        data=b.mesh_mlb(b.read_obj(path),2)
        model=struct.unpack_from('<i',data,20)[0]
        pl=struct.unpack_from('<i',data,16+model+4)[0]
        chunks,_=rdx.parse_plist(data[16:],pl)
        self.assertEqual([(tid,len(strips)) for tid,strips in chunks],[(0,1),(1,2)])

    def test_door_record_and_script_patch(self):
        self.j['scripts']=['25000400010b001600000000','33000400010b00160000']
        r=self.compile()
        linked=rdx.Room(b.link_door(r,0,[1,15,0]))
        self.assertEqual(linked.records('triggers')[0]['prm1'],15)
        start,offs=linked.script_header()
        for off in offs: self.assertEqual(linked.d[start+off+4:start+off+7],bytes([1,15,0]))


if __name__=='__main__': unittest.main()
