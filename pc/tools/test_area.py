"""Disc-free regression tests for multi-enemy ownership, annex geometry and lock scripts."""
import copy
import json
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch
import area_validate as check
import build_area as area
import rdx
import rdxbuild as b


class EnemyDependencies(unittest.TestCase):
    def donor(self):
        def chunk(textured=True):
            return struct.pack('<I',16)+struct.pack('<4siii',b'MDL\x80',0,0 if textured else -1,0)
        self.zombie=chunk()+struct.pack('<I4s',4,b'SKIN')+chunk(False)+chunk()+struct.pack('<i',-1)
        self.dog=chunk()+struct.pack('<i',-1)
        records=[b.zero(rdx.ETTY,flg=1,id=1,mdlver=1),b.zero(rdx.ETTY,flg=1,id=1,mdlver=1),
                 b.zero(rdx.ETTY,flg=1,id=4)]
        class Donor:
            def records(self,key):return records
        self.tables={'model':[b'ROOM',self.zombie,self.zombie,self.dog],
                     'motion':[b'ZOMBIE MOTION',b'',b'DOG MOTION',b'',b'',b''],
                     'texture':[b'ROOM TEX',b'Z HEAD',b'Z BODY',b'DOG TEX']}
        return Donor()

    def test_nonowner_and_nonzero_donor_texture_indices(self):
        donor=self.donor()
        with patch.object(b,'split_table',side_effect=lambda r,k:self.tables[k]):
            model,motion,textures=b.enemy_dependency(donor,1)
            self.assertEqual((model,motion,textures),(self.zombie,b'ZOMBIE MOTION',[b'Z HEAD',b'Z BODY']))
            self.assertEqual(b.enemy_dependency(donor,2),(self.dog,b'DOG MOTION',[b'DOG TEX']))
            with self.assertRaisesRegex(ValueError,'outside'):b.enemy_dependency(donor,3)
        self.assertEqual(b.enemy_textured_parts(self.zombie),2)

    def test_compiler_shares_textures_and_one_motion_set_per_type(self):
        import rdx_demo
        donor=self.donor()
        self.tables['texture']=[b.texture_list([(8,8,bytes([90,120,140,255])*64)],100+k)
                                for k in range(4)]
        original=b.split_table
        def split(room,kind):return self.tables[kind] if room is donor else original(room,kind)
        with tempfile.TemporaryDirectory(dir=Path.cwd()) as tmp:
            root=Path(tmp);rdx_demo.generate(root)
            path=root/'room.json';j=json.loads(path.read_text())
            j['enemies']=[b.zero(rdx.ETTY,flg=1,id=1,mdlver=1),
                          b.zero(rdx.ETTY,flg=1,id=1,mdlver=1),b.zero(rdx.ETTY,flg=1,id=4)]
            j['enemy_assets']=[{'room':'fake','record':i} for i in (1,0,2)]
            path.write_text(json.dumps(j))
            with patch.object(b,'split_table',side_effect=split), patch.object(rdx,'load_room',return_value=donor):
                with self.assertRaisesRegex(ValueError,'wrk_no'):b.compile_source(path)
                for k,e in enumerate(j['enemies']):e['wrk_no']=k
                path.write_text(json.dumps(j))
                result=rdx.Room(b.compile_source(path))
            self.assertEqual(len(result.model_table()),6) # room, three enemies, prop, pickup
            self.assertEqual([bool(x) for x in result.motion_table()],[True,False,True,False,False,False])
            self.assertEqual(len(result.texture_table()),6) # room, zombie x2, dog, prop, pickup
            self.assertTrue(original(result,'model')[2].startswith(self.zombie))
            self.assertTrue(original(result,'model')[3].startswith(self.dog))

    def test_mix_children_and_alias_limits(self):
        def e(id,ver=0):return b.zero(rdx.ETTY,id=id,mdlver=ver)
        b.validate_enemy_mix([e(1,1),e(4)])
        b.validate_enemy_mix([e(4),e(4)])
        for records,reason in [([e(1),e(5)],'helper'),([e(1,16)],'alias'),
                               ([e(1)]*11,'slots'),([e(24)],'unsupported')]:
            with self.assertRaisesRegex(ValueError,reason):b.validate_enemy_mix(records)
        with self.assertRaisesRegex(ValueError,'variant'):b.validate_enemy_mix([dict(e(4),type=1)])
        with self.assertRaisesRegex(ValueError,'unterminated'):b.enemy_textured_parts(bytes(4))


