---
id: msvc-crt/strcpy-family
title: strcpy 家族：複製、串接與它們的邊界
libraries: [msvc-1.0-crt]
goals: [craft, re, oracle]
evidence: 已證實
triggers:
  - 反組譯 16 位元程式，看到 repne scasb 後面接著整塊搬移，想確認是哪個複製函式
  - 同一個 strcpy 在 small 版與 large 版執行檔裡長度不一樣
  - remake 要重做 strncpy 不補 NUL、strncat 的 count 語意與 64K 飽和行為
  - 連結時多重定義只發生在 strcpy／strcat 身上（不是你的問題）
symbols: [strcpy, strncpy, strcat, strncat, _fstrcpy, _fstrncpy, _fstrcat, _fstrncat, repne scasb, rep movsw, STRCAT, STRNCAT]
related: [msvc-crt/strlen-anatomy, msvc-crt/library-combination]
---

# `strcpy` 家族：複製、串接與它們的邊界

一句話：Visual C++ 1.0 的四個複製／串接函式裡，
`strcpy` 与 `strcat` 共用「先量長度、再整塊搬」的骨架，
far 版各有一道 64K 飽和分支；
`strncat` 藏了一個 `count=0` 的真 bug，
但只在目的尾端恰落段偏移 0 時發作——其餘情況它是對的。
延伸閱讀：量長度那半邊的完整解剖見
[`strlen` 的兩種長相](strlen-anatomy.md)；
五個記憶體模型各編出一份 `.LIB` 的來龍去脈見
[VC++ 1.0 的函式庫怎麼切份](library-combination.md)。

## 結論

四個函式分成兩種寫法。`strcpy` 與 `strcat` 是兩遍式：
第一遍用 `repne scasb` 把來源長度量出來，第二遍用
`rep movsw` 整塊搬；搬之前先看位址奇偶湊對齊
（`strcpy` 看目的端，`strcat` 看來源端，理由見下），
奇位址先搬一個位元組把後面湊成偶數。
`strncpy` 與 `strncat` 是有界的：前者逐位元組拷貝、
提早遇到結束符就把剩下的 `count` 補零；
後者先找到目的字串尾端，再把來源至多接上 `count` 個字元、
最後一定補一個結束符——除了 `count=0`
且目的尾端恰在段偏移 0 的組合，它會失控複製
65535 個位元組（下面詳述，出貨版就這樣，
1.52 版也沒修；尾端不在 0 就是正確的零寫入）。

far 資料模型（compact／large）下，`strcpy` 與
`strcat` 各有一道守衛，處理「掃完 64K 還沒看到結束符」
的情況：整段搬完。`strncpy` 以 `count` 為界，
沒有這種整段掃描、也沒有守衛；`strncat`
找目的尾端那次掃描是不設界的，卻也沒有守衛——
64K 无結束符的目的字串會靜默繞回（沒有探測，記未知）。
near 模型信任呼叫端給的是正常字串，沒有這道分支。
四個函式都不保存 `BX`（拿它當暫存，碼面確定），
`SI`／`DI` 則進出一致（同為碼面確定，見行為規格表）。

## 根本問題

1989 年的 C 字串是以 `0` 結尾的一串位元組，複製它有三個現實約束。
先說行話：near 指標 2 位元組（只有段內位移），
far 指標 4 位元組（段加位移）；small／medium
用 near 資料，compact／large 用 far 資料；
一個段最多 64K（65536 個位元組）。
第一，長度事先不知道：要嘛先量再搬（兩遍），
要嘛邊搬邊看（單遍），前者搬得快、後者省一遍掃描。
第二，計數器只有 16 位元：`CX` 從 `FFFFh`（65535）
倒數，一遍最多掃 65535 個位元組；掃完還沒命中時
`CX` 歸零——這個零代表「掃完整段沒找到」，
不是「長度為零」，far 版得決定它算哪一種，
未處理的話複製函式會靜默搬錯份量。
第三，`SI`／`DI` 進出一致是這家函式庫的寫法慣例：
函式內部拿它們當來源／目的指標，就得先存後還；
而 `BX` 被四個函式拿去當暫存，呼叫完是殘值——
這是碼面現象（全 `STRING` 目錄零 `push bx`），
不是約定背書：呼叫端若假設 `BX` 不變就會中招。

