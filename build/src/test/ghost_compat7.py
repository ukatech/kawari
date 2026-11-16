#!/usr/bin/env python3
# Compatibility test of the Kawari 7.5.0 compatible mode on a real Kawari 7 ghost.
#
#   python build/src/test/ghost_compat7.py --ghost <ghost>/ghost/master [--old-dll <dll>] [--new-dll <dll>]
#
# The old DLL (default: the shiori.dll of the ghost, which must be the real Kawari 7.5.0) and the new
# DLL (default: build/mach/mingw/shiori.dll) get the same SHIORI/2.x requests. The two use the same
# random generator (MT19937) and the ghost is started with the same "randomseed", so the answers must
# be identical byte for byte (except the version numbers on purpose).
#
# The ghost is never touched: it is copied for every check. In the copies every program (*.exe ...)
# is deleted and every DLL except shiori.dll (SAORI and so on) is replaced by harness/stub_saori.c.
#
# Checks:
#   replay    the old DLL walks through the ghost (SHIORI/2.x events, then the \q[] links of the
#             answers, depth first); the new DLL gets the same requests: the answers must be the same
#   utf8      not tested (Kawari 7.5.0 has no UTF-8 mode)
#
# Needs Windows and a 32bit MinGW gcc (the DLLs are 32bit). Exit codes: 0 = all same, 1 = differences,
# 2 = the test could not run. This file is ASCII only.

import argparse
import collections
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import ghost_compat as gc  # noqa: E402  (reuse: build_tools, Session, pe_machine, constants)

REPO = gc.REPO
DEFAULT_NEW_DLL = os.path.join(REPO, 'build', 'mach', 'mingw', 'shiori.dll')
SEED = 12345


def req(first, headers, cs='Shift_JIS'):
    """first : the first line (without CRLF), headers : [(name, value)]"""
    lines = [first, 'Sender: SSP', 'SecurityLevel: local', 'Charset: ' + cs]
    lines += ['%s: %s' % (k, v) for k, v in headers]
    return ('\r\n'.join(lines) + '\r\n\r\n').encode('cp932', 'replace')


def event(name, refs):
    return req('GET Sentence SHIORI/2.2', [('Event', name)] + [('Reference%d' % i, r) for i, r in enumerate(refs)])


def start_requests():
    """what SSP sends around a start (SHIORI/2.x). Items are (label, bytes, follow)"""
    s = []
    s.append(('version', req('GET Version SHIORI/2.6', []), False))
    s.append(('owner', req('NOTIFY OwnerGhostName SHIORI/2.6', [('Ghost', 'kuro')]), False))
    s.append(('other', req('NOTIFY OtherGhostName SHIORI/2.3b',
                           [('Ghost', 'emily'), ('Ghost', 'tama'), ('GhostEx', 'emily,0'), ('GhostEx', 'tama,10')]), False))
    s.append(('Event OnFirstBoot', event('OnFirstBoot', ['0']), False))
    s.append(('Event OnBoot', event('OnBoot', ['master']), False))
    for i in range(3):
        s.append(('Event OnSecondChange %d' % i, event('OnSecondChange', ['1', '0', '0', '1', str(i)]), False))
    s.append(('Event OnMinuteChange', event('OnMinuteChange', ['1', '0', '0', '1']), False))
    s.append(('Event OnMouseMove', event('OnMouseMove', ['10', '10', '0', '0', 'Head', '0']), False))
    # many random talks: any difference in the order of the random calls shows up as a drift
    for i in range(120):
        s.append(('Sentence (no event) %d' % i, req('GET Sentence SHIORI/2.0', []), False))
    for i in range(150):
        s.append(('Event OnSecondChange (long) %d' % i,
                  event('OnSecondChange', [str(i % 60), '0', '0', '1', str(i % 3)]), False))
    for i in range(40):
        s.append(('Event OnMinuteChange (long) %d' % i, event('OnMinuteChange', [str(i % 60), '0', '0', '1']), False))
    for i, part in enumerate(('Head', 'Face', 'Bust', 'Hand', 'Skirt', 'Head', 'Bust', 'Face')):
        for ev in ('OnMouseClick', 'OnMouseDoubleClick', 'OnMouseMove'):
            s.append(('Event %s %s %d' % (ev, part, i), event(ev, ['10', '10', '0', str(i % 2), part, '0']), False))
    s.append(('Status', req('GET Status SHIORI/2.0', []), False))
    for rid in ('homeurl', 'sakura.recommendsites', 'sakura.portalsites', 'kero.recommendsites', 'menu.homeurl'):
        s.append(('String ' + rid, req('GET String SHIORI/2.5', [('ID', rid)]), False))
    for t in ('\\ms', '\\mz', '\\mc', '\\mh', '\\mt', '\\me', '\\mp', '\\m?', '\\dms'):
        s.append(('Word ' + t, req('GET Word SHIORI/2.0', [('Type', t)]), False))
    s.append(('Teach', req('TEACH SHIORI/2.4', [('Word', 'hello')]), False))
    s.append(('Translate', req('TRANSLATE Sentence SHIORI/2.6', [('Sentence', '\\0\\s[0]Hello\\e')]), False))
    for i, text in enumerate(('hello', 'weather', 'good morning')):
        s.append(('Communicate %d' % i, req('GET Sentence SHIORI/2.3b',
                                           [('Sentence', text), ('Age', '0'), ('Surface', '0,10')]
                                           + [('Reference%d' % k, '') for k in range(8)]), False))
    for name, refs in (('OnMouseDoubleClick', ['10', '10', '0', '0', 'Head', '0']),
                       ('OnMouseClick', ['10', '10', '0', '0', 'Head', '0']),
                       ('OnMouseWheel', ['10', '10', '1', '0', 'Head', '0']),
                       ('OnSurfaceChange', ['0', '10', '', '']),
                       ('OnSurfaceRestore', ['0', '10']),
                       ('OnKeyPress', ['a', '65']),
                       ('OnChoiceTimeout', ['x']),
                       ('OnChoiceEnter', ['a', 'b']),
                       ('OnBalloonBreak', ['0', '0', '0']),
                       ('OnBalloonTimeout', ['0']),
                       ('OnBalloonClose', ['0']),
                       ('OnUserInput', ['name', 'test input']),
                       ('OnVanishSelecting', []),
                       ('OnVanishSelected', []),
                       ('OnVanishCancel', []),
                       ('OnGhostChanged', ['emily', '', '', '', '', '', '', 'master']),
                       ('OnShellChanged', ['master', 'master', 'master']),
                       ('OnInstallComplete', ['ghost', 'x', 'y']),
                       ('OnFileDrop', ['C:\\a.txt']),
                       ('OnMusicPlay', ['Title', 'Artist']),
                       ('OnSSTPBreak', ['a', 'b']),
                       ('OnGhostChanging', ['emily', 'manual']),
                       ('OnGhostChanging', ['emily', 'automatic']),
                       ('OnTeach', ['hello']),
                       ('OnClose', ['user'])):
        s.append(('Event ' + name, event(name, refs), name != 'OnClose'))
    return s


