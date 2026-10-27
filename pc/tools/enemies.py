#!/usr/bin/env python3
"""Enemy census over the room files (docs/ENEMIES.md). Reads the player's disc image at run time
through pc/tools/rdx.py (imported, not changed); prints counts, ids and script references only.

  room ROOM       the room's enemy records and every script reference to them, with the
                  conditions of the blocks they sit in
  census          all 205 rooms: types per room, counts, kill flags, script control, helper-slot
                  conflicts, per-case and per-difficulty variants
  sources         per (type, model version): the smallest room file carrying its model and its
                  type's motion set (what a spawner loads from)
  selftest        checks the script walker on a few rooms

ISO: --iso, $RECVX_ISO or ISO/SLUS-20184 ROM.iso (as rdx.py).
"""
import argparse, os, struct, sys
from collections import Counter, defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rdx  # noqa: E402

OP = {n: i for i, n in enumerate(rdx.OP_NAMES) if n != 'dm0'}
# tests: they continue or skip the block (as coop_glue.c is_cond)
COND = {4, 6, 7, 16, 17, 27, 30, 33, 38, 56, 77, 93, 96, 98, 114, 129, 138, 140, 142, 155, 172, 177, 190}
# ops whose byte 1 is an enemy record (event.c handlers: rom->enep[b1])
REC_B1 = {'DieCk', 'EneSetCk', 'InitMotionPause', 'InitMotionPauseEx', 'ObjLinkSetEneItem', 'MotionPosSetEnePly',
          'EneRenderSet', 'EneLookFlgSet', 'EneLifeSetM', 'ZombieUpDieCk', 'EnemyHpUp', 'FacePauseSet',
          'FaceReSet', 'FaceRep'}
REC_B2 = {'ObjLinkSetObjEne', 'EneSeOn'}
# bhEne_SetCallFunc users: type -> helper slots it overwrites in bhJumpEnemy
HELPERS = {1: range(31, 39), 2: [31], 3: [31], 5: [31], 6: [31], 13: [31], 17: [31, 32], 23: [31]}
NAMES = {0: 'exploding prop', 1: 'zombie', 2: 'Gulp Worm', 3: 'Black Widow', 4: 'Cerberus', 5: 'Hunter',
         6: 'moth', 7: 'bat', 8: 'fish', 9: 'Bandersnatch', 10: 'ant A', 11: 'Spotter', 12: 'Alexia 1',
         13: 'Alexia 2', 14: 'Alexia 3', 15: 'Nosferatu', 16: 'Alfred', 17: 'Monster Steve',
         18: 'Alexia tentacle', 19: 'Tyrant', 20: 'ant B', 21: 'albinoid infant', 22: 'albinoid adult',
         23: 'Giant Black Widow', 24: 'baby spider', 25: 'ceiling trap', 26: 'anatomist zombie',
         27: 'moth egg', 28: 'empty', 29: 'tentacle', 30: "Alexia's baby", 53: 'Nosferatu core',
         54: 'knight A', 55: 'knight B', 56: 'scripted object', 71: 'ant C'}
CMPB_WHAT = {0: 'stage', 1: 'room', 21: 'case', 23: 'gm_mode', 24: 'ply_id', 25: 'costume'}


def name(i):
    return NAMES.get(i, 'NPC %d' % i if i >= 31 else '?%d' % i)


def cond_text(d, o):
    op, nm = d[o], rdx.OP_NAMES[d[o]]
    if nm in ('Ck', 'CmpB', 'DieEventCk'):
        if nm == 'CmpB' and d[o + 1] in CMPB_WHAT:
            return '%s %s %d' % (CMPB_WHAT[d[o + 1]], ['==', '>', '>=', '<', '<=', '!='][d[o + 2] % 6], d[o + 3])
        if nm == 'DieEventCk':
            return 'dead(ed_flg %d..%d) %s %d' % (struct.unpack_from('<H', d, o + 2)[0], struct.unpack_from('<H', d, o + 2)[0] + d[o + 1] - 1,
                                                  ['==', '>', '>=', '<', '<=', '!='][d[o + 4] % 6], d[o + 5])
        return rdx.comment(d, o)
    if nm == 'CmpW' and d[o + 2] == 10:
        return 'hp(record %d) cmp %d' % (d[o + 1], struct.unpack_from('<H', d, o + 4)[0])
    return nm


