---
id: msvc-crt/strlwr-family
title: strlwr 家族：就地改寫的五種手法
libraries: [msvc-1.0-crt]
goals: [craft, re, oracle]
evidence: 已證實
triggers:
  - 反組譯 16 位元程式，看到迴圈裡 sub／cmp／jnb 接著寫回 [bx]，想確認是不是大小寫轉換
  - strlwr 与 strupr 在執行檔裡只差兩個位元組，想一次認出兩支
  - remake 要重做 strrev 的空串守衛与 strnset 的 count 語意
  - 同源碼重建与出貨逐位元組全同的一批，想看零差異長什麼樣
symbols: [__strlwr, __strupr, __strset, __strnset, __strrev, rep stosb, xchg]
related: [msvc-crt/strchr-family, msvc-crt/strcmp-family]
---

# `strlwr` 家族：就地改寫的五種手法

一句話：Visual C++ 1.0 的五個就地改寫函式裡，
`strlwr／strupr` 是鏡像手寫迴圈
（30 位元組全同除兩個常數、註解各錯一行），
`strset／strnset` 是量長加 `rep stosb`
（有界那支用單一減法取 `min` 且永不覆寫結束符），
`strrev` 是雙指針互換（空串專用守衛防指針繞行）。
延伸閱讀：搜尋家族見
[`strchr` 家族](strchr-family.md)，
比較家族見
[`strcmp` 家族](strcmp-family.md)。

## 結論

五個函式全是就地改寫：讀寫同一串、
回傳原指標（存在 `DX`，`xchg` 交卷）、
不配置任何記憶體。`strlwr／strupr`
拿 `BX` 當游標逐字改寫，區間檢查用無號技巧
（`sub 'A'／cmp 26／jnb`，一次排除區間兩側），
迴圈用跳入式（先測結束符、空串直出），
`cEnd` 手寫收尾（不存 `SI／DI`，
因為只動 `BX`；`BX／DX` 照髒）。
兩檔是鏡像複貼：30 位元組碼面只差兩處常數
（`sub` 与 `add` 的 `'A'／'a'` 互換；
`cmp` 同為 26，組出來一字不差），
註解卻各錯一行（`strlwr` 檔頭方向寫反、
`strupr` 入口寫成 lowercase——
複貼時兩邊都沒校）。
`strset` 量長（`not` 加 `dec` 扣掉結束符）
後 `rep stosb` 全填；`strnset` 先 `BX＝count`、
`jcxz` 擋零、有界 `repne` 量長、
`inc` 扣掉結束符、`sub bx,cx` 一次得
`min(len,count)`——結束符永遠不在填寫範圍內。
`strrev` 量長後 `SI` 指頭、`DI` 指尾，
`cmp si,di／jae` 相遇即停，
`AH／AL` 互換後寫回兩端；
空串有專用守衛（`cmp cx,-2`，
`CX＝-2` 即長度零），單字元由相遇檢查擋下
（`SI＝DI` 即停，一次都不換）。

`BX` 去留：`strset` 保持
（全程只需字串起點，`DX` 存回傳值足矣，
用不到 `BX`）；其餘四支必髒
（游標、計數、頭指標各取所需）。
檔頭 `Uses: BX, CX, DX` 在 `lwr／upr`
白紙黑字——与 `strcpy／strcat／strcmp` 等多檔同式，
是這家人的集體自述，不是哪一支的首創；
探針与自述吻合。

## 根本問題

就地改寫有四個現實約束。
第一，輸入即輸出：省下配置与回傳新串的麻煩，
代價是原串被破壞（呼叫端傳字串常數進來就等著當機——
這是介面契約，不是實作能救的）。
第二，區間檢查：大小寫轉換每字元都要問
「在不在 `A–Z`」，樸素寫法兩次比較；
無號減法把它壓成一次（減 `A` 後小于 26 即在區間內，
小於 `A` 的 wraparound 成大數一併排除）。
第三，上界与結束符：填寫必須停在結束符之前
（`strset` 量長扣一，`strnset` 取 `min`），
反轉必須處理長度零与一（退化案不設防就繞行或空轉）。
第四，暫存器分工：8086 通用暫存器只有四個半，
游標、計數、回傳值各據其一，
`BX` 拿去當游標就沒有乾淨的 `BX` 可言——
`Uses` 自述即承認。

三條指令一句話：`rep stosb` 是
「`CX` 倒數向 `ES:DI` 填同一個位元組」；
`xchg ax,dx` 是原地互換（這家人愛拿它交卷：
回傳值暫存 `DX`，結尾一換即交）；
`jnb` 是無號不小於跳（進位為零即跳，
區間檢查的收尾）。

## 推導

### strlwr／strupr：鏡像迴圈，註解各錯一行

