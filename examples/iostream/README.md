# iostream 測試程式

三支程式驗 Borland C++ 2.0 的 iostream（`iostream.h`／`fstream.h`）。
說明見 [iostream](../../docs/10-borland-crtl/iostream.md)。

## 需要

- 你自己的 Borland C++ 2.0，先用 `tools/bcpp20/install.sh` 裝好（本 repo 不含任何 Borland 檔案）
- Docker
- dosgolem 的 `bcc20-toolchain` 分支

## 執行

```sh
cd examples/iostream
export BCPP=~/bcpp20 DOSGOLEM=~/dosgolem
./run.sh        # small 模型
./run.sh h      # huge 模型
```

五個記憶體模型——small（s）、compact（c）、medium（m）、large（l）、huge（h）——
輸出全部相同。

## 程式與驗的行為

| 程式 | 輸出 | 驗什麼 |
|---|---|---|
| `FMT.CPP` | `FMT.TXT` | 整數的 dec/oct/hex 與 showbase/showpos/uppercase；`width` 一次性消費；`fill`；left/right/internal；浮點預設（precision 0 視為 6：小數位數＝exp＋6、剝尾零——**0.0001 會印成 0**）、setprecision、fixed/scientific |
| `STD.CPP` | `STD.TXT` | 預設串流的狀態（`cin.tie(&cout)`、`cerr` 的 unitbuf、`cout` 的 unitbuf 與否取決於 isatty(1)——dosgolem 回 0）；空 stdin 的 `cin >> x`（failbit、無 eofbit、值不動）；`sync_with_stdio` 之後 cout 永遠 unitbuf |
| `CTOR.CPP` | `CTOR.TXT` | 全域物件建構順序：`Iostream_init`（優先序 16）先於使用者全域建構（32）——全域物件的建構式裡 `cout` 已可用；解構在結束時反向 |

主控台輸出（`cout`/`printf` 交錯）留在 `out/run-STD/report.txt`，`run.sh` 額外檢查
`sync_with_stdio` 是否觸發 Borland 的空指標哨兵（"Null pointer assignment"）——
**觸發與否取決於記憶體模型的 DGROUP 版面**（s、m 觸發；c、l、h 不觸發），兩種結果都如實回報。

## 已知差異（相對現代 C++）

- `left`／`right`／`internal` **不是操作子**，要用 `setiosflags(ios::left)` 這條路。
- 浮點預設不是「6 位有效數字」：是「整數位＋6 位小數、剝尾零」，所以 0.0001 → `0`。
