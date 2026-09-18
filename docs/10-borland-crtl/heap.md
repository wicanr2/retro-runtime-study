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

8086 的位址由兩個 16 位元的數湊成：段（segment）與位移（offset），實際位址是段乘 16 再加位移。
`DS`（資料段暫存器）指到的那 64 KB 裡，位址只要存位移、2 bytes 就夠，存取一個指標省一半空間、
少一次段暫存器操作。可是一支遊戲要的資料常常超過 64 KB，那就得跨段定址，指標變成 4 bytes。

一套配置器沒辦法同時把兩件事做好：用 2 bytes 指標管 64 KB 以內的東西，和用 4 bytes 指標管整條 DOS 記憶體。
所以 Borland 寫了兩套，讓編譯期的記憶體模型決定編進哪一套。

### heap 與堆疊共用同一塊空間

near heap 長在 `DS` 裡，靜態資料的後面，位址由小往大長；堆疊從同一個 64 KB 的高位址端往小的方向長。
中間那塊空地兩邊都想要，而 8086 沒有任何硬體機制能在越界時通知程式。

### 函式庫自己的程式碼也要省

一支 1991 年的 DOS 程式，整個 RTL 連同程式碼通常只有幾十 KB 的預算。
配置器每多一條判斷路徑就多幾十個位元組，這個成本要跟它省下的記憶體放在一起算。

### 向 DOS 要記憶體很貴

DOS 的記憶體服務以段落為單位，而且一支程式啟動時通常已經拿到一整塊（連結器決定的大小）。
每要一次都是一次 `int 21h`，還要處理 MCB 鏈（Memory Control Block，DOS 拿來串所有已配置記憶體區塊的鏈狀結構）。
配置器不能每次 `malloc` 都去問 DOS。

## 推導

<p align="center"><img src="../../img/borland-heap-layouts.svg" width="920" alt="near heap 與 far heap 的區塊頭欄位對照：near 用 byte 為單位、size 加 1 表示在用中；far 用段落為單位、prev_real 為 0 表示 free，多一個 prev_real2 欄位；兩者的 free 區塊都串成雙向環狀佇列"></p>

### 區塊頭：把旗標藏在對齊的縫裡

每個區塊不論在用或空著都有一個頭。**在用中只需要兩個欄位**（大小、實體前一塊），空著時才需要多兩個
（free 佇列的前後）。near heap 的位址都在 64 KB 內，一個欄位就是一個 16 位元字（2 bytes），
所以已配置區塊的頭是 4 bytes、free 區塊的頭是 8 bytes——而這也決定了最小區塊：
一塊空著時至少要放得下 8 bytes 的頭。

「這塊在用嗎」沒有獨立的欄位。near heap 的區塊大小一律偶數（配置時捨入到偶數，公式見下方行為規格），
最低位元永遠是 0，配置器就拿它當旗標：
**大小欄位加 1 代表在用中**。

far heap 做不到同一招——它的大小是段落數，沒有保證偶數的位元可借。所以改成
**`prev_real` 欄位為 0 代表這塊是 free**，真正的「實體前一塊」搬到第五個欄位 `prev_real2`。
far heap 的 free 頭因此是 10 bytes。

| | near heap | far heap |
|---|---|---|
| 編進哪些模型 | tiny、small、medium | compact、large、huge |
| 單位 | byte | 段落（16 bytes） |
| 已配置區塊頭 | 4 bytes | 4 bytes |
| free 區塊頭 | 8 bytes | 10 bytes |
| 最小區塊 | 8 bytes | 16 bytes |
| 「在用中」怎麼表示 | 大小欄位 +1（大小恆為偶數） | `prev_real` 不為 0 |
| 管理變數放哪 | `DS`（`_DATA` 段） | **程式碼段**，省下切換 `DS` |
| 向下要空間的方式 | `__sbrk` 推高 break level | `_sbrk` → `setblock` 調整程式自己的 MCB |

### 三個字就管完一個 heap

