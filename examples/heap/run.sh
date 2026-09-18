#!/bin/sh
# 在 dosgolem 裡用 Borland C++ 2.0 編譯 HEAP.C 並執行，印出 OUT.TXT。
#
#   BCPP=~/bcpp20 DOSGOLEM=~/dosgolem ./run.sh [模型 s|c|m|l|h] [BCC 選項…]
#   例：./run.sh l -O -G -Z
#
# 產物：
#   out/heap.asm   BCC 以 -S 輸出的組語（編譯器對 HEAP.C 產生的指令，含原始碼行註解）
#   out/heap.obj   不經組譯器、BCC 直接產生的目的檔（反組譯看位元組用這份）
#   out/run/scratch/OUT.TXT   執行結果，與 expected.txt（near 資料）或 expected-far.txt（far 資料）比對
set -eu
here=$(cd "$(dirname "$0")" && pwd)
model=${1:-s}; [ $# -gt 0 ] && shift
BCPP=${BCPP:?請設 BCPP＝tools/bcpp20/install.sh 裝出的目錄}
DOSGOLEM=${DOSGOLEM:?請設 DOSGOLEM＝dosgolem 的 checkout（bcc20-toolchain 分支）}
bcc=$here/../../tools/bcpp20/bcc.sh

rm -rf "$here/out"
"$bcc" "$here" BCC.EXE -m$model -S "$@" HEAP.C > "$here/build.log"
mv "$here/out/heap.asm" "$here/heap.asm.tmp"
"$bcc" "$here" BCC.EXE -m$model "$@" HEAP.C >> "$here/build.log"
mv "$here/heap.asm.tmp" "$here/out/heap.asm"
test -f "$here/out/heap.exe" || { cat "$here/build.log"; echo "沒有產出 out/heap.exe" >&2; exit 1; }

run=$here/out/run
mkdir -p "$run/root" "$run/scratch"
cp "$here/out/heap.exe" "$run/root/HEAP.EXE"
timeout 10m docker run --rm --network none -u "$(id -u):$(id -g)" \
    --memory 1g --cpus 1 --pids-limit 64 --log-opt max-size=10m --log-opt max-file=3 \
    -v "$run/root":/root:ro -v "$run/scratch":/out -v "$DOSGOLEM/workplace/dosrun":/dosrun:ro \
    retro-runtime-study-tools:1 /dosrun -prog /root/HEAP.EXE -root /root -scratch /out \
    -cpu 186 -steps 200000000 > "$run/report.txt"
out=$run/scratch/OUT.TXT
cat "$out"
# sizeof(FILE) 隨資料指標寬度而不同：near 資料（small、medium）16 bytes，far 資料（compact、large、huge）20 bytes
case $model in s|m) want=$here/expected.txt ;; *) want=$here/expected-far.txt ;; esac
if cmp -s "$out" "$want"; then echo "與 $(basename "$want") 相同"; else echo "與 $(basename "$want") 不同" >&2; exit 1; fi
