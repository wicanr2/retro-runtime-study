# M4 Watcom：從 16 位元到 DOS extender

## 目標

沿著 Watcom C 的版本演進，整理 1988–1993 年間 Watcom 編譯器的 runtime、產生碼與執行環境：

| 版本 | 年份 | 執行環境 | 這一版要回答的問題 |
|---|---|---|---|
| Watcom C 6.5 | 1988 | 16 位元 DOS | Watcom 在 16 位元時代的呼叫慣例、runtime helper、啟動碼；和 M1 的 Borland 有什麼不同 |
| Watcom C/386 7.0 | 1989 | 32 位元，Phar Lap 386\|DOS-Extender 與 OS/386 | 第一個 32 位元版本怎麼產生碼、怎麼依附別家的 DOS extender、輸出什麼執行檔格式 |
| Watcom C 8.0/386 | 1990 | 32 位元 | 7.0 之後改了什麼（收藏說明提到加入保護模式版的編譯器 `WCC386P`） |
| Watcom C/386 9.01、C/C++ 9.5 | 1992–1993 | 32 位元，DOS/4GW、DPMI、LE 格式 | Doom 年代的工具鏈：extender 怎麼交棒、DPMI 服務、LE／LX 執行檔 |

重點放在 1990 年以前的 6.5 與 7.0：它們是後來 32 位元 DOS 遊戲工具鏈的起點，也是目前最少被整理的部分。
DPMI 與 LE 格式 1990 年以後才出現，在演進軸的後段處理。

做到讀者拿到一支 Watcom 編譯的程式時，能判斷大約是哪一代工具鏈、認出 runtime 與啟動碼，
並知道它依附的是哪一種 DOS extender 介面。

## 來源