兩套配置器都只用三個全域變數：第一塊、最後一塊、還有一個「漫遊指標」（rover）。
表格裡的 **break level** 指的是 heap 目前的邊界——配置器手上要到的空間到哪裡為止，
要更多就把它往外推。
free 區塊串成**雙向環狀佇列**，順序是邏輯的、與實體位置無關；只剩一塊 free 時，它的前後都指向自己。

`malloc` 從漫遊指標指的那塊開始沿佇列繞一圈，**第一個裝得下的就用**（first fit）。
為什麼不是「最合適的」？因為 best fit 要走完整條佇列，而 first fit 通常走幾步就找到。

漫遊指標**不是**「上次找到哪裡」的游標——原始碼自己把它註解成「指向任意一塊 free 區塊」。
它只在兩種時候移動：整塊被取走（從佇列移除）時指向鄰居，以及佇列從空變成非空時指向新進來的那塊。
**切割配置不動它**，所以連續配置小塊時每次都從同一塊開始找。它的作用是「佇列的把手」，不是分散壓力的游標。

### 切割從尾端切

找到的區塊比需要的大時，配置器要決定切不切。near heap 的門檻是**多出來的空間必須放得下一個 free 區塊頭**（8 bytes），
否則整塊給出去（使用者拿到比要求多一點的空間，這是內部碎片）。
far heap 沒有這個門檻：多一個段落就切，所以它不會把多餘空間送人。

真的要切時，新區塊是從**空洞的尾端**切出來的，前半段留在 free 佇列裡、大小改小。
這個方向不是隨意選的：留在佇列裡的那半段位址沒變，佇列的前後指標不必改；如果從頭切，
被切走的那一半是佇列節點本身，還得把節點搬過去。

實測看得很清楚：放掉一塊 64 bytes、再要 16 bytes，新區塊落在空洞起點 +48 的位置。

### `realloc` 放大為什麼不就地擴張

BC++ 2.0 的 `realloc` 先把新大小進位成區塊大小再跟舊區塊比，所以**同一個進位級距內的增減什麼都不做**
（`malloc(8)` 之後 `realloc(p, 7)` 指標不變、區塊不動）。真的變大時它**不看隔壁**：直接 `malloc` 一塊新的、
把舊資料複製過去、`free` 舊的。真的變小才就地處理——把尾巴切成一塊 free 區塊，
但**吐出來的部分要放得下一個 free 區塊頭才會切**，否則整塊不動；如果縮的正好是 heap 最後一塊，還會把 break level 降回去。

這是拿程式碼大小換執行效率的取捨：就地擴張要判斷實體下一塊是不是 free、夠不夠大、合併之後要不要再切，
每一條路徑都是程式碼，而 1991 年一支 DOS 程式的整個 RTL 預算才幾十 KB。

對 remake 的意義很直接：**`realloc` 之後任何指向舊區塊的指標都失效**，包括存在結構裡的自己人。

### `calloc` 的溢位是在 32 位元擋掉的

`calloc(n, size)` 先用 32 位元乘法算出總量，超過 64 KB 就直接回 NULL——**根本不呼叫 `malloc`**，
所以 `errno` 不會被設定。far 資料模型也一樣，因為 `size_t` 在五個模型下都是 16 位元。

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
far 資料模型的 `coreleft` 轉呼叫 `farcoreleft`，算的是 `_heaptop − _brklvl`，差值大於 16 時再扣 16，
最後同樣捨去到 16 的倍數。量的是 **break level 以上**的空間——heap 內部那些放掉的區塊不算在裡面。
兩個數字量的是不同的東西，不能互相比較。

### far heap 怎麼向 DOS 要空間

far heap 的 `_sbrk` 最後會呼叫 DOS 的「調整記憶體區大小」服務（`setblock`，`int 21h AH=4Ah`），
改的是**程式自己那塊 MCB**。兩個細節是當年的省成本設計：

