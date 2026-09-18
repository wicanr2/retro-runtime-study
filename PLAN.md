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
| R7 | 檔案 I/O 與緩衝（完成，已過專家與學生審查） | `docs/10-borland-crtl/stdio-file-io.md`、兩張 SVG、`examples/stdio/` |
| R8 | printf／scanf 引擎與浮點的連結開關（完成，已過專家與學生審查） | `docs/10-borland-crtl/printf-engine.md`、兩張 SVG、`examples/printf/` |
| R9 | near heap 與 far heap（完成，已過專家與學生審查） | `docs/10-borland-crtl/heap.md`、兩張 SVG、`examples/heap/` |
| R10 | conio 與文字畫面（完成，已過專家與學生審查） | `docs/10-borland-crtl/conio-screen.md`、一張 SVG、`examples/conio/` |
| R11 | VC++ 1.0 的函式庫怎麼切份（完成，已過專家與學生審查） | `docs/20-msvc-crt/library-combination.md`、一張 SVG |
| R12 | DSMI 的三套介面（審查前第一版） | `docs/40-dsmi/interface-split.md`、一張 SVG |
| R13 以後 | 依 worklist 排序 | DMX 版本差異、亂數與時間、iostream |
| T1 | 工具鏈教學（完成，已過專家與學生審查） | `docs/70-toolchain/bcc20-on-dosgolem.md`、`tools/bcpp20/`、`examples/tetris/`、流程圖與遊玩截圖 |

**M1（Borland runtime 分析）兩批都完成**：第一批是記憶體模型、編譯器 helper、啟動與結束鏈（R2–R4），
第二批是遊戲真正會呼叫的四組——檔案 I/O、`printf` 家族、heap、conio（R7–R10）。
四組都有研究筆記、公開文章、五個記憶體模型可重跑的範例，兩個 repo 的 worklist 與 issue 都清空。
選配的亂數與時間（`rand`、`time` 家族）沒做，留在 R11 以後。

**M3 的簽章規劃有一條硬限制**：VC++ 1.0 的原廠說明檔自己預告，用這份原始碼重建出來的 `.LIB`
不保證與出貨版逐位元組相同（出貨庫是 pre-production 工具建的）。所以就算日後取得工具鏈，
**位元組簽章必須取自真正的出貨 `.LIB`，不能用重建產物**。

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
