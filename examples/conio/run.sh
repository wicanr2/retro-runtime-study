#!/bin/sh
# 在 dosgolem 裡用 Borland C++ 2.0 編譯 CONIO.C 並執行，印出 OUT.TXT。
#
#   BCPP=~/bcpp20 DOSGOLEM=~/dosgolem ./run.sh [模型 s|c|m|l|h] [BCC 選項…]
#   例：./run.sh l -O -G -Z
#
# 產物：
#   out/conio.asm   BCC 以 -S 輸出的組語（編譯器對 CONIO.C 產生的指令，含原始碼行註解）
#   out/conio.obj   不經組譯器、BCC 直接產生的目的檔（反組譯看位元組用這份）
#   out/run/scratch/OUT.TXT   執行結果，與 expected.txt（near 資料）或 expected-far.txt（far 資料）比對
set -eu
here=$(cd "$(dirname "$0")" && pwd)
model=${1:-s}; [ $# -gt 0 ] && shift
BCPP=${BCPP:?請設 BCPP＝tools/bcpp20/install.sh 裝出的目錄}
DOSGOLEM=${DOSGOLEM:?請設 DOSGOLEM＝dosgolem 的 checkout（bcc20-toolchain 分支）}
bcc=$here/../../tools/bcpp20/bcc.sh

rm -rf "$here/out"
"$bcc" "$here" BCC.EXE -m$model -S "$@" CONIO.C > "$here/build.log"
mv "$here/out/conio.asm" "$here/conio.asm.tmp"
"$bcc" "$here" BCC.EXE -m$model "$@" CONIO.C >> "$here/build.log"
mv "$here/conio.asm.tmp" "$here/out/conio.asm"
test -f "$here/out/conio.exe" || { cat "$here/build.log"; echo "沒有產出 out/conio.exe" >&2; exit 1; }

run=$here/out/run
mkdir -p "$run/root" "$run/scratch"
cp "$here/out/conio.exe" "$run/root/CONIO.EXE"
timeout 10m docker run --rm --network none -u "$(id -u):$(id -g)" \
    --memory 1g --cpus 1 --pids-limit 64 --log-opt max-size=10m --log-opt max-file=3 \
    -v "$run/root":/root:ro -v "$run/scratch":/out -v "$DOSGOLEM/workplace/dosrun":/dosrun:ro \
    retro-runtime-study-tools:1 /dosrun -prog /root/CONIO.EXE -root /root -scratch /out \
    -cpu 186 -steps 200000000 > "$run/report.txt"
out=$run/scratch/OUT.TXT
cat "$out"
# conio 的行為與資料指標寬度無關，五個模型輸出相同
want=$here/expected.txt
# dosgolem 跑完會 dump 文字畫面，那是這支程式最直接的證據，一起比對
sed -n '/文字畫面：/,$p' "$run/report.txt" > "$run/screen.txt"
rc=0
cmp -s "$out" "$want" || { echo "與 $(basename "$want") 不同" >&2; rc=1; }
cmp -s "$run/screen.txt" "$here/expected-screen.txt" || { echo "文字畫面與 expected-screen.txt 不同" >&2; rc=1; }
[ $rc -eq 0 ] && echo "輸出與文字畫面都與預期相同"
exit $rc