迴圈是手排的：`jmp first_char` 跳入式進入
（先讀字測結束符，空串一次迴圈體都不跑；
`even` 把迴圈頭對齊偶位址），
`sub al,'A'／cmp al,26／jnb` 無號區間檢查
（`'A'(41h)`→減得 0→小於 26 命中；
`'a'(61h)`→減得 `20h`→不小於 26 跳過；
`'0'(30h)`→減得 `F0h`→不小於 26 跳過），
命中加 `'a'`（`upr` 版加 `'A'`）寫回 `[bx]`，
`inc bx` 前進。`BX` 游標、`DX` 存回傳原址，
結尾 `xchg ax,dx` 交卷——
`cEnd` 不存 `SI／DI`（只動 `BX`，无須存），
`BX／DX` 照髒（檔頭 `Uses` 自述）。
兩檔碼面 30 位元組只差兩處
（`sub` 常數 `41h／61h`、`add` 常數 `61h／41h`
互換；`cmp` 同為 `1Ah`），
出貨与重建的交叉正對照（拿 `upr` 重建品
对 `lwr` 出貨品比）只報這兩處——
`omfdiff` 誤報三處是它把不同檔案擺錯了位
（跨檔比較本來就不合法），不是碼有問題。
註解是鏡像複貼各錯一行：`strlwr` 檔頭
把方向寫成「小寫轉大寫」（与標題行矛盾），
`strupr` 入口把參數寫成「map to lowercase」——
複貼時兩邊都沒校，碼都是對的。

### strset／strnset：量長一律，min 用減法一次得

`strset` 是量長加填寫：`repne` 量長、
`not` 得長度加一、`dec` 扣掉結束符、
`rep stosb` 填滿——結束符永遠不動。
far 版多一道找到才 `dec` 的分支
（`jne`：找到結束符才有長度可扣；
掃完 64K 沒找到就填 `FFFFh` 個，
与 `strcpy` 的飽和哲學同源）；
near 版信任呼叫端，无分支。
全程只需字串起點（`DX` 存回傳值足矣），
用不到 `BX`——全家族唯一的 `BX` 保持填寫函式，
探針證實（`set=1234 keep` 跨版命中）。

`strnset` 先 `BX＝count`（`BX` 當上限暫存），
`jcxz` 擋零（`count=0` 回原串，一個不填），
`repne` 以 `count` 為界量長。
`inc` 扣掉結束符只在「界內命中」時執行
（`jne nonull` 跳過未命中案），
再 `sub bx,cx` 一次得 `min`：
命中（`k` 處結束符）→`CX＝count-(k+1)`→
`inc` 得 `count-k`→`sub` 得 `k`；
未命中→`CX＝0`→`sub` 得 `count`。
算一次：`"hello",3`→`CX＝5-0…` 且慢——
`repne` 以 `count＝3` 為界，界內无結束符，
`CX＝0`，`sub` 得 3，填三個（`YYYlo`）；
`"hello",8`→界內在 `k＝5` 命中，
`CX＝8-6＝2`→`inc` 得 3→`sub` 得 5，
全填（`YYYYY`）。`BX` 當計數交卷必髒，
是 `nset` 与 `set` 在 `BX` 去留上分流的原因：
要不要計數器，回傳型別說了不算，
算法說了算。

### strrev：雙指針相遇即停，空串專防

<p align="center"><img src="../../img/strrev-flow.svg" width="700" alt="strrev 雙指針（僅本支）：SI 指頭 DI 指尾，互換後相向，cmp 相遇即停；空串 cmp cx,-2 專防"></p>

量長後 `SI` 指頭、`DI` 指尾
（`dec di` 兩次：掃過頭一格加結束符一格），
`cmp si,di／jae` 相遇即停，
`AH／AL` 分取兩端互換寫回。
偶數長（`"abcd"`）換兩次相遇，
奇數長（`"abc"`）中間字元 `SI＝DI` 即停、
一次不換——相遇檢查包辦兩種奇偶，
不需要長度除二。
空串是退化案：`CX＝len+1＝1` 時
`DI` 退兩次會繞到段尾，
故先 `cmp cx,-2`（`not` 後的 `CX`：
空串長零对应 `CX＝-2`，即 `FFFEh`）專防直出。
far 版多一段无結束符即整段反轉的分支
（`je` 找到走正常路，否則 `dec di` 指最後一格
直入迴圈），是三支 far 特判裡最激進的一支。
`SI` 先存 `BX`（`mov bx,si`），
`BX` 照髒——探針与碼面一致。

## 在執行檔裡怎麼認

開場分三派。`strlwr／strupr` 无存檔直入
（`BX` 游標、`DX` 回傳，`cEnd` 手寫收尾）：

```text
55 8B EC 8B 5E 04 8B D3        ; push bp / mov bp,sp / mov bx,[bp+4] / mov dx,bx
```

後面緊跟 `EB 0B`（跳入迴圈）接
`2C 41／61 3C 1A 73 04`（無號區間檢查；
`lwr` 版 `sub` 常數 `41h`、`upr` 版 `61h`，
兩支 30 位元組全同除此兩處）。
`strset／strnset` 手寫 `push di`：

