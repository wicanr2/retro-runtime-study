---
id: msvc-crt/locale-stubs
title: locale 樁与未建的寬字串
libraries: [msvc-1.0-crt]
goals: [craft, re, oracle]
evidence: 已證實
triggers:
  - 反組譯看到 strcoll 直接調 strcmp，想確認 1.0 是否根本沒有 locale 排序
  - strxfrm 回傳值到底是搬了幾個還是來源多長
  - 寬字串函式在 1.0 出貨庫裡找得到嗎
  - 同一批 .C 檔用什麼編譯旗標編出逐位元組一致的成品
symbols: [strcoll, strxfrm, wcscoll, wcsxfrm, /Gs, /Ot]
related: [msvc-crt/strcpy-family, msvc-crt/strcmp-family]
---

# `locale` 樁与未建的寬字串

一句話：Visual C++ 1.0 的 `strcoll／strxfrm` 是 C locale
恆等樁（前者直調 `strcmp`，
後者 `strncpy` 加回 `strlen`），
建置旗反推是 `CL /Gs /Ot`（八組 12／12 全同）；
寬版雙生（`wcscoll／wcsxfrm`）与 17 檔 `WCS*.ASM`
全未建（建置檔零命中、出貨零成員），
三處世代差證明寬版後寫，
完成度低吻合未建。
延伸閱讀：被轉發的實作見
[`strcpy` 家族](strcpy-family.md)（`strncpy`）与
[`strcmp` 家族](strcmp-family.md)（`strcmp`）。

## 結論

C locale 下排序等同位元組序，
故 `strcoll` 全函只需一句轉發；
`strxfrm` 恆等轉換：搬 `strncpy(d,s,n)`、
回 `strlen(s)`——
回傳值是來源長度，与 `n` 無關；
`retval>=count` 時目的无結束符
（`strncpy` 語意直承）。
檔頭允許 `strxfrm(NULL,s,0)` 量長而不搬：
碼面无 `NULL` 保護，直打 `strncpy(NULL,s,0)`——
依賴 `strncpy` 零步不碰目的
（`xfrm-null=3 alive=1`，兩邊皆存活）。
兩樁出貨碼是典型編譯器產物：
`coll` 16 位元組（取參、調 `strcmp`、
`mov sp,bp` 清參數），
`xfrm` 28 位元組加 `NOP` 墊
（兩次取參調用，尾對齊）。

建置旗用排除法反推：
1.52 `CL` 預設編出 32 位元組
（堆疊探針、`SI／DI`、`add sp` 清參），
`/Gs` 去探針但 `SI／DI` 殘留，
`/Gs /Ot` 八組 12／12 重現出貨；
`/Gs /Os` 在 small-`xfrm` 差一字节
（1.52 用 `pop bx` 清最後一參，
出貨用 `mov sp,bp` 加 `NOP`——
1.52 的 size-peephole 在 1.0 不存在或未啟用），
故 `/Os` 一票否决；
`/Ox` 把 `strcmp` 內聯，更不對。
結論：1.0 的 `.C` 建置旗是 `/Gs /Ot` 系
（強推論；`strcoll` 在 `/Os` 下同形是巧合，
本函无 peephole 窗口）。
殘差只有 `COMENT` 前言
（1.52 自注 `defaultlib` 与編譯器名，
出貨无——編譯器硬編碼，開關去不掉）；
版本戳兩邊一致（同 8.0 世代旁證）。

寬版四樁之二是未建品：
`wcscoll／wcsxfrm` 与窄版同義
（`wcscmp` 轉發／`wcsncpy` 加 `wcslen`），
但建置檔三份零 `wcs`、
出貨五庫零成員
（`wcs` 字樣只剩 `mbstowcs／wcstombs` 轉換件）。
三處世代差：ANSI 原型（參數括自成行、逐參一行）
對窄版 K&R 式（同行參數加分離宣告）、
版權 1990-1992（窄版 1988-1992）、
多 `#include <locale.h>`——
寬版後寫或後改，非同期鏡像（強推論）。
17 檔 `WCS*.ASM` 同未建：
全數通過 `ML 6.11` 組譯（无語法腐爛），
骨架与窄版同（`w` 化指令），
但 far 守衛系統性缺席、零 `AHINCR`、
註解懸空、慣用法相異、`WCSREV` 非鏡像重構——
完成度低吻合未建。
為何未建（時程砍還是本來就沒要）未知，
封存內无說法，不猜。

