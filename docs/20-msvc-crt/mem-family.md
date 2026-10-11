---
id: msvc-crt/mem-family
title: mem 家族：記憶體塊的複製、比較、搜尋与填寫
libraries: [msvc-1.0-crt]
goals: [craft, re, oracle]
evidence: 已證實
triggers:
  - 反組譯 16 位元程式，看到 shr／rep movsw／adc／rep movsb 四件套，想確認是不是 mem 函式
  - memmove 的重疊判斷与倒搬路徑，想看前進倒搬怎麼分家
  - far 版跨段搬運的 MIN 分塊与段步進，想知道上界寫在哪
  - 同一份源碼編出多個庫成員（memcpy／memmove 同源），想看機械怎麼分
symbols: [_memcpy, _memmove, __memccpy, _memchr, _memcmp, __memicmp, _memset, __AHINCR, rep movsw, sbb]
related: [msvc-crt/strcpy-family, msvc-crt/strcmp-family]
---

# `mem` 家族：記憶體塊的複製、比較、搜尋与填寫

一句話：Visual C++ 1.0 的七個記憶體塊函式共用一套字組搬運骨架
（奇位址先搬一 byte、`shr` 取字組數、`rep movsw`、`adc` 收尾），
`memmove` 用重疊判斷分前進倒搬兩路徑，
far 版用無分支 `MIN` 分塊加段步進跨段，
比較函式用 `sbb` 對子无分支取正負。
延伸閱讀：字串複製見
[`strcpy` 家族](strcpy-family.md)，
字串比較見
[`strcmp` 家族](strcmp-family.md)。

## 結論

七個函式全是「給起點加長度」：沒有結束符，
`count` 就是唯一的邊界。
`memcpy／memmove` 同一份源碼，
靠 `MEM_MOVE` 開關編出兩件庫成員；
far-data 模型（compact／large／huge）
另有 `hmemcpy` 源，靠 `MEM_MOVE／MEM_CCPY`
一源三件；大小寫不敏感比較另有
`CASE_INSENSITIVE` 開關。
同源多件在庫裡靠檔名後綴拆件
（`MEMCPY` 与 `MEMCPY~2` 是不同函式——
名為 `MEMCPY` 的成員裝的是 `memmove`）。

搬運骨架七支共用（`memccpy／memicmp` 系逐 byte
比對故除外）：`test` 目的奇位址先 `movsb` 一 byte、
`dec` 計數、`shr` 取字組數（餘數進 `CF`）、
`rep movsw`、`adc cx,cx` 把 `CF` 變回 0 或 1、
`rep movsb` 收尾。
只查目的對齊、不查來源——
四種對齊組合只加速「雙奇」一種，
是刻意取捨（註解自述），不是疏漏。

`memmove` 的重疊判斷只問一件事：
目的起點有沒有落在「來源區間內且在來源起點之後」。
near 版：`cmp di,si／jbe` 先擋掉目的在前，
再比 `dst>=src+count` 擋掉首尾相接；
兩關都過才倒搬。
far 版：把差值交給外部 helper
（`ahdiff`，回傳 `DX:AX` 有號差）
再做同式判斷。
`dst==src` 与首尾相接都走前進——
无實質重疊，前進正確。
倒搬是 `SI／DI+=count-1` 指到尾、
`std` 借方向旗標、`rep movsb`、`cld` 必還、
跳過前進段直出。

far 版前進是分塊迴圈：
每輪 `Num=MIN(Count,65536-SI,65536-DI)`，
搬完 `SI／DI==0` 才把 `DS／ES` 加一段
（`__AHINCR`＝4096 個段落單位）；
倒搬是 `Num=MIN(Count,SI+1,DI+1)`，
`SI／DI==FFFFh` 就減一段。
`MIN` 用 `sub／sbb／and／add` 無分支惯用法，
`memcmp` 系的 −1／＋1 用 `sbb ax,ax／sbb ax,-1`
（`1B C0 83 D8 FF`，`CY=1→−1`），far `memicmp` 的大小寫映射
用 `sub／cmp／sbb／and／add` 無分支轉小寫——
near `memicmp` 卻是雙 `cmp／ja` 跳轉式，
与 `cProc` 保存风格等三處差異指向不同作者。