def walk(room):
    """every instruction of the room's scripts: (where, offset, conditions of the enclosing blocks)"""
    base, offs = room.script_header()
    d = room.d
    starts = sorted(set(offs))
    for i, off in enumerate(offs):
        where = 'scd0' if i == 0 else 'scd1' if i == 1 else 'ev%d' % (i - 2)
        o, stack = base + off, []   # stack: [end, conds, in_else]
        nxt = min([s for s in starts if s > off], default=0)
        collecting = None
        while o < len(d):
            while stack and stack[-1][0] <= o:
                stack.pop()
            op = d[o]
            n = rdx.insn_len(d, o)
            if n <= 0:
                break
            nm = rdx.OP_NAMES[op]
            if collecting is not None and op in COND:
                collecting.append(cond_text(d, o))
            else:
                collecting = None
            conds = [('not(%s)' % ' and '.join(c) if e else ' and '.join(c)) for _, c, e in stack if c]
            yield where, o, nm, conds
            if nm == 'IfelCk':
                stack.append([o + d[o + 1] + 1, [], False])
                collecting = stack[-1][1]
            elif nm == 'ElseCk':
                c = stack.pop()[1] if stack else []
                stack.append([o + d[o + 1], c, True])
            elif nm == 'Endif' and stack:
                stack.pop()
            o += n
            if not stack and (op == 0 or (i >= 2 and op == 255)):
                break
            if nxt and o > base + nxt:
                break


def record_refs(room):
    """record index -> [(where, op, detail, conditions)]"""
    d, refs = room.d, defaultdict(list)
    for where, o, nm, conds in walk(room):
        recs, det = [], ''
        if nm in REC_B1:
            recs = [d[o + 1]]
            if nm in ('DieCk', 'EneSetCk'):
                det = 'ed_flg %d' % (struct.unpack_from('<H', d, o + 2)[0] & 0x3FF)
        elif nm in REC_B2:
            recs = [d[o + 2]]
        elif nm == 'ObjLinkSetEneEne':
            recs = [d[o + 1], d[o + 2]]
        elif nm == 'InitModelSet' and d[o + 1] == 1:
            recs, det = [d[o + 2]], 'show' if d[o + 3] else 'hide'
        elif nm in ('LoadWork', 'LoadWork2', 'LoadWorkEx') and d[o + 1] == 1:
            recs = [d[o + 2]]
        elif nm == 'CmpW' and d[o + 2] == 10:
            recs = [d[o + 1]]
        elif nm == 'SvW' and 11 <= d[o + 1] <= 15:
            recs = [d[o + 1] - 11]
        for r in recs:
            refs[r].append((where, nm, det, conds))
    return refs


def load_all(args):
    afs = rdx.open_disc(args)
    for i, n in enumerate(rdx.room_names()):
        yield n, rdx.Room(rdx.expand(afs.get(i)), n)


def classify(recs, refs):
    """per record: kill flag, hidden at entry, shown by a condition/event, scripted acting"""
    out = []
    for k, e in enumerate(recs):
        r = refs.get(k, [])
        tags = []
        die = [x for x in r if x[1] == 'DieCk']
        if die:
            tags.append('kill ' + die[0][2])
        hid0 = [x for x in r if x[1] == 'InitModelSet' and x[2] == 'hide' and x[0] == 'scd0']
        shows = [x for x in r if x[1] == 'InitModelSet' and x[2] == 'show']
        if hid0:
            tags.append('hidden at entry')
        for w, _, _, c in shows:
            tags.append('shown in %s%s' % (w, ' if ' + ' and '.join(c) if c else ''))
        if any(x[1].startswith('LoadWork') for x in r):
            tags.append('event-driven (' + ','.join(sorted({x[0] for x in r if x[1].startswith('LoadWork')})) + ')')
        other = sorted({x[1] for x in r} - {'DieCk', 'EneSetCk', 'InitModelSet', 'LoadWork', 'LoadWork2', 'LoadWorkEx'})
        if other:
            tags.append('ops ' + ','.join(other))
        out.append(tags)
    return out


