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
| R12 | DSMI 的三套介面（完成，已過專家與學生審查） | `docs/40-dsmi/interface-split.md`、一張 SVG |
| R13 | DMX 的五份封存（完成，已過專家與學生審查） | `docs/30-dmx/version-history.md`、一張 SVG |
| R14 | Watcom C 6.5 的 runtime（完成，已過專家與學生審查） | `docs/80-watcom/watcom65-runtime.md`、一張 SVG |
| R15 | 第一代 32 位元 Watcom 與 DOS extender（完成，已過專家與學生審查） | `docs/80-watcom/watcom386-extenders.md`、一張 SVG |
| R16 | DPMI 與 DOS/4GW（完成，已過專家與學生審查） | `docs/80-watcom/dos4gw-startup.md`、一張 SVG |
| R17 | Watcom 版本演進總表（完成，已過專家與學生審查） | `docs/80-watcom/watcom-lineage.md`、一張 SVG |
| R18 | WPK 解碼器解開，補上 9.01 的內容缺口（完成） | 私有 `tools/wpk/`；`dos4gw-startup.md` 與 `watcom-lineage.md` 各補一輪、兩張 SVG 更新 |
| R19 以後 | 依 worklist 排序 | Easy OMF-386 解析器、Watcom 8.0 與 9.5、亂數與時間、iostream |
| T1 | 工具鏈教學（完成，已過專家與學生審查） | `docs/70-toolchain/bcc20-on-dosgolem.md`、`tools/bcpp20/`、`examples/tetris/`、流程圖與遊玩截圖 |

**M1（Borland runtime 分析）兩批都完成**：第一批是記憶體模型、編譯器 helper、啟動與結束鏈（R2–R4），
第二批是遊戲真正會呼叫的四組——檔案 I/O、`printf` 家族、heap、conio（R7–R10）。
四組都有研究筆記、公開文章、五個記憶體模型可重跑的範例，兩個 repo 的 worklist 與 issue 都清空。
選配的亂數與時間（`rand`、`time` 家族）沒做，留在 R11 以後。

**M3（其他函式庫）第一批完成**：VC++ 1.0 的庫怎麼切份（R11）、DSMI 的三套介面（R12）、
DMX 的五份封存（R13），三篇都過了專家與學生審查，公開與私有各三個 issue 已關。
第二批兩條也有結論：DMX 的權利狀態查不到任何釋出聲明，以「已窮盡可取得的公開來源」結案；
VC++ 1.0 的工具鏈沒有合法的公開下載來源，剩 MSDN 訂閱封存區與二手實體媒體兩條路，
私有 #14 保持開啟。因此**對拍與位元組簽章這一輪沒做，也不該做**。

**M4 第一批完成**：Watcom C 6.5 與 C/386 7.0 的素材取得、盤點與一篇 6.5 的 runtime 文章（R14）。
過程中修正了 goal 的一個前提（Watcom 附了啟動碼的原始碼，只有函式庫本體沒有），
並把 open-watcom-v2 放在私有工作區的 `refs/` 而不是 `vendor/`——閘門因此一行都不用改。
6.5 在 dosgolem 下「能讀不能編」：`WCC` 前端、`WLIB`、`WDISASM` 都跑得動，碼產生階段卡在 E142，
另開 worklist 追。這份封存的 `CLIBS.LIB` 與 `CLIBC.LIB` 損壞（原廠工具也拒讀），
模型比較只用 medium／large／huge。

**M4 第二批完成**：7.0 的 runtime 研究與一篇文章（R15）。與第一批相反的是，
7.0 的 32 位元工具在 dosgolem 下跑得動（編譯器本身是 16 位元 MZ 程式，只是產生 32 位元碼），
所以這篇有實測撐著。未解的是出貨庫讀不動（Phar Lap 的 Easy OMF-386 變體），
以及沒有連結器就沒有可執行檔。