- **以 1 KB 為級距**：把「PSP 到新的 break level」這一整塊的段落數進位到 64 段落的倍數（並夾在 `_heaptop` 以內），
  避免每次都去問 DOS。
- **記住上次的大小**：這次算出來的級距與上次相同就只改記錄、不呼叫 DOS。

`_brk`／`_sbrk` 另外先擋掉超出 `_heapbase`、`_heaptop` 的請求。這兩個界線由啟動碼 C0 在 `main` 之前設定：
`_heapbase` 由資料段大小算出來，`_heaptop` 來自 PSP 裡記的「這支程式能用到哪裡」，C0 同時把多餘的記憶體還給 DOS。
near heap 那邊不必算——它的 break level 初值是**連結時**就決定的靜態資料結尾。見[啟動與結束鏈](startup-and-exit.md)。

far heap 的起點在程式自己那塊 DOS 記憶體裡，緊接在 PSP（Program Segment Prefix，
DOS 為每支程式建立的描述區）與程式映像後面。

## 在執行檔裡怎麼認

| 看到 | 推論 |
|---|---|
| 三個相鄰的字，被一群互相呼叫的小函式讀寫；其中一個只在兩個地方被寫，寫進去的不是某個 heap 起點就是 0 | 那個字是「第一塊」（建立整個 heap 與 heap 被清空時才寫），三個字就是 heap 的管理變數 |
| 一個函式把參數減 4 之後才開始處理，接著比對一個全域字 | `free`；減掉的是已配置區塊頭，比對的是「最後一塊」 |
| 配置路徑上出現「加 5 再 `and 0FFFEh`」，以及與 8 比大小 | near heap 的 `malloc`：算區塊大小與最小區塊下限 |
| 讀出大小欄位之後 `dec`，或對大小欄位 `inc` | near heap 的「加 1 表示在用中」 |
| 某個函式取 `SP`、減一個 512 之類的常數、與一個全域字比較 | `__brk`／`__sbrk` 的護欄 |
| 程式碼段裡夾著三個資料字，且被 `cs:` 前綴存取 | far heap 的管理變數（它們住在 `_TEXT`） |
| `int 21h` 搭 `AH=4Ah`，段值從一個全域值算出來 | far heap 在擴張自己的 MCB |

容易誤判的地方：

- **「參數減 4 再比對一個全域字」不一定是 `free`**：`heapchecknode` 與 `heapwalk` 是同一個形狀，
  差別只在比對的是「第一塊」還是「最後一塊」。
- **「加 5 再 `and 0FFFEh`、與 8 比大小」在 `realloc` 裡也有一份**，不是 `malloc` 專屬。
- **`AH=4Ah` 不一定是 heap 在擴張**：啟動碼在 `main` 之前就用它把多餘記憶體還給 DOS。
- **512 這個常數太常見**（`BUFSIZ` 也是 512）。真正獨特的是 `coreleft` 用的 **544**（`512 + 32`），
  整個 RTL 只有那裡出現。
- far heap 有個幾乎不會誤判的特徵：把 `SS` 當暫存段暫存器用（`cli` → 改 `SS` → 操作 → 還原 → `sti`），
  正常程式不會這樣寫。
- **near heap 與 far heap 的 `malloc` 長得完全不同**，先判斷記憶體模型再比對，
  判斷方式見[從執行檔判斷 runtime 廠牌、版本與記憶體模型](../60-re-fingerprints/identify-vendor-and-model.md)。
- **`farmalloc` 在所有模型都可以呼叫**，所以 small 模型的程式裡同時出現兩套配置器是正常的。
- 這一節的特徵是從機制推得的，還沒有逐一在第三方程式上驗證。

## 給 remake 的行為規格

實測條件：`examples/heap`，五個記憶體模型，在 dosgolem 執行；記憶體設定固定。
near 資料模型指 small、medium，far 資料模型指 compact、large、huge。

**以下各列是同一支程式依序做出來的**，不是每列都從乾淨的 heap 重來。
位址類的結果（新區塊落在哪）不受影響，因為每組測試自己建立要觀察的狀態；
但像「放掉之後可用量回不回升」這種與 heap 尾端有關的數字會受前面的測試影響，該處另外註明。

