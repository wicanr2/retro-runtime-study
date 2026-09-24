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

## 分批與進度

M4 分批做：

- **第一批（2026-09-18 完成）**：素材取得與盤點，加一篇 6.5 的 runtime 文章
  （`docs/80-watcom/watcom65-runtime.md`，已過專家與學生審查）。
  6.5 是 16 位元的，可以直接和 M1 的 Borland 逐項對照，用它把流程跑順。
  第一批的完成條件與收穫記在 `PLAN.md`；未解的 `WCC` E142 另開私有 issue 追。
- **第二批（2026-09-18 完成）**：7.0 的第一代 32 位元與 extender
  （`docs/80-watcom/watcom386-extenders.md`，已過兩個審查）。
  意外收穫是 7.0 的工具在 dosgolem 下跑得動（它們本身是 16 位元程式），所以那篇有實測撐著。
- **第三批（2026-09-18 完成）**：9.x 的 DOS/4GW 與 DPMI（`docs/80-watcom/dos4gw-startup.md`）。
  9.01 已取得並盤點；封包內容因為 WPK 解碼器未完成還讀不到，文章的證據不平均已標明。
- **第四批（2026-09-18 完成）**：**版本演進總表**（`docs/80-watcom/watcom-lineage.md`，
  已過兩個審查）——把 6.5、7.0、9.01 三版收束成一張「拿到執行檔怎麼判斷是哪一代」的對照，
  這是 M4 目標本身的驗收項。
- **第五批（2026-09-18 完成）**：**WPK 解碼器**——沒解開，但把卡點從「整個解碼器都不對」
  收斂成單一一點（同碼長內符號的排列順序）。過程見私有工作區 `tools/wpk/README.md`。
- **第六批（本輪）**：**反組譯 `INSTALL.EXE` 的解包器**，把上一批的卡點解掉。
  **結果：解開了**，9.01 的內容缺口一併補上，見下。
- **後續批次（本輪不做）**：8.0 的差異、LE／LX 格式細節。

### 第六批的結果（WPK 解碼器，完成）

上一批把問題收斂到「同一碼長內，符號按什麼順序對應碼值」——那由當年那個 qsort 對
**等值元素**的處理順序決定。這一批直接去讀 1992 年的解包器本體。

**答案在磁片自己身上**：`disk01/INSTALL.EXE` 是 MZ＋NE、目標作業系統是 OS/2，
所以跑不起來，但 NE 段裡就是那支解包器。定位不必逐段讀——`Mask[]` 那九個位元組
（`00 01 03 07 0F 1F 3F 7F FF`）在檔案裡只出現一次，而 `d_code`、`d_len`、`Mask`
三張表**連續排列、順序與 open-watcom 的 `decode.c` 完全一致**，這本身就說明兩者同源。
接著用 IDA 查參照，`GetByte`、`DecodePosition`、`MakeShannonTrie`、`AssignCodes`、
`SortLengths` 一路對上，qsort 本體是 `sub_3409`。

**open-watcom 的 `wqsort.c` 是個陷阱。** 它的註解說自己是「把 Watcom CLIB 的 qsort
整份搬過來，免得它在別的平台上變掉」，看起來正是要找的東西——但那是後來為了跨平台才固定
下來的三路分割版本。1992 年出貨的是**單向掃描的 quicksort**，兩者對等值元素的排列不同。
照反組譯重寫之後，第一個封包就通過 CRC。

**結果**：181 個封包、630 個成員全部解出，每一個都通過封包自帶的 CRC-32；
兩份各自獨立寫的實作（C 版與 Python 版）輸出逐位元組相同。
`tools/extract.sh` 已接上解包，所以解出的內容也進了外洩閘門的比對來源。

**順帶收回一個假修正**：第五批曾從位元流起點推出「1992 版碼表條目數不加 1」，
改掉之後第一個符號從 `.` 變成 `/`，看起來是對的。**那是排序錯誤造成的假象**——
換上正確的 qsort 之後 `+1` 才對。教訓寫成規則：**主要機制還沒對之前，
靠輸出「看起來比較合理」推出來的區域性修正不能當結論，它只是把錯誤挪了位置。**

### 解開之後補進文章的內容

兩篇既有文章的「讀不到」全部清掉（`dos4gw-startup.md` 3 處、`watcom-lineage.md` 8 處），
兩張 SVG 的對應欄位一併更新：

- **stub 的機制**：`wstub.c` 不是小型載入器，是普通 DOS 程式——用 `_searchenv` 在
  `DOS4GPATH` 與 `PATH` 找 `dos4gw.exe`，`execvp` 換掉自己，並把自己的路徑當參數傳過去
  讓 extender 回頭讀 LE 本體。