零計數多半 `jcxz` 直出；
far `memmove` 卻用七位元組三跳
（`or／jnz／jmp`，語意等價，動機不明）。
回傳值多半從堆疊重讀目的起點
（經 `SS` 取址，`DS／ES／SI／DI` 已髒無妨）；
`memccpy` 命中回字元之後、走完回 `NULL`，
`memchr` 命中回命中位址（near 版 `AX=DI-1`、
far 版 `ES:DI-1`）、否則 `NULL`。
near 源裡的 `MODELINDEP`（`_f*` 名）分支
六檔全死（出貨 89 庫零命中）——
出貨的 far 名全來自 `hmem*` 源；
与 `str*` 系同結構卻活著正好對讀。

## 根本問題

記憶體塊操作有四個現實約束。
第一，没有結束符：字串函式量長靠掃 `NUL`，
`mem` 系只能信 `count`——
零計數、奇計數、奇位址全是呼叫端传進來的形狀，
碼裡一個都不能假設。
第二，重疊：`memcpy` 契約不管重疊，
`memmove` 必須處理——
目的在來源之後且重疊時前進會先蓋掉還沒搬的來源，
只能倒搬；判斷本身必須廉價（兩次比較）。
第三，對齊：8086 字組搬運在偶位址最快，
但來源与目的各奇各偶有四種組合——
全查要四路分派，只查目的只要一次 `test`。
第四，分段：far 指標 16 位元位移隨時到段尾，
一段 64K 的塊可能橫跨兩段——
near 版假裝段夠大，far 版必須分塊加步進，
而 `count` 16 位元本身把跨段上限釘死在一段。

## 推導

### 一源多件：開關与拆件

`memcpy` 源无開關編出 `memcpy`，
加 `-DMEM_MOVE` 編出 `memmove`：
兩件 `THEADR`（目的檔內記的源檔名）同為
`memcpy.asm`，建庫時同名拆成
`MEMCPY.OBJ`（裝 `memmove`）与
`MEMCPY~2.OBJ`（裝 `memcpy`）。
far-data 源同理一源三件：
無開關、`MEM_MOVE`、`MEM_CCPY` 各一件，
庫內是 `HMEMCPY／~2／~3`
（C／L 兩庫是 `memccpy／memmove／memcpy` 順序；
`LIBH` 的 `_f` 版是 `fmemccpy／fmemcpy／fmemmove`——
`fmemcpy` 在 `fmemmove` 之前，順序不同）。
比較源另有 `CASE_INSENSITIVE`：
`hmemcmp` 无開關是 `memcmp`、
有開關是 `_memicmp`。
認執行檔時記住：成員名是源檔名，
符號名才是函式——兩者不一定對得上。

### 字組骨架：四件套

near `memcpy` 全函式 44 位元組，
骨架是：存 `SI／DI`（`mov dx,di／mov bx,si`）、
`ES=DS`、取三參、存回傳值（`mov ax,di`）、
`jcxz` 擋零、`test al,1` 奇位址先 `movsb`、
`dec`、`shr cx,1`、`rep movsw`、
`adc cx,cx`、`rep movsb`、還原、返回。
`memset` 同骨架（`rep stosw／stosb`，
`AL→AH` 先擴字），且因無來源連 `DS` 都不碰——
对读 `memcpy` 的 `push ds` 是 `lds si,src` 逼的，
省一個是一個。
far 版前進段內搬法与 near 同四件套，
只把取參換成 `lds／les` 連段取。

### 重疊：前進或倒搬

near `memmove` 中段是重疊判斷：
`dst<src` 走前進（`jbe` 含等於），
否則比 `dst-src` 与 `count`：
`dst>=src+count`（首尾相接或分離）仍走前進，
真正重疊才倒搬。
倒搬四步：指針加到尾、借方向旗標、
整段 `rep movsb`、`cld` 還旗標後跳過前進段。
`dst==src` 走前進：自己蓋自己，前進正確，
且省下倒搬的設指針成本。

far 版判斷交給 `ahdiff`：
`dst、src` 各推兩字（四字參數），
回 `DX:AX` 有號差——
負值（異段且目的在前）走前進，
`差<count` 才倒搬。
呼叫本身是 compact／large 唯一碼差：
near 呼叫 3 位元組、far 呼叫 5 位元組
（`LEDATA` 200 對 202）。
行為側證：上搬下搬輸出不等
（確實走不同路徑），跨段正反向全過，
異段負值走前進。

下圖是三種相對位置的走法：

<p align="center"><img src="../../img/memmove-overlap.svg" width="640" alt="memmove 三種重疊走法：目的在前走前進、重疊在後走倒搬、相等相接走前進"></p>

### 無分支慣用法：MIN、sbb、映射