## 根本問題

ANSI C 要求 `strcoll／strxfrm` 按當前 locale 排序；
但 1.0 只實作 C locale，
而 C locale 的排序序就是位元組序——
樁是誠實的完備實作，不是偷懶：
需求退化成恆等轉換，碼就該是一句轉發。
寬字串側是反面：`wchar_t` 介面已寫
（17 檔組譯加兩樁，頭檔就緒），
卻沒人把它編進任何一庫——
問題從「怎麼實作」變成「為何缺席」，
證據只夠說到「未建」，不夠說「為何」。

## 推導

### 樁轉發：兩句的完備實作

`strcoll` 全函一句：回 `strcmp` 結果。
`strxfrm` 兩步：`strncpy(d,s,n)` 搬運、
回 `strlen(s)` 長度。
注意回傳值語意：恆等轉換下轉換後長度恆等於來源長度，
故与 `n` 無關——
截斷案（`n=3`）回 5（來源 `hello` 長），
目的只取前三字；
零案（`n=0`）回 5，目的原樣不動；
空串回 0。
`retval>=count` 时目的无結束符：
`strncpy` 不補 `NUL`，樁原樣繼承，不加料。

### NULL 鏈：祝福、依賴、存活

檔頭允許 `strxfrm(NULL,s,0)`：
傳 `NULL` 目的加零計數，只問長度不搬運。
碼面沒有 `if (d)` 保護——
直打 `strncpy(NULL,s,0)`，
賭 `strncpy` 零步連讀都不讀目的。
探針證兩邊皆成立：
`strxfrm(NULL,"abc",0)=3` 且程式存活
（`alive=1`），A（重建）B（1.52）同。
依賴鏈：樁的契約由被調函式的邊界行為背書，
不是由自己的檢查背書——
讀樁必須連被調一起讀。

### 旗標排除史：一票否决

`.C` 配方建置檔沒留字（`$(MEM)` 展開不在檔內），
只能逐位元組反推。
1.52 `CL` 預設 32 位元組對出貨 16：
差堆疊探針（`/Gs` 去）、`SI／DI` 存檔、
`add sp` 清參。
`/Gs` 去探針，`SI／DI` 仍在，不對。
`/Gs /Os` 与 `/Gs /Ot` 在 `strcoll` 同形——
本函太短，无 peephole 窗口，兩旗收斂。
`strxfrm` 投下否决票：
`/Gs /Os` 在 small 版差一字节——
1.52 用 `pop bx`（1 位元組，還髒 `BX`）
清最後一參，出貨用 `mov sp,bp` 加 `NOP` 墊。
`/Ox` 更把 `strcmp` 內聯進 `strcoll`，
形狀全非。
排除剩 `/Gs /Ot`（M／L 另加 `/NT_TEXT`），
八組 12／12 全同。
附帶：large 在 `/Gs /Os` 下反而全同——
far 收尾窗口不同，peephole 未觸發；
同一優化只在 small 現形，
觸發与否与模型有關。

### 世代差：三源一致的後寫證據

窄寬四樁並排，三處差全指向同一年表：
函式定義窄版 K&R 式、寬版 ANSI 原型
（參數括自成行、逐參一行——
不是同一隻手同期寫的排版）；
版權窄版 1988-1992、寬版 1990-1992；
寬版多 `#include <locale.h>`。
任一處都可單獨解釋（改排版、改年份、加頭檔），
但三處同向：寬版後寫或後改（強推論）。
注意推論邊界：只能說源碼的相對新舊，
不能說寬版為哪一版準備、也不能說何時寫的——
封存內无日期證據。

### 未建調查：零命中与完成度

