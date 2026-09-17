# Borland C++ 2.0 DOS 函式庫的短位元組簽章

`dos.json` 是從 Borland C++ 2.0（1991-04 磁片版）出貨的 13 個 DOS 函式庫產生的 2,726 筆樣式，
用來在執行檔裡標出連結進去的函式庫函式。每筆最多 64 bytes，連結時才填的位元組已經遮掉，不含完整函式。

產生與掃描都用 [`tools/gen_signatures.py`](../../tools/gen_signatures.py)。從自己的 `.LIB` 以同樣的順序重新產生，結果應該逐位元組相同
（`dos.json` 的 `inputs` 列出每個 `.LIB` 的 SHA-256，可以先核對是不是同一版）：

```sh
L=~/bcpp20/LIB
python tools/gen_signatures.py make $L/CS.LIB $L/CC.LIB $L/CM.LIB $L/CL.LIB $L/CH.LIB \
    $L/MATHS.LIB $L/MATHC.LIB $L/MATHM.LIB $L/MATHL.LIB $L/MATHH.LIB $L/EMU.LIB $L/FP87.LIB $L/OVERLAY.LIB \
    --source borland-crtl-2.0 -o dos.json
```

## 怎麼用

```sh
python tools/gen_signatures.py scan signatures/borland-crtl-2.0/dos.json GAME.EXE -o matches.json
```

`matches.json` 的每一筆是一個命中位置：

| 欄位 | 意思 |
|---|---|
| `offset` | 檔案位移（含 MZ 檔頭）。減掉檔頭大小（MZ 檔頭 `08h` 的 `e_cparhdr` × 16）得到載入映像內的位移；IDA 預設把映像放在 `1000:0000`，IDA 位址 ＝ `10000h` ＋ 這個值 |
| `names` | 這個樣式對應的公開符號；別名或前 64 bytes 相同的函式會列好幾個 |
| `fixed` | 樣式裡固定（未遮罩）的位元組數，越多越可信 |
| `libs` | 樣式出現在哪些 `.LIB`，可以拿來推記憶體模型（`CS`＝small、`CC`＝compact、`CM`＝medium、`CL`＝large、`CH`＝huge）。`scan` 另外印出並在輸出檔的 `single_lib_hits` 記下每個函式庫「只屬於它」的命中數 |
| `alternatives` | 同一位置也命中、但固定位元組較少的其他樣式 |

**推記憶體模型**：把所有命中的 `libs` 取交集。只出現在單一模型的樣式越多，結論越強；
多模型共用的樣式（例如與記憶體模型無關的 helper）不提供資訊。

簽章檔的欄位：

| 欄位 | 意思 |
|---|---|
| `signatures[].pattern` | 十六進位位元組，`..` 是遮罩（目的檔裡有修正的位置） |
| `signatures[].modules` | 函式所在的模組名（函式庫裡的目的檔名） |
| `skipped` | 固定位元組少於 12 的函式，不收，只列名稱 |

## 來源

`CS`、`CC`、`CM`、`CL`、`CH.LIB`（C 函式庫，含 iostream）、`MATHS`／`C`／`M`／`L`／`H.LIB`（數學函式庫）、
`EMU.LIB`（8087 模擬器）、`FP87.LIB`（有 8087 時用）、`OVERLAY.LIB`（VROOMM overlay 管理器）。

不含：Windows 版函式庫（`CWIN?.LIB`，NE 格式程式用）、`GRAPHICS.LIB`（BGI）、tiny 模型（與 small 共用 `CS.LIB`）。

## 準確度

| 測試 | 內容 | 結果 |
|---|---|---|
| 正對照 | 本 repo 的 `examples/codegen`、`helpers`、`startup`、`tetris` 各以五個記憶體模型重編，共 20 支執行檔；逐筆與 TLINK 的 map 檔核對 | 1,171 個命中位置**全部正確**；map 裡有樣式的函式名稱 1,315 個，命中 1,283 個（97.6%） |
| 反對照 | 60 支以其他工具鏈連結的 DOS 程式（runtime 版權字串為 Microsoft C 44 支、Watcom C 13 支、Turbo Pascal 3 支；Microsoft 裡 9 支是 EXEPACK 壓縮檔） | 命中 0 |
| 反對照 | 1 MB 決定性假亂數 | 命中 0 |

