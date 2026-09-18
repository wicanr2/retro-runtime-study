---
id: borland-crtl/heap
title: near heap 與 far heap：兩套配置器，一條與堆疊的邊界
libraries: [borland-crtl-2.0]
goals: [craft, re, oracle]
evidence: 強推論
triggers:
  - remake 要重現原版的配置行為，或懷疑原版有 heap 破壞
  - 反組譯看到三個全域字被一大群配置相關的函式共用
  - 想知道 BC++ 2.0 的 malloc 會不會就地擴張、realloc 之後舊指標還能不能用
  - 想知道 malloc 失敗時 errno 是什麼、coreleft 的數字怎麼來的
  - 同一支程式換記憶體模型之後配置行為整個變了
  - 老程式在某台機器上配置失敗，而可用記憶體看起來還很多
symbols: [malloc, free, realloc, calloc, coreleft, farmalloc, farfree, farrealloc, farcoreleft, __brk, __sbrk, _brk, _sbrk, __brklvl, _heapbase, _heaptop, heapcheck, heapchecknode, heapwalk, farheapwalk, allocmem, setblock, _HEAPOK, _HEAPCORRUPT, _BADNODE, _FREEENTRY, _USEDENTRY, MARGIN]
related: [borland-crtl/memory-model-macros, borland-crtl/startup-and-exit, borland-crtl/stdio-file-io]
---

# near heap 與 far heap：兩套配置器，一條與堆疊的邊界

## 結論

BC++ 2.0 附的不是一個 `malloc`，是兩個。記憶體模型決定編進去哪一個：small 與 medium 用 near heap
（在 `DS` 那 64 KB 裡，單位是 byte），compact、large、huge 用 far heap（在程式那塊 DOS 記憶體的尾端，
單位是段落＝16 bytes）。兩者的區塊頭、free 的判別方式、甚至管理變數放在哪個段都不一樣。

對 remake 影響最大的幾條：

- **`malloc(0)` 回 NULL**，不是回一個可以 `free` 的最小區塊。
- **`realloc` 放大一律搬家**：配一塊新的、複製、放掉舊的，所以舊指標一定失效；縮小則就地切開，指標不變。
- **切割從空洞的尾端切**，所以「放掉一塊再要小一點的」拿到的位址不是原位址，而是原位址加上剩餘量。
- **near heap 與堆疊之間只有 512 bytes 的護欄**，而且只擋一個方向：擋得住 heap 長進堆疊，擋不住堆疊往下踩進 heap。
- **`coreleft` 在兩種模型算的是不同的東西**，數字不能互相比較。

機制來自原始碼，行為規格來自一支自寫測試程式（[`examples/heap/`](../../examples/heap/)）在五個記憶體模型下、
在 dosgolem 裡的實跑結果。

## 根本問題

### 一個程式能碰到的記憶體有兩種形狀

8086 的位址由段與位移湊成。`DS` 指到的那 64 KB 裡，位址只要 2 bytes 就能表示，存取一個指標省一半空間、
少一次段暫存器操作。可是一支遊戲要的資料常常超過 64 KB，那就得跨段定址，指標變成 4 bytes。

一套配置器沒辦法同時把兩件事做好：用 2 bytes 指標管 64 KB 以內的東西，和用 4 bytes 指標管整條 DOS 記憶體。
所以 Borland 寫了兩套，讓編譯期的記憶體模型決定編進哪一套。

### heap 與堆疊共用同一塊空間

near heap 長在 `DS` 裡，靜態資料的後面；堆疊在同一個 64 KB 的另一端往下長。中間那塊空地兩邊都想要，
而 8086 沒有任何硬體機制能在越界時通知程式。

### 向 DOS 要記憶體很貴

DOS 的記憶體服務以段落為單位，而且一支程式啟動時通常已經拿到一整塊（連結器決定的大小）。
每要一次都是一次 `int 21h`，還要處理 MCB 鏈。配置器不能每次 `malloc` 都去問 DOS。

## 推導

<p align="center"><img src="../../img/borland-heap-layouts.svg" width="920" alt="near heap 與 far heap 的區塊頭欄位對照：near 用 byte 為單位、size 加 1 表示在用中；far 用段落為單位、prev_real 為 0 表示 free，多一個 prev_real2 欄位；兩者的 free 區塊都串成雙向環狀佇列"></p>

### 區塊頭：把旗標藏在對齊的縫裡

每個區塊不論在用或空著都有一個頭。**在用中只需要兩個欄位**（大小、實體前一塊），空著時才需要多兩個
（free 佇列的前後）。於是已配置區塊的頭是 4 bytes，free 區塊的頭是 8 bytes——而這也決定了最小區塊：
一塊空著時至少要放得下 8 bytes 的頭。

