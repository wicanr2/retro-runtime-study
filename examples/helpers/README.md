# 編譯器 helper 測試程式

讓 Borland C++ 2.0 產生各種編譯器 helper 呼叫（`LXMUL@`、`LDIV@`、`LXLSH@`、`PADD@`、`SCOPY@`、`SPUSH@`、`OVERFLOW@`、`FTOL@` 等），
並把邊界值的計算結果寫到 `OUT.TXT`。說明見[編譯器 helper：程式裡看不到的函式呼叫](../../docs/10-borland-crtl/compiler-helpers.md)。

## 需要

- 你自己的 Borland C++ 2.0，先用 `tools/bcpp20/install.sh` 裝好（本 repo 不含任何 Borland 檔案）
- Docker
- dosgolem 的 `bcc20-toolchain` 分支

## 執行

```sh
cd examples/helpers
export BCPP=~/bcpp20 DOSGOLEM=~/dosgolem
./run.sh            # small 模型、8086 模式
./run.sh h 186      # huge 模型、186 模式
```

`run.sh` 以 `-m<模型> -N` 編譯，在 dosgolem 執行，印出 `OUT.TXT` 並與 `expected.txt` 比對。
small、large、huge 三個模型與 8086、186 兩種模式的輸出都與 `expected.txt` 相同。

想看 helper 的呼叫端，用 `tools/bcpp20/bcc.sh . BCC.EXE -c -m<模型> -N HELPERS.C` 只編譯，再把 `out/helpers.obj` 交給反組譯工具。

## 輸出的每一行

| 行首 | 內容 |
|---|---|
| `mul` | 123456 × −789；0x7FFFFFFF × 3 |
| `div` | −7 / 2；7 / −2；0x80000000 / −1 |
| `mod` | −7 % 2；7 % −2；0x80000000 % −1 |
| `udiv` | 0xFFFFFFFF / 7；0xFFFFFFFF % 0x10001（無號） |
| `shl` | 1 << 31；0x12345678 << 32、40、48 |
| `shr` | −256 >> 4；−1 >> 40；0x12345678 >> 48（有號） |
| `ushr` | 0x80000000 >> 31；0x12345678 >> 48（無號） |
| `ftol` | −1.5、2.999、3.0e9、−3.0e9 轉 `long` |
| `big` | 37 bytes 結構的回傳與以值傳遞 |
| `huge` | `1234:0008` 加 0x10000；兩者相減；`p < p+1` |
| `hinc` | huge 指標 `++` |
