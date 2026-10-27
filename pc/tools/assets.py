#!/usr/bin/env python3
"""Retail asset catalogue. All extracted bytes stay in ignored scratch/; references are portable.

python3 pc/tools/assets.py inventory [--previews] [--geometry]
python3 pc/tools/assets.py search wood
Models retain their native MLB, textures retain every TIM2 palette. PNG is a preview only.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import rdx
import rdxbuild as b


def model_header(asset):
    offset = 0 if asset[:3] == b'MDL' else struct.unpack_from('<I', asset)[0] + 8
    if asset[offset:offset+3] != b'MDL':
        raise ValueError('unsupported object model header')
    return offset, struct.unpack_from('<i', asset, offset+8)[0] != -1


def dependencies(room):
    """Replay room.c texture ownership, including untextured and repeated records."""
    models = b.split_table(room, 'model')
    ti, mi = 1, 1
    owners = {}
    for rec in room.records('enemies'):
        if not rec['flg'] & 1:
            continue
        for part in range(b.enemy_textured_parts(models[mi])):
            key = (rec['id'], rec['mdlver'], part)
            if key not in owners:
                owners[key] = ti
                ti += 1
        mi += 1
    out = {}
    for kind in ('objects', 'items'):
        owners = {}
        for index, rec in enumerate(room.records(kind)):
            if kind == 'objects' and index < 4:
                continue
            _, textured = model_header(models[mi])
            if textured and rec['id'] not in owners:
                owners[rec['id']] = ti
                ti += 1
            out[kind, index] = (mi, owners.get(rec['id']) if textured else None)
            mi += 1
    # then one list per effect id 30..99 / 400+ (first record of an id). room.c reads the lists
    # in this order and skips none: a list too many shifts every later effect onto its
    # neighbour's texture and palette (a second door of id 900 with its own list did that).
    ti += len({e['id'] for e in room.records('effects') if 30 <= e['id'] < 100 or e['id'] >= 400})
    if ti != len(room.texture_table()):
        raise ValueError('room texture table has %d lists, the game reads %d: %s' % (len(room.texture_table()), ti, room.name))
    return out


def owns_texture(room, kind, rec_id):
    """a record of this id already brings the texture list for it (room.c: first record of an id)"""
    return any(k == kind and ti is not None and room.records(kind)[i]['id'] == rec_id
               for (k, i), (_, ti) in dependencies(room).items())


def dependency(room, kind, index):
    mi, ti = dependencies(room)[kind, index]
    return b.split_table(room, 'model')[mi], (b.split_table(room, 'texture')[ti] if ti is not None else None)


def item_display_id(iso, item_id):
    """Replay sub1.c's dsptbl -> itemflg -> itemidtbl aliases from the disc ELF."""
    import re
    if not 1 <= item_id < 156:raise ValueError('not a retail inventory item id (1..155)')
    lsn,size=iso.find('SLUS_201.84');elf=iso.read(lsn,(size+2047)//2048)[:size]
    if elf[:5]!=b'\x7fELF\x01':raise ValueError('expected retail ELF32')
    phoff=rdx.u32(elf,28);stride,count=struct.unpack_from('<HH',elf,42)
    symbols=Path(rdx.REPO,'decomp/config/symbol_addrs.txt').read_text()
    def table(name,size):
        address=int(re.search(r'^'+name+r'\s*=\s*(0x[0-9A-Fa-f]+)',symbols,re.M)[1],16)
        for i in range(count):
            typ,off,va,_,filesz=struct.unpack_from('<5I',elf,phoff+i*stride)
            if typ==1 and va<=address and address+size<=va+filesz:return elf[off+address-va:off+address-va+size]
        raise ValueError('item display table outside ELF loads')
    ds=table('dsptbl',160*8);flags=table('itemflg',41*8);ids=table('itemidtbl',16*2)
    group=struct.unpack_from('<h',ds,item_id*8+6)[0]
    index=flags[group*8+6]
    display=struct.unpack_from('<H',ids,index*2)[0] or item_id
    return {131:5,136:33}.get(display,display)


def inventory_model(iso, item_id, archive='ITEM1.AFS'):
    display=item_display_id(iso,item_id)
    data=rdx.Afs(iso,archive).get(display-1)
    size=rdx.u32(data,0);model=data[4:4+size]
    if model[:3]!=b'MDL':raise ValueError('item has no native display model')
    p=4+size;word=rdx.u32(data,p);p+=4
    if word&0x80000000:p=(p+31)&~31
    texture=data[p:]
    if texture[:4]!=b'TIM2':raise ValueError('invalid item texture pack')
    return model,texture,display


def inventory_displays(iso,dest):
    directory=dest/'inventory';directory.mkdir(exist_ok=True)
    entries=[]
    for item_id in range(1,156):
        try:
            model,texture,display=inventory_model(iso,item_id)
        except ValueError as error:
            entries.append({'id':item_id,'unavailable':str(error)})
            continue
        (directory/('%03d.mdl'%display)).write_bytes(model)
        (directory/('%03d.pvp'%display)).write_bytes(texture)
        entries.append({'id':item_id,'archive':'ITEM1.AFS','entry':display-1,
                        'model':'inventory/%03d.mdl'%display,'textures':'inventory/%03d.pvp'%display})
    return entries


def extract_inventory_archives(iso,dest):
    """Keep both disc partitions, including display variants not selected by item aliases."""
    entries=[]
    for archive in ('ITEM1.AFS','ITEM2.AFS'):
        afs=rdx.Afs(iso,archive);directory=dest/archive[:-4];directory.mkdir(exist_ok=True)
        for index in range(144):
            data=afs.get(index);size=rdx.u32(data,0)
            if data[4:7]!=b'MDL':
                entries.append({'archive':archive,'entry':index,'unavailable':'not a model packet'})
                continue
            p=4+size;word=rdx.u32(data,p);p+=4
            if word&0x80000000:p=(p+31)&~31
            (directory/('%03d.mdl'%index)).write_bytes(data[4:4+size])
            (directory/('%03d.pvp'%index)).write_bytes(data[p:])
            entries.append({'archive':archive,'entry':index,'model_bytes':size,'texture_bytes':len(data)-p})
    return entries


def find_item(args, item_id):
    """Prefer a room display; fall back to that item's inventory model, never a placeholder."""
    if not 0 <= item_id < 200: raise ValueError('item id outside runtime table')
    afs=rdx.open_disc(args)
    for i,name in enumerate(rdx.room_names()):
        room=rdx.Room(rdx.expand(afs.get(i)),name)
        for index,rec in enumerate(room.records('items')):
            if rec['id']==item_id:
                dependency(room,'items',index)  # validate ownership before returning
                return {'room':name,'record':index}
    inventory_model(afs.iso,item_id)
    return {'inventory_item':item_id}


def door_pack(iso, number):
    data = rdx.Afs(iso, 'MRY.AFS').get(71+number)
    if data[:4] != b'MPAC':
        raise ValueError('door pack is not MPAC')
    p = 4 + rdx.u32(data, 4)
    parts = []
    while p+4 <= len(data):
        size = struct.unpack_from('<i', data, p)[0]
        p += 4
        if size == -1:
            break
        if size < 0 or p+size > len(data):
            raise ValueError('bad door section')
        parts.append(data[p:p+size]); p += size
    if len(parts) < 4:
        raise ValueError('incomplete door pack')
    return data, parts


def output_dir(path):
    path = Path(path).resolve()
    scratch = Path(rdx.REPO, 'scratch').resolve()
    if not path.is_relative_to(scratch):
        raise ValueError('extracted assets must be inside this clone\'s ignored scratch/')
    path.mkdir(parents=True, exist_ok=True)
    return path


def inventory(args):
    dest = output_dir(args.output)
    afs = rdx.open_disc(args)
    catalogue = {'format':'recvx-assets-1', 'rooms':[], 'doors':[]}
    for i, name in enumerate(rdx.room_names()):
        room = rdx.Room(rdx.expand(afs.get(i)), name)
        directory = dest/name[:7]; directory.mkdir(exist_ok=True)
        refs = dependencies(room)
        entry = {'room':name, 'textures':[], 'models':[], 'objects':[], 'items':[],
                 'collision':{k:room.records(k) for k in ('walls','floors','misc') if k in [x[0] for x in rdx.SECTIONS]}}
        for kind in ('model','motion','texture'):
            for index, blob in enumerate(b.split_table(room, kind)):
                if blob:
                    (directory/('%s%d.bin'%(kind,index))).write_bytes(blob)
        for index, blob in enumerate(b.split_table(room,'model')):
            entry['models'].append({'index':index,'bytes':len(blob),'sha256':hashlib.sha256(blob).hexdigest()})
        for kind in ('objects','items'):
            for index, rec in enumerate(room.records(kind)):
                if (kind,index) in refs:
                    mi,ti=refs[kind,index]
                    entry[kind].append({'record':index,'id':rec['id'],'model':mi,'texture_list':ti,'placement':rec})
        for li, textures in rdx.texture_blocks(room):
            for index,t in enumerate(textures):
                stem='tex%d_%d'%(li,index)
                (directory/(stem+'.tm2')).write_bytes(room.d[t['ofs']:t['ofs']+t['size']])
                if args.previews:
                    rdx.write_png(str(directory/(stem+'.png')),t['w'],t['h'],rdx.tim2_rgba(room.d,t))
                entry['textures'].append(dict(t,index=index,list=li,path=str((directory/(stem+'.tm2')).relative_to(dest))))
        if args.geometry:
            rdx.export_obj(room,str(directory/'room.obj'))
        mo=rdx.model_objects(room)
        entry['geometry_nodes']=mo['objects']
        catalogue['rooms'].append(entry)
        if i%20==0: print('catalogued',i+1,name,flush=True)
    # Discover every actual MPAC door, not a guessed hard-coded door count.
    mry=rdx.Afs(afs.iso,'MRY.AFS')
    for index in range(71,len(mry.toc)):
        if mry.get(index)[:4] != b'MPAC': continue
        raw,parts=door_pack(afs.iso,index-71)
        directory=dest/'doors'/str(index-71);directory.mkdir(parents=True,exist_ok=True)
        (directory/'door.mpac').write_bytes(raw)
        for k,part in enumerate(parts): (directory/('part%d.bin'%k)).write_bytes(part)
        catalogue['doors'].append({'type':index-71,'archive':'MRY.AFS','entry':index,
                                   'parts':[len(p) for p in parts], 'path':str(directory.relative_to(dest))})
    catalogue['inventory_items']=inventory_displays(afs.iso,dest)
    catalogue['inventory_models']=extract_inventory_archives(afs.iso,dest)
    (dest/'catalogue.json').write_text(json.dumps(b.json_safe(catalogue),indent=2,allow_nan=False)+'\n')
    print('catalogue:',len(catalogue['rooms']),'rooms,',len(catalogue['doors']),'door packs')


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--iso')
    s=p.add_subparsers(dest='cmd',required=True)
    q=s.add_parser('inventory');q.add_argument('--output',default='scratch/assets');q.add_argument('--previews',action='store_true');q.add_argument('--geometry',action='store_true')
    q=s.add_parser('item');q.add_argument('id',type=int)
    q=s.add_parser('search');q.add_argument('query');q.add_argument('--catalogue',default='scratch/assets/catalogue.json')
    args=p.parse_args()
    if args.cmd=='inventory':inventory(args)
    elif args.cmd=='item':print(json.dumps(find_item(args,args.id)))
    else:
        cat=json.loads(Path(args.catalogue).read_text())
        curated=json.loads(Path(__file__).with_name('area').joinpath('village_assets.json').read_text())
        for entry in curated['assets']:
            if args.query.lower() in json.dumps(entry).lower():print(json.dumps(entry))
        for kind in ('rooms','doors','inventory_items','inventory_models'):
            for entry in cat[kind]:
                if args.query.lower() in json.dumps(entry).lower():print(json.dumps(entry))

if __name__=='__main__':main()
