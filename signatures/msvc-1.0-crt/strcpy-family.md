# Visual C++ 1.0 CRT 的複製／串接家族短位元組簽章

`strcpy-family.json` 是從 Visual C++ 1.0 出貨的四個 DOS CRT
函式庫（small／medium／compact／large 各一）與模型無關
helper 庫 `LIBH.LIB` 取出的十六筆樣式：
`strcpy／strncpy／strcat／strncat` 各四筆
（`LLIBCR` 與 `LIBH` 位元組相同，合併為一筆）。
每筆都是函式開頭連續的 27 bytes，無需遮罩
（四個函式內都沒有連結時才填的位址），固定位元組數等於樣式長度。

`inputs` 列出五個 `.LIB` 的 SHA-256，先核對版本再用
（与 `strlen.json` 同一批函式庫，雜湊相同）。
`LIBH` 筆的本體與 `LLIBCR` 筆逐位元組相同，差別只在公開符號
（`__fstrcpy` 對 `_strcpy` 等）：它是給 small／medium
程式呼叫的 far 版，呼叫端寫 `_fstrcpy` 等。

## 各函式的四筆怎麼分

共同前綴：`strcpy／strcat` 以 `8B D7 8B DE`
（`mov dx,di／mov bx,si`，暫存器存檔，不 `push`）開場；
`strncpy／strncat` 以 `57 56`（`push di／push si`）開場。
呼叫方式看結尾（`C3` 對 `CB`）與取參位移；
資料寬度看取參指令（`8B` 對 `C4／C5`）：

| 函式 | `SLIBCR` | `MLIBCR` | `CLIBCR` | `LLIBCR／LIBH` |
|---|---|---|---|---|
| `strcpy` | `8B 76 06`，`C3` | `8B 76 08`，`CB` | `C5 76 08`，`C3` | `C5 76 0A`，`CB` |
| `strncpy` | `8B 4E 08`，`C3` | `8B 4E 0A`，`CB` | `8B 4E 0C`，`C3` | `8B 4E 0E`，`CB` |
| `strcat` | `8B 7E 04`，`C3` | `8B 7E 06`，`CB` | `C4 7E 04`，`C3` | `C4 7E 06`，`CB` |
| `strncat` | `8B 7E 04`，`C3` | `8B 7E 06`，`CB` | `C4 7E 04`，`C3` | `C4 7E 06`，`CB` |

（取參欄只列第一個有別的位移；far 版另有 `1E`／`06`
的段暫存。`strncpy` 另以 `E3`（`jcxz` 零數守衛）、
`strcat` 以 `8D 75 FF`（掃尾倒帶）、`strncat` 以 `4F`
（`dec di`，同作用）為家族特徵，見主文章。）

語意、邊界行為与為何長這樣，見
[`strcpy 家族：複製、串接與它們的邊界`](../../docs/20-msvc-crt/strcpy-family.md)。
樣式以 `tools/gen_signatures.py make` 從出貨 `.LIB` 產生
（取前 27 bytes），再以 `scan` 在 `STRTA10.EXE`
（small 模型 1.0 世界實測程式）驗證命中四筆且模型判讀正確。
