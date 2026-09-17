#!/bin/sh
# 在 dosgolem 裡用 Borland C++ 2.0 編譯 CODEGEN.C：產出組語清單、目的檔與執行結果。
#
#   BCPP=~/bcpp20 DOSGOLEM=~/dosgolem ./run.sh [模型 s|c|m|l|h] [BCC 選項…]
#   例：./run.sh l -O -G -Z
#
# 產物：
#   out/codegen.asm   BCC 以 -S 輸出的組語（編譯器對 CODEGEN.C 產生的指令，含原始碼行註解）
#   out/codegen.obj   不經組譯器、BCC 直接產生的目的檔（反組譯看位元組用這份）
#   out/run/OUT.TXT   執行結果，與 expected.txt 比對
set -eu
here=$(cd "$(dirname "$0")" && pwd)
model=${1:-s}; [ $# -gt 0 ] && shift
BCPP=${BCPP:?請設 BCPP＝tools/bcpp20/install.sh 裝出的目錄}
DOSGOLEM=${DOSGOLEM:?請設 DOSGOLEM＝dosgolem 的 checkout（bcc20-toolchain 分支）}
bcc=$here/../../tools/bcpp20/bcc.sh

rm -rf "$here/out"
"$bcc" "$here" BCC.EXE -m$model -S "$@" CODEGEN.C > "$here/build.log"
mv "$here/out/codegen.asm" "$here/codegen.asm.tmp"
"$bcc" "$here" BCC.EXE -m$model "$@" CODEGEN.C >> "$here/build.log"
mv "$here/codegen.asm.tmp" "$here/out/codegen.asm"
test -f "$here/out/codegen.exe" || { cat "$here/build.log"; echo "沒有產出 out/codegen.exe" >&2; exit 1; }

run=$here/out/run
mkdir -p "$run/root" "$run/scratch"
cp "$here/out/codegen.exe" "$run/root/CODEGEN.EXE"
timeout 10m docker run --rm --network none -u "$(id -u):$(id -g)" \
    --memory 1g --cpus 1 --pids-limit 64 --log-opt max-size=10m --log-opt max-file=3 \
    -v "$run/root":/root:ro -v "$run/scratch":/out -v "$DOSGOLEM/workplace/dosrun":/dosrun:ro \
    retro-runtime-study-tools:1 /dosrun -prog /root/CODEGEN.EXE -root /root -scratch /out \
    -cpu 186 -steps 200000000 > "$run/report.txt"
out=$(ls "$run/scratch"/* | head -1)
cat "$out"
if cmp -s "$out" "$here/expected.txt"; then echo "與 expected.txt 相同"; else echo "與 expected.txt 不同" >&2; exit 1; fi
