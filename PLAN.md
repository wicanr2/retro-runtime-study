# 計畫

未完成項的權威是 `worklist.json`，本檔只放分輪安排與勘誤。

## 分輪

| 輪 | 主題 | 產出 |
|---|---|---|
| R0 | 骨架 | `CLAUDE.md`、`README.md`、`CONTEXT.md`、`worklist.json`、`tools/worklist.py` |
| R1 | 總覽（完成） | `docs/00-overview/era-and-libraries.md`、兩張 SVG |
| R2 | Borland 記憶體模型機制 | 一份原始碼怎麼編出五個 `.LIB` |
| R3 | 編譯器 helper | 長整數乘除、結構複製、堆疊檢查：原始碼、呼叫慣例、執行檔裡的樣子 |
| R4 | 啟動與結束鏈 | Borland 與 MS 的比較 |
| R5 以後 | 依 worklist 排序 | heap、printf 引擎、檔案 I/O、iostream、DMX、DSMI |
| T1 | 工具鏈教學（完成，未經審查） | `docs/70-toolchain/bcc20-on-dosgolem.md`、`tools/bcpp20/`、`examples/tetris/`、流程圖與遊玩截圖 |

每輪收尾：外洩閘門 → 專家與學生審查 → 套修正 → `tools/worklist.py` → commit → push。

## 預定目錄

```
docs/00-overview/
docs/10-borland-crtl/
docs/20-msvc-crt/
docs/30-dmx/
docs/40-dsmi/
docs/50-cross-vendor/      Borland 與 MS 同一功能的做法比較
docs/60-re-fingerprints/   執行檔裡怎麼認出這些函式庫
docs/70-toolchain/         原廠工具鏈怎麼在今天的環境裡執行（對拍的前置）
examples/                  自己寫、用原廠工具鏈編譯的範例程式
signatures/
img/
```

目錄在寫第一篇文章時才建立，不先放空殼。

## 勘誤

| 原斷言 | 更正 | 依據 |
|---|---|---|
| README 把 DMX 標為 1992–1994 | 1993–1994 | 程式碼檔頭版權年份；1992 年的只有範例 WAV，「1992」來自封存名稱 |
| README 把 DSMI 標為 1992–1993 | 1992–1994 | 檔頭版權年份有 8 個檔案標 1994 |
