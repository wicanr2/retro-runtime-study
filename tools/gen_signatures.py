"""從你自己的 OMF 函式庫（.LIB）產生短位元組簽章，並拿簽章掃執行檔。

用法：
    python gen_signatures.py make CS.LIB [CC.LIB …] -o sigs.json [--source borland-crtl-2.0] [--modules SCROLL,…]
    python gen_signatures.py scan sigs.json PROGRAM.EXE [-o matches.json] [--map PROGRAM.MAP]
    python gen_signatures.py --self-test

make：
    每個模組裡、放在 CODE 類別段落的公開符號視為一個函式，範圍到同段下一個公開符號或段落結尾。
    取函式開頭最多 64 bytes，目的檔裡有修正（fixup）的位元組遮成 `..`——那些位置要等連結時才填，
    每支程式都不一樣。固定位元組少於 --min-fixed（預設 12）的函式太短、太容易撞，不收，另列在 skipped。
    同一個樣式對到多個名稱（別名、或前 64 bytes 相同的函式）時合併成一筆，names 全列。
    --modules 只收指定的模組（模組名取 THEADR，去掉路徑與副檔名、轉大寫）。
scan：
    在整個檔案裡找每個樣式，回報檔案位移、名稱、固定位元組數。同一位移有多個樣式命中時，
    固定位元組最多的排第一，其餘列在 alternatives。
    --map 給 TLINK 的 map 檔時，逐筆核對命中位置的公開符號名稱，算出正確、錯誤與漏掉的數目。
    single_lib_hits 數每個函式庫「只出現在它裡面」的命中數；BC++ 2.0 的 CS／CC／CM／CL／CH 最多的那個就是記憶體模型。

本工具不含任何函式庫內容；簽章檔只有遮罩過的短樣式與符號名。
"""
import argparse
import hashlib
import json
import os
import re
import struct
import sys
import tempfile

MAX_BYTES = 64
LOC_SIZE = {0: 1, 1: 2, 2: 2, 3: 4, 4: 1, 5: 2, 9: 4, 11: 6, 13: 4}


def index(body, i):
    if body[i] & 0x80:
        return (body[i] & 0x7F) << 8 | body[i + 1], i + 2
    return body[i], i + 1


def records(data):
    i = 0
    while i + 3 <= len(data):
        t, n = data[i], data[i + 1] | data[i + 2] << 8
        yield t, data[i + 3:i + 3 + n - 1]
        i += 3 + n
        if t in (0x8A, 0x8B):
            return


def lib_modules(path):
    d = open(path, 'rb').read()
    if not d or d[0] != 0xF0:
        raise ValueError(f'{path} 不是 OMF 函式庫')
    page = (d[1] | d[2] << 8) + 3
    i = page
    while i < len(d) and d[i] != 0xF1:
        start = i
        while True:
            t, n = d[i], d[i + 1] | d[i + 2] << 8
            i += 3 + n
            if t in (0x8A, 0x8B):
                break
        yield d[start:i]
        i = (i + page - 1) // page * page