### 配置與釋放

| 情況 | near 資料模型 | far 資料模型 |
|---|---|---|
| `malloc(n)` 佔掉的總空間 | `max(8, (n + 5) & ~1)`——`n` 很小時被下限蓋掉（`n` = 1 算出來是 6，實際佔 8） | 進位到段落，最少 16 bytes |
| `malloc(1)` 的相鄰間距 | 8 | 16 |
| `malloc(8)`／`malloc(9)`／`malloc(10)` | 12／14／14 | 16／16／16 |
| `malloc(0)` | NULL | NULL |
| `free(NULL)` | 靜默略過，heap 不受影響（`heapcheck` 仍回 `_HEAPOK`） | 同左 |
| 放掉一塊再要同樣大小 | 拿回同一個位址 | 拿回同一個位址 |
| 放掉中間一塊 64、再要 16 | 新位址 = 空洞起點 + 48 | 新位址 = 空洞起點 + 48 |
| 相鄰兩塊（各 64）都放掉再要 120 | 合併成功，新位址 = 第一塊 + 12 | 合併成功，新位址 = 第一塊 + 32 |
| `malloc(65000)` | NULL，`errno` = 8（`ENOMEM`） | **成功**（far heap 拿得到） |
| `calloc(30000, 4)`（乘積 120000 超過 64 KB） | NULL，**`errno` 不變** | NULL，`errno` 不變 |
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
| `farheapwalk()` 走到底 | `_HEAPEND`（5），序列依段遞增 |

其他常數：`_HEAPEMPTY` 1、`_HEAPCORRUPT` −1、`_BADNODE` −2、`_BADVALUE` −3。

### far heap

| 情況 | 結果 |
|---|---|
| `farmalloc` 回傳位址的位移 | 4（區塊頭在前面） |
| `farmalloc(1000)` 讓 `farcoreleft` 少掉 | 1008（63 個段落，含區塊頭） |
| `farheapwalk` 的序列 | 依段遞增，每項給大小與「在用中」旗標；**大小是整塊含標頭**（要 1000 bytes 的區塊會報 1008） |
| 在**全新的** far heap 上配一塊再放掉 | heap 清空（`farheapwalk` 回 `_HEAPEMPTY`），`farcoreleft` 完全回到原點 |
| 配兩塊再依序放掉 | **兩種資料模型結果相反**：near 資料模型下 `farcoreleft` 完全不回升、區塊留在 heap 裡；
  far 資料模型下逐塊回升，最後回到原點 |
| 在留著 free 區塊的 heap 上再配置 | 重用那塊空間，`farcoreleft` 不再減少 |

**唯一可以依賴的規格是：放掉的空間一定會被之後的 `farmalloc` 重用，但 `farcoreleft` 不保證反映它。**
remake 若拿 `farcoreleft` 當「還剩多少可以配」的依據會低估。
機制上只有兩條路徑會把 break level 降回去（放掉 heap 的最後一塊、`farrealloc` 縮小最後一塊），
而且降的動作可能靜默失敗——呼叫端不看回傳值。上面那個模型間的差異還沒對到原始碼的哪一條分支（見未知）。

### 量測上的兩個坑

- **far 指標不能直接相減**。`(char *)hi - (char *)lo` 對 far 指標只算位移，跨段就得出 0。
- **不要用 `FP_SEG` 相減或 huge 指標算術去量 far 區塊的距離**——兩種算法會給出彼此一致、
  但與實際相反的數字（本 repo 量到過 −4096 段與 −65536 bytes，而真實的序列是往上長、每塊 63 段落）。
  可靠的來源是 `farheapwalk` 走出來的序列，或自己把段與位移攤成線性位址。

## 證據與未知

