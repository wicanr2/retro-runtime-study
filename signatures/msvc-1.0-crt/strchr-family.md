# Visual C++ 1.0 CRT 的搜尋家族短位元組簽章

`strchr-family.json` 是從 Visual C++ 1.0 出貨的四個 DOS CRT
函式庫（small／medium／compact／large 各一）與模型無關
helper 庫 `LIBH.LIB` 取出的廿四筆樣式：
`strchr／strrchr／strpbrk／strspn／strcspn／strstr／strtok`
七個函式，其中 `strspn` 与 `strcspn` 開場全同、
合併為四筆（`LLIBCR` 与 `LIBH` 位元組相同再合併），
其餘五個函式各四筆。
每筆都是函式開頭連續的 40 bytes，無需遮罩
（開場 40 位元組內都沒有連結時才填的位址），
固定位元組數等於樣式長度。

`inputs` 列出五個 `.LIB` 的 SHA-256，先核對版本再用
（与 `strcpy-family.json` 同一批函式庫，雜湊相同）。
`LIBH` 筆的本體與 `LLIBCR` 筆逐位元組相同，差別只在公開符號
（`__fstrchr` 對 `_strchr` 等）：它是模型無關的 far 版。

## 三支表函式開場共用骨架

`strpbrk／strspn／strcspn` 共用同一套 256 位元表骨架，
開場位元組有很長一段完全相同（small 模型）：
三支前 53 個位元組全同（含 `83 EC 20` 棧上開表、
`rep stosw` 清表、建表迴圈），
`strpbrk` 在第 53 位元組（0-based）分流，
`strspn／strcspn` 到第 81 位元組才分流——
分流處正是收尾跳轉極性（`75`＝`strspn` 失配即停 对 `74`＝`strcspn` 命中即停），
見主文章。
實務上：40 bytes 樣式命中三處即三支表函式各一，
要指名是哪一支，往後對到第 53／81 位元組。

## 各函式的四筆怎麼分

開場分三派：`strchr／strrchr` 以 `55 8B EC 57`
（手寫 `push di`）開場；`strpbrk／strspn／strcspn／strtok`
以 `55 8B EC 83 EC 20 56 57`（棧上開 32 位元組表）開場，
其中 `strtok` 緊接 `16 07`（`push ss／pop es`，
表在堆疊段）；`strstr` 以 `55 8B EC 83 EC 02 56 57`
（棧上只留 2 位元組槽）開場。
呼叫方式看結尾（`C3` 對 `CB`）與取參位移；
資料寬度看取參指令（`8B` 對 `C4／C5`）。
`strtok` 建表先於取參（字串可能為 `NULL`），
故四筆前 14 個位元組完全相同，是家族裡最穩的錨點。

| 函式 | `SLIBCR` | `MLIBCR` | `CLIBCR` | `LLIBCR／LIBH` |
|---|---|---|---|---|
| `strchr` | `8B 7E 04`，`C3` | `8B 7E 06`，`CB` | `C4 7E 04`，`C3` | `C4 7E 06`，`CB` |
| `strrchr` | `8B 7E 04`，`C3` | `8B 7E 06`，`CB` | `C4 7E 04`，`C3` | `C4 7E 06`，`CB` |
| `strpbrk` | `8B 76 06`，`C3` | `8B 76 08`，`CB` | `C5 76 08`，`C3` | `C5 76 0A`，`CB` |
| `strspn／strcspn` | `8B 76 06`，`C3` | `8B 76 08`，`CB` | `C5 76 08`，`C3` | `C5 76 0A`，`CB` |
| `strstr` | `8B 7E 06`，`C3` | `8B 7E 08`，`CB` | `C4 7E 08`，`C3` | `C4 7E 0A`，`CB` |
| `strtok` | 前 14B 四庫同 | 前 14B 四庫同 | 前 14B 四庫同 | 前 14B 四庫同 |

（取參欄只列第一個有別的位移。）

各函式的獨門特徵：`strchr` 第二遍 `repne` 後接
`8D 45 FF`（`lea ax,[di-1]`，掃過頭退一格交卷）；
`strrchr` 有 `FD`（`std` 倒掃開關）与 `FC`
（`cld` 還原）夾著第二遍，交卷是 `8D 45 01`；
`strstr` 開場不久有 `E3`（`jcxz` 空針守衛，
空針回原址）；`strtok` 有 `16 07`
（`push ss／pop es`）与靜態 `token` 的四處存取。
表組的建表迴圈含 `F3 AB`（`rep stosw` 清表）
与 `B0 01 … D2 E0`（置位用位移）。

## 掃描驗證

以 R120 的 small 測試執行檔驗證：
A 側（1.0 源碼重建，`SSTTA.EXE`）与 B 側
（1.52 出貨庫，`SSTTB.EXE`）七個函式全命中——
small 模型重建与出貨逐位元組一致
（`strtok` 的 `token+2` 編碼差只在 far-data 模型出現，
small 版兩種組譯器發射一致）。
表組三支的 40B 樣式在兩支執行檔裡各命中同一三處，
正是開場共用骨架的直接證據。