```text
55 8B EC 57                    ; push bp / mov bp,sp / push di
```

`strset` 接量長（`F2 AE F7 D1 49`：
`repne`、`not`、`dec` 扣結束符）
再 `F3 AA`（`rep stosb` 全填）；
`strnset` 開場多 `E3`（`jcxz` 零數守衛）
与 `2B D9`（`sub bx,cx` 取 `min`）。
`strrev` 多存 `SI`：

```text
55 8B EC 57 56                 ; push bp / mov bp,sp / push di / push si
```

接量長（`B9 FF FF F2 AE`）再
`83 F9 FE`（`cmp cx,-2` 空串守衛）
与 `3B F7`（`cmp si,di` 相遇檢查）。
取參位移与結尾和其他家族同規則
（遠呼叫／遠資料各加 2，`C3` 对 `CB`；
`lwr／upr` 取參走 `BX`：`8B 5E／C4 5E`）。

完整樣式（五函式 20 筆：`LLIBCR` 与 `LIBH`
合併、每筆開頭連續 30 bytes、出貨 `.LIB`
取出、无需遮罩）在
[`strlwr-family.json`](../../signatures/msvc-1.0-crt/strlwr-family.json)，
用法与各筆差異見同目錄的
[簽章說明](../../signatures/msvc-1.0-crt/strlwr-family.md)，
取樣前先核對該檔列出的五個函式庫 SHA-256。

## 行為規格（已證實）

測試環境：自寫測試程式，A 連 1.0 源碼重建的目的檔
（本批函式）、B 連 1.52 出貨庫；
每案在 DOSBox 与 dosgolem 兩種模擬器各跑一遍，
四格輸出逐位元組一致才算過。
`ret=1` 表示回傳指標与傳入字串頭相同
（就地改寫的簽名）。

| 呼叫（small 字面） | 結果 | 範圍／方法 |
|---|---|---|
| `_strlwr("aBcD12!Zz@")` | `abcd12!zz@`（非字母不動） | small＋large（同字面） |
| `_strlwr` 回傳 | 原址（`ret=1`） | small 實測 |
| `_strlwr("")` | 空串（零寫入） | small 實測 |
| `_strupr("aBcD12!Zz@")` | `ABCD12!ZZ@` | small＋large（同字面） |
| `_strupr("")` | 空串 | small 實測 |
| `_strset("hello",'X')` | `XXXXX`（結束符不動） | small＋large（同字面） |
| `_strset("",'X')` | 空串 | small 實測 |
| `_strnset("hello",'Y',3)` | `YYYlo` | small＋large（同字面） |
| `_strnset("hello",'Y',0)` | `hello`（一個不填） | small 實測 |
| `_strnset("hello",'Y',8)` | `YYYYY`（全填） | small＋large（同字面） |
| `_strrev("abcd")` | `dcba`（偶數換兩次） | small＋large（同字面） |
| `_strrev("abc")` | `cba`（中間不換） | small＋large（同字面） |
| `_strrev("a")` | `a` | small 實測 |
| `_strrev("")` | 空串（守衛直出） | small＋large（同字面） |
| `_strlwr／_strupr／_strnset／_strrev` 呼叫後 | `BX` 必髒 | 靜讀＋探針（R121 有效集） |
| `_strset` 呼叫後 | `BX` 保持 | 靜讀（零 `BX` 指令）＋探針 |

small 十八案（17457 步）与 large 九案（10168 步），
large 輸入与 small 字面全同，四格輸出全一致。
`BX` 保持集以 R121 重測為準
（`BXA10≡BXB`，30918 步同；本批 `set` 在保持集內）。

## 證據、方法與未知

兩種驗證各證明不同的事（靜態比重建与出貨的語意、
行為比兩種庫跑兩種模擬器的輸出，見前篇導讀）。
靜態：`MASM 6.11` 的 `ML` 依原廠旗標重編——
五組（S／M／C／L／LIBH）中五檔全數 7／7
相同（只差 `COMENT A3`），是本目錄第一批
零編碼差的全同批；Windows `wc／wl` 兩組
（`wc` 快照旗標、`wl` 去 `_WINDOWS`）同样全同，
且出貨成員与 DOS C／L 版逐位元組一致。
附帶正對照：拿 `strupr` 重建品对 `strlwr`
出貨品比只差兩處常數位元組——
同源碼不同常數的檔案就該長这样，
側證比對工具有在做事。
A 側連結乾淨（本批顯式集无 `STRCPY／STRCAT`，
`LINKA.LOG` 零多重定義）。

未知：`strset／strrev` far 版 64K 无結束符長串的行為
（碼面有分支：填 `FFFFh` 个／整段反轉，行為一概未探）；
`strlwr／strupr／strnset` far 版同場景
（碼面无特判，未探）。