far 版 `MIN` 不用跳轉：
`sub／sbb／and／add` 四件算一次
`MIN(Count,65536-SI)`，再來一次对 `DI`——
兩次四件得三者最小。
`memcmp` 系歧異出口：`sbb ax,ax` 得 0 或 −1，
`sbb ax,-1` 得 ＋1 或 −1——
`cmpsb` 的借位直接變符號，
`buf1<buf2→CY=1→−1`。
far `memicmp` 的大小寫映射：
`sub 'A'／cmp 26／sbb bl,bl／and 20h／add`
兩邊各一遍，无分支轉小寫再比。
near `memicmp` 同功能卻是
`cmp 'A'／jb／cmp 'Z'／ja` 跳轉式——
加上 `cProc<si,di>` 巨集保存
（对 `memcmp` 手寫 `dx／bx`）
与全檔小寫參數巨集，
三處風格差指向不同作者
（`memccpy` near 版与 `memicmp`
共享四處指紋：小寫巨集、`sorc` 拼字、
`lupe` 標號、`push ds／pop es`——
強推論同一人）。

### 零計數：jcxz 与七位元組怪寫

`memcpy／memccpy／memset／memchr／memcmp`
零計數全 `jcxz` 直出，
回傳值在跳轉前已備好
（目的起點、`NULL`、0 各就其位）。
far `memmove` 例外：`or cx,cx／jnz／jmp done`
七位元組三跳，語意与 `jcxz` 等價——
簽章裡看得到（`0B C9 75 03 E9` 对它支的 `E3`），
動機不明（已證實的怪寫法，不猜）。

### 跨段：AHINCR 步進与型別裡的上界

far 版段暫存器是手動維護的：
前進每輪搬 `MIN` 量，
`SI／DI` 歸零表示到段尾，
`DS／ES+=__AHINCR`（4096，
`1<<__AHSHIFT`，絕對符號由外部提供）
進下一段；倒搬鏡像（`FFFFh` 減一段）。
`memcpy／memcmp` 要迴圈
（兩端各跨，塊數不定），
`memset／memchr` 卻无迴圈：
只有目的（或掃描）一端，
`count` 16 位元故 `DI＋65535` 至多跨一段——
上界寫在型別裡，不在碼裡。
分塊用同式無分支 `MIN`
（`DX=min(65536−DI,CX)`，
`CX` 取本段量、`DX` 留下段量），
`DI=0→DX=0→jz` 連判斷都省了。

### 回傳与紀律：重讀、ES、不變量

回傳目的起點有三式：
near `memcpy` 入門 `mov ax,di` 存好；
`memset` 用 `xchg ax,bx` 交卷
（`BX` 存的目的）；
far 版從堆疊重讀（經 `SS`，
`DS／ES` 已髒無妨——行為十案 `ret=1` 全證）。
`memchr` near 版入門只 `xor ax,ax`
（`cwd` 与命中路的 `dx=es` 都包在
`sizeD` 條件裡，本源只編 near-data 故从未組出——
near 回傳只用 `AX`，`DX` 不動）；
`DI` 存 `BX`，`repne scasb`，
掃完 `mov ax,0`（`B8 00 00`）先備 `NULL`、
`jne` 走完即回，
命中 `lea ax,[di-1]`（`8D 45 FF`）。
far 版分塊掃：`found` 取 `dx=es、ax=di-1`，
`notfound` 重設 `xor／cwd`——
不依賴入門值，結構更乾淨。
`ES=DS` 有兩式：`mov ax,ds／mov es,ax`
（`memcpy／memcmp` 系）与
`push ds／pop es` 三位元組（`memicmp／memccpy` 系——
又是筆跡線索）。
收尾：无 `push` 的用 `cEnd <nogen>` 配手寫
`pop bp／ret`（`sp` 全程不動，
省掉 `mov sp,bp`）；
有 `push` 的用產生式 `cEnd`
（`mov sp,bp` 保留，
手寫 `pop si／di／ds` 在前）。
medium 对 small 只差四處：
取參位移各加 2、`C3→CB`（far 返回）。

### 死分支：mem 全死、str 全活

六個 near 源的 `MODELINDEP` 分支
（`_fmemcpy／_fmemset／_fmemchr／_fmemcmp／
_fmemicmp／_fmemccpy`）建置出貨兩面皆死：
用 `.n` 的規則不帶此開關，
帶此開關的 `libh` 建置被 `.h` 覆寫源為 `hmem*`；
89 個出貨庫零命中。
出貨的 `_f` 名全來自 `hmem*` 源
（`MODELINDEP` 只換名不改碼：
large 与 `LIBH` 的 `LEDATA` 長度各對相等）。
对读 `str*` 系：同樣的
`ifdef MODELINDEP／cProc _fX` 結構，
`strlen` 的有人用——
結構對稱，命運不對稱。