def second_requests():
    """a second run on the state (save data) left by the first one"""
    s = [req('GET Version SHIORI/2.6', []), req('NOTIFY OwnerGhostName SHIORI/2.6', [('Ghost', 'kuro')]),
         event('OnBoot', ['master'])]
    for i in range(30):
        s.append(event('OnSecondChange', [str(i % 60), '0', '0', '1', str(i % 3)]))
    for i in range(30):
        s.append(req('GET Sentence SHIORI/2.0', []))
    s.append(event('OnMouseDoubleClick', ['10', '10', '0', '0', 'Head', '0']))
    s.append(event('OnClose', ['user']))
    return s


LINK = [re.compile(r'\\_?q\[([^\],]*),([^\]]*)\]'), re.compile(r'\\!\[(?:raise|notify),([^\]]*)\]')]


def followups(script):
    out = []
    for m in LINK[0].findall(script):
        title, rest = m
        parts = rest.split(',')
        if re.match(r'^On\w+$', parts[0]):
            out.append(event(parts[0], parts[1:]))
        else:
            out.append(event('OnChoiceSelect', [parts[0], title]))
    for m in LINK[1].findall(script):
        parts = m.split(',')
        out.append(event(parts[0], parts[1:]))
    return out


def sentence_of(resp):
    m = re.search(rb'\r\nSentence: (.*?)\r\n', resp)
    return m.group(1).decode('cp932', 'replace') if m else None


REAL_SAORI = set()


def prepare(src, dest, dll, stub):
    if os.path.exists(dest):
        shutil.rmtree(dest)
    shutil.copytree(src, dest, ignore=shutil.ignore_patterns(*['*' + e for e in gc.HEAVY_EXT]))
    for root, _, files in os.walk(dest):
        for f in files:
            low = f.lower()
            p = os.path.join(root, f)
            if low.endswith(gc.PROGRAM_EXT):
                os.remove(p)
            elif low.endswith('.dll') and low not in REAL_SAORI:
                shutil.copy(stub, p)
    shutil.copy(dll, os.path.join(dest, 'shiori.dll'))
    ini = os.path.join(dest, 'kawari.ini')
    data = open(ini, 'rb').read() if os.path.exists(ini) else b''
    if data and not data.endswith(b'\n'):
        data += b'\r\n'
    open(ini, 'wb').write(data + b'randomseed : %d\r\n' % SEED)