「這塊在用嗎」沒有獨立的欄位。near heap 的區塊大小一律偶數，最低位元永遠是 0，配置器就拿它當旗標：
**大小欄位加 1 代表在用中**。

far heap 做不到同一招——它的大小是段落數，沒有保證偶數的位元可借。所以改成
**`prev_real` 欄位為 0 代表這塊是 free**，真正的「實體前一塊」搬到第五個欄位 `prev_real2`。
far heap 的 free 頭因此是 10 bytes。

| | near heap | far heap |
|---|---|---|
| 編進哪些模型 | small、medium | compact、large、huge |
| 單位 | byte | 段落（16 bytes） |
| 已配置區塊頭 | 4 bytes | 4 bytes |
| free 區塊頭 | 8 bytes | 10 bytes |
| 最小區塊 | 8 bytes | 16 bytes |
| 「在用中」怎麼表示 | 大小欄位 +1（大小恆為偶數） | `prev_real` 不為 0 |
| 管理變數放哪 | `DS`（`_DATA` 段） | **程式碼段**，省下切換 `DS` |
| 向下要空間的方式 | `__sbrk` 推高 break level | `_sbrk` → `setblock` 調整程式自己的 MCB |

### 三個字就管完一個 heap

兩套配置器都只用三個全域變數：第一塊、最後一塊、還有一個「漫遊指標」。
free 區塊串成**雙向環狀佇列**，順序是邏輯的、與實體位置無關；只剩一塊 free 時，它的前後都指向自己。

`malloc` 從漫遊指標指的那塊開始沿佇列繞一圈，**第一個裝得下的就用**（first fit）。
為什麼不是「最合適的」？因為 best fit 要走完整條佇列，而 first fit 通常走幾步就找到，
還順便把配置壓力分散到 heap 各處。漫遊指標讓下一次搜尋接著上次的位置開始，不會每次都從頭撞同一批小碎片。

### 切割從尾端切

找到的區塊比需要的大時，配置器要決定切不切。**多出來的空間必須放得下一個 free 區塊頭**，否則整塊給出去
（使用者拿到比要求多一點的空間，這是內部碎片）。

真的要切時，新區塊是從**空洞的尾端**切出來的，前半段留在 free 佇列裡、大小改小。
這個方向不是隨意選的：留在佇列裡的那半段位址沒變，佇列的前後指標不必改；如果從頭切，
被切走的那一半是佇列節點本身，還得把節點搬過去。

實測看得很清楚：放掉一塊 64 bytes、再要 16 bytes，新區塊落在空洞起點 +48 的位置。

### `realloc` 放大為什麼不就地擴張

BC++ 2.0 的 `realloc` 放大時**不看隔壁**：直接 `malloc` 一塊新的、把舊資料複製過去、`free` 舊的。
縮小才是就地處理——把尾巴切成一塊 free 區塊，如果縮的正好是 heap 最後一塊，還會把 break level 降回去。

這是拿程式碼大小換執行效率的取捨：就地擴張要判斷實體下一塊是不是 free、夠不夠大、合併之後要不要再切，
每一條路徑都是程式碼，而 1991 年一支 DOS 程式的整個 RTL 預算才幾十 KB。

對 remake 的意義很直接：**`realloc` 之後任何指向舊區塊的指標都失效**，包括存在結構裡的自己人。

### 與堆疊的那 512 bytes

near heap 要長高時呼叫 `__sbrk`，它會檢查新的 break level 是否已經逼近**當下的 `SP`**，
差距不足 `MARGIN`（512 bytes）就失敗，回 −1 並把 `errno` 設成 `ENOMEM`。

<p align="center"><img src="../../img/borland-heap-stack-margin.svg" width="900" alt="DS 那 64 KB 的佈局：靜態資料在低位址、near heap 往上長、堆疊從高位址往下長，中間 512 bytes 的護欄只在 heap 要長高時檢查；far heap 則在程式的 DOS 記憶體區尾端，靠 setblock 以 1 KB 為級距擴張"></p>

這條護欄有兩個性質值得記住：

- **它只在 heap 這一側檢查。** 堆疊往下長沒有人檢查，遞迴太深就直接踩進 heap 的資料，
  而且不會當場出錯——壞掉的是之後某次 `malloc` 走到被改掉的區塊頭。
- **它用的是呼叫當下的 `SP`。** 在很深的呼叫層次裡 `malloc`，可用空間就比在 `main` 裡少一截。