- **extender 從編譯期選擇變成執行時偵測**：9.01 的啟動碼有 `__Extender`（六個值），
  偵測手法是取 DOS 版本號那一次呼叫兼作探測器（先把 `EBX` 填成 `'PHAR'`，
  看 `EAX` 高 16 位回什麼識別字）。**啟動碼支援的比安裝程式問的多一家**（Intel Code Builder）。
- **兩處推翻了原本的「延續前代推論」**：`__StartTime` 在 7.0 有、9.01 又拿掉了；
  `AH=4Ah` 在 6.5 一律做、7.0 不做、9.01 只對 Intel Code Builder 做。
  **相鄰版本的外推在這條線上不可靠。**
- **空指標哨兵**：6.5 會掃描並印訊息，7.0 與 9.01 都沒有；9.01 留的是哨兵資料與 `BEGTEXT`。
- **堆疊慣例的命名**原本只能從啟動碼內部符號外推，現在有實物：`clib3s.lib` 裡
  `strlen_`／`printf_`／`fopen_` 一個都沒有，`clib3r.lib` 三個都有。

**函式庫格式**：7.0 與 9.01 的函式庫是 Phar Lap 的 Easy OMF-386 變體——外層 `.LIB`
封裝照標準走，模組記錄的欄位加寬。第二批時自寫解析器讀不了、只能到字串層級的結論；
R19 補上變體語意後已可逐符號解析（見 `PLAN.md` 該輪紀錄）。

### 第三批要回答的問題

1. **DOS/4GW 是什麼關係**：9.x 綁的是 Rational Systems 的 DOS/4GW，它與 7.0 那兩家有什麼不同？
   是隨編譯器附的還是要另外買？
2. **stub 怎麼運作**：DOS/4GW 年代的執行檔是「16 位元的 stub ＋ 32 位元本體」黏在一起。
   stub 做什麼、怎麼把控制權交給 extender？
3. **DPMI 進來了沒有**：7.0 走的是各家自己的介面；9.x 是不是改走 DPMI 這個標準？
   啟動碼看得出來嗎？
4. **啟動碼又變了什麼**：9.x 的啟動碼與 7.0 差在哪；`__psp` 那組全域變數還在不在。
5. **與 DMX 對得上嗎**：M3 已知 DMX 用 `wcc386p`、`SYSTEM dos4g` 建，
   它的 `realint.asm`（真實模式中斷處理）對應的是這一層的哪個機制？

### 第二批要回答的問題

7.0 是 Watcom 第一個 32 位元版本，也是「DOS 程式跑在保護模式」這條路的起點。要回答：

1. **它依附誰**：這個包不含連結器與除錯器，要搭 Phar Lap 或 A.I. Architects 的開發工具。
   那兩套 extender 對程式的介面長什麼樣？`WCL386` 產生的是給別人連結器用的命令檔，內容是什麼？
2. **產生碼變了什麼**：32 位元 flat 模型下，6.5 那套 helper 還在嗎？指標運算的 helper
   （`__PTS` 那一組）在沒有段的世界裡應該消失或改變——實際如何？
3. **兩種呼叫慣例的分家**：`clib3r.lib` 與 `clib3s.lib` 差在哪；`cstart3r.lbj`／`cstart3s.lbj`
   這種 `.LBJ` 是什麼格式（Lahey 連結器用的）。
4. **啟動碼**：7.0 的 `CSTART3R.ASM`／`CSTART3S.ASM` 與 6.5 的 `CSTART.ASM` 差多少？
   保護模式下「把多的記憶體還給 DOS」這類動作還做不做？
5. **能不能實測**：6.5 在 dosgolem 下能讀不能編；7.0 的工具是 32 位元保護模式程式，
   dosgolem 支援到哪還不知道——先試，不行就記下卡點，不擋文章。

這一輪與 M1 最大的差別是**函式庫本體沒有原始碼**。Borland 那邊是「原始碼＋對拍」，
Watcom 出貨包裡的 `CLIB*.LIB`、`MATH*.LIB` 只有目的碼。

不過 2026-09-18 盤點時發現**啟動碼是附原始碼的**：6.5 的 `DISK2/SRC/STARTUP/` 有
`CSTART.ASM`、`CMAIN.C` 與六個模型各一支的 wrapper，7.0 也有對應的兩份（暫存器版與堆疊版）。
所以啟動碼那一段的證據等級比原先預期的高，可以到「已證實（原文）」，甚至有機會對拍。
其餘部分仍然是：

