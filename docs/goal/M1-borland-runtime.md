# M1 Borland runtime 分析

## 目標

把 Borland C++ 2.0 的執行時期函式庫寫到「不看原始碼也能重做」的程度。分四批：

- **第一批（已完成）**：證明原始碼能代表出貨的函式庫，並寫完三個每支程式都會用到的機制——
  一份原始碼怎麼編出五個記憶體模型、編譯器自動插入的 helper、啟動與結束鏈。
- **第二批（已完成）**：遊戲真正呼叫的那些函式——檔案 I/O 與緩衝、`printf` 家族、heap、conio 與文字畫面。
  每篇要給得出行為規格：同樣的輸入，remake 照規格做會得到同樣的輸出與錯誤碼。
- **第三批（R20，已完成）**：當年列為選配的亂數與時間——`rand` 家族與 `time` 家族。
  規格同第二批：機制、行為規格、在執行檔裡怎麼認。
- **第四批（R21，本輪）**：最後一塊——iostream。C++ 的類別階層、虛擬繼承與靜態物件建構
  是這一輪的新題目，與前三批的 C 世界不同；範圍與完成條件見文末第四批一節。

## 從第一批帶過來的前提

| 前提 | 對第二批的影響 |
|---|---|
| C 函式庫與 iostream 共 491 個模組 × 五個模型（2,455 次）、far 版 31 個、啟動碼 20 個目的檔與出貨版逐模組相同 | 本輪要讀的模組全部在對拍範圍內，結論可以標「已證實（對拍）」 |
| 數學函式庫的 `STRTOD`、`FPERR`、`_MATHERR`、`SCANTOD`、`EFCVT`、`GCVT` 重現不出來 | `printf` 的浮點格式化與 `scanf` 的字串轉數字會碰到這幾個模組，引用時只能標「已證實（原文）」 |
| 兩版出貨的 C 函式庫只有 `SCROLL` 不同，行為差異已實跑確認 | conio 那篇直接引用，不必重做 |
| 工具鏈：`tools/bcpp20/bcc.sh`、dosgolem `bcc20-toolchain` 分支（含 `int 10h` 文字模式、檔案代號、決定性時鐘） | 範例程式可以編譯、實跑、比對輸出 |
| M2 的產生碼特徵與 `signatures/borland-crtl-2.0/dos.json` | 每篇的「在執行檔裡怎麼認」直接用簽章裡的符號名，不必重新歸納 |
| 範例程式的既有形狀：`examples/<主題>/` 內含 `.C`、`run.sh`、`expected.txt`、`README.md` | 沿用，不另立格式 |

## 工作項目（第二批）

每個主題一組：私有研究筆記 → 公開文章（含 SVG 與範例程式）。A–D 已登記在兩邊的 `worklist.json` 與 GitHub issue（milestone「M1 Borland runtime 分析」）。

