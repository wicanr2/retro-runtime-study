#!/bin/sh
# 在 dosgolem 裡用 Borland C++ 2.0 編譯並執行三支 iostream 測試程式，比對輸出。
#
#   BCPP=~/bcpp20 DOSGOLEM=~/dosgolem ./run.sh [模型 s|c|m|l|h]
#
# FMT.TXT／STD.TXT／CTOR.TXT 與 expected-*.txt 比對（輸出與記憶體模型無關）。
# 主控台輸出（cout/cerr/printf 交錯）留在 out/run-*/report.txt 供目視，不比對。
set -eu
here=$(cd "$(dirname "$0")" && pwd)
model=${1:-s}
BCPP=${BCPP:?請設 BCPP＝tools/bcpp20/install.sh 裝出的目錄}
DOSGOLEM=${DOSGOLEM:?請設 DOSGOLEM＝dosgolem 的 checkout（bcc20-toolchain 分支）}
bcc=$here/../../tools/bcpp20/bcc.sh

rm -rf "$here/out"
: > "$here/build.log"
for prog in FMT STD CTOR; do
    "$bcc" "$here" BCC.EXE -m$model $prog.CPP >> "$here/build.log" 2>&1 ||
        { cat "$here/build.log"; echo "編譯 $prog 失敗" >&2; exit 1; }
done

for prog in FMT STD CTOR; do
    run=$here/out/run-$prog
    mkdir -p "$run/root" "$run/scratch"
    cp "$here/out/"$(echo "$prog" | tr 'A-Z' 'a-z')".exe" "$run/root/$prog.EXE"
    timeout 10m docker run --rm --network none -u "$(id -u):$(id -g)" \
        --memory 1g --cpus 1 --pids-limit 64 --log-opt max-size=10m --log-opt max-file=3 \
        -v "$run/root":/root:ro -v "$run/scratch":/out -v "$DOSGOLEM/workplace/dosrun":/dosrun:ro \
        retro-runtime-study-tools:1 /dosrun -prog /root/$prog.EXE -root /root -scratch /out \
        -cpu 186 -steps 200000000 > "$run/report.txt"
    got=$run/scratch/$prog.TXT
    cat "$got"
    if [ "$prog" = STD ]; then
        if grep -q "Null pointer assignment" "$run/report.txt"; then
            echo "== sync_with_stdio 觸發空指標哨兵（此模型觸發）"
        else
            echo "== sync_with_stdio 未觸發空指標哨兵（此模型版面不觸發）"
        fi
    fi
    if cmp -s "$got" "$here/expected-$prog.txt"; then
        echo "== $prog 與 expected-$prog.txt 相同"
    else
        echo "== $prog 與 expected-$prog.txt 不同：" >&2
        diff "$here/expected-$prog.txt" "$got" >&2 || true
        exit 1
    fi
done
