"""由 docs/NN-主題/*.md 的 YAML 前置欄位產生 kb-index.json，並檢查欄位。

用法：
    python build_index.py              # 產生 repo 根目錄的 kb-index.json；有錯誤時不寫檔、exit 1
    python build_index.py --check      # 只檢查；kb-index.json 與前置欄位不一致也算錯
    python build_index.py --self-test  # 正反對照

只掃 docs/ 底下名稱是「兩位數字-主題」的目錄（docs/goal/ 這類不算文章）。
檢查：
    必填欄位 id、title、libraries、goals、evidence、triggers、symbols、related 都在
    id 全庫唯一，而且等於「目錄名去掉數字前綴／檔名去掉 .md」
    libraries 是 CONTEXT.md「來源代號」表裡的代號
    goals 只能是 craft、re、oracle；evidence 只能是 已證實、強推論、假說
    triggers 至少一條；related 指到的 id 存在
前置欄位只支援本 repo 用到的 YAML 子集：純量、[a, "b c"] 行內清單、「- 項目」區塊清單。
"""
import json
import os
import re
import sys
import tempfile

FIELDS = ['id', 'title', 'libraries', 'goals', 'evidence', 'triggers', 'symbols', 'related']
GOALS = {'craft', 're', 'oracle'}
EVIDENCE = {'已證實', '強推論', '假說'}
ARTICLE_DIR = re.compile(r'^\d\d-(.+)$')


def unquote(s):
    s = s.strip()
    if len(s) >= 2 and s[0] == s[-1] and s[0] in '"\'':
        return s[1:-1]
    return s


def inline_list(s):
    body, items, cur, quote = s.strip()[1:-1], [], '', None
    for ch in body:
        if quote:
            cur += ch
            if ch == quote:
                quote = None
        elif ch in '"\'':
            cur += ch
            quote = ch
        elif ch == ',':
            items.append(cur)
            cur = ''
        else:
            cur += ch
    if cur.strip():
        items.append(cur)
    return [unquote(i) for i in items if i.strip()]


def front_matter(text):
    lines = text.split('\n')
    if not lines or lines[0].strip() != '---':
        raise ValueError('沒有前置欄位（第一行不是 ---）')
    try:
        end = next(i for i in range(1, len(lines)) if lines[i].strip() == '---')
    except StopIteration:
        raise ValueError('前置欄位沒有結尾的 ---')
    meta, key = {}, None
    for n, line in enumerate(lines[1:end], start=2):
        if not line.strip() or line.lstrip().startswith('#'):
            continue
        m = re.match(r'^\s+-\s+(.*)$', line)
        if m:
            if key is None or not isinstance(meta.get(key), list):
                raise ValueError(f'第 {n} 行：清單項目前面沒有「欄位:」')
            meta[key].append(unquote(m.group(1)))
            continue
        m = re.match(r'^([A-Za-z_][\w-]*):\s*(.*)$', line)
        if not m:
            raise ValueError(f'第 {n} 行看不懂：{line}')
        key, value = m.group(1), m.group(2).strip()
        if value == '':
            meta[key] = []
        elif value.startswith('['):
            if not value.endswith(']'):
                raise ValueError(f'第 {n} 行：行內清單沒有結尾的 ]')
            meta[key] = inline_list(value)
        else:
            meta[key] = unquote(value)
    return meta


def library_codes(root):
    path = os.path.join(root, 'CONTEXT.md')
    codes, inside = set(), False
    for line in open(path, encoding='utf-8'):
        if line.startswith('## '):
            inside = line.strip() == '## 來源代號'
            continue
        m = re.match(r'^\|\s*`([^`]+)`\s*\|', line)
        if inside and m:
            codes.add(m.group(1))
    return codes


def collect(root):
    errors, articles = [], []
    docs = os.path.join(root, 'docs')
    codes = library_codes(root)
    for d in sorted(os.listdir(docs)):
        m = ARTICLE_DIR.match(d)
        if not m or not os.path.isdir(os.path.join(docs, d)):
            continue
        for f in sorted(os.listdir(os.path.join(docs, d))):
            if not f.endswith('.md'):
                continue
            rel = f'docs/{d}/{f}'
            try:
                meta = front_matter(open(os.path.join(docs, d, f), encoding='utf-8').read())
            except ValueError as e:
                errors.append(f'{rel}：{e}')
                continue
            missing = [k for k in FIELDS if k not in meta]
            if missing:
                errors.append(f'{rel}：缺欄位 {", ".join(missing)}')
                continue
            for k in ('libraries', 'goals', 'triggers', 'symbols', 'related'):
                if not isinstance(meta[k], list):
                    errors.append(f'{rel}：{k} 要是清單')
            for k in ('id', 'title', 'evidence'):
                if not isinstance(meta[k], str) or not meta[k]:
                    errors.append(f'{rel}：{k} 要是非空字串')
            if errors and errors[-1].startswith(rel):
                continue
            want = f'{m.group(1)}/{f[:-3]}'
            if meta['id'] != want:
                errors.append(f'{rel}：id 是 {meta["id"]}，照路徑應為 {want}')
            for lib in meta['libraries']:
                if lib not in codes:
                    errors.append(f'{rel}：libraries 的 {lib} 不在 CONTEXT.md 的來源代號裡')
            for g in meta['goals']:
                if g not in GOALS:
                    errors.append(f'{rel}：goals 的 {g} 不是 craft／re／oracle')
            if meta['evidence'] not in EVIDENCE:
                errors.append(f'{rel}：evidence 的 {meta["evidence"]} 不是 已證實／強推論／假說')
            if not meta['triggers']:
                errors.append(f'{rel}：triggers 至少要一條')
            articles.append(dict({k: meta[k] for k in FIELDS}, path=rel))
    seen = {}
    for a in articles:
        if a['id'] in seen:
            errors.append(f'{a["path"]}：id {a["id"]} 與 {seen[a["id"]]} 重複')
        else:
            seen[a['id']] = a['path']
    for a in articles:
        for r in a['related']:
            if r not in seen:
                errors.append(f'{a["path"]}：related 的 {r} 找不到對應文章')
    return articles, errors


