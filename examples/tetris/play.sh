#!/bin/sh
# 在瀏覽器裡玩 out/tetris.exe：dosgolem 的 webplay 在容器裡跑，埠只開在本機。
#
#   BCPP=~/bcpp20 DOSGOLEM=~/dosgolem ./play.sh      → 開 http://127.0.0.1:8086/
#
# Ctrl-C 結束（容器帶 --rm，結束就刪除）。PORT 可以改埠號。
set -eu
here=$(cd "$(dirname "$0")" && pwd)
BCPP=${BCPP:?請設 BCPP＝tools/bcpp20/install.sh 裝出的目錄}
DOSGOLEM=${DOSGOLEM:?請設 DOSGOLEM＝dosgolem 的 checkout（bcc20-toolchain 分支）}
PORT=${PORT:-8086}
test -f "$here/out/tetris.exe" || { echo "先跑 ./build.sh" >&2; exit 1; }
test -d "$DOSGOLEM/cmd/webplay" || { echo "$DOSGOLEM 不是 bcc20-toolchain 分支（缺 cmd/webplay）" >&2; exit 1; }

if [ ! -x "$DOSGOLEM/workplace/webplay" ] || [ "${REBUILD:-}" = 1 ]; then
    (cd "$DOSGOLEM" && CGO_ENABLED=0 tools/go.sh build -o workplace/webplay ./cmd/webplay)
fi

# 執行時的目錄：程式本身＋BGI 的 VGA 驅動（initgraph 會在目前目錄找 EGAVGA.BGI）
play=$here/out/play
rm -rf "$play"; mkdir -p "$play"
cp "$here/out/tetris.exe" "$play/TETRIS.EXE"
cp "$BCPP/BGI/EGAVGA.BGI" "$play/"

echo "開 http://127.0.0.1:$PORT/ 玩；Ctrl-C 結束"
tty=""; [ -t 0 ] && tty=-it
exec docker run --rm $tty --name "tetris-webplay-$PORT" -p "127.0.0.1:$PORT:8086" \
    -u "$(id -u):$(id -g)" --memory 1g --cpus 2 --pids-limit 64 \
    --log-opt max-size=10m --log-opt max-file=3 \
    -v "$play":/game:ro -v "$DOSGOLEM/workplace/webplay":/webplay:ro \
    retro-runtime-study-tools:1 /webplay -prog /game/TETRIS.EXE -root /game -cpu 186
