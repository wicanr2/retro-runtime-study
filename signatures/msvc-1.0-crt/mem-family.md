# Visual C++ 1.0 CRT 的記憶體塊家族短位元組簽章

`mem-family.json` 是從 Visual C++ 1.0 出貨的四個 DOS CRT
函式庫（small／medium／compact／large 各一）與模型無關
helper 庫 `LIBH.LIB` 取出的廿八筆樣式：
`memcpy／memmove／memccpy／memchr／memcmp／memicmp／memset`
各四筆（`LLIBCR` 與 `LIBH` 位元組相同，合併為一筆）。
每筆都是函式開頭連續的 30 bytes；
廿六筆無需遮罩，
`hmemmove` 兩筆各含 `ahdiff` 呼叫位址遮罩
（`E8` 後兩位元組連結時填，`fixed` 28／27）。

`inputs` 列出五個 `.LIB` 的 SHA-256，先核對版本再用
（与 `strcpy-family.json` 同一批函式庫，雜湊相同）。
`LIBH` 筆的本體與 `LLIBCR` 筆逐位元組相同，差別只在公開符號
（`__fmemcpy` 对 `_memcpy` 等）：它是模型無關的 far 版。

## 開場分派：near 五派、far 三派

near 版（`SLIBCR／MLIBCR`）看前 7 個位元組分五派：

| 開場 | 函式 | 備註 |
|---|---|---|
| `55 8B EC 8B D7 8B DE` | `memcpy／memmove／memcmp` | 存 `DI／SI`，`ES=DS` 在後 |
| `55 8B EC 8B D7 8C D8` | `memset` | 只存 `DI`（无來源） |
| `55 8B EC 33 C0 8B 4E` | `memchr` | 先清 `AX`（`NULL` 備位） |
| `55 8B EC 56 57 8B 76` | `memicmp` | `cProc<si,di>` 存檔在前 |
| `55 8B EC 57 56 1E 07` | `memccpy` | `push di／si`＋`push ds／pop es` |

其中第一派三支的走向：
`memcmp` 第 14 個起取參順序不同
（`buf1→SI／buf2→DI`），
`memcpy／memmove` 前 23 個全同、
第 24 個分家（`0E` 对 `2A`，
`jcxz` 位移不同，後者後跟重疊判斷 `3B FE 76`）。

far 版（`CLIBCR／LLIBCR／LIBH`）看前 6 個分三派：

| 開場 | 函式 | 備註 |
|---|---|---|
| `55 8B EC 8B 4E` | `hmemcpy` 系（含 `memccpy`） | 取 `count` 先行 |
| `55 8B EC 33 C0` | `hmemcmp` 系（含 `memicmp`） | 先清 `AX` |
| `55 8B EC 8B 4E … E3 … 57` | `hmemset／hmemchr` | `jcxz` 後才 `push di` |

`hmemcmp` 系兩支開場全同除 `jcxz` 位移
（`E3 4F` 对 `E3 79`，`CASE_INSENSITIVE`
迴圈較長）；`hmemmove` 獨門是
`0B C9 75 03 E9`（七位元組零數檢查，
它支此處皆 `E3`）。
呼叫方式看取參位移（C 对 L 差 2，
`memccpy` 因多 `chr` 參數再加 2）
与結尾 `C3／CB`。

## 各函式的獨門特徵

`memcpy` 系有 `A8 01 74 02 A4 49`
（`test al,1／jz／movsb／dec` 奇位址先搬）
接 `D1 E9 F3 A5`（`shr／rep movsw`）；
`memmove` near 有 `3B FE 76`
（`cmp di,si／jbe`），far 有 `ahdiff` 呼叫
（`1E 56 06 57 E8`：推四字參數再呼叫）；
`memccpy` near 有 `AC AA 3A C4 74 04 E2`
（`lodsb／stosb／cmp／je／loop`），
far 有 `MIN` 分塊接逐 byte 比對；
`memchr` 有 `F2 AE`（`repne scasb`）：
near 接 `B8 00 00 75`（`mov ax,0／jne`：
先備 `NULL` 再問命中）、
far 接 `74`（`je` 直跳命中）；
`memcmp` near 有 `F3 A6`（`repe cmpsb`）
接 `1B C0 83 D8 FF`（`sbb` 對子取 −1／＋1，
far 版同對子在窗外）；
`memicmp` near 有 `B7 41 B3 5A B6 20`
（預載 `'A'／'Z'／' '`）
接 `3A E7 72 06 3A E3 77 02 02 E6`
（`cmp／jb／cmp／ja／add` 夾大寫轉小寫，
兩邊各一遍）、
far 有 `2C 41 3C 1A 1A DB 80 E3 20 02 C3 04 41`
（`sub／cmp／sbb／and／add／add` 無分支映射，
兩邊各一遍，0-based 第 90 位元組起）；
`memset` near 有 `8A E0`（`AL→AH` 擴字）
接 `F7 C7 01 00 74`（`test di,1／jz`）
与 `F3 AB`（`rep stosw`；
far 版无奇位址試探，直入 `shr`）。

## 掃描驗證

以 R112／R113／R115／R116 的測試執行檔驗證
（`--map` 对位，命中位置与符號全对，
零誤報、零漏報——除下述一處预期 miss）：

| 執行檔 | 命中 | 說明 |
|---|---|---|
| `MEMTA／MEMTB`（small，R112） | 3／3 兩側 | `cpy／mov`＋附帶 `set`（測試程式備緩衝拉進） |
| `MEMTA／MEMTB`（medium，R112） | 3／3 兩側 | 同上（`MLIBCR` 筆） |
| `HMEMTA／HMEMTB`（large，R113） | 3／3 兩側 | `cpy／mov／ccy`（`LLIBCR／LIBH` 合併筆） |
| `MEMSTA／MEMSTB`（small，R115） | 4／4 兩側 | `set／chr／cmp／icmp` |
| `HMEMSTA／HMEMSTB`（large，R115） | 4／4 兩側 | 同上（合併筆） |
| `MCCPYTB`（B 側，R116） | 1／1 | near `ccy` |
| `MCCPYTA`（A 側，R116） | 0／1（预期） | 見下 |

`MCCPYTA` 未命中是預期的：
重建用的 `ML 6.11` 与出貨組譯器對 `cmp al,ah`
選了不同編碼（`38 E0` 对 `3A C4`，
0-based 第 23 位元組，R116 已證行為等價），
簽章保留出貨編碼；
B 側（1.52 出貨庫）命中——
1.52 沿用了与 1.0 出貨相同的編碼選擇
（与 `strcmp` 家族的 `stricmp` 同式）。
far `memicmp` 的兩處編碼差与 near `memicmp`
的五處皆在 30B 窗外，兩側全命中。
