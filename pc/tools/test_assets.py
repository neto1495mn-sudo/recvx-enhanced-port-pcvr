"""Disc-free tests of native model ownership, palette preservation and kit transforms."""
import copy
import json
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch
import assets
import kitbash
import rdx
import rdxbuild as b
import rdx_demo


class RetailAssetsTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory(dir=Path.cwd());self.addCleanup(self.tmp.cleanup)
        self.root=Path(self.tmp.name);rdx_demo.generate(self.root)
        self.path=self.root/'room.json';self.j=json.loads(self.path.read_text())
        self.j['enemies']=[];self.j['enemy_assets']=[]
        self.j['scripts']=['0000','0000']
        self.path.write_text(json.dumps(self.j));self.donor=rdx.Room(b.compile_source(self.path))
    def build(self):
        self.path.write_text(json.dumps(self.j))
        with patch.object(rdx,'load_room',return_value=self.donor):
            return rdx.Room(b.compile_source(self.path))
    def test_native_item_and_repeated_texture_owner(self):
        model,texture=assets.dependency(self.donor,'items',0)
        self.j.pop('item_meshes');self.j['item_assets']=[{'room':'donor','record':0}]*2
        self.j['items']*=2
        room=self.build()
        self.assertEqual(len(room.texture_table()),3) # room, object, one shared item list
        for i in (0,1):
            m,t=assets.dependency(room,'items',i)
            self.assertEqual(m.rstrip(b'\0'),model.rstrip(b'\0'))
            self.assertEqual(t.rstrip(b'\0'),texture.rstrip(b'\0'))
    def test_wrong_item_model_rejected(self):
        self.j.pop('item_meshes');self.j['item_assets']=[{'room':'donor','record':0}]
        self.j['items'][0]['id']=21
        with self.assertRaisesRegex(ValueError,'item id must match'):self.build()
    def test_retail_palette_bytes_preserved(self):
        self.j.pop('textures');self.j['retail_textures']=[{'room':'donor','index':i} for i in range(3)]
        room=self.build()
        old=rdx.texture_blocks(self.donor)[0][1];new=rdx.texture_blocks(room)[0][1]
        for a,z in zip(old,new):self.assertEqual(self.donor.d[a['ofs']:a['ofs']+a['size']],room.d[z['ofs']:z['ofs']+z['size']])
    def test_transform_and_dedupe(self):
        with patch.object(rdx,'load_room',return_value=self.donor):
            kit=kitbash.Kit(None)
            lo,hi=kit.place('donor',[0],(10,20,30),(5,6,7))
            self.assertEqual(lo,[10,20,30]);self.assertEqual(hi,[15,26,37])
            self.assertEqual(kit.texture('donor',0),kit.texture('same-content',0))
    def test_append_door_preserves_retail_records_and_item_binding(self):
        import build_village
        from types import SimpleNamespace
        model,texture=assets.dependency(self.donor,'objects',4)
        before_item=assets.dependency(self.donor,'items',0)
        with patch.object(assets,'door_pack',return_value=(b'', [model,texture,b'',b''])), \
             patch.object(rdx,'open_disc',return_value=SimpleNamespace(iso=None)):
            room=rdx.Room(build_village.patch_graveyard(self.donor,None))
        self.assertEqual(room.records('objects')[:-1],self.donor.records('objects'))
        self.assertEqual(room.records('triggers')[:-1],self.donor.records('triggers'))
        self.assertEqual(room.records('positions')[:-1],self.donor.records('positions'))
        after_item=assets.dependency(room,'items',0)
        for a,z in zip(before_item,after_item):self.assertEqual(a.rstrip(b'\0'),z.rstrip(b'\0'))
        self.assertEqual(room.records('triggers')[-1]['prm1'],22)

    def test_second_door_shares_texture_list(self):
        # room.c reads one texture list per object id; an extra one shifts every effect texture
        import build_village
        from types import SimpleNamespace
        model,texture=assets.dependency(self.donor,'objects',4)
        with patch.object(assets,'door_pack',return_value=(b'', [model,texture,b'',b''])), \
             patch.object(rdx,'open_disc',return_value=SimpleNamespace(iso=None)):
            one=rdx.Room(build_village.patch_graveyard(self.donor,None))
            two=rdx.Room(build_village.patch_graveyard(one,None))
        self.assertEqual(len(one.texture_table()),len(self.donor.texture_table())+1)
        self.assertEqual(len(two.texture_table()),len(one.texture_table()))
        tex=b.split_table(one,'texture');tex.insert(2,tex[1])
        extra=rdx.Room(b.relayout(one,one.to_json(),{4:b.table_blob(tex,True)}))
        with self.assertRaisesRegex(ValueError,'the game reads'):assets.dependencies(extra)

    def test_inventory_pack_alignment_and_native_model(self):
        model,texture=assets.dependency(self.donor,'items',0)
        pack=bytearray(struct.pack('<I',len(model))+model+struct.pack('<I',0x80000000))
        pack.extend(bytes((-len(pack))%32));pack.extend(texture)
        class Archive:
            def get(self,index):
                if index!=11:raise AssertionError('incorrect inventory index')
                return bytes(pack)
        with patch.object(assets,'item_display_id',return_value=12),patch.object(rdx,'Afs',return_value=Archive()):
            m,t,index=assets.inventory_model(None,12)
        self.assertEqual((m,t,index),(model,texture,12))

    def test_extraction_path_guard(self):
        with self.assertRaises(ValueError):assets.output_dir(self.root)
    def test_unsafe_object_dispatch_rejected(self):
        self.j['objects'][4]['id']=1299
        with self.assertRaisesRegex(ValueError,'dispatch'):self.build()

if __name__=='__main__':unittest.main()
