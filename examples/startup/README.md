# 啟動與結束順序測試程式

用 `#pragma startup`、`#pragma exit` 與 `atexit` 註冊幾個函式，每個函式執行時在一個字串後面加一個字元，
最後由優先序 64 的結束函式把字串寫到 `ORDER.TXT`。說明見[啟動與結束鏈](../../docs/10-borland-crtl/startup-and-exit.md)。

## 需要

- 你自己的 Borland C++ 2.0，先用 `tools/bcpp20/install.sh` 裝好（本 repo 不含任何 Borland 檔案）
- Docker
- dosgolem 的 `bcc20-toolchain` 分支

## 執行

```sh
cd examples/startup
export BCPP=~/bcpp20 DOSGOLEM=~/dosgolem
./run.sh        # small 模型
./run.sh h      # huge 模型
```

五個記憶體模型的輸出都是 `cbaM21BAC`，與 `expected.txt` 相同。

## 字元的意思

| 字元 | 函式 | 註冊方式 | 優先序 |
|---|---|---|---|
| `a` | `s100a` | `#pragma startup`，先宣告 | 100 |
| `b` | `s100b` | `#pragma startup`，後宣告 | 100 |
| `c` | `s70` | `#pragma startup` | 70 |
| `M` | `main` | — | — |
| `1` | `ax1` | `atexit`，先註冊 | — |
| `2` | `ax2` | `atexit`，後註冊 | — |
| `A` | `e100a` | `#pragma exit`，先宣告 | 100 |
| `B` | `e100b` | `#pragma exit`，後宣告 | 100 |
| `C` | `e70` | `#pragma exit` | 70 |
| （寫檔） | `dump` | `#pragma exit` | 64 |
