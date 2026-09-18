# M4 Watcom：從 16 位元到 DOS extender

## 目標

沿著 Watcom C 的版本演進，整理 1988–1993 年間 Watcom 編譯器的 runtime、產生碼與執行環境：

| 版本 | 年份 | 執行環境 | 這一版要回答的問題 |
|---|---|---|---|
| Watcom C 6.5 | 1988 | 16 位元 DOS | Watcom 在 16 位元時代的呼叫慣例、runtime helper、啟動碼；和 M1 的 Borland 有什麼不同 |
| Watcom C/386 7.0 | 1989 | 32 位元，Phar Lap 386\|DOS-Extender 與 OS/386 | 第一個 32 位元版本怎麼產生碼、怎麼依附別家的 DOS extender、輸出什麼執行檔格式 |
| Watcom C 8.0/386 | 1990 | 32 位元 | 7.0 之後改了什麼（收藏說明提到加入保護模式版的編譯器 `WCC386P`） |
| Watcom C/386 9.01、C/C++ 9.5 | 1992–1993 | 32 位元，DOS/4GW、DPMI、LE 格式 | Doom 年代的工具鏈：extender 怎麼交棒、DPMI 服務、LE／LX 執行檔 |

做到讀者拿到一支 Watcom 編譯的程式時，能判斷大約是哪一代工具鏈、認出 runtime 與啟動碼，
並知道它依附的是哪一種 DOS extender 介面。

M3 的 DMX 文章已經證明這條線有現成的落點：那套音效函式庫就是用 `wcc386p`、flat model、
`SYSTEM dos4g` 建的，而它服務的是 1993–1995 年那批 32 位元 DOS 遊戲。
從 Watcom 這一側看，等於把那些遊戲的執行環境補齊。

## 這一輪的範圍

M4 分批做，**本輪只做第一批**：

- **第一批（本輪）**：把素材拿到手並盤點清楚，然後做完 6.5 這一版的 runtime 研究與一篇公開文章。
  6.5 是 16 位元的，可以直接和 M1 的 Borland 逐項對照，是整條線裡最容易確立方法的一版。
- **後續批次（本輪不做）**：7.0／8.0 的第一代 32 位元與 extender、9.x 的 DOS/4GW 與 DPMI、
  LE／LX 格式、版本演進總表。這些等第一批把流程跑順再排。

這一輪與 M1 最大的差別是**沒有現成的函式庫原始碼**。Borland 那邊是「原始碼＋對拍」，
Watcom 出貨包裡只有編譯器、函式庫與文件，沒有 runtime 的原始碼。所以：

- 結論主要來自**反組譯出貨的函式庫模組**與**原廠文件**，不是讀原始碼。
- open-watcom-v2（2000 年代以後的開源版本）只當解讀輔助，不當當年版本的證據。
- 「對拍」在這條線上的意義也不同：能做的是「用 6.5 自己編一支程式，看產生的碼與函式庫模組對不對得上」，
  不是「重編原廠函式庫再比對」。

## 手上有什麼、還沒有什麼

| 項目 | 狀態 |
|---|---|
| Watcom 各版出貨檔 | **還沒有**。要從 archive.org 的收藏下載（見下表），這是本輪第一件事 |
| open-watcom-v2 | 還沒有。要 sparse checkout 固定 commit |
| DPMI 規格書 | 還沒有。後續批次才需要，本輪不找 |
| 反組譯環境 | 有。IDA Pro 9.4 容器與 `tools/ida/` 流程沿用 M2 |
| DOS 執行環境 | 有。dosgolem 可跑 16 位元 DOS 程式；32 位元 extender 的支援程度未知，後續批次才會用到 |
| 與 Borland 的對照基準 | 有。M1 的四篇主題文章與 M2 的產生碼特徵、簽章 |

### 來源