def module_functions(obj):
    """回傳 (模組名, [(公開名, 位元組, 遮罩)])；遮罩為 True 表示該位元組有修正。"""
    name, lnames, segs, pubs = '', [], [], []
    image, mask, last = {}, {}, None
    for t, b in records(obj):
        if t == 0x80:
            name = b[1:1 + b[0]].decode('latin-1').replace('\\', '/').split('/')[-1]
            name = os.path.splitext(name)[0].upper()
        elif t == 0x96:
            j = 0
            while j < len(b):
                lnames.append(b[j + 1:j + 1 + b[j]].decode('latin-1'))
                j += 1 + b[j]
        elif t in (0x98, 0x99):
            attr, j = b[0], 1
            if attr >> 5 == 0:
                j += 3
            if t == 0x98:
                length, j = b[j] | b[j + 1] << 8, j + 2
            else:
                length, j = struct.unpack_from('<I', b, j)[0], j + 4
            if attr & 2:
                length = 0x10000 if t == 0x98 else length
            seg_name, j = index(b, j)
            cls_name, j = index(b, j)
            segs.append((lnames[seg_name - 1], lnames[cls_name - 1], length))
            image[len(segs)] = bytearray(length)
            mask[len(segs)] = bytearray(length)
        elif t in (0x90, 0x91):
            grp, j = index(b, 0)
            seg, j = index(b, j)
            if seg == 0:
                j += 2
            while j < len(b):
                n = b[j]
                pname = b[j + 1:j + 1 + n].decode('latin-1')
                j += 1 + n
                if t == 0x90:
                    off, j = b[j] | b[j + 1] << 8, j + 2
                else:
                    off, j = struct.unpack_from('<I', b, j)[0], j + 4
                _type, j = index(b, j)
                if seg:
                    pubs.append((seg, off, pname))
        elif t in (0xA0, 0xA1):
            seg, j = index(b, 0)
            if t == 0xA0:
                off, j = b[j] | b[j + 1] << 8, j + 2
            else:
                off, j = struct.unpack_from('<I', b, j)[0], j + 4
            payload = b[j:]
            image[seg][off:off + len(payload)] = payload
            last = (seg, off)
        elif t in (0xA2, 0xA3):
            last = None      # LIDATA 的修正位置不追蹤；程式碼段幾乎不用 LIDATA
        elif t in (0x9C, 0x9D) and last is not None:
            j, big = 0, t == 0x9D
            while j < len(b):
                h = b[j]
                if h & 0x80 == 0:                       # THREAD
                    d_bit, method = h & 0x40, (h >> 2) & 7
                    j += 1
                    if (d_bit == 0 and method < 4) or (d_bit and method < 3):
                        _, j = index(b, j)
                    continue
                locat = (h << 8) | b[j + 1]
                loc, where = (locat >> 10) & 0xF, locat & 0x3FF
                j += 2
                fix = b[j]
                j += 1
                if not fix & 0x80 and ((fix >> 4) & 7) < 3:
                    _, j = index(b, j)
                if not fix & 0x08:
                    _, j = index(b, j)
                if not fix & 0x04:
                    j += 4 if big else 2
                seg, base = last
                for k in range(LOC_SIZE.get(loc, 2)):
                    if base + where + k < len(mask[seg]):
                        mask[seg][base + where + k] = 1
    funcs = []
    for seg_index in {p[0] for p in pubs}:
        seg_name, cls, length = segs[seg_index - 1]
        if not cls.upper().endswith('CODE'):
            continue
        here = sorted((off, pname) for s, off, pname in pubs if s == seg_index)
        for k, (off, pname) in enumerate(here):
            end = length
            for off2, _ in here[k + 1:]:
                if off2 > off:
                    end = off2
                    break
            end = min(end, off + MAX_BYTES)
            funcs.append((pname, bytes(image[seg_index][off:end]), bytes(mask[seg_index][off:end]), end - off))
    return name, funcs


def pattern_text(data, m):
    return ' '.join('..' if mk else '%02X' % v for v, mk in zip(data, m))


def make(libs, source, min_fixed, modules=None):
    merged, skipped, inputs = {}, [], []
    for lib in libs:
        raw = open(lib, 'rb').read()
        label = os.path.splitext(os.path.basename(lib))[0].upper()
        inputs.append({'lib': os.path.basename(lib), 'sha256': hashlib.sha256(raw).hexdigest()})
        for obj in lib_modules(lib):
            module, funcs = module_functions(obj)
            if modules and module not in modules:
                continue
            for pname, data, m, _n in funcs:
                fixed = len(data) - sum(m)
                if fixed < min_fixed:
                    skipped.append({'name': pname, 'module': module, 'lib': label, 'fixed': fixed})
                    continue
                key = pattern_text(data, m)
                e = merged.setdefault(key, {'names': [], 'modules': [], 'libs': [], 'fixed': fixed, 'pattern': key})
                for field, value in (('names', pname), ('modules', module), ('libs', label)):
                    if value not in e[field]:
                        e[field].append(value)
    sigs = sorted(merged.values(), key=lambda e: (e['names'][0], e['pattern']))
    return {'schema': 'retro-runtime-study.signatures.v1', 'source': source, 'max_bytes': MAX_BYTES,
            'min_fixed': min_fixed, 'inputs': inputs, 'count': len(sigs), 'signatures': sigs,
            'skipped': sorted(skipped, key=lambda s: (s['name'], s['lib']))}


def compile_pattern(p):
    return re.compile(b''.join(b'.' if t == '..' else re.escape(bytes([int(t, 16)])) for t in p.split()), re.S)


