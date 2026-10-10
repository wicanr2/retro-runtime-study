---
id: msvc2-crt/heap
title: 堆：描述子鏈加 region
libraries: [msvc-2.0-crt]
goals: [craft, re]
evidence: 已證實
triggers:
  - malloc 在 Win32 底下到底向誰要記憶體（不是 brk 也不是段）
  - 為什麼 free 非法指標會整批 abort 而不是悄悄壞掉
  - realloc 什麼時候原地長大、什麼時候搬家
  - calloc 的乘法溢出在這版有沒有檢查
  - new handler（_set_new_handler）的重試迴圈長什麼樣
symbols: [_nh_malloc, _malloc_lk, _heap_search, _heap_grow, _heap_split_block, _heap_expand_block, _heap_abort, _HEAP_MAXREQ, _heap_regions, rover, _pnhHeap]
related: [msvc2-crt/startup, msvc-crt/library-combination, borland-crtl/heap]
---

# 堆：描述子鏈加 region

## 結論

VC++ 2.0 的堆跟 1.0 的 DOS 段堆是完全不同的本體：
Win32 沒有段了，記憶體來自 `VirtualAlloc` 的頁
（向 OS 要整頁的系統呼叫，4K 起跳），
CRT 在頁上建了一套「描述子鏈加 region」的分配器。
每個記憶體塊由一個 8 位元組的描述子管理
（下一塊指標加帶 2 位狀態的塊位址），
塊頭再藏一個 4 位元組回指——`free` 靠回指驗證指標真偽，
對不上就整批 `abort`，不讓堆靜悄悄爛掉。

分配走固定管線：超過上限（`0xFFFFD000`）直接拒 →
大小無條件進位到 4 的倍數 →
從上次停留點（rover）找第一個夠大的（首適），
邊搜邊合併閒塊 → 搜不到就長 region 再搜一次
（長失敗走 handler 迴圈，不 abort；
只有長成功還搜不到才 `abort`）→
太大就切尾 → rover 指到下一塊。
切分需要一個空描述子；描述子池空了先長頁，
長不出來整塊給出去（尾部浪費掉）。

搜尋從 rover 開始環一圈，是嚴格首適：
第一個夠大的就拿，不找更好的。
`free` 不合併（合併發生在下次搜尋路過時），
只做兩件事：標閒，外加 rover 啟發式
（夠大的閒塊把 rover 拉回來）。

記憶體分兩層：region 是向系統預訂的位址窗
（保留時不吃實體記憶體），commit 才變成堆可用的頁。
region 大小倍增成長：小窗 16K 起、256K 頂，
大窗 1M 起、16M 頂；小窗只給 Win32s 這類
「保留等於配置」的環境用（詳 region 節）。
最多 64 個 region，表滿了就配不出來。

`realloc` 原位優先：自己夠大、
或吞掉後面全部連續閒塊就夠，就不搬；
否則配新塊、拷貝、釋放舊塊。
`calloc` 的 `num × size` 沒有溢出檢查，
乘爆了就按包覆後的大小配。
`new／delete` 直接走同一條路
（`new` 就是開了 handler 的 `malloc`）。

## 根本問題

堆要解決的是「一塊連續記憶體切碎了用」。
DOS 版靠段暫存器長大（向 DOS 要段），
Win32 版只能向 `VirtualAlloc` 要頁，
而且有三個新約束。第一，頁是 4K 起跳的，
幾十位元組的小配額不可能每次都找系統要，
中間必須有一層切分與回收。第二，
程式是多線程的，整條管線要能鎖，
而且 `new` 失敗時 C++ 規矩是先調 handler
搶救、不是直接回空。第三，
還給系統的只有整頁（decommit），
「堆縮小」只能發生在 region 尾巴，
中間的洞再大也還不回去——這決定了
`_heapmin` 只能從尾巴下手。

## 推導

### 描述子：8 位元組管一塊

<p align="center"><img src="../../img/msvc2-heap-blocks.svg" width="640" alt="描述子鏈：next 指標、帶狀態的塊位址、塊頭回指、rover"></p>