三條指令一句話：`repne scasb` 是「`CX`
倒數掃描、找到目標位元組或數完為止」；
`rep movsw` 是「按字（一次 2 個位元組）整塊搬」；
`movsb` 是「搬一個位元組」。字串指令的寫入端
固定是 `ES:DI`，所以動手前得先把 `ES`
切到對的段：量長時 `ES` 暫借來源段
（流程圖上寫 `ES=DS` 就是這回事），開搬前再切到目的段。

## 推導

### strcpy：先量再搬，奇位址先墊一個位元組

<p align="center"><img src="../../img/strcpy-flow.svg" width="700" alt="strcpy 流程：量長度、對齊、整塊搬移，far 版多一道直達整塊搬的飽和分支"></p>

長度未知但搬移論塊計價，所以拆兩遍是划算的：
第一遍 `ES` 暫借來源段、`repne scasb` 從 `FFFFh`
倒數找到結束符，`not` 加 `dec` 換算成「字元數加一」
（含結束符本身）；第二遍把 `ES:DI` 切到目的端整塊搬。
搬之前讀一次目的位址的最低位：奇位址先 `movsb` 一個位元組，
剩下的偶數字節數 `shr` 取半用 `rep movsw` 搬。
`shr` 掉出去的那個位元進了 carry 旗標，
`adc cx,cx` 把它變成 0 或 1 的計數
（此時 `CX` 已被 `rep movsw` 歸零，
`adc` 等於 `CX＝0＋0＋carry`），
再用 `rep movsb` 補最後零或一個位元組：
7 個位元組就是「`shr` 得 3、`rep movsw` 搬 3 個字、
`adc` 得 1、`rep movsb` 補 1 個」，6 個位元組則
`adc` 得 0、最後一步搬空。
8086 上字組搬移比位元組搬移快約一倍，對齊的代價只是一次測試、
至多一個位元組的零頭，這筆帳怎麼算都划算。

far 版多一道飽和分支，長在量完長度之後：
正常找到結束符就直行；`CX` 歸零（掃了 65535 個位元組還沒看到 `0`）
就先搬一個字出去、計數器扣掉對應份量，
然後跳過對齊檢查、直達整塊搬——
結果是整整 64K 全搬。「先搬一個字」具體是：
`movsw` 搬 2 個位元組、`CX` 從 `FFFFh` 減一成 `FFFEh`，
剩下的 `FFFEh`（65534）恰好是「尾段位元組數加一」的形狀，
後面照正常路徑搬完，總量正好 64K。
對齊檢查本來就是純粹的效能優化，跳過它只會慢、不會錯，
罕見的飽和案就這麼辦（原註解稱 superfluous）。
判斷用的是 `repne` 留下的零旗標：
`repne` 与判斷之間隔著 `not`、`les`、`mov` 三條指令，
三者都不動旗標，所以沿用是安全的
（`not` 不動旗標是例外中的例外：
`dec`、`add`、`sub` 都會改旗標，讀這類慣用法時要逐條確認）。
這是全函式庫通用的小慣用法（`strlen`、`strset` 的 far 版同式）。

回傳值是目的位址：位移放 `AX`，far 版段位址另放 `DX`。

### strncpy：逐位元組是為了中途停下

`strncpy` 不能照搬兩遍式，因為語意要求「來源提早結束就把
`count` 補滿零」：搬到第幾個停下事先不知道，整塊搬反而礙事。
所以它是單遍逐位元組迴圈：取一個、是結束符就跳出、
否則存下、`loop` 計數。跳出後（或 `count` 用完後）
`CX` 剩多少就補幾個零——`count` 恰好用完時剩零、
一個不補，這正是標準說的「截斷時不補結束符」。
`count=0` 在迴圈前用 `jcxz` 擋掉：零寫入、直接回傳目的位址。
逐位元組在 8086 上比 `rep movsw` 慢，但 `strncpy`
的語意本來就不是為速度設計的（定寬欄位填充），正確優先。

### strcat：找尾、量長、搬，三段串起來

`strcat` 是「`strlen(dst)`＋`strcpy` 後半」的拼裝，
分三步：① 掃出目的字串的結束符位置
（`repne` 會停在結束符下一格，所以找到後要退一格，
碼面是 `lea di,[di-1]`），② 對來源跑一遍
`strcpy` 式的量長，③ 把來源（含結束符）搬到目的尾端。
far 版的飽和處理与 `strcpy` 同結果（64K 全搬）但寫法不同：
它在開搬前（對齊檢查之前）用「計數器是零」認出飽和案，
補搬一個字再把計數器修回來，同樣跳過對齊直達整塊搬。
一個小考據：註解說對齊檢查看的是目的端，
實際程式碼看的是來源端（`test si,1`，此時 `SI` 是來源）；
看哪一邊都是合法的效能取捨
（來源對齊同樣加速 `rep movsw`），現行
（來源端）寫法行為測試全過，所以是註解抄錯，不是程式寫錯。