| 主題 | 私有研究筆記（issue） | 公開文章與範例（issue） | 要回答的問題 |
|---|---|---|---|
| A 檔案 I/O 與緩衝 | `borland-stdio-file-io`（私有 #22） | [#9](https://github.com/wicanr2/retro-runtime-study/issues/9)：`docs/10-borland-crtl/stdio-file-io.md`、`examples/stdio/` | `FILE` 的欄位與緩衝策略（什麼時候全緩衝、行緩衝、不緩衝）；文字模式的 CR／LF 與 `Ctrl-Z`；`fopen` 模式字串的解析；低階代號層（`open`／`read`／`write`／`lseek`）與 `FILE` 的關係；`errno`、`_doserrno`、`perror` 的對應；`fseek`／`ftell` 在文字模式的邊界 |
| B `printf` 家族 | `borland-printf-engine`（私有 #23） | [#10](https://github.com/wicanr2/retro-runtime-study/issues/10)：`docs/10-borland-crtl/printf-engine.md`、`examples/printf/` | 格式化引擎怎麼被 `printf`／`fprintf`／`sprintf`／`cprintf` 共用；支援哪些轉換與旗標、寬度與精度的邊界；浮點格式化為什麼要另外連結（`floating point formats not linked` 從哪來）；`scanf` 家族的輸入規則與回傳值 |
| C heap | `borland-heap`（私有 #24） | [#11](https://github.com/wicanr2/retro-runtime-study/issues/11)：`docs/10-borland-crtl/heap.md`、`examples/heap/` | near heap 與 far heap 兩套配置器的資料結構與差異；`malloc`／`free`／`realloc`／`calloc`／`coreleft` 的行為與失敗條件；`brk`／`sbrk` 與堆疊的邊界；各記憶體模型下 heap 放在哪裡；與 DOS 記憶體配置（`allocmem`）的關係 |
| D conio 與文字畫面 | `borland-conio-screen`（私有 #25） | [#12](https://github.com/wicanr2/retro-runtime-study/issues/12)：`docs/10-borland-crtl/conio-screen.md`、`examples/conio/` | `directvideo` 決定直接寫 `B800` 還是走 BIOS；`window`／`gotoxy`／`wherex` 的座標與邊界；`gettext`／`puttext`／`movetext`；捲動與清除（含 1991-04 與 1991-08 版的差異）；`getch`／`getche`／`kbhit` 的鍵盤行為與擴充鍵 |
| E 亂數與時間（第三批，R20，已完成） | `borland-rand-time`（[私有 #40](https://github.com/wicanr2/retro-runtime-study-private/issues/40)） | [#23](https://github.com/wicanr2/retro-runtime-study/issues/23)：`docs/10-borland-crtl/rand-time.md`、`examples/randtime/` | `rand`／`srand` 的序列公式與 `RAND_MAX`；`time`／`clock`／`ftime`／`getdate` 讀哪些 DOS 服務；remake 要重現同一串亂數時該怎麼做 |
| F iostream（第四批，R21） | `borland-iostream`（[私有 #41](https://github.com/wicanr2/retro-runtime-study-private/issues/41)） | [#24](https://github.com/wicanr2/retro-runtime-study/issues/24)：`docs/10-borland-crtl/iostream.md`、（視份量）`docs/60-re-fingerprints/bcc20-cxx-codegen.md`、`examples/iostream/` | `ios`／`streambuf`／`istream`／`ostream` 的類別階層與虛擬繼承；`filebuf` 走 handle 還是 `FILE*`；全域物件建構與啟動鏈；格式化狀態的預設值；C++ mangling 與 vtable 的辨識法 |

A–D 已完成；E 排入 R20（工作項目細節見文末「第三批」一節）。

## 建議順序

1. **A 檔案 I/O**：`printf` 與 conio 都會用到 `FILE` 與代號層，先做它，後兩篇才不必重複解釋。
2. **B `printf` 家族**：依賴 A 的緩衝與 `FILE` 結論。
3. **C heap**：獨立，可以與 B 並行；`realloc` 的行為規格要實跑。
4. **D conio**：材料最齊（SCROLL 已實跑、dosgolem 已補 `int 10h`），可隨時插入。
5. E（第三批，R20）：亂數與時間。
6. F（第四批，R21）：iostream。

## 每一項的做法

- 研究在私有工作區 `~/cht/borland/`，公開文章在本 repo，照兩邊 `CLAUDE.md` 的契約；一行原始碼都不放。
- 分析、編譯、實跑一律在 docker；只清自己建立的 container，不做任何 prune 或 `rmi`。
- 每篇的「給 remake 的行為規格」要有**實跑**支持：範例程式涵蓋邊界值（空字串、超長欄位、配置失敗、視窗邊緣、檔尾），
  在 dosgolem 執行，輸出存成 `expected.txt`，五個記憶體模型都要跑。
- 「在執行檔裡怎麼認」引用 `signatures/borland-crtl-2.0/dos.json` 的符號名與 M2 的產生碼特徵，不重新歸納。
- 結論分四級（已證實／強推論／假說／未知）；引用第一批無法重現的模組時標「已證實（原文）」。
- 公開文章：外洩閘門 → `tools/build_index.py` → 專家與學生兩個審查（`model: sonnet`、唯讀）→ 逐條查證後修正 → 再過閘門推送。
- dosgolem 缺功能時，先照 DOSBox-X 原始碼寫規格（`READY`）再實作，放 `bcc20-toolchain` 分支。
- 某一項的實驗出現「怎麼調都對不上」時，最多換兩輪假設；仍解釋不了就把排除過的假設寫進筆記，另開一條 worklist。
- 每完成一項：`tools/worklist.py` 確認該條已不成立 → commit（`Closes #N`）→ push → 從 worklist 移除。

## 完成條件

- A–D 四組的研究筆記與文章完成，對應 issue 關閉，兩個 repo 的 worklist 沒有這一批的條目。
- 四篇文章都有「給 remake 的行為規格」與「在執行檔裡怎麼認」兩節，且行為規格的每一條都指得出是哪一支範例程式跑出來的。
- `examples/stdio`、`examples/printf`、`examples/heap`、`examples/conio` 在五個記憶體模型可重跑，輸出與 `expected.txt` 相同。
- `README.md` 的文章表、`CONTEXT.md` 的術語、`kb-index.json` 都已更新。

## 風險與未知

- **浮點格式化牽涉到無法重現的模組。** `printf` 的 `%f`／`%e`／`%g` 走 `EFCVT`、`GCVT`，`scanf` 的數字轉換走 `STRTOD`、`SCANTOD`；
  這些模組重編後與出貨版不同，文章對它們只能標「已證實（原文）」，行為規格改以實跑結果為準。
- **dosgolem 的 DOS 服務可能不夠。** 檔案 I/O 的邊界（`lseek` 超過檔尾、唯讀檔案、磁碟滿）與時間服務可能要補規格；
  補到第二個服務仍不夠時停下來評估，不在原項目裡無限延伸。
- **conio 的實跑只能看文字緩衝區。** dosgolem 沒有真實螢幕，`getch` 這類互動行為要靠送鍵腳本；擴充鍵的行為可能測不完整。
- **heap 的行為與 DOS 版本有關。** `coreleft`、`sbrk` 會受可用記憶體影響，範例程式要固定 dosgolem 的記憶體設定，否則輸出不穩定。
- **iostream 當年就不在第二批**（量大且與 C 函式庫交錯），第三批也不含；排入第四批（R21），見文末。

## 第三批（R20）：亂數與時間

### 範圍（模組盤點已確認）

| 來源 | 模組 |
|---|---|
| `CLIB1` | `RAND.C`（`srand`／`rand`；LCG 乘數 `0x015A4E35`、增量 1、`static long Seed`）、`STIME.C`、`CTIME.C`（`ctime`／`asctime`／`localtime`／`gmtime`／`mktime` 一帶）、`TIMECVT.C`（時間轉換的共用結構）、`FTIME.C`、`GETDATE.C`、`LOCALE.C` |
| `CLIB2` | `CLOCK.CAS`、`TZSET.CAS`、`RANDBLK.CAS`、`GETFTIME.CAS`、`SETFTIME.CAS`、`SETDATE.CAS` |

### 要回答的問題

1. **亂數**：`rand` 的遞推公式與 `RAND_MAX`；種子為 0／1／`RAND_MAX` 時的序列前幾項；
   週期與位元模式（低位的規律性——拿它做擲骰的遊戲會用到）；`random(max)`／`randomize()`
   是函式還是巨集、`randomize` 讀哪個時鐘；`RANDBLK.CAS` 的角色。
2. **時間的來源**：`time`／`stime`、`_ftime`、`getdate`／`gettime`／`setdate`／`setftime`／`getftime`
   各自走哪個 DOS 服務（`int 21h` 的 `2Ah`–`2Fh` 一帶）或 BIOS 服務；`clock` 的計時來源
   （BIOS tick？DOS？）與解析度、歸零條件。
3. **轉換與時區**：`localtime`／`gmtime`／`mktime`／`asctime`／`ctime` 怎麼共用 `TIMECVT`；
   `tzset` 讀的 `TZ` 環境變數格式與預設時區；閏年與年份範圍的邊界（`mktime` 正規化、
   1970／2038 這類界線在 16 位元 `time_t` 下是什麼樣子）。
4. **給 remake 的行為規格**：同種子 → 同序列；固定時間 → `time` 家族輸出一致；
   五個記憶體模型下的差異（far 版有哪些）。
5. **在執行檔裡怎麼認**：符號名與簽章（引用 `signatures/borland-crtl-2.0/dos.json`）。

### 做法與前提

沿用第二批的流程（研究筆記 → 公開文章與範例 → 審查與閘門），並且：

- 模組全在既有對拍範圍（491 模組 × 五模型），結論可標「已證實（對拍）」。
- dosgolem `bcc20-toolchain` 分支已有**決定性時鐘**，時間行為可以實跑且可重現；
  `clock` 若走 BIOS tick（`int 1Ah`）而 dosgolem 尚未支援，先寫 `READY` 規格再補，最多補兩個服務。
- 範例程式固定種子與時鐘，輸出存 `expected.txt`，五個模型都要跑。

### 完成條件

- 公開文章 `docs/10-borland-crtl/rand-time.md`（含 SVG 與前置欄位）通過審查與閘門，issue #23 關閉。
- `examples/randtime/` 五個模型可重跑且輸出與 `expected.txt` 相同。
- 行為規格的每一條都指得出是哪一支範例程式跑出來的。

### 風險

- `clock`／BIOS tick 的 dosgolem 支援度未知，可能要補規格（見做法）。
- 時區測試要控制 DOS 環境變數 `TZ`；dosgolem 的環境變數傳遞若不夠，退回讀原始碼標「已證實（原文）」。
- `setdate`／`setftime` 這類**寫入**系統時間的函式，實跑只能驗證呼叫的服務與回傳值，
  不能真的改宿主時間——規格以服務編號與參數為準。

## 第四批（R21）：iostream

### 範圍（盤點已知）

| 來源 | 內容 |
|---|---|
| `IOSTRM1`（100 檔） | 新式 iostream（`iostream.h`／`fstream.h`）：`FS*`（filebuf）、`IS*`／`OS*`（istream／ostream 成員）、`ST*`（streambuf）、`IO*`／`IFS*`（iostream 與 withassign）等 |
| `IOSTRM2`（93 檔） | 舊式 stream（`STREAM.H`）的 `OST*`、`SD*`／`SRB*`，以及新式的一部分 |
| `CLIB2/IOSTREAM.RSP` | 191 個模組**進主庫** `c<模型>.lib` |
| `CLIB2/OLDSTRM.RSP` | 8 個舊式核心模組（FILEBUF、FORMAT、ISTREAM、OSTREAM、ISTRF、OSTRF、STDFILE、STREAMBF）**獨立**成 `oldstrm<模型>.lib` |
| 出貨標頭 | `IOSTREAM.H`（710 行：ios/streambuf/istream/ostream/iostream/*_withassign）、`FSTREAM.H`、`STREAM.H`（326 行） |

全部 191 個 iostream 模組在既有對拍範圍（491 模組 × 五模型），結論可標「已證實（對拍）」。

### 要回答的問題

1. **類別地圖**：`ios`（狀態與格式化旗標）→ 虛擬基礎；`streambuf` 的 get/put 區與
   `seekoff`/`overflow`/`underflow` 原語；`istream`/`ostream`/`iostream`；`*_withassign`。
   每個類別的職責一句話講清。
2. **虛擬繼承的產生碼**：`class istream : virtual public ios` 在 BCC 2.0 下
   vtable 長什麼樣、虛擬基礎的位址調整怎麼做（這是 C++ 指紋，與 M2 的 C 產生碼特徵合流）。
3. **與 stdio 的關係**：`filebuf` 走低階 handle 還是 `FILE*`；`cout` 綁 stdout 的機制；
   緩衝策略與 stdio 篇的對照（行緩衝？完全緩衝？）。
4. **靜態物件與啟動鏈**：`cout`/`cin` 等全域物件的建構發生在啟動鏈哪一段
   （`_INIT_` 表？優先序？），與啟動與結束鏈那篇接上；解構順序。
5. **格式化狀態**：`flags`/`width`/`precision`/`fill` 的預設值與作用範圍
   （width 為什麼「一次有效」）；`<< float` 走哪條路（對照 printf 的浮點連結開關）；
   操作子（`endl`/`hex`/`dec`/`flush`）的機制。
6. **建置取捨**：191 模組進主庫 vs 8 個舊式模組獨立成 `oldstrm<模型>.lib`——
   為什麼舊式 stream 要分家（連結取捨、相容性）。
7. **在執行檔裡怎麼認**：Borland 的 C++ name mangling 格式、vtable 符號、
   靜態建構的辨識法；簽章（dos.json）涵蓋到什麼程度、要補什麼。

### 做法與前提

沿用第三批的流程（研究筆記 → 公開文章與範例 → 審查與閘門），並且：

- 對拍結論直接引用第一批成果（iostream 模組全數相同）。
- dosgolem `bcc20-toolchain` 分支已支援編譯與實跑；C++ 程式的靜態建構走啟動鏈，
  範例要驗「全域物件建構的時機」。
- 產出規劃：主文一篇（機制＋行為規格）；C++ 產生碼指紋若份量足夠則獨立成
  `docs/60-re-fingerprints/bcc20-cxx-codegen.md`，不足則併入主文——開工第一週決定。

### 完成條件

- 公開文章（含 SVG 與前置欄位）通過審查與閘門，issue #24 關閉。
- `examples/iostream/` 五個模型可重跑且輸出與 expected 檔相同。
- 行為規格的每一條都指得出是哪一支範例程式跑出來的；
  C++ 指紋的每一條都指得出是哪個編譯產物看到的。

### 風險

- **193 個 `.cpp` 是三批以來最大的單一主題**：靠「先類別地圖、後逐模組」的順序控制；
  若兩週內類別地圖完成不了，切成「新式 iostream」與「舊式 stream」兩輪。
- BCC 2.0 的 C++ 產生碼沒有現成指紋文件，vtable／mangling 要自己從編譯產物歸納
  （工具鏈現成，`-S` 與 `-v` 可用）。
- 舊式 stream（`STREAM.H`）的遊戲使用率低，優先序放最後；份量不足就只寫「怎麼認」。