未建是建置出貨兩面夾擊的結論：
三份建置檔全文零 `wcs`（大小寫不敏感），
出貨五庫含 `wcs` 符號只剩兩轉換件、
1.52 全庫掃同（各 CRT 庫同兩件，
他庫零 `wcs`）——
`wcs*` 字串函式零成員。
源碼側吻合：17 檔全過組譯（无語法腐爛），
但 far 守衛系統性缺席
（`WCSLEN` 无條件、`WCSCPY` 无飽和頭、
`WCSSTR` 缺兩處長度守衛——
窄版同位全有）、
全目錄零 `AHINCR`（huge 步進在寬版不存在，
`MODELINDEP` 分支只是 far 版）；
`WCSCPY` 註解懸空（`rep movsw` 下寫
窄版 `shr` 才有的進位說明，碼對註錯）；
`WCSSET` 取長用 `inc／inc／neg`
（窄版 `not／dec` 少一指令——異手筆旁證）；
`WCSREV` 非鏡像重構
（`sub di,4` 對調指針角色、
`xchg／stosw` 三指令交換——
動機可解：`stosw` 綁死 `DI`，
故前端指針必須是 `DI`）。
每一條單看都是小毛病，
合在一起是「沒人跑完它」的形狀。

## 在執行檔裡怎麼認

本批无簽章檔，理由兩條。
第一，寬版 17 檔加兩樁全未建：
出貨庫裡沒有這些位元組，
簽章无處取樣，也无處命中。
第二，兩窄樁是編譯器產物且太短：
`coll` 全函 16 位元組、`xfrm` 28 位元組加墊，
形狀是通用呼叫包裝
（取參、調 `strcmp／strncpy／strlen`、
`mov sp,bp` 清參數）——
短到任何同旗編出的轉發函式都長这样，
固定位元組撑不起可用的簽章。
手認只認形狀：找 `call` 到
`_strcmp／_strncpy／_strlen` 的小包裝，
前後是 `push bp／mov bp,sp` 开場
与 `mov sp,bp／pop bp／ret` 收尾；
`xfrm` 有兩次呼叫，尾巴多一顆 `NOP`。
`wcscoll／wcsxfrm` 与 `WCS*` 在 1.0／1.52
出貨執行檔裡不該出現——
出現了就是別家（或自寫）的寬字串。

完整對讀見下圖（左建右未建）：

<p align="center"><img src="../../img/locale-stubs.svg" width="640" alt="locale 四樁對照：窄樁已建轉發、寬樁未建、世代差三處"></p>

## 行為規格（已證實）

測試矩陣同前篇（A 連 1.0 源碼重建、
B 連 1.52 出貨庫，兩種模擬器四格一致）。
`sign` 把比較結果壓成 −1／0／＋1。

| 呼叫 | 結果 | 範圍／方法 |
|---|---|---|
| `strcoll("a","b")` 等三案 | `−1／＋1／0` | small＋large |
| `strcoll("ab","abc")`（前綴） | `−1` | small 实測 |
| `strxfrm(d,"hello",64)` | `d=hello`、回 5 | small＋large |
| `strxfrm(d,"hello",3)`（截斷） | 回 5、`d` 前三字 `hel` | small＋large（large 只驗回傳） |
| `strxfrm(d,"hello",0)`（零） | 回 5、`d` 不動 | small 实測 |
| `strxfrm(d,"",64)`（空） | 回 0 | small 实測 |
| `strxfrm(NULL,"abc",0)`（獨立探針） | 回 3、程式存活 | small 实測（3316 步） |

small 八案（10491 步）与 large 五案（7122 步），
四格輸出全一致。

## 證據、方法與未知

靜態八組（兩樁 × S／M／C／L，
1.52 `CL /Gs /Ot` 重編，
M／L 另加 `/NT_TEXT`）：12／12 全同。
`strcoll` 在 `/Os` 下同形是巧合
（无 peephole 窗口），
`strxfrm` 在 `/Gs /Os` 下 small 差一字节、
large 全同——
旗標結論以 `strxfrm` small 案為準
（強推論：一票否决）。
殘差僅 `COMENT` 前言自注差異；
版本戳 `9D 30 73 4F` 兩邊一致。
行為見上表；A 側連結乾淨。
宽版无靜態无行為（未建品），
結論全為源碼層（`reviewed`）。

未知：寬版為何未建（時程砍還是本來就沒要）；
寬版源碼的絕對寫作時間
（只有与窄版的相對新舊）；
`mbstowcs／wcstombs` 兩轉換件的實作
（出貨有成員，源碼在 `CONVERT` 目錄，
本輪未讀）。