**M4 第三批完成**：9.01 取得盤點、DMX 與 extender 的互動、一篇 DPMI 的文章（R16）。
這一輪有兩件事沒照計畫走：收藏附的「解好的內容」壓縮檔雜湊對不上（已刪除不用），
以及 WPK 解碼器沒寫完（卡在 wpack 依賴 Watcom CLIB 那個特定 qsort 的等值排序順序）。
所以 9.01 的封包內容當時還讀不到，文章的證據因此不平均——**這一點在 R18 已經補上**。
審查抓到三處實質錯誤：DMA 的 1 MB 因果寫反、實模式中斷常式只用在 IRQ 8–15
（Sound Blaster 常用的 IRQ 5／7 沒有這層備援）、「複製到低位記憶體」其實有逐行證據不該標推論。

**M4 第四批完成**：版本演進總表（R17），M4 的目標問題（拿到執行檔怎麼判斷是哪一代）有了答案。
WPK 解碼器再試一輪仍未成功——照原廠的 qsort 完整移植後結果沒變，所以「排序順序是卡點」
這個假設被證偽，剩下三個可疑處寫進筆記。審查抓到的最大問題是結構性的：
原本的「三個問題」分不出 7.0 與 9.01，補上第四題（執行檔有沒有 DOS/4GW 的 stub）才收得起來。

**M4 第五批**：WPK 解碼器沒解開，但卡點從「整個解碼器都不對」收斂成單一一點
（同碼長內符號的排列順序）。過程中修掉 open-watcom `wqsort.c` 在 64 位元下的越界 bug。

**M4 第六批完成（R18）**：反組譯 `INSTALL.EXE` 的 NE 段，找出 1992 年那個 qsort
（單向掃描版，與 open-watcom 後來換上的三路分割版不同），WPK 全部解開——
181 個封包、630 個成員都通過封包自帶的 CRC，兩份獨立實作輸出逐位元組相同。
解包接進 `tools/extract.sh`，解出的內容因此也進了外洩閘門的比對來源。
兩篇既有文章的 11 處「讀不到」清掉，勘誤段新增 7 條被推翻的舊斷言——
**其中兩條是「相鄰版本外推」造成的**（`__StartTime`、`AH=4Ah` 都不是單調演進），
一條是第五批自己推出來的假修正。

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
| 總表把 9.01 的 `__StartTime` 標成「延續前代推論」（7.0 有，所以 9.01 也有） | **9.01 又拿掉了** | 解開封包後讀 `cstart3r.asm`：7.0 有 4 處、9.01 0 處 |
| 總表把 9.01 的 `AH=4Ah`（向 DOS 歸還記憶體）標成「讀不到」 | **只在 Intel Code Builder 那條路徑上做**；6.5 一律做、7.0 完全不做 | 同上，`cstart3r.asm` 的 extender 分支 |
| DOS/4GW 那篇說「9.01 自己也走 DPMI」（強推論） | **9.01 的啟動碼自己不呼叫 `int 31h`**，它用 `int 21h` 偵測 extender；DPMI 是上層程式（如 DMX）在用 | 同上 |
| 兩篇都把 9.01 的執行環境判斷寫成編譯期的事 | 9.01 改成**執行時偵測**，結果記在 `__Extender`（六個值）；7.0 才是編譯期 `ifdef` 二選一 | 同上 |
| 總表說「堆疊慣例的一般函式是裸名」只有啟動碼內部符號的外推 | 有實物佐證：`clib3s.lib` 裡 `strlen_`／`printf_`／`fopen_` 一個都沒有，`clib3r.lib` 三個都有 | 解開後的兩份函式庫（字串層級；那是 Easy OMF-386，解析器讀不了符號表）|
| 私有筆記與盤點工具把 `.WPK` 檔名長度的最高位元讀成「這個成員沒有壓縮」 | 那是 `NO_SHANNON_CODE`：不用 Shannon-Fano 編碼，**仍然是 LZSS 壓縮** | open-watcom 的 `wpack.h` 與實際解碼結果 |
| 第五批推出「1992 版的碼表條目數不加 1」（當時第一個符號因此從 `.` 變成 `/`，看起來是對的） | **假修正，已收回**：那是排序錯誤造成的假象，換上正確的 qsort 之後 `+1` 才對 | 630 個成員全部通過封包自帶的 CRC |
