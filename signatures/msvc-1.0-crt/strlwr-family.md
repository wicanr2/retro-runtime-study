# Visual C++ 1.0 CRT 的大小寫家族短位元組簽章

`strlwr-family.json` 是從 Visual C++ 1.0 出貨的四個 DOS CRT
函式庫（small／medium／compact／large 各一）與模型無關
helper 庫 `LIBH.LIB` 取出的廿筆樣式：
`strlwr／strupr／strset／strnset／strrev` 各四筆
（`LLIBCR` 與 `LIBH` 位元組相同，合併為一筆）。
每筆都是函式開頭連續的 30 bytes，無需遮罩
（開場 30 位元組內都沒有連結時才填的位址），
固定位元組數等於樣式長度。

`inputs` 列出五個 `.LIB` 的 SHA-256，先核對版本再用
（与 `strcpy-family.json` 同一批函式庫，雜湊相同）。
`LIBH` 筆的本體與 `LLIBCR` 筆逐位元組相同，差別只在公開符號
（`__fstrlwr` 對 `__strlwr` 等）：它是模型無關的 far 版。

## strlwr 与 strupr 只差兩個位元組

small 模型兩支各 30 個位元組，全同除第 11 与第 17
（0-based）：`sub` 常數 `41h／61h`（`'A'／'a'`）
与 `add` 常數 `61h／41h` 互換；
範圍比較同為 `3C 1A`（兩邊區間等長 26，
組出來一字不差）。認出其中一支，
把這兩處常數對調即另一支。

## 各函式的四筆怎麼分

開場分三派：`strlwr／strupr` 以
`55 8B EC 8B 5E 04 8B D3`
（無 `push` 存檔：`BX` 當游標、`DX` 存回傳）開場；
`strset／strnset／strrev` 以 `55 8B EC 57`
（手寫 `push di`，`strrev` 另加 `56`）開場，
其中 `strnset` 緊接 `jcxz`（`E3`）零數守衛。
呼叫方式看結尾（`C3` 對 `CB`）與取參位移；
資料寬度看取參指令（`8B` 對 `C4／C5`）：

| 函式 | `SLIBCR` | `MLIBCR` | `CLIBCR` | `LLIBCR／LIBH` |
|---|---|---|---|---|
| `strlwr` | `8B 5E 04`，`C3` | `8B 5E 06`，`CB` | `C4 5E 04`，`C3` | `C4 5E 06`，`CB` |
| `strupr` | `8B 5E 04`，`C3` | `8B 5E 06`，`CB` | `C4 5E 04`，`C3` | `C4 5E 06`，`CB` |
| `strset` | `8B 7E 04`，`C3` | `8B 7E 06`，`CB` | `C4 7E 04`，`C3` | `C4 7E 06`，`CB` |
| `strnset` | `8B 7E 04`，`C3` | `8B 7E 06`，`CB` | `C4 7E 04`，`C3` | `C4 7E 06`，`CB` |
| `strrev` | `8B 7E 04`，`C3` | `8B 7E 06`，`CB` | `C4 7E 04`，`C3` | `C4 7E 06`，`CB` |

（取參欄只列第一個有別的位移；
`strlwr／strupr` 目的字串走 `BX`，
故取參是 `5E` 而非 `7E`。）

各函式的獨門特徵：`strlwr／strupr` 開場不久有
`EB 0B`（跳入迴圈先測 `NUL`）接
`2C 41／61 3C 1A 73 04`（無號區間檢查）；
`strset` 有 `F7 D1 49`（`not` 得長度加一、
`dec` 扣掉 `NUL`）接 `F3 AA`（`rep stosb` 全填）；
`strnset` 有 `2B D9`（`sub bx,cx` 取 `min`）；
`strrev` 有 `83 F9 FE`（`cmp cx,-2` 空串守衛）
接 `3B F7`（`cmp si,di` 相遇檢查）。

## 掃描驗證

以 R121 的 small 測試執行檔驗證：
A 側（1.0 源碼重建，`SCSTTA.EXE`）与 B 側
（1.52 出貨庫，`SCSTTB.EXE`）五個函式全命中——
本批零編碼差，重建与兩版出貨逐位元組一致。