- 結論主要來自**反組譯出貨的函式庫模組**與**原廠文件**，不是讀原始碼。
- open-watcom-v2（2000 年代以後的開源版本）只當解讀輔助，不當當年版本的證據。
- 「對拍」在這條線上的意義也不同：能做的是「用 6.5 自己編一支程式，看產生的碼與函式庫模組對不對得上」，
  不是「重編原廠函式庫再比對」。

## 手上有什麼、還沒有什麼

| 項目 | 狀態 |
|---|---|
| Watcom 各版出貨檔 | 6.5 與 7.0 **已取得**（2026-09-18，雜湊與收藏的 `SHA256SUMS` 相符），已解出並產 manifest；來歷與版本判定見私有 `notes/watcom-6.5/PROVENANCE.md` |
| open-watcom-v2 | **已取得**（commit `3605d031a737`，放私有工作區的 `refs/`，不在 `vendor/` 所以不進閘門比對）|
| DPMI 規格書 | 還沒有。後續批次才需要，本輪不找 |
| 反組譯環境 | 有。IDA Pro 9.4 容器與 `tools/ida/` 流程沿用 M2 |
| DOS 執行環境 | 有。dosgolem 可跑 16 位元 DOS 程式；32 位元 extender 的支援程度未知，後續批次才會用到 |
| 與 Borland 的對照基準 | 有。M1 的四篇主題文章與 M2 的產生碼特徵、簽章 |

### 來源

