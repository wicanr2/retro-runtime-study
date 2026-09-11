"""讀 FAT12 軟碟映像：列出檔案，或把檔案原封不動解到指定目錄。

用法：
    python fat12.py IMG                 # 列清單（類型、路徑、大小、DOS 時間戳）
    python fat12.py IMG OUTDIR          # 同時解出檔案

只做唯讀解析，不依賴 mtools。
"""
import calendar
import os
import struct
import sys


def walk(img, outdir=None):
    bps, spc, rsv, nfat, nroot, _tot, _media, spf = struct.unpack_from('<HBHBHHBH', img, 11)
    root_off = (rsv + nfat * spf) * bps
    data_off = root_off + nroot * 32
    fat = img[rsv * bps:(rsv + spf) * bps]
    cluster_bytes = spc * bps

    def next_cluster(c):
        v = struct.unpack_from('<H', fat, c * 3 // 2)[0]
        return v >> 4 if c & 1 else v & 0xFFF

    def read_chain(c, size=None):
        parts = []
        while 2 <= c < 0xFF8:
            off = data_off + (c - 2) * cluster_bytes
            parts.append(img[off:off + cluster_bytes])
            c = next_cluster(c)
        data = b''.join(parts)
        return data if size is None else data[:size]

    out = []

    def entries(buf, prefix):
        for i in range(0, len(buf), 32):
            e = buf[i:i + 32]
            if len(e) < 32 or e[0] == 0:
                break
            if e[0] == 0xE5 or e[11] == 0x0F:
                continue
            name = e[:8].decode('cp437').rstrip()
            ext = e[8:11].decode('cp437').rstrip()
            if name in ('.', '..'):
                continue
            attr = e[11]
            t, d, first, size = struct.unpack_from('<HHHI', e, 22)
            stamp = '%04d-%02d-%02d %02d:%02d:%02d' % (
                1980 + (d >> 9), (d >> 5) & 15, d & 31, t >> 11, (t >> 5) & 63, (t & 31) * 2)
            path = prefix + '/' + name + ('.' + ext if ext else '')
            if attr & 0x08:
                out.append(('VOL', name + ext, 0, stamp))
            elif attr & 0x10:
                out.append(('DIR', path, 0, stamp))
                entries(read_chain(first), path)
            else:
                out.append(('F', path, size, stamp))
                if outdir:
                    dst = os.path.join(outdir, path.lstrip('/'))
                    os.makedirs(os.path.dirname(dst), exist_ok=True)
                    with open(dst, 'wb') as fh:
                        fh.write(read_chain(first, size))
                    # DOS 時間戳沒有時區；比照容器內 unzip（TZ=UTC）一律當 UTC，manifest 才能互相比較
                    ts = calendar.timegm((1980 + (d >> 9), (d >> 5) & 15, d & 31,
                                          t >> 11, (t >> 5) & 63, (t & 31) * 2))
                    os.utime(dst, (ts, ts))

    entries(img[root_off:data_off], '')
    return out


def main():
    if len(sys.argv) not in (2, 3):
        sys.exit(__doc__)
    with open(sys.argv[1], 'rb') as fh:
        img = fh.read()
    for kind, path, size, stamp in walk(img, sys.argv[2] if len(sys.argv) == 3 else None):
        print('%-4s %-40s %8d %s' % (kind, path, size, stamp))


if __name__ == '__main__':
    main()