`coreleft` 在 near 資料模型算的正是這段距離：`SP` 減 break level，再扣掉 512＋32
（多留 32 給 `malloc` 自己會用掉的堆疊），最後無條件捨去到 16 的倍數。
far 資料模型的 `coreleft` 轉呼叫 `farcoreleft`，算的是 `_heaptop − _brklvl − 16`，
也就是**已配置區塊以上**的空間——heap 內部那些放掉的區塊不算在裡面。
兩個數字量的是不同的東西，不能互相比較。

### far heap 怎麼向 DOS 要空間

far heap 的 `_sbrk` 最後會呼叫 DOS 的「調整記憶體區大小」服務（`setblock`，`int 21h AH=4Ah`），
改的是**程式自己那塊 MCB**。兩個細節是當年的省成本設計：

- **以 1 KB 為級距**：要的段落數先進位到 64 段落的倍數，避免每次都去問 DOS。
- **記住上次的大小**：這次算出來的級距與上次相同就只改記錄、不呼叫 DOS。

`_brk`／`_sbrk` 另外先擋掉超出 `_heapbase`、`_heaptop` 的請求，這兩個界線由啟動碼 C0 在
`main` 之前算好（依 `__stklen`、`__heaplen`），見[啟動與結束鏈](startup-and-exit.md)。

## 在執行檔裡怎麼認

| 看到 | 推論 |
|---|---|
| 三個相鄰的字，被一群互相呼叫的小函式讀寫，其中一個只在配置路徑上更新 | heap 的「第一塊／最後一塊／漫遊指標」；那群函式就是配置器 |
| 一個函式把參數減 4 之後才開始處理，接著比對一個全域字 | `free`；減掉的是已配置區塊頭，比對的是「最後一塊」 |
| 配置路徑上出現「加 5 再 `and 0FFFEh`」，以及與 8 比大小 | near heap 的 `malloc`：算區塊大小與最小區塊下限 |
| 讀出大小欄位之後 `dec`，或對大小欄位 `inc` | near heap 的「加 1 表示在用中」 |
| 某個函式取 `SP`、減一個 512 之類的常數、與一個全域字比較 | `__brk`／`__sbrk` 的護欄 |
| 程式碼段裡夾著三個資料字，且被 `cs:` 前綴存取 | far heap 的管理變數（它們住在 `_TEXT`） |
| `int 21h` 搭 `AH=4Ah`，段值從一個全域值算出來 | far heap 在擴張自己的 MCB |

容易誤判的地方：

- **near heap 與 far heap 的 `malloc` 長得完全不同**，先判斷記憶體模型再比對，
  判斷方式見[從執行檔判斷 runtime 廠牌、版本與記憶體模型](../60-re-fingerprints/identify-vendor-and-model.md)。
- **`farmalloc` 在所有模型都可以呼叫**，所以 small 模型的程式裡同時出現兩套配置器是正常的。
- 這一節的特徵是從機制推得的，還沒有逐一在第三方程式上驗證。

## 給 remake 的行為規格

實測條件：`examples/heap`，五個記憶體模型，在 dosgolem 執行；記憶體設定固定。
near 資料模型指 small、medium，far 資料模型指 compact、large、huge。

### 配置與釋放

| 情況 | near 資料模型 | far 資料模型 |
|---|---|---|
| `malloc(n)` 佔掉的總空間 | `(n + 5) & ~1`，最少 8 bytes | 進位到段落，最少 16 bytes |
| `malloc(1)` 的相鄰間距 | 8 | 16 |
| `malloc(8)`／`malloc(9)`／`malloc(10)` | 12／14／14 | 16／16／16 |
| `malloc(0)` | NULL | NULL |
| 放掉一塊再要同樣大小 | 拿回同一個位址 | 拿回同一個位址 |
| 放掉中間一塊 64、再要 16 | 新位址 = 空洞起點 + 48 | 新位址 = 空洞起點 + 48 |
| 相鄰兩塊（各 64）都放掉再要 120 | 合併成功，新位址 = 第一塊 + 12 | 合併成功，新位址 = 第一塊 + 32 |
| `malloc(65000)` | NULL，`errno` = 8（`ENOMEM`） | **成功**（far heap 拿得到） |
| `calloc(30000, 4)`（乘積溢位） | NULL，**`errno` 不變** | NULL，`errno` 不變 |
| `calloc(40, 2)` | 成功且全部清零 | 成功且全部清零 |

### `realloc`

