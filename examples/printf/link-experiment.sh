#!/bin/sh
# 浮點轉換的連結實驗：四支程式，看 _CVTSEG／_SCNSEG 兩個向量段各由誰填。
#
#   BCPP=~/bcpp20 DOSGOLEM=~/dosgolem ./link-experiment.sh [模型 s|c|m|l|h]
#
# 每支印一行：名稱 _CVTSEG長度 _SCNSEG長度 訊息在不在執行檔
# 最後把兩支會走到浮點轉換入口、而真正的轉換模組又沒連進來的程式實際跑一遍。
# 結果與 expected-linkage.txt 比對。
set -eu
here=$(cd "$(dirname "$0")" && pwd)
model=${1:-s}
BCPP=${BCPP:?請設 BCPP＝tools/bcpp20/install.sh 裝出的目錄}
DOSGOLEM=${DOSGOLEM:?請設 DOSGOLEM＝dosgolem 的 checkout（bcc20-toolchain 分支）}
bcc=$here/../../tools/bcpp20/bcc.sh
work=$here/out-linkage
rm -rf "$work"
mkdir -p "$work"
report=$work/report.txt
: > "$report"

seglen() {   # $1=map 檔 $2=段名；map 的第三欄是長度，形如 0000CH
    hex=$(awk -v seg="$2" 'index($0, seg) { print substr($3, 1, length($3)-1); exit }' "$1")
    printf '%d\n' "0x$hex"
}

for src in NOCVT FLOATONLY NOFLOAT CVTFAKE PRINTF; do
    rm -rf "$here/out"
    "$bcc" "$here" BCC.EXE -m$model -M "$src.C" > "$work/$src.build.log" 2>&1
    exe=$here/out/$(echo "$src" | tr 'A-Z' 'a-z').exe
    map=$here/out/$(echo "$src" | tr 'A-Z' 'a-z').map
    cvt=$(seglen "$map" "_CVTSEG")
    scn=$(seglen "$map" "_SCNSEG")
    if strings -a "$exe" | grep -qi "formats not linked"; then msg=有; else msg=無; fi
    echo "$src $cvt $scn $msg" >> "$report"
    cp "$exe" "$work/$src.EXE"
    cp "$map" "$work/$src.map"
done

# 實跑兩支：一支只有備用模組填向量，一支兩個模組都沒進來
mkdir -p "$work/root" "$work/scratch"
for name in CVTFAKE NOCVT; do
    cp "$work/$name.EXE" "$work/root/$name.EXE"
    rm -f "$work/scratch/OUT.TXT"
    timeout 10m docker run --rm --network none -u "$(id -u):$(id -g)" \
        --memory 1g --cpus 1 --pids-limit 64 --log-opt max-size=10m --log-opt max-file=3 \
        -v "$work/root":/root:ro -v "$work/scratch":/out -v "$DOSGOLEM/workplace/dosrun":/dosrun:ro \
        retro-runtime-study-tools:1 /dosrun -prog "/root/$name.EXE" -root /root -scratch /out \
        -cpu 186 -steps 200000000 > "$work/$name.run.txt" 2>&1 || true
    code=$(awk '/回傳碼/ { gsub(/[^0-9]/, "", $NF); print $NF; exit }' "$work/$name.run.txt")
    if grep -q "formats not linked" "$work/$name.run.txt"; then printed=有; else printed=無; fi
    echo "run.$name $code $printed" >> "$report"
done

cat "$report"
if cmp -s "$report" "$here/expected-linkage.txt"; then
    echo "與 expected-linkage.txt 相同"
else
    echo "與 expected-linkage.txt 不同" >&2
    exit 1
fi