### 註解三錯与未解

碼对、註解錯三處（行為側證碼是对的）：
`memset` 的 `jz` 註解 yes／no 寫反
（跳轉條件是「非奇」）；
`hmemcpy` 倒搬 `MIN` 註解寫 `65535-DI`，
實為 `DI`（倒搬以距段首距離為限）；
同檔 `DX=-SI` 實為 `~SI`（差一）。
未解：`ahdiff` 本體未深讀
（呼叫約定由行為側證）；
far `memmove` 七位元組零數檢查動機不明；
huge 模型（`-AH`）行為未驗
（large 已覆 far-data 碼路，
huge 差在呼叫端正規化，不在被測檔）。

## 在執行檔裡怎麼認

開場分兩大派。near 版（`SLIBCR／MLIBCR`）
`ES=DS` 先行：

```text
55 8B EC 8B D7 8B DE 8C D8 8E C0  ; memcpy／memmove／memcmp：push bp/mov bp,sp/存di,si/ES=DS
55 8B EC 8B D7 8C D8 8E C0        ; memset：省 SI（无來源，BX 存目的足矣）
55 8B EC 33 C0 ...                ; memchr：先清 AX（NULL 備位）
55 8B EC 56 57 ...                ; memicmp：cProc<si,di> 存檔在前
55 8B EC 57 56 1E 07              ; memccpy：push di/si + push ds/pop es
```

`memcpy` 与 `memmove` near 版開頭 23 位元組全同，
第 24 個分家：`0E`（`jcxz` 近跳，
直入字組骨架）对 `2A`（`jcxz` 遠跳，
後跟 `3B FE 76`＝`cmp di,si／jbe` 重疊判斷）。
`memcmp` near 接 `repe cmpsb`（`F3 A6`）与
`sbb` 對子（`1B C0 83 D8 FF`；
far 版同對子在 30B 窗外）；
`memchr` 接 `repne scasb`（`F2 AE`，
near 後跟 `B8 00 00 75` 先備 `NULL`、
far 後跟 `74` 直跳命中）；
`memccpy` near 接 `lodsb／stosb／cmp／je／loop`
（`AC AA 3A C4 74 04 E2 F8`；
重建品此處是 `38 E0`——組譯器版本差，
簽章保留出貨編碼）。

far 版（`CLIBCR／LLIBCR／LIBH`）先取 `count`：

```text
55 8B EC 8B 4E 0C 1E 57 56        ; hmemcpy：取count/push ds,si,di（C 版位移 0C，L 版 0E）
55 8B EC 33 C0 8B 4E 0C E3 ...    ; hmemcmp：先清 AX，jcxz 擋零
55 8B EC 8B 4E 0A E3 ... 57       ; hmemset／hmemchr：取count/jcxz/push di
```

`hmemcpy` 接 `lds／les` 連段取
（`C5 76／C4 7E`）与 `MIN` 分塊；
`hmemmove` 獨門是七位元組零數檢查
（`0B C9 75 03 E9`，它支皆 `E3`）
与 `ahdiff` 呼叫（`E8` 後兩位元組連結時填，
簽章遮罩）；`hmemset／hmemchr` 接
`8B D7 F7 DA`（`mov dx,di／neg dx` 取段內剩餘）。
取參位移与結尾和其他家族同規則
（遠呼叫／遠資料各加 2，`C3／CB` 分 near／far 返回）。

完整樣式（七函式 28 筆：`LLIBCR` 与 `LIBH`
合併、每筆開頭連續 30 bytes、出貨 `.LIB`
取出；`hmemmove` 兩筆各含 `ahdiff` 呼叫遮罩，
`fixed` 28／27）在
[`mem-family.json`](../../signatures/msvc-1.0-crt/mem-family.json)，
用法与各筆差異見同目錄的
[簽章說明](../../signatures/msvc-1.0-crt/mem-family.md)，
取樣前先核對該檔列出的五個函式庫 SHA-256。

## 行為規格（已證實）

測試環境：自寫測試程式，測試矩陣是
2 種庫（A 連 1.0 源碼重建的目的檔、
B 連 1.52 出貨庫）× 2 種模擬器
（DOSBox、dosgolem）＝四格，
輸出逐位元組一致才算過；
small／medium／large 是記憶體模型各跑一遍。
`ret=1` 表示回傳指標与傳入目的頭相同。

