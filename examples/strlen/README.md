# strlen 邊界行為測試程式

`STRLEN.C` 把 BC++ 2.0 的 `strlen`／`_fstrlen` 邊界行為跑出來：一般、空字串、
未對齊、far NULL 衛兵、64K 無結束符。結果寫到 `OUT.TXT`。
說明見 [strlen 的兩種長相](../../docs/20-msvc-crt/strlen-anatomy.md)。

## 需要

- 你自己的 Borland C++ 2.0，先用 `tools/bcpp20/install.sh` 裝好（本 repo 不含任何 Borland 檔案）
- Docker
- dosgolem 的 `bcc20-toolchain` 分支

## 執行

```sh
cd examples/strlen
export BCPP=~/bcpp20 DOSGOLEM=~/dosgolem
./run.sh          # small 模型
./run.sh l        # large 模型
```

`run.sh` 編譯、在 dosgolem 執行，並和 `expected.txt` 比對。
Microsoft 側的行為（far 飽和回 65535）在此工具鏈跑不出來，
見文章的證據節；這裡只驗 Borland 側。

## 輸出的每一行

| 標籤 | 測的是 |
|---|---|
| `empty`、`short`、`long` | 空字串、`"hello"`、300 字元字串的長度 |
| `unalign` | 從奇位址起掃（`buf+1`），排除對齊捷徑的可能 |
| `refmatch` | 照行為規格重寫的參照實作與原廠 `strlen` 是否一致 |
| `fshort` | far 版一般字串 |
| `fnull` | far 版空指標：Borland 回 0（NULL 衛兵） |
| `sat` | `allocmem` 直配整段 64K 全填無 NUL：回 65534 |
| `edge` | 上段第 65534 號位元組補 0：回 65534 |

注意：Borland 的 `allocmem` 成功回 `-1`（失敗才回最大可用塊大小），
與 MSVC 的 `_dos_allocmem`（成功回 0）慣例相反。