def nav(room):
    """floor records of type 2 (enemy areas: prm0 = enemy type, prm1 = what, e.g. Cerberus wake-up
    areas, Hunter stairs/jumps; zonzon1.c bhEne_CheckEventAtr) per type, and the route box count"""
    c = Counter(f['prm0'] for f in room.records('floors') if f['type'] == 2)
    return dict(c), room.cnt['rut']


def cmd_room(args):
    room = rdx.load_room(args, args.room)
    recs = room.records('enemies')
    mt = room.motion_table()
    refs = record_refs(room)
    print('%s: %d enemy records; enemy floor areas per type %s; route boxes %d' % ((room.name, len(recs)) + nav(room)))
    for k, e in enumerate(recs):
        print('%2d  id %2d %-18s type %2d mdlver %2d flg %#x wrk %d floor %d motions %s pos %.0f %.0f %.0f' % (
            k, e['id'], name(e['id']), e['type'], e['mdlver'], e['flg'], e['wrk_no'], e['flr_no'],
            'own' if mt[k] else '-', e['px'], e['py'], e['pz']))
        for w, nm, det, conds in refs.get(k, []):
            print('      %-5s %-16s %-8s %s' % (w, nm, det, ('if ' + ' and '.join(conds)) if conds else ''))


def cmd_census(args):
    types_rooms, max_cnt, per_room = defaultdict(set), Counter(), []
    conflicts, gm_rooms, ply_rooms, hidden, event_driven, killflag, nokill = [], [], [], 0, 0, 0, 0
    total, cases = 0, defaultdict(dict)
    zombie_mdlver = Counter()
    nav_types, route_rooms = Counter(), 0
    for n, room in load_all(args):
        recs = room.records('enemies')
        refs = record_refs(room)
        tags = classify(recs, refs)
        ids = Counter(e['id'] for e in recs)
        areas, rut = nav(room)
        route_rooms += rut > 0
        for t in areas:
            nav_types[t] += 1
        for e in recs:
            types_rooms[e['id']].add(n[3:7])
            if e['id'] == 1:
                zombie_mdlver[e['mdlver']] += 1
        for i, c in ids.items():
            max_cnt[i] = max(max_cnt[i], c)
        total += len(recs)
        helpers = [i for i in ids if i in HELPERS]
        slot31 = [i for i in helpers if 31 in HELPERS[i]]
        npc_hit = [i for i in ids if 31 <= i <= 38 and 1 in ids]
        if len(slot31) > 1 or npc_hit:
            conflicts.append('%s: %s' % (n, ' '.join(map(str, slot31 + npc_hit))))
        d = room.d
        for where, o, nm, conds in walk(room):
            if nm == 'CmpB' and d[o + 1] == 23 and n not in gm_rooms:
                gm_rooms.append(n)
            if nm == 'CmpB' and d[o + 1] == 24 and n not in ply_rooms:
                ply_rooms.append(n)
        for t in tags:
            hidden += any(x.startswith('hidden') for x in t)
            event_driven += any(x.startswith('event') for x in t)
            killflag += any(x.startswith('kill') for x in t)
        nokill += sum(1 for k, e in enumerate(recs) if e['id'] <= 30 and e['id'] not in (0, 28) and not any(x.startswith('kill') for x in tags[k]))
        enemies = [e for e in recs if e['id'] <= 30 or 53 <= e['id'] <= 56 or e['id'] == 71]
        per_room.append((n, len(recs), len(enemies), ' '.join('%s x%d' % (name(i), c) for i, c in sorted(ids.items()))))
        cases[n[3:6]][n[6]] = tuple(sorted((e['id'], e['mdlver']) for e in enemies))
    print('records in all rooms: %d; rooms with records: %d' % (total, sum(1 for r in per_room if r[1])))
    print('records hidden at entry (scd0 InitModelSet hide): %d; driven by events (LoadWork): %d; with a kill flag (DieCk): %d; '
          'hostile records (ids 1..30) without a kill flag: %d' % (hidden, event_driven, killflag, nokill))
    print('\nper type: rooms, most records in one room')
    for i in sorted(types_rooms):
        print('  %2d %-18s %3d rooms  max %2d   %s' % (i, name(i), len(types_rooms[i]), max_cnt[i], ' '.join(sorted(types_rooms[i]))[:150]))
    print('\nzombie model versions (records):', dict(sorted(zombie_mdlver.items())))
    print('rooms with enemy floor areas (type 2 floors), per enemy type:', dict(sorted(nav_types.items())))
    print('rooms with route boxes (rutp):', route_rooms)
    print('\nhelper-slot conflicts (two slot-31 users, or zombies with NPC ids 31..38):', conflicts or 'none')
    print('rooms whose script compares gm_mode (difficulty / battle game):', ' '.join(gm_rooms) or 'none')
    print('rooms whose script compares ply_id:', len(ply_rooms))
    diff = [(k, v) for k, v in sorted(cases.items()) if len(v) > 1 and len(set(v.values())) > 1]
    print('\nrooms whose case variants (RM_SRRC, C = case) hold different enemies: %d of %d with several cases' % (
        len(diff), sum(1 for v in cases.values() if len(v) > 1)))
    for k, v in diff:
        print('  %s: %s' % (k, '; '.join('case %s: %s' % (c, ' '.join('%s/%d' % (name(i), m) for i, m in t) or '-') for c, t in sorted(v.items()))))
    if args.rooms:
        print('\nper room: records, enemies (ids 1..30, 53..56, 71), types')
        for r in per_room:
            if r[1]:
                print('  %s %2d %2d  %s' % r)