def crawl(harness, ghost, limit, timeout):
    s = gc.Session(harness, ghost, timeout)
    labels, reqs, resps = [], [], []
    try:
        queue = collections.deque(start_requests())
        seen = set()
        while queue and len(reqs) < limit:
            label, r, follow = queue.popleft()
            if r in seen:
                continue
            seen.add(r)
            resp = s.send(r)
            labels.append(label)
            reqs.append(r)
            resps.append(resp)
            if follow:
                sc = sentence_of(resp)
                if sc:
                    queue.extendleft(reversed([('follow', f, True) for f in followups(sc)]))
    finally:
        rc, err = s.close()
    return labels, reqs, resps, rc, err


def replay(harness, ghost, reqs, timeout):
    s = gc.Session(harness, ghost, timeout)
    try:
        resps = [s.send(r) for r in reqs]
    finally:
        rc, err = s.close()
    return resps, rc, err


def normalize(b):
    """the version numbers and the name of the two DLLs are not a difference"""
    t = b.decode('cp932', 'replace')
    t = re.sub(r'(?m)^(ID|Craftman|Version): .*$', r'\1: <v>', t)
    # the answer to GET Version says SHIORI/3.0 (the 7.5.0 says SHIORI/2.6): the baseware decides 2.x or 3.0 by it.
    # The response of GET Version is the only one that has ID / Craftman / Version
    if '<v>' in t and t.startswith('SHIORI/2.6 200 OK'):
        t = 'SHIORI/3.0 200 OK' + t[len('SHIORI/2.6 200 OK'):]
    # GET Status : the numbers of words and entries are internal statistics (they differ by how
    # the two engines store words), so only the shape is compared
    t = re.sub(r'(?m)^(Status): (\d+),(\d+),(\d+),[^,\r\n]*,', r'\1: <n>,<n>,\4,<v>,', t)
    return t


def show(b, n=240):
    t = b.decode('cp932', 'replace').replace('\r\n', ' | ')
    return t if len(t) <= n else t[:n] + '...'