| 呼叫 | 結果 | 範圍／方法 |
|---|---|---|
| `memcpy` 偶／奇目的、計數 7 | 校驗 `10200／10000／1960`、回目的 | small＋medium |
| `memcpy(dst,src,0)` | 目的不動（`Z`）、回目的 | small＋medium 实測 |
| `memmove` 無重疊 | 同 `memcpy`（`10200`） | small＋medium |
| `memmove` 上搬（`dst<src`） | `mov-up=15638` | small＋medium |
| `memmove` 下搬（`dst>src` 重疊） | `mov-down=27664`（≠上搬，確走倒搬） | small＋medium |
| `memmove(dst,src,0)` | 目的不動（`Q`）、回目的 | small＋medium 实測 |
| far `memcpy` 基本／跨段（`src=seg:FFF0` 長 48） | `28282／26302`、回目的 | large 实測 |
| far `memmove` 上搬／下搬／跨段下搬 | `62020／61364／41446` | large 实測 |
| far `memmove` 異段 `dst<src` | 走前進（`mov-seg=49082`） | large 实測 |
| far `_memccpy` 命中／未中／零計數 | `670` 回字元之後／`NULL`／`NULL` | large 实測 |
| near `_memccpy` 命中／未中／首／尾／超界／零 | `670`／`NULL`／`65`／`5642`／`NULL`／`NULL` | small 六案 |
| `memset` 基本／奇位址（`set-basic=51968／odd=46903`） | 全填正確、回目的 | small 兩案＋large 基本案 |
| `memset` 跨段（`seg:FFF0` 長 48，`25016`）／零計數 | 全填正確／目的不動（`Z`） | large 跨段、small 零計數 |
| `memchr` 命中（small `found=2`、large `found=0`）／未中 | 命中回位址、否則 `NULL` | small＋large |
| `memchr` 零計數／目標在次段（`chr-cross=32`） | `NULL`／跨段命中 | small 零計數、large 跨段 |
| `memcmp` 小（`cmp-lt=−1`） | `−1` | small＋large |
| `memcmp` 等／大／零／中段歧異（`'K'` 对 `'k'`） | `0／＋1／0／−1` | small 实測 |
| `memcmp` 歧異點在次段（`cmp-cross`） | `−1 1`（兩案） | large 实測 |
| `_memicmp` 等／小／大 | `0／−1／＋1` | small＋large |
| `_memicmp` 非字母（`'1'` 对 `'A'`）／零 | `−1／0` | small 实測 |

small 十六案（24092 步）与 large 十案（23678 步）
是 `set／chr／cmp／icmp` 批；
`cpy／mov` 批 small 八案（17540 步）、
medium（17592 步，遠呼叫多 52 步）；
far `cpy／mov／ccy` 十案（61054 步）；
near `ccy` 六案（14047 步）。
四格輸出全一致。

## 證據、方法與未知

兩種驗證各證明不同的事（靜態比重建与出貨的語意、
行為比兩種庫跑兩種模擬器的輸出）。
靜態 75 組（`ML 6.11` 依原廠旗標重編，
管線直跑）：R112 四組（S／M × `cpy／mov`）
7／7 全同（44／72 B）；
R113 九組（C／L／H × `cpy／mov／ccy`）
六組 9／9 全同、
三組 `memccpy` 各差一處等價編碼
（`cmp ah,al`：`38 C4` 对 `3A E0`）；
R115 廿組（near 四檔 × S／M 八組＋H 三檔十二組）
十五組全同、
五組 `memicmp` 純 `cmp` 編碼差
（near 各五處、far 各兩處）；
R116 六組（near `ccy`：S／M＋四個 Windows 庫）
各一處同式編碼差；
R114／R117 Windows 卅六組
（near 碼快照旗標即中、
far 碼去 `_WINDOWS` 即中，
出貨 `LEDATA` 与 DOS 版逐位元組一致）。
編碼差全是 `38` 系对 `3A` 系
（同語意不同編碼，組譯器版本差），
行為側全一致證等價；
其餘差異只有出貨端 `COMENT A3`
（`-Fo` 目的檔名註記）。
A 側連結乾淨（重建目的檔与 1.0／1.52 庫共存
无多重定義，MAP 对位：A 在 `_TEXT` 前段、
B 在庫區）。

未知：`ahdiff` 本体未深讀；
far `memmove` 七位元組零數檢查動機不明；
huge 模型行為未驗；
Windows 行為无環境不驗
（靜態 36 組全收，`STRDUP` 帶 `?IA` 首尾是全目錄例外，
成因待查證）。