| 結論 | 出處 | 等級 |
|---|---|---|
| 本篇引用的 12 個 RTL 模組（兩套配置器、兩套檢查工具、`brk`、`coreleft`、`calloc` 等）重編後與 1991-04、1991-08 兩個出貨庫逐位元組相同 | 以原廠工具鏈重編的對拍 | 已證實（對拍） |
| near／far 兩套配置器、區塊頭欄位與單位、free 的判別方式 | RTL 的 `NEARHEAP.ASM`、`FARHEAP.ASM` 的資料結構說明與程式碼 | 已證實（對拍） |
| 三個管理變數、free 佇列是雙向環狀、first fit 從漫遊指標起算、漫遊指標不隨切割移動 | 同上 | 已證實（對拍） |
| 切割從尾端切、切割門檻是 free 區塊頭大小 | 同上 ＋ `examples/heap` 的 `split`、`coalesce` 實測 | 已證實（原文＋實測） |
| `realloc` 的三個分支（同級距不動／放大搬家／縮小就地切）與各自的門檻 | RTL 的 `NEARHEAP.ASM`、`FARHEAP.ASM` | 已證實（對拍） |
| `MARGIN` 是 512、檢查用當下的 `SP`、失敗設 `ENOMEM`（**只有 near 資料模型**；far 的 `brk` 沒有任何 `SP` 檢查） | RTL 的 `BRK.CAS` | 已證實（對拍） |
| `coreleft` 的兩個公式 | RTL 的 `CORELEFT.CAS`、`FCORELFT.C` | 已證實（原文） |
| far heap 用 `setblock`、1 KB 級距、快取上次大小 | RTL 的 `FBRK.C` | 已證實（原文） |
| `_heapbase`／`_heaptop`／`_brklvl` 由 C0 設定 | 出貨的 `C0.ASM` | 已證實（原文） |
| heap 檢查工具的回傳常數 | 出貨的 `alloc.h` | 已證實（原文） |
| 行為規格各條 | `examples/heap` × 五個模型，在 dosgolem 執行 | 已證實（實測） |
| 辨識特徵 | 由機制推得，未逐一在第三方程式上驗證 | 強推論 |

未知：

- **兩種資料模型下「配兩塊再依序放掉」的結果相反**，還沒對到原始碼的哪一條分支。
  已知的機制邊界是：只有「放掉 heap 最後一塊」與「`farrealloc` 縮小最後一塊」兩條路徑會呼叫降 break level 的函式，
  而那個函式在三種情況下會**靜默失敗**（目標低於 heap 起點、高於 heap 上限、或 DOS 的調整記憶體服務回報失敗），
  呼叫端不檢查回傳值。要定案得再跑一次帶儀器的實驗（把每次釋放前後的 break level 與 heap 上限印出來）。
- **`farheapwalk` 的「在用中」旗標對 heap 第一塊的語意沒確認**（它的判準是「實體前一塊」欄位非 0，
  而第一塊本來就沒有前一塊）。本文因此不引用單塊層級的 free／used 計數。
- **`farmalloc` 的上限**是請求加 19 個位元組之後不超過 1 MB（段落數放在一個 16 位元欄位），
  也就是單塊可以超過 64 KB；但 RTL 不替呼叫端正規化指標（回傳恆為 `段:0004`），跨 64 KB 存取要自己處理。
  這條是讀原始碼得到的，沒有實測。
- `heapfillfree`、`heapcheckfree` 的機制已讀（沿 free 佇列逐塊填或比對資料區），但沒有實跑。
  `heapcheck` 在 heap 真的壞掉時的偵測能力也沒測——它比對實體串列與 free 佇列兩邊的 free 總量，
  抓不到「只把資料區寫壞」與「大小欄位被改成仍然自洽的值」。
- `allocmem`／`freemem` 直接向 DOS 另外要一塊記憶體，與 far heap 的資料結構無關；
  但它會佔掉 far heap 之後想擴張時需要的相鄰空間。這條沒有實測。
- Windows 版的 heap 是不是同一套沒讀。
- Microsoft C 的對應做法要等 M3。
