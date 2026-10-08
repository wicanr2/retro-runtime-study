#!/bin/sh
# 在 dosgolem 裡用 Borland C++ 2.0 編譯 STRLEN.C 並執行，印出 OUT.TXT。
#
#   BCPP=~/bcpp20 DOSGOLEM=~/dosgolem ./run.sh [模型 s|c|m|l|h]
#
# 產物：
#   out/strlen.exe  編出的執行檔
#   out/run/scratch/OUT.TXT   執行結果，與 expected.txt 比對
set -eu
here=$(cd "$(dirname "$0")" && pwd)
model=${1:-s}; [ $# -gt 0 ] && shift
BCPP=${BCPP:?請設 BCPP＝tools/bcpp20/install.sh 裝出的目錄}
DOSGOLEM=${DOSGOLEM:?請設 DOSGOLEM＝dosgolem 的 checkout（bcc20-toolchain 分支）}
bcc=$here/../../tools/bcpp20/bcc.sh

rm -rf "$here/out"
"$bcc" "$here" BCC.EXE -m$model "$@" STRLEN.C > "$here/build.log"
test -f "$here/out/strlen.exe" || { cat "$here/build.log"; echo "沒有產出 out/strlen.exe" >&2; exit 1; }

run=$here/out/run
mkdir -p "$run/root" "$run/scratch"
cp "$here/out/strlen.exe" "$run/root/STRLEN.EXE"
timeout 10m docker run --rm --network none -u "$(id -u):$(id -g)" \
    --memory 1g --cpus 1 --pids-limit 64 --log-opt max-size=10m --log-opt max-file=3 \
    -v "$run/root":/root:ro -v "$run/scratch":/out -v "$DOSGOLEM/workplace/dosrun":/dosrun:ro \
    retro-runtime-study-tools:1 /dosrun -prog /root/STRLEN.EXE -root /root -scratch /out \
    -cpu 186 -steps 200000000 > "$run/report.txt"
out=$run/scratch/OUT.TXT
cat "$out"
if cmp -s "$out" "$here/expected.txt"; then echo "與 expected.txt 相同"; else echo "與 expected.txt 不同" >&2; exit 1; fi