描述子只有兩個欄：下一塊的描述子指標、
這一塊的起始位址。狀態偷藏在位址的低 2 位
（塊位址保證 4 對齊，低 2 位本來就是零）：
0 是使用中，1 是閒置，2 是啞塊
（佔位不斷鏈、搜尋跳過，詳健檢節）。
塊大小不存：下一塊位址減這一塊位址、
再扣掉塊頭，就是可用大小——
所以描述子鏈的順序必須跟位址順序一致，
亂序整條鏈的算式全錯。

使用者拿到的指標不是塊頭，而是塊頭加 4。
塊頭那 4 個位元組藏著回指（指回自己的描述子）。
`_msize` 就是讀回指、套算式，
連搜尋都不用。`free` 先驗回指：
回指指的描述子、其塊位址加頭等於傳入指標，
才算合法；否則 `_heap_abort`。
這是整套堆唯一的防呆，代價是每塊多 4 位元組、
每次 `free` 多一次驗證。

堆頭管四樣：鏈頭、rover、空描述子池、
尾哨兵（鏈尾終點標記，不存使用者資料）。
描述子本身存在另外配的頁裡
（描述子頁鏈），跟使用者記憶體分開——
使用者寫穿了也只會壞到回指驗證那關，
描述子池本身碰不到（除非連回指一起偽造對，
那是另一回事）。

### malloc 管線：拒、圓、搜、長、切

`malloc` 本體只有一行：轉調 `_nh_malloc(size, _newmode)`。
真正的管線在 `_malloc_lk`（MT 版加鎖調它，
ST 版就是它），順序寫死：

1. **拒**：超過 `_HEAP_MAXREQ`（`0xFFFFD000`，
   約 4G 減 12K）直接回空，連 handler 都不調。
2. **圓**：大小 rounding 到 `_GRANULARITY`
  （x86 是 4，MIPS 是 8）的倍數。
3. **搜**：`_heap_search` 找夠大的閒塊。
4. **長**：搜不到就 `_heap_grow` 長 region。
   長成功就再搜一次，還搜不到才 `_heap_abort`
   （註解寫「不該發生、發生了就是大事壞了」）；
   長失敗（表滿或系統不給）不 abort，
   回空走外面的 handler 迴圈。
5. **切**：找到的塊太大就一切為二，
   尾部標閒。切分要消耗一個空描述子；
   池子空了先長描述子頁（一頁一頁長），
   長不出來才不切、原塊整塊給。
6. **rover 前進**：rover 指到配出塊的下一塊。

重試迴圈包在外面（`_nh_malloc`）：
配出來、或沒開 handler 模式、或沒裝 handler、
或 handler 回 0，就結束；否則無窮重試。
MT 版與 ST 版各寫一遍同樣的迴圈
（`#ifdef` 雙寫，不是共用）。

### 搜尋：rover 首適，邊走邊併

搜尋從 rover 開始往後掃到哨兵，
沒找到再從鏈頭掃到 rover，
是環一圈的首適。路過閒塊就看大小：
夠大拿走；不夠大就看下一塊——
下一塊也閒就併進來（被併掉的描述子丟回空池），
繼續比，直到夠大或撞上使用中的塊。

合併只在搜尋時發生。`free` 本人不合併，
連鄰居都不看。這表示剛 `free` 的大洞，
要等下次搜尋路過才會跟鄰居連起來；
好處是 `free` 永遠 O(1)，壞處是碎片要等一輪。

有個特判處理「rover 被併掉」：
第二圈（鏈頭到 rover）合併時如果吞掉了 rover 指的那塊，
rover 改指合併後的大塊，搜尋直接結束
（夠大就拿，不夠也算了——環已經走完一圈，
無處可再找，回去也是重複）。

`_heapmin` 開頭調一次「大小為 -1 的搜尋」：
任何塊都不夠大，於是搜尋被迫走完整圈、
把所有能併的全併了——拿搜尋函式當全併工具用。

### region：保留是窗，commit 才是肉

<p align="center"><img src="../../img/msvc2-heap-region.svg" width="640" alt="region 表：reserve 保留、commit 提交、倍增成長、64 上限"></p>