| 情況 | 結果 |
|---|---|
| 32 → 200（放大） | **指標改變**，內容完整複製過去 |
| 200 → 32（縮小） | 指標不變，內容保留 |
| 縮小之後吐出來的空間 | 可以再配出去 |
| `realloc(p, 0)` | 等於 `free(p)`，回 NULL |
| `realloc(NULL, n)` | 等於 `malloc(n)` |

### 檢查工具

| 呼叫 | 回傳 |
|---|---|
| `heapcheck()` | `_HEAPOK`（2） |
| `heapchecknode(在用中的區塊)` | `_USEDENTRY`（4） |
| `heapchecknode(已放掉的區塊)` | `_FREEENTRY`（3） |
| `heapwalk()` 走到底 | `_HEAPEND`（5） |
| `farheapwalk()` 走到底 | `_HEAPOK` 之外的結束碼，序列依段遞增 |

其他常數：`_HEAPEMPTY` 1、`_HEAPCORRUPT` −1、`_BADNODE` −2、`_BADVALUE` −3。

### far heap

| 情況 | 結果 |
|---|---|
| `farmalloc` 回傳位址的位移 | 4（區塊頭在前面） |
| `farmalloc(1000)` 讓 `farcoreleft` 少掉 | 1008（63 個段落，含區塊頭） |
| `farheapwalk` 的序列 | 依段遞增，每項給大小與「在用中」旗標 |
| `farfree` 之後 `farcoreleft` 回不回升 | **不保證**：只有放掉的是 heap 尾端才會；放掉的空間仍會被之後的 `farmalloc` 重用 |

### 量測上的兩個坑

- **far 指標不能直接相減**。`(char *)hi - (char *)lo` 對 far 指標只算位移，跨段就得出 0。
- **不要用 `FP_SEG` 相減或 huge 指標算術去量 far 區塊的距離**——兩種算法會給出彼此一致、
  但與實際相反的數字（本 repo 量到過 −4096 段與 −65536 bytes，而真實的序列是往上長、每塊 63 段落）。
  可靠的來源是 `farheapwalk` 走出來的序列，或自己把段與位移攤成線性位址。

## 證據與未知

| 結論 | 出處 | 等級 |
|---|---|---|
| near／far 兩套配置器、區塊頭欄位與單位、free 的判別方式 | RTL 的 `NEARHEAP.ASM`、`FARHEAP.ASM` 的資料結構說明與程式碼 | 已證實（原文） |
| 三個管理變數、free 佇列是雙向環狀、first fit 從漫遊指標起算 | 同上 | 已證實（原文） |
| 切割從尾端切、切割門檻是 free 區塊頭大小 | 同上 ＋ `examples/heap` 的 `split`、`coalesce` 實測 | 已證實（原文＋實測） |
| `realloc` 放大一律搬家、縮小就地切 | RTL 的 `NEARHEAP.ASM`（`realloc` 的兩個輔助函式） | 已證實（原文） |
| `MARGIN` 是 512、檢查用當下的 `SP`、失敗設 `ENOMEM` | RTL 的 `BRK.CAS` | 已證實（原文） |
| `coreleft` 的兩個公式 | RTL 的 `CORELEFT.CAS`、`FCORELFT.C` | 已證實（原文） |
| far heap 用 `setblock`、1 KB 級距、快取上次大小 | RTL 的 `FBRK.C` | 已證實（原文） |
| `_heapbase`／`_heaptop`／`_brklvl` 由 C0 設定 | 出貨的 `C0.ASM` | 已證實（原文） |
| heap 檢查工具的回傳常數 | 出貨的 `alloc.h` | 已證實（原文） |
| 行為規格各條 | `examples/heap` × 五個模型，在 dosgolem 執行 | 已證實（實測） |
| 辨識特徵 | 由機制推得，未逐一在第三方程式上驗證 | 強推論 |

未知：

- **`farfree` 之後 break level 什麼時候真的降回去**。原始碼裡「放掉最後一塊」的路徑有降 break level 的動作，
  但實測在 near 資料模型下 `farcoreleft` 沒有回升、far 資料模型下有，條件還沒對清楚。
- **`farmalloc` 對 64 KB 以上請求**怎麼處理（huge 指標的正規化）沒讀也沒測。
- `heapfillfree`、`heapcheckfree` 沒測；`heapcheck` 在 heap 真的壞掉時的偵測能力沒測
  （本 repo 沒有故意破壞區塊頭的測試）。
- `allocmem`／`freemem`（直接向 DOS 要整塊記憶體）與 far heap 的互動沒測。
- Windows 版的 heap 是不是同一套沒讀。
- Microsoft C 的對應做法要等 M3。