### strncat：有界附加，與 count=0 的條件式真 bug

`strncat(dst, src, count)` 先找到目的尾端，
再以 `count` 為界量來源：界內看到結束符就排除它、
算出實搬數；界內沒看到就搬滿 `count`。
搬完一定補一個結束符——這個保證是 `strncat` 与 `strncpy`
最大的不同（後者截斷時不補）。補的那個零不是現寫的，
是迴圈初就歸零的累加器一路帶下來的
（中間的掃描搬移指令都不碰它）。

bug 在 `count=0`，而且是有條件的。
量來源的那次 `repne` 以 `count` 為界，
`count=0` 時零步退出、零旗標沿用舊值——
但舊值不是「找目的尾端」那次 `repne` 留下的：
兩次 `repne` 之間隔著一條 `dec di`
（找尾後退一格的那步），`dec` 會改旗標，
所以 `jne` 測到的是「目的尾端偏移是否為零」。
尾端偏移為零（例如空字串頂著段起點）才走 bug 路：
`jne` 不跳、`CX` 加一（0→1）、減去 `count`（1−0＝1）、
取負（0−1＝`FFFFh`），**複製 65535 個位元組外加一個結束符**；
尾端偏移非零就一切正常：`jne` 跳過加一、
實搬數算成 0，只重寫一遍尾端結束符。

三組 large 實測互相對照（自寫測試，兩塊 68K 記憶體，
目的段預填零、來源段填字母圖案）：
空字串在段偏移 0（尾端 0）→ `dst[0]` 与 `dst[100]`
皆被來源圖案覆寫（bug）；目的 `"Z"` 在偏移 0
（尾端 1）→ 零寫入；空字串在偏移 5
（尾端 5）→ 零寫入。small 三案（目的 `"hello"`、
段內空字串、首個靜態陣列，尾端皆非零）同樣零寫入。
1.52 的出貨版与 1.0 同犯同免；
2.0 的 Alpha 版把這函式改寫成 C 之後才修好，
2.0 x86 版無源碼、未驗。

## 在執行檔裡怎麼認

四個函式的開場白分兩派，`strcpy`／`strcat` 用暫存器存檔
（`SI`／`DI` 的舊值塞進 `BX`／`DX`，不動堆疊）：

```text
55 8B EC 8B D7 8B DE        ; push bp / mov bp,sp / mov dx,di / mov bx,si
```

`strncpy`／`strncat` 用堆疊存檔：

```text
55 8B EC 57 56              ; push bp / mov bp,sp / push di / push si
```

再往下看取參：near 資料模型用 `8B 76／8B 7E`
（`mov si/di,[bp+n]`，段內取參），far 模型用 `C5 76／C4 7E`
（`lds si／les di`，連段帶位移一起取）。
取參位移在兩種情況下各加 2：far 呼叫
（medium／large）的返回位址佔 4 個位元組、
far 資料（compact／large）的指標參數本身較寬——
以 `strcpy` 為例，S＝06、M＝08、C＝08、L＝0A，
compact 那 2 來自後者。
結尾 `C3` 是 near 呼叫、`CB` 是 far 呼叫。
各函式的獨門特徵：`strncpy` 開場不久有 `E3`
（`jcxz` 零數守衛，看到它就是 `strncpy` 家）；
`strcat` 有 `8D 75 FF`（`lea di,[di-1]`，
掃過頭退一格）；`strncat` 有 `4F`
（同作用的 `dec di`，也是 `count=0` 誤判的源頭）。
`strcpy` far 版在量長之後有一小段飽和頭
（`je` 分岔、`movsw` 開路），near 版沒有。

完整樣式（每函式四筆：S／M／C／L，
其中 `LLIBCR` 与 `LIBH` 位元組相同合併為一、
每筆開頭連續 27 bytes、出貨 `.LIB` 取出、无需遮罩）在
[`strcpy-family.json`](../../signatures/msvc-1.0-crt/strcpy-family.json)，
用法与各筆差異見同目錄的
[簽章說明](../../signatures/msvc-1.0-crt/strcpy-family.md)，
取樣前先核對該檔列出的五個函式庫 SHA-256。
`LLIBCR` 与 `LIBH` 的樣式逐位元組相同（差別只在公開符號
`_strcpy` 對 `__fstrcpy` 等），JSON 已合併。