class Annex(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory(dir=Path.cwd())
        cls.addClassCleanup(cls.temp.cleanup)
        cls.root=Path(cls.temp.name)
        cls.spec=json.loads(area.SPEC.read_text());cls.sources={};cls.compiled={}
        for r in cls.spec['rooms']:
            dest=cls.root/r['id'];j=area.room_source(r,dest,cls.spec['key_flag']);cls.sources[r['id']]=j
            # Geometry/scripts are original; donor resolution has independent tests above.
            edited=copy.deepcopy(j);edited['enemies']=[];edited['enemy_assets']=[]
            (dest/'room.json').write_text(json.dumps(edited))
            cls.compiled[r['id']]=rdx.Room(b.compile_source(dest/'room.json'))

    def test_all_room_geometry_and_texture_budgets(self):
        for rid,j in self.sources.items():
            with self.subTest(room=rid):
                self.assertEqual([e['wrk_no'] for e in j['enemies']],list(range(len(j['enemies']))))
                result=check.room(j,self.compiled[rid],100000)
                self.assertGreater(result['walkable_samples'],1000)
                self.assertEqual(len(result['trigger_witnesses']),len(j['triggers']))

    def test_graph_flags_and_failures(self):
        check.graph(self.spec);check.allocations(self.spec,{'it_flg':[],'ed_flg':[]})
        bad=copy.deepcopy(self.spec);bad['rooms'][1]['doors'][0][2]=0
        with self.assertRaisesRegex(ValueError,'reciprocal'):check.graph(bad)
        with self.assertRaisesRegex(ValueError,'collision'):check.allocations(self.spec,{'it_flg':[464],'ed_flg':[]})
        bad=copy.deepcopy(self.sources['1160']);bad['room']['w']=65
        with self.assertRaisesRegex(ValueError,'shell'):check.room(bad,self.compiled['1160'],1000)
        broken=copy.deepcopy(self.compiled['1160']);broken.d=bytearray(broken.d)
        pos=broken.records('positions')[0];pos['px']=0
        rdx.pack_into(rdx.POS,broken.d,broken.ptr['posp'],pos)
        with self.assertRaisesRegex(ValueError,'entry'):check.room(self.sources['1160'],broken,1000)
        # The first, untested layout: a camera above the ceiling behind the south wall.
        bad=copy.deepcopy(self.sources['1160']);bad['enemies']=[];bad['enemy_assets']=[]
        bad['cuts'][0]['cam'][0].update(py=65,pz=-30,ay=32768)
        path=self.root/'1160'/'outside.json';path.write_text(json.dumps(bad))
        with self.assertRaisesRegex(ValueError,'camera outside'):
            check.room(bad,rdx.Room(b.compile_source(path)),1000)
        self.assertTrue(check.segment_hits_box((0,0,0),(10,10,10),(4,4,4,1,1,1)))
        self.assertFalse(check.segment_hits_box((0,0,0),(10,0,0),(4,1,-1,1,1,2)))

    def test_lock_bytecode_and_reentry(self):
        # Small independent interpreter for the actual subset of emitted scd0/scd1.
        def run(block,picked,enabled=True):
            code=bytes.fromhex(block);o=0;ends=[]
            while code[o]:
                op=code[o]
                if op==1:ends.append(o+code[o+1])
                elif op==3:ends.pop()
                elif op==4:
                    bank=code[o+1];bit=struct.unpack_from('<H',code,o+2)[0]
                    result=bank==7 and bit in picked
                    if bool(code[o+5]):result=not result
                    if not result:o=ends.pop()+2;continue
                elif op==11 and code[o+1]==1:enabled=not code[o+2]
                o+=rdx.OP_LEN[op]
            return enabled
        j=self.sources['1190']
        for block in j['scripts']:
            self.assertFalse(run(block,set()))
            self.assertTrue(run(block,{464},False))
            self.assertFalse(run(block,{465}))
        # Return side contains no lock, so direct teleport never traps the tester.
        for block in self.sources['1200']['scripts']:
            code=bytes.fromhex(block);o=0
            while code[o]:self.assertNotEqual(code[o],11);o+=rdx.OP_LEN[code[o]]

    def test_hall_return_matches_piano_exit(self):
        # Reception's west door repeats RM_1110's hall exit: case and door attr by ev_flg.
        def run(flags):
            code=bytes.fromhex(self.sources['1160']['scripts'][1]);o=0;ends=[];calls=[]
            while code[o]:
                op=code[o]
                if op==1:ends.append(o+code[o+1])
                elif op==3:ends.pop()
                elif op==4:
                    hit=(code[o+1],struct.unpack_from('<H',code,o+2)[0]) in flags
                    if hit==bool(code[o+5]):o=ends.pop()+2;continue
                elif op==55:case=code[o+1]
                elif op==51:calls.append((case,code[o+2],tuple(code[o+4:o+8])))
                o+=rdx.OP_LEN[op]
            return calls
        at=(10,23)  # interaction bit, checked together with the trigger index
        for flags,want in [({at},(0,4)),({at,(1,151)},(1,4)),({at,(1,151),(1,181)},(1,0x24)),
                           ({at,(1,151),(1,181),(1,142)},(1,0x64))]:
            self.assertEqual(run(flags),[(*want,(1,2,3,22))])
        self.assertEqual(run({(1,151)}),[])

    def test_deterministic_original_textures(self):
        a=self.root/'1160'/'wood.png';before=a.read_bytes()
        area.materials(a.parent,'RECEPTION',1160)
        self.assertEqual(before,a.read_bytes())
        groups=b.read_obj(a.parent/'room.obj')
        label=next(face for group in groups for tid,face in group if tid==6)
        self.assertLess(label[0][0][0],label[1][0][0])
        self.assertGreater(label[0][1][0],label[1][1][0]) # +X is screen-left
        for name in area.MATERIALS:
            w,h,pix=rdx.read_png(str(a.parent/(name+'.png')))
            self.assertEqual((w,h),(128,128));self.assertTrue(all(x==255 for x in pix[3::4]))


if __name__=='__main__':unittest.main()