| 代號（預定） | 內容 | 取得方式 | 本輪要不要 |
|---|---|---|---|
| `watcom-6.5` | `watcom-6.5/Watcom C.ver.6.5.English.zip` | archive.org 收藏 [watcom-c-cpp-compilers-collection](https://archive.org/details/watcom-c-cpp-compilers-collection) | **已取得**，本輪主角 |
| `watcom-7.0` | `watcom-7.0/Watcom C 386.ver.7.0.English.zip` | 同上 | **已取得**，盤點用；研究留到後續批次 |
| `watcom-8.0` | `watcom-8.0/CNW386-1.ZIP`～`CNW386-6.ZIP` | 同上 | 不下載，後續批次再取。⚠ 9.01 的 README 提到「V8.5 到 V9.0 的變更」，所以 8.x 這段還有一個 **8.5**，要補 8.x 時別只抓 8.0 |
| `watcom-9.01` | `watcom-9.01/floppies/Disk01.img`～`Disk06.img`（未打補丁的基礎版） | 同上 | **第三批要下載** |
| `watcom-9.5` | `watcom-9.5/discmaster/Sybase - Watcom C++ 9.5b.zip` | 同上 | 不下載，後續批次再取 |
| `open-watcom-v2@3605d031a737` | 固定 commit 的四個目錄 | `git` sparse checkout | **已取得**，解讀輔助 |
| `dpmi-0.9`、`dpmi-1.0` | DPMI 規格書 | 待找可信原文 | 第三批要試著找；找不到就以「查不到可信原文」結案 |

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

## 開工前要先改的三件事（第一批已完成）

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

## 工作項目（第六批，本輪）

| worklist id | 內容 | 在哪 |
|---|---|---|
| `wpk-decoder` | 反組譯 `INSTALL.EXE` 的 NE 段，找出 1992 年那個 qsort，把解包做完 | 私有 |
| `watcom-901-content-gap` | 解開之後把 9.01 的內容補進既有兩篇文章與兩張 SVG | `docs/80-watcom/` |

## 工作項目（第四批，已完成）

| worklist id | 內容 | 在哪 |
|---|---|---|
| `wpk-decoder` | 把 `tools/wpk.py` 的解壓縮做完（要移植 Watcom CLIB 的 qsort，因為 wpack 依賴它對等值元素的排序順序）。**解不開就誠實收在卡點，不擋總表那篇** | 私有 |
| `article-watcom-lineage` | 公開文章：Watcom C 的版本演進——拿到一支執行檔怎麼判斷是哪一代 | `docs/80-watcom/` |

## 工作項目（第三批，已完成）

| worklist id | 內容 | 在哪 |
|---|---|---|
| `watcom9-download` | 下載 9.01 的六個磁片映像（必要時加 9.5b），驗雜湊、解出、產 manifest 與盤點 | 私有 |
| `watcom9-extender` | DOS/4GW：stub 怎麼載入本體、啟動碼交棒時的環境、有沒有走 DPMI；與 7.0、6.5 對照 | 私有研究筆記 |
| `dmx-dpmi-usage` | DMX 怎麼與 extender 互動（真實模式中斷段、DPMI 呼叫、鎖定記憶體）；只做原始碼層級 | 私有 |
| `article-dos4gw-dpmi-le` | 公開文章：DOS/4GW 年代的 32 位元 DOS 程式怎麼啟動 | `docs/80-watcom/` |

## 工作項目（第二批，已完成）

| worklist id | 內容 | 在哪 |
|---|---|---|
| `watcom70-386` | 7.0 的 32 位元產生碼與 helper、兩種呼叫慣例的分家、啟動碼與 6.5 的差異、依附的 extender 介面 | 私有研究筆記 |
| `watcom70-toolchain` | 7.0 的工具在 dosgolem 下能跑到哪（32 位元保護模式程式）；跑不動就記卡點 | 私有 |
| `article-watcom386-extenders` | 公開文章：第一代 32 位元 Watcom 與它依附的 DOS extender | `docs/80-watcom/` |

## 工作項目（第一批，已完成）

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
| `watcom80-delta` | 8.x 相對 7.0 的變化：保護模式版編譯器、函式庫與啟動碼的差異。**8.0 與 8.5 是兩個版本**（依據是 9.01 自己的 README）|
| `le-lx-format` | LE 與 LX 的結構：物件表、分頁、fixup、入口 |
| `re-watcom-tools` | 各版編譯器與連結器的 IDA 匯出，當作辨識 Watcom runtime 與格式解析的實例 |

## R27（本輪）：驗 WCC 6.5 自載 WCG 假設，解 E142

接 M4 第一批留下的 `watcom65-wcg-overlay`（私有 #32）：6.5 在 dosgolem 下「能讀不能編」，
`WCC`／`WCL` 卡在 `E142 ***FATAL*** Stack Overflow`（CPU 子集、記憶體、缺檔已排除）。
本輪驗「`WCC` 自己載入 `WCG.EXE` 當 overlay、不走 DOS EXEC」——證據是 dosgolem 開檔紀錄有
`WCG.EXE` 而 EXEC 紀錄是空的。

方法：dosgolem `-memops` 重跑定位「開檔→載入→跳轉」三步；反組譯 `WCC` 開 `WCG` 那一段
（先用跑得動的 `WDISASM`，不夠再上 IDA）；與 dosgolem 載入器行為對照。
完成條件：E142 成因定位到具體機制，或證偽並提下一假設（各有排除證據）；私有筆記
`notes/toolchains/watcom65.md`「還沒驗證的假設」段更新。
本輪不做：修 dosgolem（缺服務就寫 READY 規格另開條目）、6.5 編譯對拍、7.0 工具。

## R28（本輪）：查 WCG 主程式靜默 exit(1) 的觸發條件

接 R27（私有 #32，`watcom65-wcg-exit1`）：overlay 已證偽，E142 鏈條已定位——
子行程主程式跑約三萬九千道、回呼父行程取 quad、零開檔零輸出，走全檔唯一
`AH=4Ch` 回 1。本輪靜態反組譯為主：從共享指標找主程式讀 quad 的入口與版面假設，
收斂到 `exit(1)` 的第一條路徑。完成條件：觸發條件定位到具體機制
（含 quad 版面哪個欄位不合期望），或收斂到下一可驗假設並記排除證據；
筆記「下一步假設，待驗」段更新。若需 dosgolem 記憶體傾印能力，寫 READY 規格
另開條目，不在本輪實作。本輪不做：修 dosgolem、WCC 側改動、7.0、對拍。

## R29（本輪）：找回收 1 的 func#（窄 watch-read 計數＋父行程接續追蹤）

接 R28（私有 #32，`watcom65-wcg-exit1`）：對稱跳板、分派表基 `0x15D7:0x1052`、
`func 0Dh＝sub_112EE`、回傳鏈原樣傳遞皆已釘死。本輪兩手：`trace-after-exit`
放大到數千條，從 `AH=4Dh` 取回 1 跟到 E142 印出（判定是結束碼對映還是恢復後
堆疊檢查真爆）；對字偏移表（`0x16DC2＋2k`）逐 `func#` 窄 watch-read 計數
（先跑靜態出現過的 `{0, 8, 9, 0xA, 0xB, 0xD}`），排出呼叫樹形狀。
完成條件：回 1 的 `func#` 指名，或收斂到下一可驗假設；筆記待驗段更新。
本輪不做：修 dosgolem、WCC 側改動、7.0、對拍。

## R30（本輪）：定位 driver phase 的 E142 合成者

接 R29（私有 #32，`watcom65-wcg-exit1`）：結束碼對映已推翻，E142 主控台輸出
在 chained codegen 進行中由父行程側印出。本輪靜態為主：從 `Error!` 字串引用
回找診斷合成函式（訊息表取號→格式化→輸出），再找呼叫者落在 driver 回呼鏈哪一段；
候選仍是跨界巢套真溢位。完成條件：合成者指名（含真溢位／誤報判定），根因定位
到具體機制，或收斂到下一可驗假設；筆記待驗段更新。本輪不做：修 dosgolem、
WCC 側改動（定位後的修法另議）、7.0、對拍。

## R31（本輪）：查 ds:0x1856 堆疊下限算法＋實機版面會不會爆

接 R30（私有 #32，`watcom65-wcg-exit1`）：合成者已指名，AL=4 來自子行程堆疊守衛
`cmp sp, ds:0x1856`，跨界巢套每 quad 吃 `0x16` 不回彈。本輪找下限寫入者重建算法
（含不含記憶體版面），代入 dosgolem 版面與實機版面，看 102 quad 的 drift 哪種會跌破。
完成條件：算法寫得出、artifact 與否可判定（各有證據）；若是 artifact，加 dosgolem
backlog 並寫 READY 規格另開條目（本輪只寫規格）。本輪不做：修 dosgolem、WCC 側改動、
7.0、對拍。

## R47（本輪）：父 SI=1 指名＋清除路徑 FP 相依點

接 R46（私有 #32，`watcom65-wcg-exit1`）：映射修正，算術說。本輪：(a) 父 `SI＝1`
＝`sub_112C0`（逐函式管線 `loc_1E843` 內）✓；(b) 清除路徑 FP 相依：init 流全無
FP 算術（71k 步唯一 ESC 是探測三件套）、env 表兩邊應皆無——FP 無處可依，
算術子集規格拒寫；(c) 新嫌疑：`loc_12FA9` 的 `cmp dx,0x32`（`DX＝0x32` 走閂鎖，
`ja` 走 `loc_12FC8`）——DX 來源未定。完成條件：(a)(b) 指名＋證據；筆記更新。
本輪不做：修 dosgolem、WCC 側改動、7.0、對拍、READY 規格（(c) 未定不寫）。

## R44（本輪）：dd@0x16B4 候選＋28E1:000A 特徵（事後補記）

接 R43。`28E1:000A` 在 `dd@0x16B6` 取後 1 步執行，疑為另一次取的目標；
prologue＋遠呼叫特徵可對 IDA。結果：SI 定值仍懸一跳，轉 R45。

## R45（本輪）：結案盤點（事後補記）

接 R44。全程零浮點算術（EMPTY 全覆蓋）；參照必未進 `loc_12FA9`。E142 機制部分
（鏈條／合成者／閂鎖／artifact／FPU／trace 缺口）已足夠，index 選擇器殘留；
依 41 暫停過一次（後 R46 重開：映射修正）。

## R48（本輪）：DX 來源＋版面假說實證

接 R47（私有 #32，`watcom65-wcg-exit1`）：`DX＝0x32` 查明是前端預設常數
（排除）；`loc_12FC8` 直落閂鎖；dosgolem 改子行程 `PSP:0002` 為行程自有上限
（＋單元測試）但三支照爆（版面假說此路排除）；init 窗中斷列舉僅 `int 21h／10h`。
完成條件達成情况：兩修落地、排除清單完備；殘留 DOSBox 分歧點，依 41 暫停。
本輪不做：x87 算術、WCC 側改動、7.0、對拍。

## R52（本輪）：QEMU 參照＋E142 真兇（堆疊守衛＋自環）

接 R51（私有 #32，`watcom65-wcg-exit1`）：QEMU＋FreeDOS 真機級參照跑通
（`Code size: 11`，`.OBJ 234 bytes`；GDB stub 取證：`continue` 因執行緒狀態 bug
不可用，唯讀／`detach` 正常）。同 boot 死後比對：參照側閂鎖 `FF`、上限 `0x25D0`、
quad 區 `0x01`——與 dosgolem 跑完全相同，閂鎖平反（R31 鏈修正為相關非因果）。
E142 真鏈（dosgolem 全程 trace 步號）：`sub_2CE17` 入口守衛第 4 層自遞迴跳脫
（步 56927）→拷「`Stack Overflow`」到父緩衝區→`func#0xC／AL＝4`（步 57027）→
訊息 `0x8E`（步 57054）→`exit(1)`。下降鏈三節點載入後零寫入。
完成條件：真兇指名＋同 boot 證據；筆記更新。本輪不做：修 dosgolem（缺口未定）、
WCC 側改動、7.0、對拍。

## R53（本輪）：walker 上游分岔點

接 R52（私有 #32，`watcom65-wcg-exit1`）：真鏈已定，殘留 walker 上游
（約步 54k–55k）記憶體值條件分岔——候選 `[0x18EC]／[0x282]／[0x2F6]／[0x1766]`；
找 DOS 呼叫衍生的那一個即 dosgolem 缺口（找到就寫 READY 規格）。
完成條件：分岔點指名＋值來源；筆記更新。本輪不做：實作、WCC 側改動、7.0、對拍。

## R32（本輪）：查閂鎖語意（正常初始化 vs FPU 分歧誤設）

接 R31（私有 #32，`watcom65-wcg-exit1`）：SP 守衛說已推翻，AL=4 源是尾檢查閂鎖
（啟動即設、清除者沒跑）。本輪三手：單獨跑 WCG 對照閂不閂鎖（資料段 `0x2AC`）；
chained 跑找分派 `loc_12FA9` 者；查 dosgolem FPU 語意。完成條件：閂鎖語意判定，
artifact 屬 dosgolem 就加 backlog＋READY 規格（只寫規格）。本輪不做：修 dosgolem、
WCC 側改動、7.0、對拍。

## R33（本輪）：輸入變因（空程式編譯裁決）

接 R32（私有 #32，`watcom65-wcg-exit1`）：對照無效、FPU 排除、分派部分。本輪換輸入
不換環境：`int main(void){return 0;}`（無含入、無呼叫）同命令列編譯。編過→內容觸發
（往哪種 quad 觸發閂鎖收斂）；同 E142→基礎設施全滅（往 EXEC／版面差異收斂）。
完成條件：結果＋memops 裁決二選一；筆記待驗段更新。本輪不做：修 dosgolem
（artifact 寫規格另開條目）、WCC 側改動、7.0、對拍。

## R35（本輪）：real-DOS 參照（DOSBox-X 跑 EMPTY.C）

接 R34（私有 #32，`watcom65-wcg-exit1`）：三手齊，READY 規格缺 real-DOS 參照。
本輪找現成 binary（工作區，無則評估建置成本，超半天停手），同 DOS 根跑 `EMPTY.C`：
也 E142＝真 bug（remake 照抄）；編過＝artifact（回頭 diff 版面寫 READY 規格）。
完成條件：binary 有無＋對照結果或建置卡點；筆記待驗段更新。本輪不做：建置
DOSBox-X（超半天停）、修 dosgolem、WCC 側改動、7.0、對拍。

## R37（本輪）：DOSBox fpu=false 的 fnstcw 回值探針

接 R36（私有 #32，`watcom65-wcg-exit1`）：call 樹有序，func#0 無辜。本輪自寫
`FPUTEST.COM` 量 DOSBox 預設 vs `fpu=false` 的控制字。後者若回 `037Fh`→分歧解釋，
寫 FPU READY 規格（實作另開條目）；同樣缺席→回頭查版面。完成條件：兩設定實測值
＋裁決；筆記待驗段更新。本輪不做：修 dosgolem、WCC 側改動、7.0、對拍。

## R38（本輪）：實作 dosgolem spec-202＋重跑驗證

接 R37（私有 #32，`watcom65-wcg-exit1`）：分歧已解釋，READY 規格已寫。本輪在
dosgolem 側實作最小 x87 控制字（`FNINIT` 設 `037Fh`、`FNSTCW` 存回；不碰算術），
照該 repo 慣例測試，然後回本 repo 重跑兩支範例（應編過、E142 消失；通過則移除
worklist 條目）。完成條件：dosgolem 測試全過、探針回 `037Fh`、範例編過、兩邊推送。
本輪不做：x87 算術、模擬器陷入、WCC 側改動、7.0、對拍。

## R36（本輪）：收斂分歧窗（732214–732670 步 call 樹重建）

接 R35（私有 #32，`watcom65-wcg-exit1`）：artifact 確立，FPU 二度排除。本輪 watch
子行程堆疊區按步號重建進 `loc_12FA9` 的 call 樹，指名 dosgolem 缺的行為（可寫
READY 規格就寫，照 M1 慣例）。完成條件：完整路徑＋分支條件；筆記待驗段更新。
本輪不做：修 dosgolem（規格另開條目）、WCC 側改動、7.0、對拍。

## R41（本輪）：寫部分 READY 規格（FPU 定＋觀測缺口）

接 R40（私有 #32，`watcom65-wcg-exit1`）：二選一無果，承認動態 SI 鏈。本輪在
dosgolem 寫部分 READY 規格（下一號）：已定引用 spec-202；新增追蹤能力缺口
（運算元＋讀寫步號）；未定（index 選擇器）誠實標未知。實作另開條目。
完成條件：規格推送、筆記記編號、worklist 改待追蹤能力。本輪不做：實作、
修其他行為、WCC 側改動、7.0、對拍。

## R42（本輪）：實作 spec-203（追蹤時序＋運算元）

接 R41（私有 #32，`watcom65-wcg-exit1`）：部分 READY 已寫，主線暫停待追蹤能力。
本輪在 dosgolem 實作：trace 加絕對步號、watch 加觸發暫存器、watch-read 事件加步號；
照該 repo 慣例測試＋重建 dosrun。完成後回本 repo 用新欄位閉合選擇器（R43）。
完成條件：dosgolem 測試全過、新欄位實測可見、兩邊推送。本輪不做：x87 算術、
WCC 側改動、7.0、對拍、選擇器結論。

## R46（本輪）：trace-range 實作＋直擊 SI 寫入者

接 R45（私有 #32，`watcom65-wcg-exit1`）：主線暫停差全程 trace；msvc10 維持 blocked。
本輪在 dosgolem 寫 spec-204（`-trace-range lo-hi`，沿用 trace 格式）並實作＋測試，
回本 repo 抓 [732000, 733000] 全暫存器 trace 直擊 SI 寫入者。完成條件：測試全過、
新旗標可用、兩邊推送、筆記更新。本輪不做：x87 算術、WCC 側改動、7.0、選擇器結論
（R47 用新能力做）。

## R43（本輪）：追 SI 寫入者（dd@0x16D6 handler 鏈）

接 R42（私有 #32，`watcom65-wcg-exit1`）：新欄位驗證成功，SI 流量可見。本輪用
SI 欄位追 `SI＝0001→16B6`：7 步窗（`25E7:025E`→`28E1:000A`）經 `dd@0x16D6`
（`loc_13062→sub_1FD2C→sub_1FD54→sub_12E07→sub_12E14→sub_1F661→…`）；`SI＝0x16B6`
在 `28E1:000A`（函式 prologue）已就位。完成條件：指名定值函式；仍差一層記 R44。
本輪不做：修 dosgolem、WCC 側改動、7.0、對拍。

## R34（本輪）：隔離閂鎖設定觸發條件

接 R33（私有 #32，`watcom65-wcg-exit1`）：全滅裁決，func#0–4 小型查詢。本輪三手：
`25E7:0270` 正身（跳板返回恢復序列）、`func#0` 目標行為（farptr 在 `0x20E6`）、
數學分派讀寫時序（閂鎖步號 732670 為錨點）。三者齊備就寫 READY 規格（本輪可產出
規格，實作另開條目）。完成條件：觸發條件隔離到函式＋條件；筆記待驗段更新。
本輪不做：修 dosgolem、WCC 側改動、7.0、對拍。

## 工作項目（第七批，R22 本輪）

主體是「啟動的多型」，並行一個取得項。素材都已在手上（R19 的 omf386.py 直接可用），這一批不做 dosgolem 實跑（9.x 的工具跑不動）。

| 項 | 內容 | issue |
|---|---|---|
| 主體 | 9.01 的啟動多型：三顆 adi*start.obj、庫內 cstart（OS/2 路徑）、各目標版庫（.DOS/.OS2/.WIN）的差異、連結期選模組 vs 執行期偵測的兩層分工、RE 指紋 | 公開 [#25](https://github.com/wicanr2/retro-runtime-study/issues/25)、私有 #42 |
| 並行 | 取得 Watcom C 8.0 與 9.5：權利狀態查證 → 取得 → 照 M4 慣例盤點；來源不明朗就以「查過哪些來源」收尾 | 私有 #43 |

### 主體要回答的問題

1. `adsstart.obj`／`adiestrt.obj`／`adifstrt.obj` 各對應哪個 extender 或目標
   （名字的 AD/ADIE/ADIF 是什麼的縮寫）；
2. 庫內 `cstart`（EXT `__OS2Main`）與磁片上 `adsstart`（EXT `__CMain`）的分工：
   連結器怎麼選啟動模組（預設庫裡兩顆都在，靠什麼不衝突）；
3. 「連結期選啟動模組」與「cstart3r 執行期偵測 `__Extender`」兩層機制的邊界；
4. DOS／OS2／WIN 三版 `clib3r`／`clib3s` 的模組與符號差異（omf386.py 逐符號）；
5. RE：從執行檔的啟動碼形狀認出用的是哪顆啟動模組。

### 並行項的完成條件

取得並盤點（originals/＋extract＋manifest＋PROVENANCE），或記下查過的來源與
不可行的原因收尾；不為取得本身開第二輪。

## 第八批（R23，本輪）：Watcom 8.0／8.5／9.5 的取得與盤點

主軸是編譯工具本身：把版本線的實物補到 8.x 與 9.5，讓「7.0 → 8.x → 9.x」的
演進有東西可比。**這一批只做取得、驗證、解包、盤點**；版本差異的文章
（`watcom80-delta`）等素材齊了另批做。

| 版本 | 來源（archive.org） | 內容 |
|---|---|---|
| 8.0 | 收藏 `watcom-c-cpp-compilers-collection` 的 `watcom-8.0/` | `CNW386-1..6.ZIP` 六個 ZIP |
| 8.5a | `watcom85a-disks`、`watcom-8.5a`、收藏內 `watcom-8.5/` | zip ＋ `liren-c496/1..5.img` |
| 9.5b | `watcom-9.5b`、收藏內 `watcom-9.5/floppies/` | `W9516_01..04`（16 位元目標）、`W9532_01..10`（32 位元目標）、`OS2TK_1..6`，各 1,474,560 bytes 的 .vfd |

來源與 9.01 同一個收藏（先例：下載後與收藏的 SHA256SUMS 比對，相符才用；
見 `notes/watcom-9.01/PROVENANCE.md`）。權利狀態：8.x／9.x 不在 Open Watcom
開源範圍，處理方式與既有來源相同（private、不散布、閘門把關）。

### 完成條件

- `originals/` 收錄下載檔（.gitattributes 標 `-text`）、`vendor/` 可重建、
  manifest 與 CROSSCHECK 齊。
- `PROVENANCE`（`notes/watcom-8x95/` 或各版目錄）記下載日期、來源 identifier、
  雜湊驗證結果、版本判定證據（安裝日期戳、README 字串）。
- 盤點筆記：各版磁片→目錄→關鍵工具（編譯器、連結器、函式庫清單），
  386 函式庫用 `omf386.py` 做模組清單（供下一批的版本差異比較）。
- `.vfd` 格式驗證過（是否 FAT12；不是就記下格式與處理方式）。

### 這一批不做

- 8.x／9.5 的差異文章與總表更新（素材齊後另批）。
- 9.5 的安裝與實跑（先盤點；安裝工具鏈要 dosgolem 支援評估，另議）。

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

## 完成條件（第六批）

- WPK 解開，而且**有機械化的驗收**（每個成員都通過封包自帶的 CRC，不是靠肉眼看內容像不像）。
- 解包接進 `tools/extract.sh`，`vendor/` 仍可由 `originals/` 重建，解出的內容進外洩閘門的比對來源。
- 既有兩篇文章與兩張 SVG 的「讀不到」清掉；**被推翻的舊斷言要寫明是推翻，不是默默改掉**。
- 讀不到的部分照樣寫清楚（函式庫的 Easy OMF-386 問題當時另開 worklist，R19 解掉）。
- 兩個審查（都用 `model: sonnet`）通過，閘門通過，issue 關閉。

## 完成條件（第四批，已達成）

- `docs/80-watcom/` 多一篇版本演進總表，過外洩閘門與兩個審查（都用 `model: sonnet`）。
- 那篇要能回答 M4 的目標問題：**拿到一支 Watcom 編譯的程式，怎麼判斷大約是哪一代工具鏈**。
- `wpk-decoder` 有結論：解開就把 9.01 的內容補進既有筆記與文章；解不開就寫明卡在哪、排除過什麼。
- README、CONTEXT、kb-index 更新；兩邊 worklist 沒有第四批條目，issue 關閉。

## 完成條件（第三批，已達成）

- 9.01 取得並驗過雜湊，`vendor/` 可由 `originals/` 重建，有盤點表與版本判定依據。
- `watcom9-extender` 與 `dmx-dpmi-usage` 研究筆記完成，上面五個問題各有答案或明確的「查不到」。
- `docs/80-watcom/` 多一篇 DOS/4GW 的文章，過外洩閘門與兩個審查（都用 `model: sonnet`）。
- README、CONTEXT、kb-index 更新；兩邊 worklist 沒有第三批條目，issue 關閉。

## 完成條件（第二批，已達成）

- `watcom70-386` 研究筆記完成，上面五個問題各有答案或明確的「查不到，理由是什麼」。
- `docs/80-watcom/` 多一篇 7.0 的文章，過外洩閘門與兩個審查（都用 `model: sonnet`）。
- `README.md`、`CONTEXT.md`、`kb-index.json` 更新；兩邊 worklist 沒有第二批條目，issue 關閉。
- `watcom70-toolchain` 有結論：能跑就記可重跑的命令列，不能跑就寫明卡在哪、排除過什麼。

## 完成條件（第一批，已達成）

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

- **不做 8.0 的差異比較**：它夾在 7.0 與 9.x 中間，等這一輪做完再看值不值得補。
- **不下載 8.0**：要用時再取。9.01 這一輪要下載。
- **不做 LE／LX 格式的逐欄位解析**：先把啟動流程講清楚，格式細節另開一輪。
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
