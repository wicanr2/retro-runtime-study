# Visual C++ 1.0 CRT 的 `strlen` 短位元組簽章

`strlen.json` 是從 Visual C++ 1.0 出貨的四個 DOS CRT 函式庫
（small／medium／compact／large 各一）與模型無關 helper 庫
`LIBH.LIB` 取出的五筆 `strlen` 樣式。每筆都是函式開頭連續的
25 或 27 bytes，無需遮罩（函式內沒有連結時才填的位址），
固定位元組數等於樣式長度。

`inputs` 列出五個 `.LIB` 的 SHA-256，先核對版本再用。
`LIBH` 那筆的本體與 `LLIBCR` 那筆逐位元組相同，
差別只在公開符號（`__fstrlen` 對 `_strlen`）：
它是給 small／medium 程式呼叫的 far 版 `strlen`，
呼叫端寫 `_fstrlen`。

四筆按「資料指標寬度 × 呼叫方式」區分：

| 樣式 | 取參 | 結尾 | 飽和分支 |
|---|---|---|---|
| `SLIBCR` | `mov di,[bp+4]`（near） | `ret` | 無 |
| `MLIBCR` | `mov di,[bp+6]`（near，far 呼叫多 2 bytes 返回位址） | `retf` | 無 |
| `CLIBCR` | `les di,[bp+4]`（far） | `ret` | 有（`75 01`） |
| `LLIBCR`／`LIBH` | `les di,[bp+6]`（far） | `retf` | 有（`75 01`） |

飽和分支只跟資料模型走：near 資料（`SLIBCR`、`MLIBCR`）沒有它，
far 資料（`CLIBCR`、`LLIBCR`、`LIBH`）才有。原因與行為見
[`strlen` 的兩種長相](../../docs/20-msvc-crt/strlen-anatomy.md)。

Borland 那邊的對應樣式已經在
[`signatures/borland-crtl-2.0/dos.json`](../borland-crtl-2.0/dos.json)
裡（`STRLEN` 模組四筆、`FSTRLEN` 一筆），這裡不再重收。
