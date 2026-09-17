# M1 Borland runtime 分析

## 目標

證明 Borland C++ 2.0 Runtime Library Source 能代表出貨的每一個函式庫，並把三個最常在反組譯裡撞到的機制寫成文章：
一份原始碼怎麼編出五個記憶體模型、編譯器自動插入的 helper 函式、程式的啟動與結束鏈。

C 函式庫（`CS`／`CC`／`CM`／`CL`／`CH.LIB` 的一般模組）已經對拍完成，結果在[總覽文章](../00-overview/era-and-libraries.md)。
M1 補上其餘幾組：far 版字串函式、數學函式庫、Windows 版函式庫、啟動碼。

## 為什麼先做

反組譯時要先分辨「遊戲自己的程式」和「編譯器、函式庫帶進來的程式」。後者在一支遊戲裡常常占一大半函式，
而且每支 BC++ 2.0 編譯的程式都一樣。先把這部分讀懂、證明可以對應到原始碼，
M2 才能產生可信的簽章，remake 也才能照函式庫的真實行為重做（例如長整數除法遇到負數、`exit` 關檔案的順序）。

## 工作項目

### 對拍（研究項目，不公開）

| worklist id | 內容 | 完成條件 |
|---|---|---|
| `oracle-bcpp20-farfunc` | `FARFUNC.RSP` 的 31 個 far 版字串與記憶體函式（`_fstrlen` 這類） | 每個模組對兩版出貨、五個庫的比對結果 |
| `oracle-bcpp20-math` | 數學函式庫 `MATH*.LIB`，含模型無關的組語模組 | 每個模組 × 模型的比對結果；差異要有解釋或列為未解 |
| `oracle-bcpp20-math-residual` | 數學函式庫裡 `EFCVT`、`GCVT`、`SCANTOD`、`STRTOD`、`FPERR`、`_MATHERR` 對不上的原因 | 差異得到解釋，或窮盡可取得的編譯器版本後寫成結論 |
| `oracle-bcpp20-cwin` | Windows 3.x 版函式庫 `CWIN*.LIB` | 每個模組的比對結果；記下與 DOS 版在同一模組上的差異 |
| `oracle-bcpp20-c0` | 啟動碼 `C0*.OBJ`（編譯器套件的 `STARTUP.ZIP`） | 各模型、DOS 與 Windows 變體的比對結果 |

### 研究筆記（研究項目，不公開）

| worklist id | 內容 | 依賴 |
|---|---|---|
| `borland-memory-model-macros` | `RULES.ASI`、`ASMRULES.H` 與 `LDATA`／`LPROG`／`__FARFUNCS__` 在 C、`.CAS`、`.ASM` 三種原始檔裡怎麼展開；huge 與 large 的差別 | 對拍產物 |
| `borland-compiler-helpers` | 長整數乘除（`N_LXMUL@` 等）、結構複製（`SCOPY@`）、stack probe 的原始檔、呼叫慣例、各模型的目的碼 | 對拍產物 |
| `borland-startup-exit` | 從 C0 進入點到 `main`，從 `exit` 回到 DOS：初始化與結束函式表、`atexit`、關檔、中斷向量還原 | `oracle-bcpp20-c0` |
| `scroll-1991-08` | 1991-08 版 `SCROLL` 為什麼從 214 bytes 變成 383 bytes | 無 |
| `toolchain-bcpp20` | 原廠安裝時 `sys\stat.h` 這類標頭放在哪裡，扁平目錄能不能編 | 無 |

### 公開產出

| Issue | 文章 | 依賴 |
|---|---|---|
| [#1](https://github.com/wicanr2/retro-runtime-study/issues/1) | R2：一份原始碼怎麼編出五個記憶體模型（`docs/10-borland-crtl/memory-model-macros.md`） | `borland-memory-model-macros` |
| [#2](https://github.com/wicanr2/retro-runtime-study/issues/2) | R3：編譯器 helper 的原始碼、呼叫慣例、執行檔裡的樣子（`docs/10-borland-crtl/compiler-helpers.md`） | `borland-compiler-helpers` |
| [#3](https://github.com/wicanr2/retro-runtime-study/issues/3) | R4：啟動與結束鏈（`docs/10-borland-crtl/startup-and-exit.md`） | `borland-startup-exit` |

## 建議順序

1. 對拍：far 版 → 數學 → Windows 版 → 啟動碼。前面三組共用同一套比對工具，啟動碼改用組譯器。
2. 記憶體模型研究筆記 → R2。這是另外兩篇的基礎，R3、R4 都要引用各模型的 prolog／epilog 差異。
3. helper 研究筆記 → R3。
4. 啟動與結束鏈研究筆記 → R4。
5. `scroll-1991-08`、`toolchain-bcpp20`、`oracle-bcpp20-math-residual` 不擋文章，穿插處理。

## 完成條件

- 上表的研究項目與 [#1](https://github.com/wicanr2/retro-runtime-study/issues/1)–[#3](https://github.com/wicanr2/retro-runtime-study/issues/3) 全部完成。
- 三篇文章的前置欄位 `evidence` 如實標示全篇最弱的一級；有對拍支持的結論標「已證實（對拍）」。
- R3 的 helper 判準如果和維護者既有的 Borland Win16 helper 判準筆記不同，差異表已交給維護者確認，確認後才回寫。

## 風險與未知

- **數學函式庫可能有對不上的模組。** 如果原廠建庫時用的編譯器版本和出貨的 BC++ 2.0 不同，就無法完全重現。
  這不擋 R2–R4：文章只對「已對上」的模組下「已證實（對拍）」的結論，其餘標低一級。
- **R4 的 Microsoft 對照要等 M3。** VC++ 1.0 工具鏈取得前，R4 只寫 Borland，Microsoft 的部分列為未知。
- **8087 模擬器沒有原始碼。** `EMU.LIB`、`FP87.LIB` 只能寫介面與呼叫端看得到的行為。