正對照漏掉的 32 個名稱：compact、large、huge 模型的 `_malloc`、`_realloc` 是短小的轉接函式，太短未收；
`__graphexit`、`__graphgetmem`、`__graphfreemem` 在 `tetris` 裡實際連結的是 `GRAPHICS.LIB` 的版本。

## Borland C++ 2.0 自己的四支工具

| 工具 | IDA 認出的函式 | 命中位置 | 起點與 IDA 函式對上 | 命中的內容 |
|---|---:|---:|---:|---|
| TLIB.EXE | 199 | 89 | 82 | 檔案 I/O、stdio、記憶體配置、字串、結束鏈 |
| BCC.EXE | 2,378 | 9 | 8 | `F_LUMOD@`、`F_SPUSH@`、7 個 overlay 管理函式 |
| TLINK.EXE | 403 | 8 | 8 | `F_LXMUL@`、7 個 overlay 管理函式 |
| TASM.EXE | 1,150 | 0 | 0 | — |

沒有 map 檔可以核對，這些命中的對錯無法逐筆證明；同一位置出現不同名稱的樣式（衝突）與同名多處命中都是 0。
TLIB 的命中裡有 59 個只屬於 `CC.LIB`，其餘是多模型共用的樣式，強推論 TLIB 以 compact 模型編譯。
BCC、TLINK、TASM 的命中很少，也沒有 printf 引擎、錯誤訊息表這類字串，強推論它們沒有連結標準的 C 函式庫。

## 未對拍的模組

這些樣式照樣取自出貨的 `.LIB`，比對執行檔不受影響；但本 repo 以原廠工具鏈重編 RTL 原始碼時，這些模組**重現不出與出貨版相同的目的碼**，
或者根本沒有原始碼。文章裡描述它們的行為時，證據只到「原始碼寫的是這樣」，不能保證等於出貨的程式碼。

| 模組 | 樣式數 | 函式 | 原因 |
|---|---:|---|---|
| `STRTOD` | 5 | `_strtod` | 以 BCC 2.0 各種參數與 Turbo C 2.0 都重現不出來 |
| `FPERR` | 5 | `__fperror` | 同上 |
| `_MATHERR` | 5 | `__matherr` | 同上 |
| `SCANTOD`（`MATH?.LIB` 裡的那份） | 0 | — | 同上；公開符號都太短，沒有收。`C?.LIB` 裡另一份已對拍相同 |
| `EFCVT` | 10 | `_ecvt`、`_fcvt` | 只有關掉暫存器變數（`-r-`）才對得上 |
| `GCVT` | 5 | `_gcvt` | small、medium 只有加 `-r-` 才對得上；compact、large、huge 相同 |
| `EMU.LIB` 全部 | 7 | `e086_Entry`、`e087_Entry` 等 | 模擬器本體沒有附原始碼；`FPINIT` 有原始碼但沒有對拍 |
| `FP87.LIB` 全部 | 4 | `e087_Entry`、`___fpreset` 等 | 沒有附原始碼 |
| `OVERLAY.LIB` 全部 | 21 | `__OVRINIT`、`__INITEMS` 等 | overlay 管理器沒有附原始碼 |

## 限制

- **只認得出與 BC++ 2.0（1991-04）逐位元組相同的函式。** Turbo C 2.0、Turbo C++ 1.0、Borland C++ 3.x 的函式庫有部分函式相同、部分不同，
  沒命中不代表不是 Borland。帶 `Turbo C++ - Copyright 1990` 字串的 13 支程式每支命中 4–100 個，帶 `Borland C++ - Copyright 1993` 的 2 支命中 3 與 18 個，
  對錯未核對。
- **1991-08 版只有 `SCROLL` 不同。** 那一版的 `__SCROLL` 不會命中本檔，另見 [`scroll-1991-08.json`](scroll-1991-08.md)。
- 305 個函式太短不收（`skipped`），多半是只有一個跳躍或回傳常數的轉接函式。
- `--self-test` 只合成了一種修正記錄格式；實際函式庫各種修正格式的解析正確性，靠上面的正對照（1,171 個命中、0 誤判）間接確認。
- 「其他 Borland 程式」的統計不含本機工作目錄裡 BC++ 2.0 附帶工具的複本與自編測試程式。
- 樣式只比函式開頭。兩個函式開頭 64 bytes 完全相同時合併成一筆（`names` 會有多個），命中時無法分辨是哪一個。