def main():
    ap = argparse.ArgumentParser(description='Kawari 7 compatible mode on a real Kawari 7 ghost')
    ap.add_argument('--ghost', required=True, help='ghost/master folder (has kawari.ini and the 7.5.0 shiori.dll)')
    ap.add_argument('--old-dll', help='the reference DLL (default: shiori.dll of the ghost)')
    ap.add_argument('--new-dll', default=DEFAULT_NEW_DLL)
    ap.add_argument('--cc', default='gcc', help='a 32bit gcc')
    ap.add_argument('--limit', type=int, default=400, help='max number of requests')
    ap.add_argument('--timeout', type=int, default=240)
    ap.add_argument('--keep', action='store_true', help='keep the work folder')
    ap.add_argument('--real-saori', default='', help='comma separated names of SAORI DLLs that are kept as they are '
                    '(only for harmless ones that do not start programs or open windows)')
    a = ap.parse_args()
    REAL_SAORI.update(n.strip().lower() for n in a.real_saori.split(',') if n.strip())

    ghost = os.path.abspath(a.ghost)
    if not os.path.exists(os.path.join(ghost, 'kawari.ini')):
        gc.stop_setup('no kawari.ini in ' + ghost)
    old_dll = os.path.abspath(a.old_dll or os.path.join(ghost, 'shiori.dll'))
    new_dll = os.path.abspath(a.new_dll)
    for p in (old_dll, new_dll):
        if not os.path.exists(p):
            gc.stop_setup('no such file: ' + p)
    machine = gc.pe_machine(old_dll)
    work = tempfile.mkdtemp(prefix='kawari7compat_')
    try:
        harness, stub = gc.build_tools(a.cc, work, machine)
        old_g = os.path.join(work, 'old')
        new_g = os.path.join(work, 'new')
        prepare(ghost, old_g, old_dll, stub)
        prepare(ghost, new_g, new_dll, stub)

        labels, reqs, ra, rc, err = crawl(harness, old_g, a.limit, a.timeout)
        print('old DLL: %d requests, exit %s%s' % (len(reqs), rc, (' ' + err) if err else ''))
        answered = sum(1 for r in ra if r.startswith(b'SHIORI/2') and b' 200 ' in r.split(b'\r\n')[0])
        print('         %d answers with 200 OK' % answered)
        if rc != 0:
            gc.stop_setup('the old DLL failed')
        # the old DLL must answer the same twice, or the answers cannot be compared. The ghost may use the clock
        # (a minute boundary between two runs), so this is tried up to 3 times.
        for attempt in range(3):
            prepare(ghost, old_g, old_dll, stub)
            ra2, rc2, err2 = replay(harness, old_g, reqs, a.timeout)
            if [normalize(x) for x in ra] == [normalize(x) for x in ra2]:
                break
            ra = ra2          # the next run is compared with this one
        else:
            gc.stop_setup('the old DLL does not answer the same twice (the seed does not fix its answers)')

        prepare(ghost, new_g, new_dll, stub)
        rb, rc, err = replay(harness, new_g, reqs, a.timeout)
        print('new DLL: exit %s%s' % (rc, (' ' + err) if err else ''))
        diffs = [i for i in range(len(reqs)) if normalize(ra[i]) != normalize(rb[i])]
        print('replay: %d / %d answers are the same' % (len(reqs) - len(diffs), len(reqs)))
        for i in diffs[:8]:
            print('  DIFF [%d] %s' % (i, labels[i]))
            print('    old: ' + show(ra[i]))
            print('    new: ' + show(rb[i]))
        if len(diffs) > 8:
            print('  ... and %d more' % (len(diffs) - 8))
        failed = bool(diffs or rc != 0)

        # the requests that reached the SAORI (the stub SAORI writes them to <DLL>.log). The new SAORI layer asks
        # "GET Version" first to find out the charset of the module (the 7.5.0 does not), which is not a difference.
        def saori_requests(g):
            return dict((m, [x for x in lst if not x.startswith('GET Version')])
                        for m, lst in gc.saori_entries(g).items())
        sa, sb = saori_requests(old_g), saori_requests(new_g)
        count_a = sum(len(v) for v in sa.values())
        print('saori: %d requests reached the SAORI (7.5.0), %d (new)%s' % (
            count_a, sum(len(v) for v in sb.values()), ', the same' if sa == sb else ', DIFFERENT'))
        if sa != sb:
            failed = True
            for m in sorted(set(sa) | set(sb)):
                la, lb = sa.get(m, []), sb.get(m, [])
                for i in range(max(len(la), len(lb))):
                    x = la[i] if i < len(la) else '(none)'
                    y = lb[i] if i < len(lb) else '(none)'
                    if x != y:
                        print('  %s [%d]\n    old: %r\n    new: %r' % (m, i, x[:200], y[:200]))
                        break

        # save data : the first runs left their save data in old_g (written by 7.5.0, Shift_JIS) and in new_g
        # (written by the new DLL, UTF-8 with BOM). A second run from the SAME state must give the same answers
        # with the old DLL and with the new DLL: upgrade = the new DLL reads the 7.5.0 save data,
        # own save = the new DLL reads its own save data (the old DLL, as the reference, reads it too: the BOM sits
        # in front of a comment line). The ghost keeps real times in its save data and uses the clock, so a pair
        # that differs is run again (up to 3 rounds) to tell a clock boundary from a real difference.
        sreqs = second_requests()
        count = [0]

        def to_shift_jis(folder):
            """the save data of the new DLL (UTF-8 with BOM) as Shift_JIS, which the 7.5.0 DLL can read"""
            for f in os.listdir(folder):
                p = os.path.join(folder, f)
                if f.lower().endswith('.txt') and os.path.isfile(p):
                    data = open(p, 'rb').read()
                    if data[:3] == b'\xef\xbb\xbf':
                        open(p, 'wb').write(data[3:].decode('utf-8').encode('cp932', 'replace'))

        def second(state, dll, sjis=False):
            dest = os.path.join(work, 'second_%d' % count[0])
            count[0] += 1
            gc.Ghosts.clone(state, dest, dll)
            if sjis:
                to_shift_jis(dest)
            r, rc2, err2 = replay(harness, dest, sreqs, a.timeout)
            if rc2 != 0:
                print('second run: exit %s %s' % (rc2, err2))
            return r, rc2

        for tag, state in (('upgrade (7.5.0 save data)', old_g), ('own save data', new_g)):
            d = []
            for rnd in range(3):
                ref, rc_a = second(state, old_dll, sjis=True)
                cand, rc_b = second(state, new_dll)
                ref2, rc_c = second(state, old_dll, sjis=True)
                d = [i for i in range(len(sreqs)) if normalize(ref[i]) != normalize(cand[i])]
                stable = [normalize(x) for x in ref] == [normalize(x) for x in ref2]
                if rc_a or rc_b or rc_c:
                    failed = True
                if not d:
                    break
                if stable:
                    break          # the reference is stable, so the difference is real
            print('%s: %d / %d answers are the same%s' % (tag, len(sreqs) - len(d), len(sreqs),
                                                         '' if (not d or stable) else ' (the reference itself is unstable)'))
            for i in d[:3]:
                print('  DIFF [%d]' % i)
                print('    old: ' + show(ref[i]))
                print('    new: ' + show(cand[i]))
            if d:
                failed = True
        return 1 if failed else 0
    finally:
        if a.keep:
            print('work folder: ' + work)
        else:
            shutil.rmtree(work, ignore_errors=True)


if __name__ == '__main__':
    sys.exit(main())