def render(articles):
    order = ['id', 'path', 'title', 'libraries', 'goals', 'evidence', 'triggers', 'symbols', 'related']
    data = {'schema': 'retro-runtime-study.kb-index.v1',
            'articles': [{k: a[k] for k in order} for a in sorted(articles, key=lambda a: a['path'])]}
    return json.dumps(data, ensure_ascii=False, indent=1) + '\n'


def run(root, check=False, out=sys.stdout):
    articles, errors = collect(root)
    for e in errors:
        print(e, file=out)
    if errors:
        return 1
    text, path = render(articles), os.path.join(root, 'kb-index.json')
    if check:
        current = open(path, encoding='utf-8').read() if os.path.exists(path) else None
        if current != text:
            print('kb-index.json 與前置欄位不一致，請重跑 build_index.py', file=out)
            return 1
        print(f'{len(articles)} 篇，kb-index.json 是最新的', file=out)
        return 0
    with open(path, 'w', encoding='utf-8') as fh:
        fh.write(text)
    print(f'{len(articles)} 篇，寫出 kb-index.json', file=out)
    return 0


GOOD_A = """---
id: 10-x/a
title: A
libraries: [dmx]
goals: [re]
evidence: 強推論
triggers:
  - 讀者遇到 A
symbols: [foo, "EB 00"]
related: [x/b]
---
# A
"""
GOOD_B = GOOD_A.replace('id: 10-x/a', 'id: x/b').replace('title: A', 'title: B').replace('related: [x/b]', 'related: []')


def self_test():
    class Null:
        def write(self, _s):
            pass

    def case(name, files, want, check=False):
        with tempfile.TemporaryDirectory() as root:
            os.makedirs(os.path.join(root, 'docs'))
            with open(os.path.join(root, 'CONTEXT.md'), 'w', encoding='utf-8') as fh:
                fh.write('# t\n\n## 來源代號\n\n| 代號 | 指的是 |\n|---|---|\n| `dmx` | DMX |\n\n## 術語\n\n| `zzz` | 不是代號 |\n')
            for rel, text in files.items():
                p = os.path.join(root, rel)
                os.makedirs(os.path.dirname(p), exist_ok=True)
                with open(p, 'w', encoding='utf-8') as fh:
                    fh.write(text)
            got = run(root, check=check, out=Null())
            if check is False and got == 0:
                got_check = run(root, check=True, out=Null())
                if got_check != 0:
                    return f'{name}：寫出後 --check 應為 0，得到 {got_check}'
            return None if got == want else f'{name}：預期 exit {want}，得到 {got}'

    a = GOOD_A.replace('id: 10-x/a', 'id: x/a')
    cases = [
        ('正對照', {'docs/10-x/a.md': a, 'docs/10-x/b.md': GOOD_B}, 0),
        ('docs/goal 不列入也不擋', {'docs/10-x/a.md': a, 'docs/10-x/b.md': GOOD_B, 'docs/goal/README.md': '# 沒有前置欄位\n'}, 0),
        ('缺欄位', {'docs/10-x/a.md': a.replace('evidence: 強推論\n', ''), 'docs/10-x/b.md': GOOD_B}, 1),
        ('id 重複', {'docs/10-x/a.md': a, 'docs/10-x/b.md': GOOD_B, 'docs/20-x/b.md': GOOD_B}, 1),
        ('id 與路徑不符', {'docs/10-x/a.md': GOOD_A, 'docs/10-x/b.md': GOOD_B}, 1),
        ('related 斷鏈', {'docs/10-x/a.md': a.replace('[x/b]', '[x/zzz]'), 'docs/10-x/b.md': GOOD_B}, 1),
        ('libraries 不是代號', {'docs/10-x/a.md': a.replace('[dmx]', '[zzz]'), 'docs/10-x/b.md': GOOD_B}, 1),
        ('evidence 不合法', {'docs/10-x/a.md': a.replace('強推論', '已證實（實測）'), 'docs/10-x/b.md': GOOD_B}, 1),
        ('goals 不合法', {'docs/10-x/a.md': a.replace('goals: [re]', 'goals: [fun]'), 'docs/10-x/b.md': GOOD_B}, 1),
        ('triggers 空', {'docs/10-x/a.md': a.replace('triggers:\n  - 讀者遇到 A\n', 'triggers: []\n'), 'docs/10-x/b.md': GOOD_B}, 1),
        ('沒有前置欄位', {'docs/10-x/a.md': '# A\n', 'docs/10-x/b.md': GOOD_B}, 1),
        ('--check 發現過期', {'docs/10-x/a.md': a, 'docs/10-x/b.md': GOOD_B, 'kb-index.json': '{}\n'}, 1, True),
    ]
    fails = [r for r in (case(*c) for c in cases) if r]
    parsed = inline_list('[a, "b, c", \'d\']')
    if parsed != ['a', 'b, c', 'd']:
        fails.append(f'行內清單解析：{parsed}')
    for f in fails:
        print('失敗：' + f)
    print(f'self-test：{len(cases) + 1 - len(fails)}／{len(cases) + 1} 通過')
    return 1 if fails else 0


if __name__ == '__main__':
    here = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    if '--self-test' in sys.argv:
        sys.exit(self_test())
    sys.exit(run(here, check='--check' in sys.argv))