## 行為規格（已證實）

測試環境：自寫測試程式，A 連 1.0 源碼重建的目的檔、
B 連 1.52 出貨庫；每案在 DOSBox 与 dosgolem 兩邊跑，
兩邊輸出逐位元組一致才算過。步數是 dosgolem
`-cpu 8086` 的計數（附帶資訊，不在表內）。
`範圍／方法` 欄標明每一列的證據等級：
實測（列出模型）、靜讀（碼面資料流確定）、未測。

| 呼叫 | 結果 | 範圍／方法 |
|---|---|---|
| `strcpy(d, "hello")` | `d` 變 `hello`，回傳 `d` | small 實測 |
| `strcpy(d, "")` | `d` 變空字串，寫入 1 byte（結束符） | small 實測 |
| `strcpy` far、來源 64K 无結束符 | 整段搬完（含目的端最後一格） | large 實測 |
| `strncpy(d, "hi", 5)` | `d` 前兩格 `hi`，後三格補零 | small 實測 |
| `strncpy(d, "hello", 3)` | `d` 前三格 `hel`，**不補結束符** | small 實測 |
| `strncpy(d, s, 0)` | `d` 完全不動，回傳 `d` | small 實測 |
| `strcat(d, "bar")`（`d` 初值 `foo`） | `d` 變 `foobar`，回傳 `d` | small 實測 |
| `strcat(d, "")` | `d` 不變（只重寫一遍結束符） | small 實測 |
| `strcat("", s)` | 等同 `strcpy` | small 實測 |
| `strcat` large（含 far 飽和分支） | 未執行 | 未測（缺口，見下） |
| `strncat(d, "hello", 3)` | 接上 `hel` 再補結束符，回傳 `d` | small＋large 實測 |
| `strncat(d, s, 0)`，尾端偏移 0 | **bug**：複製 65535 byte＋結束符 | large 實測 |
| `strncat(d, s, 0)`，尾端偏移非零 | 零寫入（只重寫尾端結束符） | large＋small 實測 |
| 四函式呼叫後 | `SI`／`DI` 復原，`BX` 必髒 | 靜讀（四檔皆有存還／暫存對子） |

`strcat` 的 large 行為沒有實測：它的 far 飽和分支存在
（碼面与 `strcpy` 同類），但當初沒有做 large 行為探針，
這是缺口，不是「没有分支可探」。
`strncat` 目的端 64K 无結束符的繞回行為同樣未測。
其餘各列皆如表，A／B 四格一致。

## 證據、方法與未知

靜態：`MASM 6.11` 的 `ML`（WinWorld 磁片組，非 1.52 盤內）
依原廠 `BUILD.BAT` 的旗標重編，`omfdiff` 語意比對——
DOS 五組（S／M／C／L／H）与 Windows 四組
（`wc／wcdll` 快照旗標、`wl／wldll` 去 `_WINDOWS`）
全數 7／7 相同（只差 `COMENT A3` 目的檔名註記）。
行為：A 連 1.0 重建目的檔、B 連 1.52 出貨庫，
small 十二案（27023 步，含 `STRDUP` 兩案）与
large 五案（1219015 步，64K 拷貝所致，含 `STRDUP` 一案）
輸出四格全一致；PUB1 補的 `count=0` 對照六案
（large 三、small 三）另計四格一致
（large 141022 步、small 11932 步）。
`strncat` 的 `count=0` 与 `strcpy` 飽和分支是
large 下用整段記憶體實測的，不是推的。

方法上的一個教訓：A 側（1.0 源碼重建）連結必須對 1.0 自家函式庫。
把 1.0 重建的 `STRCPY.OBJ`／`STRCAT.OBJ` 与 1.52 的
`SLIBCE`（1.52 small 版出貨庫）連在一起，
`LINK` 必報多重定義（只發生在這兩個符號身上，
其餘二十二個並不會；微觀機制未明，
已排除檔名、大小寫、結構、多重匯出）。
R118（第一輪複製家族對拍）的第一版 A 側數據就栽在這上面，
後來全部改對 1.0 重連重跑才轉正——
讀者若自己重做對拍，直接避開這個組合。

未知：`strcat` 註解与程式碼看不同一邊的對齊，
是筆誤還是改寫殘留，無從分辨（現行碼行為是對的）；
Windows 版 `STRDUP` 的 far 件是全 `STRING` 目錄
唯一帶 `?IA` 首尾的成員，為何獨厚它，仍是懸案
（与本篇無關，記在 `?IA` 首尾調查）。
