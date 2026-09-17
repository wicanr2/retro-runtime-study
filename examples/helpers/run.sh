#!/bin/sh
# 在 dosgolem 裡用 Borland C++ 2.0 編譯 HELPERS.C 並執行，印出 OUT.TXT。
#
#   BCPP=~/bcpp20 DOSGOLEM=~/dosgolem ./run.sh [模型 s|c|m|l|h] [CPU 8086|186]
#
# 預設 small、8086。-N 打開堆疊檢查，讓函式開頭出現 OVERFLOW@ 的呼叫。
# 結果與 expected.txt 比對；五個模型、兩種 CPU 模式的預期輸出相同。
set -eu
here=$(cd "$(dirname "$0")" && pwd)
model=${1:-s}; cpu=${2:-8086}
BCPP=${BCPP:?請設 BCPP＝tools/bcpp20/install.sh 裝出的目錄}
DOSGOLEM=${DOSGOLEM:?請設 DOSGOLEM＝dosgolem 的 checkout（bcc20-toolchain 分支）}

rm -rf "$here/out"
"$here/../../tools/bcpp20/bcc.sh" "$here" BCC.EXE -m$model -N HELPERS.C > "$here/build.log"
test -f "$here/out/helpers.exe" || { cat "$here/build.log"; echo "沒有產出 out/helpers.exe" >&2; exit 1; }

run=$here/out/run
mkdir -p "$run/root" "$run/scratch"
cp "$here/out/helpers.exe" "$run/root/HELPERS.EXE"
timeout 10m docker run --rm --network none -u "$(id -u):$(id -g)" \
    --memory 1g --cpus 1 --pids-limit 64 --log-opt max-size=10m --log-opt max-file=3 \
    -v "$run/root":/root:ro -v "$run/scratch":/out -v "$DOSGOLEM/workplace/dosrun":/dosrun:ro \
    retro-runtime-study-tools:1 /dosrun -prog /root/HELPERS.EXE -root /root -scratch /out \
    -cpu "$cpu" -steps 200000000 > "$run/report.txt"
out=$(ls "$run/scratch"/* | head -1)
cat "$out"
if cmp -s "$out" "$here/expected.txt"; then echo "與 expected.txt 相同"; else echo "與 expected.txt 不同" >&2; exit 1; fi
