#!/bin/sh
# 在 dosgolem 裡執行 Borland C++ 2.0 的命令列工具（BCC、TASM、TLINK、TLIB…）。
#
# 用法：tools/bcpp20/bcc.sh <工作目錄> <程式> [參數…]
#   例：tools/bcpp20/bcc.sh examples/tetris BCC.EXE -ms -O TETRIS.C GRAPHICS.LIB
#
# 環境變數：
#   BCPP      tools/bcpp20/install.sh 裝出的目錄（必填）
#   DOSGOLEM  dosgolem 的 checkout，要是 bcc20-toolchain 分支（必填）
#
# 做的事：
#   1. 把 BCPP 的 BIN、INCLUDE、LIB、BGI 與工作目錄最上層的檔案攤平成一個 DOS 根目錄
#      （<工作目錄>/.dosroot，跑完刪除）。dosgolem 以檔名的最後一段找檔，所以目錄結構不需要保留；
#   2. 在容器裡用 dosgolem 的 cmd/run 執行，寫出的檔案落在 <工作目錄>/out/；
#   3. 印出 dosgolem 的報告（結束碼、主控台輸出、開過與找不到的檔）。
#
# CPU 用 186：BCC 啟動時偵測 CPU，dosgolem 預設的 386 模式會讓它走到還沒實作的 32 位元指令。
set -eu

work=$(cd "${1:?用法：bcc.sh <工作目錄> <程式> [參數…]}" && pwd); shift
prog=${1:?用法：bcc.sh <工作目錄> <程式> [參數…]}; shift
BCPP=${BCPP:?請設 BCPP＝install.sh 裝出的目錄}
DOSGOLEM=${DOSGOLEM:?請設 DOSGOLEM＝dosgolem 的 checkout（bcc20-toolchain 分支）}
here=$(cd "$(dirname "$0")" && pwd)
test -e "$BCPP/BIN/BCC.EXE" || { echo "$BCPP 不像 BC++ 2.0 的安裝目錄（缺 BIN/BCC.EXE）" >&2; exit 1; }
test -x "$DOSGOLEM/tools/go.sh" || { echo "找不到 dosgolem：$DOSGOLEM" >&2; exit 1; }
test -d "$DOSGOLEM/cmd/webplay" || { echo "$DOSGOLEM 不是 bcc20-toolchain 分支（缺 cmd/webplay；BCC 需要的修正也在那個分支）" >&2; exit 1; }

if [ ! -x "$DOSGOLEM/workplace/dosrun" ] || [ "${REBUILD:-}" = 1 ]; then
    (cd "$DOSGOLEM" && CGO_ENABLED=0 tools/go.sh build -o workplace/dosrun ./cmd/run)
fi
docker image inspect retro-runtime-study-tools:1 >/dev/null 2>&1 || \
    docker build -q -t retro-runtime-study-tools:1 "$here" >/dev/null

root=$work/.dosroot
rm -rf "$root"; mkdir -p "$root" "$work/out"
cp "$BCPP"/BIN/* "$BCPP"/INCLUDE/* "$BCPP"/LIB/* "$BCPP"/BGI/* "$root"/
find "$work" -maxdepth 1 -type f -exec cp {} "$root"/ \;
chmod -R u+w "$root"

# 攤平的根目錄含 Borland 檔案，跑完就刪，不留在工作目錄裡
trap 'rm -rf "$root"' EXIT
timeout 30m docker run --rm --network none -u "$(id -u):$(id -g)" \
    --memory 1g --cpus 2 --pids-limit 64 --log-opt max-size=10m --log-opt max-file=3 \
    -v "$root":/root:ro -v "$work/out":/out -v "$DOSGOLEM/workplace/dosrun":/dosrun:ro \
    retro-runtime-study-tools:1 /dosrun -prog "/root/$prog" -root /root -scratch /out \
    -cpu 186 -args "$*" -steps 2000000000