region 表最多 64 項，每項記基址、已提交、已保留。
成長請求先加塊頭、圓到頁邊界，
然後掃表找「保留減已提交夠大」的既有 region，
有就 commit 一段；都沒有才開新 region。

新 region 先 `VirtualAlloc(MEM_RESERVE)` 佔位址窗
（不吃實體記憶體），再 commit。
commit 量按 `_heap_growsize`（預設 64K）的倍數向上取，
不是恰好本次請求（窗內剩的不夠一整份才取剩餘）。
窗大小每次倍增：小檔 16K 起 256K 頂，
大檔 1M 起 16M 頂；請求比窗大就按請求開。
commit 失敗就把剛佔的窗整個釋放掉、回失敗。

`_heap_init` 只做一件事：選大小檔。
預設大檔；`_osver` 高位元置且 Windows 主版小於 4
（Win32s 或 Phar Lap TNT——保留等於配置、
佔大窗會吃掉實體記憶體的環境）才切小檔。
堆的第一次配置發生在這之後，
第一次 `_heap_grow` 才真正向系統要頁。

`_heapmin` 是成長的逆操作：全併之後，
逐 region 看尾巴的閒塊，能整頁還的就 decommit；
描述子頁鏈不管（描述子不還）。
`_heap_term` 更徹底：逐 region decommit 加釋放、
描述子頁全丟——調完之後堆上所有指標全失效。

### free：驗證加 rover 啟發式

`free(NULL)` 直接回（ANSI 規矩）。
非空就讀回指、驗算式，錯了 `abort`；
對了標閒，然後看 rover 要不要回拉：

- 新閒塊達到 `_heap_resetsize` 且在 rover 前面，
  rover 拉回來——下次搜尋從這個大洞開始。
  （`_heap_resetsize` 初值是 `0xffffffff`，
  除非有人調 `_heap_param` 改它，
  否則這條永遠不觸發。）
- 另有一條「後繼比較」：字面上比的是
  rover 描述子指標與下一塊的**塊位址**——
  描述子住描述子頁、塊住 region，
  兩邊位址相等幾乎不可能（除非刻意佈置），
  所以這條實務上永假，但它沒有開關：
  預設組態下 `free` 每次都比，只是比不中。
  為什麼這樣寫，原因不明（見證據節）。

### realloc、calloc、expand、msize

`realloc` 先處理退化：空指標轉 `malloc`，
大小零轉 `free`（回空），超上限拒。
正常路徑先看原位：自己夠大就留
（多太多還切尾）；不夠就吞後面全部連續閒塊
（迴圈併，吞掉的描述子回池，撞上 rover 還順手搬 rover），
併完夠就留；
都失敗才配新、拷、釋舊。
`_expand` 是「不准搬」的版本：
原位能長就長；原位不夠、但自己是 region 尾塊
（後鄰是哨兵或啞塊），就試著 commit 一段再長一次；
再不夠才回空。
上限用截的（超過改成上限）而不是拒。

`calloc` 先 `num *= size`，**沒有溢出檢查**：
乘爆了就按包覆值配，後面清零也只清包覆後的量。
清零按 `size_t` 一個字一個字寫
（註解寫明假設了塊大小是字長的倍數、
且零就是全零位元組）。

`_msize` 讀回指套算式直回，
MT 與 ST 各一版（鎖不鎖的差別）。

### new handler：C++ 的搶救迴圈

`operator new` 就是 `_nh_malloc(size, 1)`，
`delete` 就是 `free`。
handler 存在全域 `_pnhHeap`，
`_set_new_handler` 裝、`_query_new_handler` 查；
`_set_new_mode` 只准設 0 或 1（別的值直接拒）。
`malloc` 傳的是全域 `_newmode`，
所以 C 的 `malloc` 開了 `_newmode` 也會調 handler——
C 與 C++ 共用同一個搶救迴圈。

迴圈協議：handler 回 0 表示放棄（回空），
回非零表示「再試一次」。
handler 裡通常是釋放一些快取再回非零；
什麼都不做就回非零等於無窮迴圈，
原始碼不擋。

### 健檢、漫步與死檔

