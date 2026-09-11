"""跑 worklist.json 每一條的 verify，印出仍未完成的項目。

用法：
    python worklist.py [WORKLIST_JSON]     # 預設為 repo 根目錄的 worklist.json
    python worklist.py --self-test         # 正反對照：每種 kind 都要會開口也要會閉嘴

verify 為真 = 這一條仍未完成（見 ~/.claude/rulebook/61）。
kind：
    missing  paths 裡任一路徑不存在 → 未完成（綁「產出檔還沒出現」）
    present  pattern 在 paths 裡找得到 → 未完成（綁自承註解「尚未…」）
    absent   pattern 在 paths 裡找不到 → 未完成（綁「某個名稱還沒出現」）
    manual   一律未完成，報表上標 [manual]；note 寫將來該綁什麼訊號
paths 相對於 worklist.json 所在目錄；目錄會遞迴，但不掃 *_test.* 與 test_*。
"""
import json
import os
import re
import sys
import tempfile


def files_under(base, paths):
    for p in paths:
        full = os.path.join(base, p)
        if os.path.isdir(full):
            for root, _d, fs in os.walk(full):
                for f in fs:
                    if not (re.search(r'_test\.\w+$', f) or f.startswith('test_')):
                        yield os.path.join(root, f)
        elif os.path.isfile(full):
            yield full


def grep(base, paths, pattern):
    rx = re.compile(pattern)
    for f in files_under(base, paths):
        try:
            if rx.search(open(f, encoding='utf-8', errors='replace').read()):
                return True
        except OSError:
            pass
    return False


def still_open(base, v):
    kind = v['kind']
    if kind == 'manual':
        return True
    if kind == 'missing':
        return any(not os.path.exists(os.path.join(base, p)) for p in v['paths'])
    if kind == 'present':
        return grep(base, v['paths'], v['pattern'])
    if kind == 'absent':
        return not grep(base, v['paths'], v['pattern'])
    raise ValueError('未知的 verify kind：%s' % kind)


def run(path):
    base = os.path.dirname(os.path.abspath(path))
    data = json.load(open(path, encoding='utf-8'))
    open_items, done = [], []
    for it in data['items']:
        (open_items if still_open(base, it['verify']) else done).append(it)
    for it in open_items:
        tag = '[manual]' if it['verify']['kind'] == 'manual' else '[open]  '
        print('%s %-32s %s' % (tag, it['id'], it['title']))
    for it in done:
        print('[DONE?]  %-32s %s  ← verify 已不成立，確認後從清單移除' % (it['id'], it['title']))
    print('未完成 %d（其中 manual %d）；verify 已不成立 %d' % (
        len(open_items), sum(i['verify']['kind'] == 'manual' for i in open_items), len(done)))


def self_test():
    with tempfile.TemporaryDirectory() as d:
        open(os.path.join(d, 'a.md'), 'w').write('TODO: 尚未對拍\n')
        cases = [
            ({'kind': 'missing', 'paths': ['nope.md']}, True),
            ({'kind': 'missing', 'paths': ['a.md']}, False),
            ({'kind': 'present', 'paths': ['a.md'], 'pattern': '尚未對拍'}, True),
            ({'kind': 'present', 'paths': ['a.md'], 'pattern': '不存在的字'}, False),
            ({'kind': 'absent', 'paths': ['.'], 'pattern': 'oracle-verified'}, True),
            ({'kind': 'absent', 'paths': ['.'], 'pattern': 'TODO'}, False),
            ({'kind': 'manual'}, True),
        ]
        for v, want in cases:
            got = still_open(d, v)
            if got != want:
                sys.exit('self-test 失敗：%r 預期 %s 得到 %s' % (v, want, got))
    print('self-test 通過：%d 個正反案例' % len(cases))


if __name__ == '__main__':
    if sys.argv[1:] == ['--self-test']:
        self_test()
    else:
        run(sys.argv[1] if len(sys.argv) > 1 else
            os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'worklist.json'))
