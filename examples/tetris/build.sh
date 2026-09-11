#!/bin/sh
# 在 dosgolem 裡用 Borland C++ 2.0 編譯 TETRIS.C（small model，連結 BGI 繪圖庫）。
#
#   BCPP=~/bcpp20 DOSGOLEM=~/dosgolem ./build.sh
#
# 產出 out/tetris.exe。BCC 會自己用 EXEC 叫 TLINK，TLINK 從 CS.LIB、GRAPHICS.LIB 連結。
set -eu
here=$(cd "$(dirname "$0")" && pwd)
rm -f "$here/out/tetris.exe" "$here/out/tetris.obj"
"$here/../../tools/bcpp20/bcc.sh" "$here" BCC.EXE -ms -O TETRIS.C GRAPHICS.LIB
test -f "$here/out/tetris.exe" || { echo "沒有產出 out/tetris.exe" >&2; exit 1; }
echo "完成：$here/out/tetris.exe"
