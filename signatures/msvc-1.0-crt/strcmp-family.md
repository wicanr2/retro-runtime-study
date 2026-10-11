# Visual C++ 1.0 CRT 的比較家族短位元組簽章

`strcmp-family.json` 是從 Visual C++ 1.0 出貨的四個 DOS CRT
函式庫（small／medium／compact／large 各一）與模型無關
helper 庫 `LIBH.LIB` 取出的十六筆樣式：
`strcmp／strncmp／stricmp／strnicmp` 各四筆
（`LLIBCR` 與 `LIBH` 位元組相同，合併為一筆）。
每筆都是函式開頭連續的 41 bytes，無需遮罩
（開場 41 位元組內都沒有連結時才填的位址），
固定位元組數等於樣式長度。

`inputs` 列出五個 `.LIB` 的 SHA-256，先核對版本再用
（与 `strcpy-family.json` 同一批函式庫，雜湊相同）。
`LIBH` 筆的本體與 `LLIBCR` 筆逐位元組相同，差別只在公開符號
（`__fstrcmp` 對 `_strcmp` 等）：它是模型無關的 far 版。
`stricmp` 的公開符號是雙底線 `__stricmp`
（另有相容別名 `__strcmpi` 同位址）。

## 各函式的四筆怎麼分

共同開場：`strcmp／stricmp` 以暫存器存檔
（`8B D7 8B DE`／`8B D6`，`SI` 塞進 `BX` 或 `DX`，不 `push`）；
`strncmp／strnicmp` 以 `57 56`（`push di／push si`）開場，
接著 `jcxz`（`E3`）零數守衛。
呼叫方式看結尾（`C3` 對 `CB`）與取參位移；
資料寬度看取參指令（`8B` 對 `C4／C5`）：

| 函式 | `SLIBCR` | `MLIBCR` | `CLIBCR` | `LLIBCR／LIBH` |
|---|---|---|---|---|
| `strcmp` | `8B 76 04`，`C3` | `8B 76 06`，`CB` | `C5 76 04`，`C3` | `C5 76 06`，`CB` |
| `strncmp` | `8B 4E 08`，`C3` | `8B 4E 0A`，`CB` | `8B 4E 0C`，`C3` | `8B 4E 0E`，`CB` |
| `stricmp` | `8B 76 06`，`C3` | `8B 76 08`，`CB` | `C5 76 08`，`C3` | `C5 76 0A`，`CB` |
| `strnicmp` | `8B 4E 08`，`C3` | `8B 4E 0A`，`CB` | `8B 4E 0C`，`C3` | `8B 4E 0E`，`CB` |

（取參欄只列第一個有別的位移；`strncmp／strnicmp`
取的是 `count`，後面緊跟 `E3`。`stricmp`
的第二參數走 `BX`：`8B 5E`／`C4 5E`。
`strnicmp` 在 `LIBH` 的目的檔名截斷為 `FSTRNICM`
（8.3 迴避），樣式本體不受影響，見主文章。）

各函式開場內的獨門特徵：`strcmp` 有 `F2 AE … F7 D1 2B F9`
（`repne` 量長、`not`、`sub di` 復位）接 `F3 A6`
（`repe cmpsb`）；`strncmp` 有 `F7 D9 03 CB`
（`neg／add` 算 `min(len+1,count)`）；`stricmp`
有 `B0 FF … AC 8A 27`（`AL=-1` 保證入迴圈、
`lodsb`、`mov ah,[bx]`）；`strnicmp` 有
`B7 41 B3 5A B6 20`（`'A'／'Z'／' '` 轉小寫常數）。

## 掃描驗證

以 R119 的 small 測試執行檔驗證：
A 側（1.0 源碼重建，`SCMPTA.EXE`）命中兩處
（`strcmp`、`strncmp`——重建与出貨逐位元組一致），
`stricmp／strnicmp` 未命中是預期的：
重建用的 `ML 6.11` 与出貨組譯器對 `cmp` 選了不同編碼
（`38 C4` 對 `3A E0`，第 21 位元組起，行為等價已證）；
B 側（1.52 出貨庫，`SCMPTB.EXE`）四處全命中——
1.52 沿用了与 1.0 出貨相同的編碼選擇。