def scan(sigs, data):
    hits = {}
    for s in sigs['signatures']:
        for m in compile_pattern(s['pattern']).finditer(data):
            hits.setdefault(m.start(), []).append(s)
    out = []
    for off in sorted(hits):
        cands = sorted(hits[off], key=lambda s: -s['fixed'])
        best = cands[0]
        out.append({'offset': off, 'names': best['names'], 'fixed': best['fixed'], 'libs': best['libs'],
                    'alternatives': [{'names': c['names'], 'fixed': c['fixed']} for c in cands[1:]]})
    return out


def single_lib_hits(matches):
    votes = {}
    for m in matches:
        if len(m['libs']) == 1:
            votes[m['libs'][0]] = votes.get(m['libs'][0], 0) + 1
    return dict(sorted(votes.items(), key=lambda kv: -kv[1]))


def read_map(path, header_size):
    """TLINK map 檔的「Publics by Value」→ {檔案位移: 名稱}。"""
    pubs, inside = {}, False
    for line in open(path, encoding='latin-1'):
        if 'Publics by Value' in line:
            inside = True
            continue
        if inside:
            m = re.match(r'^\s*([0-9A-F]{4}):([0-9A-F]{4})\s+(?:Abs\s+|Idle\s+)?(\S+)', line)
            if m:
                pubs.setdefault(header_size + int(m.group(1), 16) * 16 + int(m.group(2), 16), []).append(m.group(3))
            elif line.strip() and not line.startswith(' '):
                inside = False
    return pubs


def evaluate(matches, pubs, sigs):
    known = {n for s in sigs['signatures'] for n in s['names']}
    right = wrong = 0
    wrong_list, found = [], set()
    for mt in matches:
        at = pubs.get(mt['offset'], [])
        if set(at) & set(mt['names']):
            right += 1
            found |= set(at) & set(mt['names'])
        else:
            wrong += 1
            wrong_list.append({'offset': mt['offset'], 'names': mt['names'], 'map': at})
    expected = {n for names in pubs.values() for n in names if n in known}
    missed = sorted(expected - found)
    return {'right': right, 'wrong': wrong, 'expected': len(expected), 'missed': missed, 'wrong_list': wrong_list}


def mz_header_size(data):
    return (data[8] | data[9] << 8) * 16 if data[:2] in (b'MZ', b'ZM') else 0


# ---------- self-test ----------

def _rec(t, body):
    body = bytes(body)
    n = len(body) + 1
    raw = bytes([t, n & 0xFF, n >> 8]) + body
    return raw + bytes([(-sum(raw)) & 0xFF])


def _name(s):
    return bytes([len(s)]) + s.encode()


def _module(mod, funcs, fix_at):
    """funcs：[(名稱, 位元組)]，依序放進 _TEXT；fix_at：要加 offset 修正的段內位移清單。"""
    code = b''.join(b for _, b in funcs)
    out = _rec(0x80, _name(mod + '.C'))
    out += _rec(0x96, _name('') + _name('_TEXT') + _name('CODE'))
    out += _rec(0x98, bytes([0x28, len(code) & 0xFF, len(code) >> 8, 2, 3, 1]))
    pub, off = b'\x00\x01', 0
    for fname, fb in funcs:
        pub += _name(fname) + bytes([off & 0xFF, off >> 8, 0])
        off += len(fb)
    out += _rec(0x90, pub)
    out += _rec(0xA0, b'\x01\x00\x00' + code)
    fx = b''
    for where in fix_at:
        locat = 0xC000 | (1 << 10) | where        # M=1、offset、資料記錄內位移
        fx += bytes([locat >> 8, locat & 0xFF, 0x56, 0x01])   # F=frame 從 target、T=segdef 1、P=1（無位移）
    out += _rec(0x9C, fx)
    out += _rec(0x8A, b'\x00')
    return out


def _library(modules):
    page = 16
    body = b''
    for m in modules:
        body += m
        body += b'\x00' * ((-(page + len(body))) % page)
    head = bytes([0xF0, (page - 3) & 0xFF, 0]) + b'\x00' * (page - 3)
    return head + body + b'\xF1\x00\x00'


