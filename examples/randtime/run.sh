#!/bin/sh
# 在 dosgolem 裡用 Borland C++ 2.0 編譯並執行四支測試程式，比對輸出。
#
#   BCPP=~/bcpp20 DOSGOLEM=~/dosgolem ./run.sh [模型 s|c|m|l|h]
#
# RANDTEST／TIMETEST／DOTIME 的輸出與 expected-*.txt 比對（輸出與模型無關）。
# MKFEB29 預期不結束（mktime 對閏年 2 月 29 無限迴圈）——用步數上限偵測。
set -eu
here=$(cd "$(dirname "$0")" && pwd)
model=${1:-s}
BCPP=${BCPP:?請設 BCPP＝tools/bcpp20/install.sh 裝出的目錄}
DOSGOLEM=${DOSGOLEM:?請設 DOSGOLEM＝dosgolem 的 checkout（bcc20-toolchain 分支）}
bcc=$here/../../tools/bcpp20/bcc.sh

rm -rf "$here/out"
for prog in RANDTEST TIMETEST DOTIME MKFEB29; do
    "$bcc" "$here" BCC.EXE -m$model $prog.C >> "$here/build.log" 2>&1 ||
        { cat "$here/build.log"; echo "編譯 $prog 失敗" >&2; exit 1; }
done

run_one() {
    prog=$1 steps=$2
    run=$here/out/run-$prog
    mkdir -p "$run/root" "$run/scratch"
    cp "$here/out/"$(echo "$prog" | tr 'A-Z' 'a-z')".exe" "$run/root/$prog.EXE"
    timeout 10m docker run --rm --network none -u "$(id -u):$(id -g)" \
        --memory 1g --cpus 1 --pids-limit 64 --log-opt max-size=10m --log-opt max-file=3 \
        -v "$run/root":/root:ro -v "$run/scratch":/out -v "$DOSGOLEM/workplace/dosrun":/dosrun:ro \
        retro-runtime-study-tools:1 /dosrun -prog /root/$prog.EXE -root /root -scratch /out \
        -cpu 186 -steps "$steps" > "$run/report.txt"
}

for prog in RANDTEST TIMETEST DOTIME; do
    run_one "$prog" 200000000
    case $prog in
        RANDTEST) out=RAND.TXT want=$here/expected-rand.txt ;;
        TIMETEST) out=TIME.TXT want=$here/expected-time.txt ;;
        DOTIME)   out=DAY.TXT  want=$here/expected-day.txt ;;
    esac
    got=$here/out/run-$prog/scratch/$out
    cat "$got"
    if cmp -s "$got" "$want"; then
        echo "== $prog 與 $(basename "$want") 相同"
    else
        echo "== $prog 與 $(basename "$want") 不同：" >&2
        diff "$want" "$got" >&2 || true
        exit 1
    fi
done

# MKFEB29：步數跑滿而沒有呼叫結束服務＝無限迴圈重現
run_one MKFEB29 100000000
if grep -q '結束服務' "$here/out/run-MKFEB29/report.txt"; then
    echo "MKFEB29 竟然結束了（bug 沒有重現？）" >&2
    cat "$here/out/run-MKFEB29/scratch/FEB.TXT" >&2 || true
    exit 1
fi
echo "== MKFEB29 未在步數內結束：mktime(1992-02-29) 無限迴圈重現"
cat "$here/out/run-MKFEB29/scratch/FEB.TXT"