`_heapchk／_heapset` 共用 `_heap_checkset`，
八處檢查：哨兵尾空、堆空、首描述子空、
塊位址在範圍內、鏈序遞增、回指對得上、
rover 恰出現一次、空池不成環。
回五種碼：正常、堆空、頭壞、節點壞、
壞指標（回指驗失敗與空池成環走這碼）；
印錯巨集是空的（`_PRINTERR` 什麼都不做），
只回碼不印字。
`_heapwalk` 從鏈頭逐塊走，跳過啞塊，
`_pentry` 續走；`_heapused` 交出三個數：
使用中（含描述子頁）、已提交合計（閒加用）、
保留總量（回傳值）——沒有單獨的「閒置」輸出。

啞塊（`DUMMY`）是佔位符：
`_heapadd` 接使用者自備記憶體的接縫、
`_heapmin` 還頁後的殘留會設啞塊。
搜尋只認閒塊，漫步明寫跳過，
所以啞塊不影響分配，只佔著位址不斷鏈。

`H／WINHEAP.H`（Win32 堆包裝的標頭）在全庫零引用，
只有 `README` 的檔案清單提到它——死檔。

## 在執行檔裡怎麼認

以下全是從原始碼推導的靜態特徵，
封存裡沒有堆的出貨 `.OBJ` 可以逐位元組對，
證據等級見證據節。

- **`malloc` 轉調**：`malloc` 本體是跳板，
  實事在 `_nh_malloc(size, _newmode)`；
  看到取全域 `_newmode` 當第二參數的就是它。
- **上限常數**：`0xFFFFD000` 與請求大小的比較，
  超過直接回空不調 handler。
- **region 常數**：`0x4000／0x40000`（小檔）
  或 `0x100000／0x1000000`（大檔）的初始／上限、
  `_osver & 0x8000` 加 `_winmajor < 4` 的大小檔選擇。
- **rover 環搜**：從全域 rover 出發、
  到哨兵回頭再到 rover 的雙迴圈，
  圈內帶合併（下一閒併入、描述子回池）。
- **回指驗證**：`free` 入口讀 `ptr-4`、
  比對後不等就走 abort 路。
- **`VirtualAlloc` 序列**：`MEM_RESERVE` 佔窗在先、
  commit 在後，窗大小倍增。

## 給 remake 的行為規格

以下全部來自原始碼直接寫明的介面與順序，
沒有實跑支持（無 Win32 工具鏈）。
命名與順序都與原檔無關：

```text
function spec_malloc(size):
    if size > 0xFFFFD000: return NULL        # 連 handler 都不調
    size = round_up(size, 4)
    loop:                                   # _nh_malloc 重試迴圈（管線節 4）
        blk = search(size)                     # rover 首適，邊搜邊併
        if blk is NULL and grow(size) == OK:
            blk = search(size)                 # 再搜
            if blk is NULL: abort()            # 長完還沒有＝大事壞了
        if blk is NULL:                        # 只有 grow 失敗才到這
            if handler 沒開／沒裝／回 0: return NULL
            else: continue                     # 重試
        if blk.size > size and empty_desc():
            split(blk, size)                   # 池空就不切，整塊給
        mark_inuse(blk); rover = blk.next
        return user_ptr(blk)

function spec_free(p):
    if p is NULL: return
    if backptr(p) invalid: abort()
    mark_free(p)
    # 預設不動 rover（resetsize 初值＝關閉）

function spec_realloc(p, size):
    if p is NULL: return spec_malloc(size)
    if size == 0: spec_free(p); return NULL
    if size > 0xFFFFD000: return NULL
    if fits_in_place(p, size): return p        # 含吞全部連續閒塊、切尾
                                                  # （realloc 節三路）
    newp = spec_malloc(size)
    if newp is NULL: return NULL
    memcpy(newp, p, oldsize); spec_free(p)
    return newp

function spec_calloc(num, size):
    total = (num * size) mod 2^32              # 無溢出檢查，包覆
    p = spec_malloc(total)
    if p: zero_by_words(p, total)
    return p
```

表一：常數。