def self_test():
    fa = bytes.fromhex('55 8B EC B8 34 12 8B 5E 04 03 C3 5D C3'.replace(' ', ''))   # 位移 4–5 放修正
    fb = bytes.fromhex('55 8B EC 33 C0 5D CB'.replace(' ', ''))                       # 固定 7 bytes，太短
    lib = _library([_module('ALPHA', [('_alpha', fa), ('_beta', fb)], [4])])
    fails = []
    with tempfile.TemporaryDirectory() as tmp:
        lp = os.path.join(tmp, 'T.LIB')
        open(lp, 'wb').write(lib)
        sigs = make([lp], 'test', 8)
        pats = {s['names'][0]: s for s in sigs['signatures']}
        if '_alpha' not in pats:
            fails.append('沒有產生 _alpha')
        elif pats['_alpha']['pattern'] != '55 8B EC B8 .. .. 8B 5E 04 03 C3 5D C3':
            fails.append('遮罩或範圍不對：' + pats['_alpha']['pattern'])
        if '_beta' in pats or not any(s['name'] == '_beta' for s in sigs['skipped']):
            fails.append('太短的 _beta 應該列在 skipped')
        body = b'\x90' * 5 + fa[:4] + b'\x78\x56' + fa[6:] + b'\x90' * 5
        got = scan(sigs, body)
        if [m['offset'] for m in got] != [5] or got[0]['names'] != ['_alpha']:
            fails.append(f'正對照應在位移 5 命中 _alpha，得到 {got}')
        broken = bytearray(body)
        broken[5 + 7] ^= 0xFF
        if scan(sigs, bytes(broken)):
            fails.append('改掉一個固定位元組後不該命中')
        ev = evaluate(got, {5: ['_alpha'], 40: ['_other']}, sigs)
        if (ev['right'], ev['wrong'], ev['missed']) != (1, 0, []):
            fails.append(f'map 核對結果不對：{ev}')
        if make([lp], 'test', 8, {'OTHER'})['count'] != 0:
            fails.append('--modules 篩掉的模組不該產生簽章')
        ev2 = evaluate(got, {5: ['_gamma']}, sigs)
        if (ev2['right'], ev2['wrong']) != (0, 1):
            fails.append(f'名稱不符應算錯誤：{ev2}')
    for f in fails:
        print('失敗：' + f)
    print(f'self-test：{7 - len(fails)}／7 通過')
    return 1 if fails else 0


def main():
    if '--self-test' in sys.argv:
        return self_test()
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest='cmd', required=True)
    a = sub.add_parser('make')
    a.add_argument('libs', nargs='+')
    a.add_argument('-o', required=True)
    a.add_argument('--source', default='')
    a.add_argument('--min-fixed', type=int, default=12)
    a.add_argument('--modules', default='')
    b = sub.add_parser('scan')
    b.add_argument('sigs')
    b.add_argument('binary')
    b.add_argument('-o')
    b.add_argument('--map')
    args = ap.parse_args()
    if args.cmd == 'make':
        mods = {m.strip().upper() for m in args.modules.split(',') if m.strip()}
        sigs = make(args.libs, args.source, args.min_fixed, mods or None)
        if mods:
            sigs['modules_filter'] = sorted(mods)
        with open(args.o, 'w', encoding='utf-8') as fh:
            json.dump(sigs, fh, ensure_ascii=False, indent=1)
        print(f'{sigs["count"]} 筆簽章，{len(sigs["skipped"])} 個函式太短不收')
        return 0
    sigs = json.load(open(args.sigs, encoding='utf-8'))
    data = open(args.binary, 'rb').read()
    matches = scan(sigs, data)
    votes = single_lib_hits(matches)
    result = {'binary': os.path.basename(args.binary), 'size': len(data), 'single_lib_hits': votes, 'matches': matches}
    line = f'{len(matches)} 個位置命中'
    if votes:
        line += '；只屬於單一函式庫的命中：' + '、'.join(f'{k} {v}' for k, v in votes.items())
    if args.map:
        ev = evaluate(matches, read_map(args.map, mz_header_size(data)), sigs)
        result['evaluation'] = ev
        line += f'；對 map：正確 {ev["right"]}、錯誤 {ev["wrong"]}、map 裡有簽章卻沒命中 {len(ev["missed"])}'
    if args.o:
        with open(args.o, 'w', encoding='utf-8') as fh:
            json.dump(result, fh, ensure_ascii=False, indent=1)
    print(line)
    return 0


if __name__ == '__main__':
    sys.exit(main())