| 代號（預定） | 內容 | 取得方式 | 本輪要不要 |
|---|---|---|---|
| `watcom-6.5` | `watcom-6.5/Watcom C.ver.6.5.English.zip` | archive.org 收藏 [watcom-c-cpp-compilers-collection](https://archive.org/details/watcom-c-cpp-compilers-collection) | **要**，本輪主角 |
| `watcom-7.0` | `watcom-7.0/Watcom C 386.ver.7.0.English.zip` | 同上 | **要**，盤點用；研究留到後續批次 |
| `watcom-8.0` | `watcom-8.0/CNW386-1.ZIP`～`CNW386-6.ZIP` | 同上 | 不下載，後續批次再取 |
| `watcom-9.01` | `watcom-9.01/floppies/Disk01.img`～`Disk06.img`（未打補丁的基礎版） | 同上 | 不下載，後續批次再取 |
| `watcom-9.5` | `watcom-9.5/discmaster/Sybase - Watcom C++ 9.5b.zip` | 同上 | 不下載，後續批次再取 |
| `open-watcom-v2@<commit>` | 固定 commit 的部分目錄 | `git` sparse checkout | **要**，解讀輔助 |
| `dpmi-0.9`、`dpmi-1.0` | DPMI 規格書 | 待找可信原文 | 不找，後續批次 |

整份收藏 4.8 GB，本輪只取 6.5 與 7.0 兩包，其餘等要用時再下載——磁碟與頻寬都是成本，
而且沒有要研究的版本先放著只會讓盤點表變髒。收藏附了 `SHA256SUMS`、`SOURCES.md`
（每個檔案的上游來源、下載日期、判定版本的依據）與 `cross-verification.md`，這三份一起抓。

6.5 與 7.0 的版本依據是封存內的 `README.1ST`／`READ.ME` 自述與檔案日期（6.5 為 1988-05-31；
7.0 的說明檔 1989-08-02、`WCC386.EXE` 1989-11-08），但上游是第三方站點，
**下載後要自己從檔案內容再確認一次版本**，不以收藏的目錄名為準（同 M3 的 DMX 教訓）。

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

## 從 M1～M3 帶過來的前提

| 前提 | 對這一輪的影響 |
|---|---|
| 文章格式：結論 → 根本問題 → 推導 → 在執行檔裡怎麼認 → 行為規格 → 證據與未知 | 沿用。沒有原始碼時「推導」的證據換成反組譯與文件，節首要標明 |
| 證據分級（已證實（對拍）／已證實（原文）／已證實（實測）／強推論／假說） | 這一輪多半落在「已證實（實測）」（反組譯與自己編的程式）與「已證實（原文）」（原廠文件）；**不會有「已證實（對拍）」**，因為沒有原廠函式庫原始碼可重編 |
| 外洩閘門掃 `vendor/` 全部來源 | Watcom 的出貨檔進私有工作區後，一樣要進閘門的比對來源 |
| 目錄名不是版本的證據（M3 的 DMX 教訓） | 每一版都要從檔案內容自己確認版本，盤點表要記下判定依據 |
| 審查抓得到的錯多半是統計與歸屬（M3：符號數、模組歸屬、模組語意） | 盤點表的每個數字都要寫清楚口徑；文章送審前自己先複驗一次 |
| M1 的範例程式形狀 `examples/<主題>/` | 6.5 能裝起來就建 `examples/watcom65/`，跑不起來就不建，不放假的預期輸出 |

## 開工前要先改的三件事

這三項是 M4 的前置條件，寫文章之前就要完成：

1. **公開 repo 的 `CLAUDE.md`**：「[HARD] 不放原文」加上 open-watcom-v2 的例外與引用格式（見下節）；
   來源清單加 Watcom 各版；簽章規則維持「只做 Borland 與 Microsoft」，Watcom 這一階段不做簽章。
2. **外洩閘門**：`vendor/open-watcom-v2/` 不進 `leak_check.py` 的比對來源，或在閘門加上
   「帶 open-watcom-v2 出處註記的區塊放行」的規則。**同時加一組正反對照測試**，
   確認 Borland、Microsoft、Watcom 出貨檔的原文照樣會被擋——閘門放寬之後沒有測試，等於沒有閘門。
3. **`README.md` 的邊界段與 `CONTEXT.md` 的來源代號**：加 Watcom 各版與 open-watcom-v2。

私有工作區同步要改的：`CLAUDE.md` 的授權邊界表加 Watcom 各版與 open-watcom-v2；
目錄表加 `originals/watcom/`、`toolchains/watcom/`、`vendor/open-watcom-v2/`；`tools/extract.sh` 跟上。

### 已決定的事項

2026-09-17 由使用者決定：

| 事項 | 決定 |
|---|---|
| Watcom 6.5～9.5 的出貨檔 | 可用於研究；只放私有工作區，記錄下載來源與 SHA-256，不進公開 repo。公開文章只寫機制與行為 |
| 位元組簽章 | 本階段不做 |
| open-watcom-v2 原始碼 | 公開文章**可以引用片段**，每段標明授權（Sybase Open Watcom Public License 1.0）並連到上游 GitHub 固定 commit 的檔案與行號 |
| 私有工作區 | 沿用 `~/cht/borland/` |

### 引用 open-watcom-v2 的規則

- **只有 open-watcom-v2 可以引用。** Borland、Microsoft、DMX、DSMI 與 Watcom 6.5～9.5 的出貨檔仍然一行都不放。
- **片段要短、要有必要。** 只引用解釋機制非引用不可的幾行；能用文字或虛擬碼說清楚的就不引用。
- **每段都附出處。** 程式碼區塊下方寫明「open-watcom-v2 `<commit 前 12 碼>`，`<路徑>:<起>-<迄>`，
  Sybase Open Watcom Public License 1.0」，連到
  `https://github.com/open-watcom/open-watcom-v2/blob/<commit>/<路徑>#L<起>-L<迄>`。
- **commit 固定。** 全階段用 `ow2-source-pin` 選定的同一個 commit，連結不會因上游更新而失效。
- **不拿引用當當年版本的證據。** 引用的是 2000 年代以後的原始碼，文章要寫清楚它和 1988–1993 年出貨版的關係與推論等級。

## 工作項目（第一批）

### A. 取得與盤點（私有）

| worklist id | 內容 | 完成條件 |
|---|---|---|
| `watcom-download` | 在容器內下載 6.5 與 7.0 兩包，連同 `SHA256SUMS`、`SOURCES.md`、`cross-verification.md`；逐檔比對雜湊 | `originals/watcom/` 下的檔案雜湊與 `SHA256SUMS` 相符；來歷筆記記下下載日期與收藏版本 |
| `watcom-extract` | 解出封存（沿用 `tools/extract.sh` 的形狀，必要時補解壓工具進映像並鎖版）；每一版產生 manifest（路徑、大小、SHA-256、封存內時間戳） | 每一版有 manifest；可從 `originals/` 重建 |
| `watcom-inventory` | 6.5 與 7.0 各自盤點：編譯器與工具、函式庫與模型分份、啟動碼模組、標頭、說明檔、附帶的 extender；**版本字串從檔案內容自己確認** | 兩份盤點表；每個數字寫明口徑；版本判定依據逐項列出 |
| `ow2-source-pin` | 選定 open-watcom-v2 commit，sparse checkout 上表目錄，記錄 commit 與授權檔 | 來歷筆記與 manifest 存在 |

### B. 研究專題（私有）

| worklist id | 內容 | 主要證據 |
|---|---|---|
| `watcom65-runtime` | 6.5 的呼叫慣例（參數走暫存器還是堆疊、誰清堆疊、回傳值在哪）、記憶體模型與函式庫分份、runtime helper 的名稱與用途、啟動碼流程；逐項和 M1 的 Borland 對照 | 6.5 函式庫模組的反組譯、說明檔、自己編的最小程式 |
| `watcom65-toolchain` | 6.5 在 dosgolem 裡裝起來、能編能連能跑；記下安裝步驟與可重跑的命令列 | 一支自己寫的範例程式編得出來並跑得動；裝不起來就寫明卡在哪 |

### C. 公開文章（本 repo，`docs/80-watcom/`）

| worklist id | 文章 | 依賴 |
|---|---|---|
| `article-watcom65-runtime` | Watcom C 6.5：16 位元時代的呼叫慣例、helper 與啟動碼，和 Borland 的對照 | `watcom65-runtime`；`watcom65-toolchain` 有成果就把實測結論一起寫進去 |

## 後續批次（本輪不做，先列著）

| worklist id | 內容 |
|---|---|
| `watcom70-386` | 7.0 的 32 位元產生碼與 helper；Phar Lap 386\|DOS-Extender 與 OS/386 的介面；輸出的執行檔格式 |
| `watcom80-delta` | 8.0 相對 7.0 的變化：保護模式版編譯器、函式庫與啟動碼的差異 |
| `watcom9-extender` | 9.01／9.5 的 DOS/4GW：stub 怎麼載入 LE 本體、啟動碼交棒時的環境、DPMI 服務的使用子集 |
| `le-lx-format` | LE 與 LX 的結構：物件表、分頁、fixup、入口 |
| `dmx-dpmi-usage` | DMX 怎麼與 extender 互動：真實模式中斷處理段、DPMI 呼叫、鎖定記憶體；只做原始碼層級 |
| `re-watcom-tools` | 各版編譯器與連結器的 IDA 匯出，當作辨識 Watcom runtime 與格式解析的實例 |
| `article-watcom386-extenders`、`article-dos4gw-dpmi-le`、`article-watcom-lineage` | 對應的三篇公開文章 |

## 每一項的做法

- 研究在私有工作區 `~/cht/borland/`，公開文章在本 repo，照兩邊 `CLAUDE.md` 的契約。
- 分析、解壓、反組譯、編譯一律在 docker；只清自己建立的 container，不做任何 prune 或 `rmi`。
- **下載那一步要開網路**（`watcom-download` 是本輪唯一需要網路的工作），其餘一律 `--network none`。
  下載完先驗雜湊再解壓，雜湊對不上就停下來回報，不要「先做著看看」。
- **反組譯出來的東西不是原始碼，但一樣不能貼**：文章寫函式名、符號、參數與回傳值的意義、
  演算法步驟；要示意用自己寫的虛擬碼。Watcom 這一階段也不做位元組簽章。
- **沒有實測就不要假裝有**：6.5 裝不起來時，文章的行為結論退回「反組譯與文件顯示」，
  並在節首標明沒有實跑支持。
- **每個盤點數字都要寫口徑**（是檔數還是模組數、含不含子目錄、以哪一份清單為準）。
  M3 那一輪被專家抓到的錯集中在這裡。
- 結論分四級（已證實／強推論／假說／未知）；這一輪不會有「已證實（對拍）」。
- 公開文章收尾：外洩閘門 → `tools/build_index.py` → **專家與學生兩個唯讀審查，都用 `model: sonnet`**
  → 逐條查證後修正 → 再過閘門推送。
  > M1、M3 的專家用 opus，代價是每輪約 20 分鐘。這一輪改用 sonnet；相對地，
  > 送審前自己先把盤點表的數字複驗一遍，不要把複驗外包給審查者。
- 派審查 agent 時，prompt 要寫死邊界：唯讀、不 commit、不 push、不做任何 docker 清理、
  回報不得引用出貨檔的原文。
- 某一項出現「怎麼查都對不上」時，最多換兩輪假設；仍解釋不了就把排除過的假設寫進筆記，另開一條 worklist。
- 每完成一項：`tools/worklist.py` 確認該條已不成立 → commit（`Closes #N`）→ push → 從 worklist 移除。

## 完成條件

- 「開工前要先改的三件事」全部完成。閘門的正反對照測試要跑得出結果，做法與判讀寫成
  公開 repo 的 `tools/leak_check_selftest.md`（測試資料留在私有工作區，說明與預期結果放公開端）。
- `originals/watcom/` 有 6.5 與 7.0 兩包，雜湊與收藏的 `SHA256SUMS` 相符；`vendor/` 可由 `originals/` 重建。
- 6.5 與 7.0 各有一份盤點表，版本判定依據寫明，**不以目錄名為準**。
- `watcom65-runtime` 研究筆記完成，每條結論標得出等級與出處。
- `docs/80-watcom/` 建立，至少一篇文章（6.5 的 runtime）通過外洩閘門與兩個審查。
- `README.md` 的文章表、`CONTEXT.md` 的來源代號與術語、`kb-index.json` 都已更新。
- 公開與私有 repo 的 worklist 沒有第一批的條目，對應 issue 全部關閉。
- `watcom65-toolchain` 有結論：能跑就記下可重跑的命令列，不能跑就寫明卡在哪、排除過什麼。

## 這一輪不做

- **不做 7.0 以後的研究與文章**：素材抓進來、盤點完就停，研究留到後續批次。
- **不下載 8.0、9.01、9.5b**：要用時再取。
- **不做 DPMI、LE／LX 格式**：那是 9.x 年代的事。
- **不做 Watcom 的位元組簽章**：已決定本階段不做。
- **不碰 VC++ 1.0 的對拍**：工具鏈仍取得不到（私有 #14 保持開啟）。
- **不寫跨廠牌並列比較文章**（`docs/50-cross-vendor/`）：那要等 Microsoft 那側也有對拍。

## 風險與未知

- **收藏可能下架或檔案更動。** 下載時同時記下收藏的版本與日期；雜湊對不上就停，不要改用其他來源湊。
- **6.5 可能裝不起來或在 dosgolem 下跑不動。** 1988 年的安裝程式對環境的假設未知。
  裝不起來時，本輪的行為結論退回反組譯與文件，`watcom65-toolchain` 記下卡點並另開 worklist，
  不擋 `article-watcom65-runtime`。
- **沒有原始碼，結論強度天生低於 M1。** Borland 那邊每條結論背後有對拍；這裡最強只到
  「反組譯 ＋ 自己編的程式對得上」。文章的定位要誠實，不要用 M1 的語氣寫 M4 的結論。
- **open-watcom-v2 與當年版本差十幾年。** 用它解讀時，任何「當年就是這樣」的句子都要標推論等級；
  最容易出錯的是符號名與啟動流程——那正是十幾年間最常被重構的部分。
- **閘門放寬的風險。** 為了讓 open-watcom-v2 可引用而改閘門，是這一輪最危險的動作。
  正反對照測試沒過就不要改閘門，寧可暫時不引用。