def cmd_sources(args):
    best = {}
    for n, room in load_all(args):
        recs = room.records('enemies')
        mt = room.motion_table()
        has_mtn = {e['id'] for k, e in enumerate(recs) if mt[k]}
        for k, e in enumerate(recs):
            key = (e['id'], e['mdlver'])
            if e['flg'] & 1 and e['id'] in has_mtn and (key not in best or len(room.d) < best[key][0]):
                best[key] = (len(room.d), n[3:7], k, e['type'])
    for (i, v), (size, n, k, t) in sorted(best.items()):
        print('%2d %-18s mdlver %2d: RM_%s record %2d (type %d, %d KB)' % (i, name(i), v, n, k, t, size >> 10))


def cmd_selftest(args):
    room = rdx.load_room(args, 'RM_0060')
    refs = record_refs(room)
    shows = [x for x in refs[3] if x[1] == 'InitModelSet' and x[2] == 'show']
    assert shows and shows[0][3] == ['ev_flg bit 116 is set and ed_flg bit 15 is clear'], shows
    assert [x[2] for x in refs[0] if x[1] == 'DieCk'] == ['ed_flg 12']
    print('RM_0060 ok')


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--iso')
    sp = ap.add_subparsers(dest='cmd', required=True)
    p = sp.add_parser('room'); p.add_argument('room'); p.set_defaults(f=cmd_room)
    p = sp.add_parser('census'); p.add_argument('--rooms', action='store_true', help='also list every room'); p.set_defaults(f=cmd_census)
    sp.add_parser('sources').set_defaults(f=cmd_sources)
    sp.add_parser('selftest').set_defaults(f=cmd_selftest)
    args = ap.parse_args()
    args.f(args)


if __name__ == '__main__':
    main()