| 代號（預定） | 內容 | 取得方式 | 用途 |
|---|---|---|---|
| `watcom-6.5` | `watcom-6.5/Watcom C.ver.6.5.English.zip` | archive.org 收藏 [watcom-c-cpp-compilers-collection](https://archive.org/details/watcom-c-cpp-compilers-collection) | 16 位元編譯器、函式庫、文件 |
| `watcom-7.0` | `watcom-7.0/Watcom C 386.ver.7.0.English.zip` | 同上 | 第一代 32 位元編譯器 |
| `watcom-8.0` | `watcom-8.0/CNW386-1.ZIP`～`CNW386-6.ZIP` | 同上 | 1990 年的 C/386 |
| `watcom-9.01` | `watcom-9.01/floppies/Disk01.img`～`Disk06.img`（未打補丁的基礎版） | 同上 | DOS/4GW 年代 |
| `watcom-9.5` | `watcom-9.5/discmaster/Sybase - Watcom C++ 9.5b.zip` | 同上 | Doom（1993）年代的工具鏈候選 |
| `open-watcom-v2@<commit>` | [open-watcom-v2](https://github.com/open-watcom/open-watcom-v2) 固定 commit 的部分目錄 | `git` sparse checkout | 解讀輔助：啟動碼、runtime、`dpmi.h`、LE／LX 結構、pragma 文件 |
| `dpmi-0.9`、`dpmi-1.0` | DPMI 規格書 | 待找可信原文，記錄出處與雜湊 | DPMI 介面的權威定義（只用於演進軸後段） |
| `dmx` | 本 repo 已有的 DMX 3.x 原始碼 | 已在私有工作區 | 9.x 年代函式庫與 extender 互動的實例 |

收藏本身附了 `SHA256SUMS`、`SOURCES.md`（每個檔案的上游來源、下載日期、判定版本的依據）與 `cross-verification.md`。
6.5 與 7.0 的版本依據是封存內的 `README.1ST`／`READ.ME` 自述與檔案日期（6.5 為 1988-05-31；7.0 的說明檔 1989-08-02、`WCC386.EXE` 1989-11-08），
但上游是第三方站點（old-dos.ru），**下載後要自己從檔案內容再確認一次版本**。

open-watcom-v2 裡預計要讀的位置（2026-09-17 查 `master` 的檔案樹確認存在）：

| 主題 | 位置 |
|---|---|
| 啟動碼（16 位元、32 位元、DOS extender 版） | `bld/clib/startup/a/cstrt086.asm`、`cstrt386.asm`、`cstrtx32.asm` 等 |
| DPMI 主機偵測與介面 | `bld/clib/startup/c/dpmihost.c`、`bld/watcom/h/dpmi.h`、`docs/doc/gml/dpmiinfo.gml` |
| LE／LX 格式結構 | `bld/watcom/h/exeflat.h`；傾印工具 `bld/exedump/` |
| 呼叫慣例與 pragma | `docs/doc/cmn/pragma.gml`、`docs/doc/cmn/386arch.gml` |
| C runtime | `bld/clib/` |

open-watcom-v2 是 2000 年代以後持續修改的版本，和 1988–1993 年的出貨函式庫差了十幾年。
它只用來幫助解讀；任何關於當年版本的結論，都要以該版本的出貨檔案（反組譯、函式庫模組、文件）為證據。

## 已決定的事項

2026-09-17 由使用者決定：

| 事項 | 決定 | 執行時要做的事 |
|---|---|---|
| Watcom 6.5～9.5 的出貨檔 | 可用於研究；只放私有工作區，記錄下載來源與 SHA-256，不進公開 repo。公開文章只寫機制與行為 | 私有 `CLAUDE.md` 的授權邊界表加上 Watcom 各版 |
| 位元組簽章 | 本階段不做 | 公開 repo `CLAUDE.md` 的簽章規則維持「只做 Borland 與 Microsoft」 |
| open-watcom-v2 原始碼 | 公開文章**可以引用片段**，每段都標明授權（Sybase Open Watcom Public License 1.0）並連到上游 GitHub 固定 commit 的檔案與行號 | 見下方「引用 open-watcom-v2 的規則」 |
| 私有工作區 | 沿用 `~/cht/borland/`，新增 `originals/watcom/`、`toolchains/watcom/`、`vendor/open-watcom-v2/` | 私有 `CLAUDE.md` 的目錄表與 `tools/extract.sh` 同步 |

### 引用 open-watcom-v2 的規則

- **只有 open-watcom-v2 可以引用。** Borland、Microsoft、DMX、DSMI 與 Watcom 6.5～9.5 的出貨檔仍然一行都不放。
- **片段要短、要有必要。** 只引用解釋機制非引用不可的幾行；能用文字或虛擬碼說清楚的就不引用。
- **每段都附出處。** 程式碼區塊下方寫明「open-watcom-v2 `<commit 前 12 碼>`，`<路徑>:<起>-<迄>`，Sybase Open Watcom Public License 1.0」，連到 `https://github.com/open-watcom/open-watcom-v2/blob/<commit>/<路徑>#L<起>-L<迄>`。
- **commit 固定。** 全階段用 `ow2-source-pin` 選定的同一個 commit，連結不會因上游更新而失效。
- **不拿引用當當年版本的證據。** 引用的是 2000 年代以後的原始碼，文章要寫清楚它和 1988–1993 年出貨版的關係與推論等級。

登記 M4 時要一起改：

1. 公開 repo `CLAUDE.md` 的「[HARD] 不放原文」：加上 open-watcom-v2 的例外與上面的引用格式。
2. 外洩閘門：`vendor/open-watcom-v2/` 不放進 `leak_check.py` 的比對來源，或在閘門加上「帶 open-watcom-v2 出處註記的區塊可放行」的規則；
   同時加一個正反對照測試，確認 Borland、Microsoft 的原文照樣會被擋。
3. `README.md` 的邊界段與 `CONTEXT.md` 的來源代號。

## 工作項目

先登記：私有與公開 repo 的 `worklist.json` 建立下列條目，開 GitHub milestone「M4 Watcom：從 16 位元到 DOS extender」與對應 issue；
並完成上一節「登記 M4 時要一起改」的三項。

### A. 取得與盤點（私有）

| worklist id | 內容 | 完成條件 |
|---|---|---|
| `watcom-download` | 在容器內從收藏下載 6.5、7.0、8.0（六個 ZIP）、9.01（六個磁片映像）、9.5b ZIP，連同 `SHA256SUMS`、`SOURCES.md`、`cross-verification.md`；逐檔比對雜湊 | `originals/watcom/` 下的檔案雜湊全部與 `SHA256SUMS` 相符；來歷筆記記下下載日期與收藏版本 |
| `watcom-extract` | 解出封存與磁片映像（沿用 `fat12.py`，必要時補 7z／RAR 工具進映像並鎖版）；每一版產生 manifest（路徑、大小、SHA-256、封存內時間戳） | 每一版有 manifest；可從 `originals/` 重建 |
| `watcom-inventory` | 每一版盤點：編譯器與工具（`WCC`、`WCC386`、`WLINK` 或其前身、函式庫管理工具）、函式庫與模型分份、啟動碼、標頭、說明檔、附帶的 extender；自述版本字串 | 各版盤點表；列出版與版之間新增、移除的檔案 |
| `ow2-source-pin` | 選定 open-watcom-v2 commit，sparse checkout 上表目錄，記錄 commit 與授權檔 | 來歷筆記與 manifest 存在 |
| `dpmi-spec` | 取得 DPMI 0.9 與 1.0 規格書，記錄出處與雜湊；查不到可信原文時記「待查證」並回報 | 來歷筆記存在 |

### B. 研究專題（私有），依演進軸

| worklist id | 內容 | 主要證據 |
|---|---|---|
| `watcom65-runtime` | 6.5 的呼叫慣例（是否已用暫存器傳參）、記憶體模型與函式庫分份、runtime helper 的名稱與參數、啟動碼流程；和 M1 的 Borland 對照 | 6.5 的函式庫模組反組譯、說明檔 |
| `watcom70-386` | 7.0 的 32 位元產生碼與 helper；支援的 extender（Phar Lap 386\|DOS-Extender、OS/386）與它們的系統呼叫介面；輸出的執行檔格式；連結流程 | 7.0 的說明檔、函式庫、工具反組譯 |
| `watcom80-delta` | 8.0 相對 7.0 的變化：保護模式版編譯器、函式庫與啟動碼的差異 | 8.0 與 7.0 的盤點與模組比對 |
| `watcom9-extender` | 9.01／9.5 的 DOS/4GW：stub 如何載入 LE 本體、啟動碼交棒時的環境、DPMI 服務的使用子集 | 9.x 出貨檔、DPMI 規格、open-watcom-v2 輔助 |
| `le-lx-format` | LE 與 LX 的結構：物件表、分頁、fixup、入口；和 7.0 時代格式的差異 | 9.x 工具與執行檔、`exeflat.h` 輔助 |
| `dmx-dpmi-usage` | DMX 如何與 extender 互動：真實模式中斷處理段、DPMI 呼叫、鎖定記憶體；只做原始碼層級 | DMX 原始碼 |
| `re-watcom-tools` | 對每一版的編譯器與連結器做 IDA 匯出，作為辨識 Watcom runtime 與格式解析的實例（沿用 `tools/ida/` 流程） | 各版工具 |

### C. 公開文章（本 repo，`docs/80-watcom/`）

| worklist id | 文章 | 依賴 |
|---|---|---|
| `article-watcom-lineage` | Watcom C 的版本演進：1988–1993 年各版的執行環境、函式庫分份與辨識線索 | A 組、各 B 專題的盤點結論 |
| `article-watcom65-runtime` | Watcom C 6.5：16 位元的呼叫慣例、helper 與啟動碼，和 Borland 的對照 | `watcom65-runtime` |
| `article-watcom386-extenders` | 第一代 32 位元 Watcom：Phar Lap 與 OS/386 時代的產生碼與 extender 介面 | `watcom70-386`、`watcom80-delta` |
| `article-dos4gw-dpmi-le` | DOS/4GW、DPMI 與 LE 執行檔：Doom 年代的 32 位元 DOS 程式怎麼啟動 | `watcom9-extender`、`le-lx-format`、`dmx-dpmi-usage` |

每篇照本 repo `CLAUDE.md` 的文章格式，配 SVG；通過外洩閘門與專家、學生審查（審查 agent 用 `model: sonnet`、唯讀）後推送。

## 建議順序

1. 「登記 M4 時要一起改」的三項 → A 組全部。下載與盤點是後面所有項目的前提。
2. `watcom65-runtime` → `article-watcom65-runtime`（1990 年前的重點，可直接和 M1 的 Borland 對照）。
3. `watcom70-386` → `watcom80-delta` → `article-watcom386-extenders`。
4. `watcom9-extender`、`le-lx-format`、`dmx-dpmi-usage` → `article-dos4gw-dpmi-le`。
5. `article-watcom-lineage` 最後寫，彙整各版結論；`re-watcom-tools` 穿插在 2～4 當驗證樣本。

## 證據紀律

- **每條結論標版本。** 寫明來自 `watcom-6.5`、`watcom-7.0`、`watcom-8.0`、`watcom-9.01`、`watcom-9.5` 哪一版的出貨檔；
  只在 open-watcom-v2 原始碼讀到、沒有在當年版本驗證過的，最高標「強推論」並註明版本落差。
- **版本字串以檔案內容為準。** 收藏的目錄名與 `SOURCES.md` 是二手資訊；封存內的說明檔、版本字串、檔案日期是一手。
- **規格與實作分開。** DPMI 規格說的是介面；DOS/4GW、Phar Lap 等實作的行為另外標明。
- **沒有原始碼的 extender 只寫介面。** Phar Lap、OS/386、DOS/4GW 的本體比照 8087 模擬器處理。
- **實跑證據的限制。** dosgolem 目前沒有保護模式與 DPMI 支援，16 位元的 6.5 可能可以在 dosgolem 執行，先驗證再決定是否用它對拍；
  32 位元版本本階段不以擴充 dosgolem 為目標，需要實跑的結論先列為未知。

## 完成條件

- A、B、C 各項完成，M4 milestone 的 issue 全部關閉。
- 四篇公開文章通過審查修訂並推送；每篇的證據表逐條標出版本。
- 引用 open-watcom-v2 的規則已寫進公開 repo `CLAUDE.md`，外洩閘門的例外有正反對照測試。
- 讀者照 `article-watcom-lineage` 能從一支執行檔的格式、版權字串、啟動碼特徵，判斷它大約來自哪一代 Watcom 工具鏈。

## 不在本階段範圍

- Watcom 10.x、11.x 與 Open Watcom 本身的工具鏈。
- 以 Watcom 編譯器實際重編對拍 32 位元函式庫（需要保護模式執行環境）。
- Phar Lap、OS/386、DOS/4GW 等 extender 本體的反組譯。
- Windows 3.x 上的 `win386`、OS/2 與 QNX 目標。

## 風險與未知

- **6.5 與 7.0 的上游是第三方站點。** 內容若和自述版本不符，以檔案內容為準並記錄差異。
- **封存可能不完整。** 單一 ZIP 的 6.5（約 1.3 MB）與 7.0（約 1.0 MB）不一定含全部磁片內容；盤點時列出缺少的部分。
- **Phar Lap 與 OS/386 的介面文件可能不在封存裡。** 只能從 Watcom 的說明檔與函式庫呼叫端反推時，標「強推論」。
- **DPMI 規格原文的可信來源待找。**
- **Doom 系列實際用的 Watcom 版本**未查證；文章提到遊戲時標出處與等級。
