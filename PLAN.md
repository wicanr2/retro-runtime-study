# 計畫

未完成項的權威是 `worklist.json`，本檔只放分輪安排與勘誤。每條 worklist 對應一個 GitHub issue（`issue` 欄位），
issue 依 milestone 分組：M1 Borland runtime 分析、M2 BCC 反組譯、M3 其他函式庫；各階段的目標、完成條件與依賴見 `docs/goal/`。

## 分輪

| 輪 | 主題 | 產出 |
|---|---|---|
| R0 | 骨架 | `CLAUDE.md`、`README.md`、`CONTEXT.md`、`worklist.json`、`tools/worklist.py` |
| R1 | 總覽（完成） | `docs/00-overview/era-and-libraries.md`、兩張 SVG |
| R2 | Borland 記憶體模型機制（完成，已過專家與學生審查） | `docs/10-borland-crtl/memory-model-macros.md`、兩張 SVG |
| R3 | 編譯器 helper（完成，已過專家與學生審查） | `docs/10-borland-crtl/compiler-helpers.md`、兩張 SVG、`examples/helpers/` |
| R4 | 啟動與結束鏈（完成，已過專家與學生審查；MS 比較待 M3） | `docs/10-borland-crtl/startup-and-exit.md`、兩張 SVG、`examples/startup/` |
| R5 | BCC 2.0 產生碼特徵（完成，已過專家與學生審查） | `docs/60-re-fingerprints/bcc20-codegen.md`、三張 SVG、`examples/codegen/` |
| R6 | 判斷廠牌、版本與記憶體模型（完成，已過專家與學生審查） | `docs/60-re-fingerprints/identify-vendor-and-model.md`、流程圖、`signatures/borland-crtl-2.0/` |
| R7 以後 | 依 worklist 排序 | heap、printf 引擎、檔案 I/O、iostream、DMX、DSMI |
| T1 | 工具鏈教學（完成，已過專家與學生審查） | `docs/70-toolchain/bcc20-on-dosgolem.md`、`tools/bcpp20/`、`examples/tetris/`、流程圖與遊玩截圖 |

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
| 總覽說啟動碼沒有隨原始碼一起提供（強推論） | 啟動碼原始檔與 `BUILD-C0.BAT` 在編譯器套件的 `STARTUP.ZIP`，RTL 的 `BUILD.BAT` 分支就是呼叫它 | 編譯器磁片的 `STARTUP.ZIP`；組出的 20 個目的檔與出貨版相同 |