| 常數 | 值 | 意思 |
|---|---|---|
| `_HEAP_MAXREQ` | `0xFFFFD000` | 單次請求上限，超了直接空 |
| 小檔 region | 16K 起、256K 頂 | Win32s／Phar Lap 用 |
| 大檔 region | 1M 起、16M 頂 | 預設 |
| region 上限 | 64 個 | 表滿配不出 |
| `_GRANULARITY` | 4（MIPS 8） | 大小 rounding 單位 |
| 塊頭 | 4 位元組 | 回指 |

表二：失敗行為。

| 情況 | 行為 |
|---|---|
| 超上限 | 回空，不調 handler |
| 搜不到且長不出 | handler 迴圈（沒開就回空） |
| 長完還搜不到 | `abort` |
| `free` 非法指標 | `abort` |
| 描述子池空且長不出頁 | 不切分，整塊給（靜默浪費尾部） |
| handler 回非零但沒釋放 | 無窮重試（不擋） |
| `calloc` 乘爆 | 按包覆值配 |

## 證據與未知

- 描述子結構、狀態偷位、回指、堆頭四件：
  標頭與原始碼直接寫明，**已證實（原文）**。
- malloc 六步管線（含長失敗走 handler、
  只有長成功再搜不到才 abort）、重試迴圈 MT／ST 雙寫、
  池空先長頁、rover 前進：
  原始碼直接寫明，**已證實（原文）**。
- rover 首適環搜、邊搜邊併、rover 被併特判、
  `_heapmin` 以 -1 全併：原始碼直接寫明，
  **已證實（原文）**。
- region 保留／提交兩層、倍增、64 上限、
  大小檔選擇條件、commit 按 growsize 倍數：
  原始碼直接寫明，**已證實（原文）**。
- `free` 回指驗證、`free(NULL)`、resetsize 預設關閉：
  原始碼直接寫明，**已證實（原文）**。
- `realloc` 原位／吞全部連續閒塊／搬家三路、
  `_expand` 尾塊可 commit 再長且截頂、
  `_msize` 直回：原始碼直接寫明，
  **已證實（原文）**。
- `calloc` 無溢出檢查、按字清零：
  原始碼直接寫明（乘完直接用），**已證實（原文）**；
  包覆是 C 語意的心算，屬**強推論**。
- new handler 協議、`_newmode` 只准 0／1、
  C／C++ 共迴圈：原始碼直接寫明，
  **已證實（原文）**。
- 健檢八處五碼（壞指標兩處來歷）、
  `_PRINTERR` 空巨集、漫步跳啞、
  啞塊兩處來歷、`_heapused` 三數語意：
  原始碼直接寫明，**已證實（原文）**。
- `WINHEAP.H` 死檔：全庫零引用，
  **已證實（原文）**。
- `free` 第二啟發式的字面比法
  （描述子指標比塊位址）：原始碼如此寫，
  **已證實（原文）**；為什麼這樣比，
  **未知**（看起來像筆誤，但沒有第二來源佐證，
  不下斷言）。
- 在執行檔裡怎麼認的六條：從原始碼推導，
  封存裡沒有堆的出貨 `.OBJ` 對位元組，
  **強推論**。
- `_heap_abort` 就是 `_amsg_exit(_RT_HEAP)`、
  描述子頁一次一頁（`_HEAP_EMPTYLIST_SIZE`）：
  原始碼直接寫明，**已證實（原文）**。
- 未知：`_heap_addblock` 三描述子的配置細節
  （沒逐行讀）、`_heap_param` 兩參數的合法範圍。

出處：Visual C++ 2.0 CRT，`HEAP` 目錄的
`MALLOC.C／FREE.C／REALLOC.C／CALLOC.C／HEAPSRCH.C／
HEAPGROW.C／HEAPINIT.C／HEAPMIN.C／HEAPADD.C／HEAPCHK.C／
HEAPWALK.C／MSIZE.C／NEW.CXX／HANDLER.CXX` 與
`H` 目錄的 `HEAP.H`；
`I386` 目錄只有輔助函式的出貨 `.OBJ`，
沒有堆的可對位元組。
首段與 1.0 的比較以後者 `VCCRT1／HEAP` 為準
（near／far／based／huge 四套段式堆）。
