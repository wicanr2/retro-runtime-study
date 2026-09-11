#!/bin/sh
# 從 Borland C++ 2.0 的安裝磁片映像，裝出命令列建置需要的 BORLANDC 目錄。
#
# 用法：tools/bcpp20/install.sh <放 *.IMG 的目錄> <安裝目的目錄>
#   例：tools/bcpp20/install.sh ~/bcpp20-disks ~/bcpp20
#
# 產出：<安裝目的目錄>/{BIN,INCLUDE,LIB,STARTUP,BGI}/ 與 install.log。
#
# 不跑原廠的 INSTALL.EXE：它是互動式全螢幕程式，不適合放進腳本。命令列建置需要的只是
# 把跨磁片的分割封存接起來、把各個 ZIP 解到固定目錄，這支腳本自己做：
#   1. 用 fat12.py 讀出每張磁片的檔案（不需要 mount，也不需要 mtools）；
#   2. 分割封存 NAME.CA1、NAME.CA2… 每一片開頭有 4 bytes 片頭，去掉之後依序接起來
#      就是一個完整的 ZIP，接完用 unzip -t 驗每個成員的 CRC32；
#   3. 依用途解到 BIN、INCLUDE、LIB、STARTUP、BGI。
#
# 磁片映像請用你自己合法取得的 Borland C++ 2.0。本腳本與它所在的 repo 不含任何 Borland 檔案。
set -eu

if [ -z "${IN_CONTAINER:-}" ]; then
    imgs=$(cd "${1:?用法：install.sh <IMG 目錄> <安裝目錄>}" && pwd)
    mkdir -p "${2:?用法：install.sh <IMG 目錄> <安裝目錄>}"
    out=$(cd "$2" && pwd)
    here=$(cd "$(dirname "$0")" && pwd)
    ls "$imgs"/*.[Ii][Mm][Gg] >/dev/null 2>&1 || { echo "$imgs 裡沒有 *.IMG" >&2; exit 1; }
    docker build -q -t retro-runtime-study-tools:1 "$here" >/dev/null
    exec timeout 600 docker run --rm --network none \
        --user "$(id -u):$(id -g)" --memory 1g --cpus 2 --pids-limit 128 \
        --log-opt max-size=10m --log-opt max-file=3 -e IN_CONTAINER=1 -e TZ=UTC \
        -v "$imgs":/imgs:ro -v "$out":/out -v "$here":/tools:ro \
        retro-runtime-study-tools:1 sh /tools/install.sh
fi

T=$(mktemp -d)
log=/out/install.log
: > "$log"
rm -rf /out/BIN /out/INCLUDE /out/LIB /out/STARTUP /out/BGI
mkdir -p /out/BIN /out/INCLUDE /out/LIB /out/STARTUP /out/BGI

# 1. 讀出每張磁片
for img in /imgs/*.[Ii][Mm][Gg]; do
    python /tools/fat12.py "$img" "$T/disks/$(basename "$img")" > /dev/null
done

# 2. 接合分割封存
for first in "$T"/disks/*/*.CA1; do
    [ -e "$first" ] || continue
    stem=$(basename "$first" .CA1)
    : > "$T/$stem.ZIP"
    n=1
    while p=$(ls "$T"/disks/*/"$stem.CA$n" 2>/dev/null) && [ -n "$p" ]; do
        tail -c +5 "$p" >> "$T/$stem.ZIP"
        n=$((n + 1))
    done
    unzip -tq "$T/$stem.ZIP" >> "$log" 2>&1 || { echo "$stem：接合後 CRC 檢查失敗" >&2; exit 1; }
    echo "接合 $stem（$((n - 1)) 片）" >> "$log"
done

# 3. 依用途解開；同名檔先解的留下，衝突記進 install.log
unpack() {  # $1 目的地子目錄，其餘為 ZIP 主檔名
    dst=$1; shift
    for stem in "$@"; do
        z=$(ls "$T/$stem.ZIP" "$T"/disks/*/"$stem.ZIP" 2>/dev/null | head -1)
        [ -n "$z" ] || { echo "沒有 $stem" >> "$log"; continue; }
        for m in $(unzip -Z1 "$z"); do
            case "$m" in */) continue ;; esac
            if [ -e "/out/$dst/$m" ]; then
                [ "$(cksum < "/out/$dst/$m")" = "$(unzip -p "$z" "$m" | cksum)" ] \
                    || echo "同名但內容不同，未採用 $stem 的 $dst/$m" >> "$log"
            else
                unzip -q -o "$z" "$m" -d "/out/$dst"
            fi
        done
    done
}
unpack BIN CMDLINE BCC TASM BIN
unpack INCLUDE INCLUDE
unpack LIB SLIB CLIB MLIB LLIB HLIB XLIB WINLIB
unpack STARTUP STARTUP
unpack BGI BGI
rm -rf "$T"

for f in BIN/BCC.EXE BIN/TASM.EXE BIN/TLINK.EXE LIB/CS.LIB LIB/GRAPHICS.LIB BGI/EGAVGA.BGI; do
    [ -e "/out/$f" ] || { echo "安裝不完整：缺 $f（見 install.log）" >&2; exit 1; }
done
echo "安裝完成：$(find /out -type f | wc -l) 個檔案"
