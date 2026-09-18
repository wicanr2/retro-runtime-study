# retro-runtime-study

1990 年代 DOS／Win16／早期 Win32 程式實際連結進去的函式庫，當年是怎麼寫的、為什麼那樣寫、
編譯後在執行檔裡長什麼樣子。研究對象是這幾份原廠釋出的函式庫原始碼：

| 代號 | 來源 | 年代 | 範圍 |
|---|---|---|---|
| `borland-crtl-2.0` | Borland C++ 2.0 Runtime Library Source | 1991 | C 函式庫、數學函式庫、iostream、Windows 版函式庫 |
| `msvc-1.0-crt` | Microsoft Visual C++ 1.0 Run-Time Library Sources | 1993 | 16 位元 CRT（MS-DOS 與 Windows 3.x） |
| `msvc-2.0-crt` | Microsoft Visual C++ 2.0 Run-Time Library Sources | 1994 | Win32 CRT（x86 與 Alpha） |
| `dmx` | DMX（Paul J. Radek） | 1993–1994 | DOS 音效卡驅動與音樂／音效播放 |
| `dsmi` | DSMI（Otto Chrons） | 1992–1994 | DOS 模組音樂與音效混音 |

## 用途

- **做 remake**：遊戲呼叫的 runtime 函式有哪些邊界行為（`printf` 的格式化細節、
  `rand` 的序列、檔案 I/O 的錯誤碼、音效庫的播放時序），照這裡的行為規格重做。
- **做逆向**：反組譯時把編譯器與函式庫產生的碼先認出來、從「未讀函式」扣掉，
  心力留給遊戲本身的邏輯。
- **理解當年的工程**：分段定址、記憶體模型、有沒有數學輔助處理器——這些約束怎麼決定了程式的寫法。

## 閱讀方式

每篇文章開頭的 YAML 前置欄位寫明適用的函式庫版本、推論等級、什麼情境該讀
（`triggers`）、可以 grep 的符號（`symbols`）。AI 讀者先比對 `triggers` 與 `symbols` 再決定要讀哪篇；
術語與來源代號見 `CONTEXT.md`。
所有文章的前置欄位彙整在 `kb-index.json`，可以一次讀進來挑文章。

## 文章

| 篇 | 內容 |
|---|---|
| [五份函式庫的全景](docs/00-overview/era-and-libraries.md) | 指標寬度、浮點、作業系統三個約束怎麼決定函式庫的份數與寫法；兩家 16 位元 CRT 的做法對照；各函式庫出現在哪些遊戲 |
| [一份原始碼怎麼編出五個記憶體模型](docs/10-borland-crtl/memory-model-macros.md) | Borland RTL 的三層機制：批次檔、編譯器、`LDATA`／`LPROG` 巨集；huge 模型為什麼自己設定 DS；f 開頭的 far 版函式；在執行檔裡怎麼判斷模型 |
| [編譯器 helper：程式裡看不到的函式呼叫](docs/10-borland-crtl/compiler-helpers.md) | `LXMUL@`、`LDIV@`、長整數位移、huge 指標正規化、結構複製、堆疊檢查、`FTOL@` 的呼叫慣例、辨識方式與邊界行為（實跑） |
| [啟動與結束鏈：從載入到 main、從 exit 回到 DOS](docs/10-borland-crtl/startup-and-exit.md) | C0 在 `main` 之前做的事、初始化表與結束表的挑選規則、`exit`／`_exit`／`abort` 的差別、空指標檢查、Windows 版 C0W 的對照 |
| [檔案 I/O：FILE、緩衝區、文字模式與錯誤碼](docs/10-borland-crtl/stdio-file-io.md) | 三層架構與 `level` 的雙向語意、緩衝策略、文字模式的 CR／LF 與 `Ctrl-Z`、`ftell` 的補償、DOS 錯誤碼到 `errno`；附五個模型實跑的行為規格 |
| [printf 家族：一個引擎、三個出口，與浮點的連結開關](docs/10-borland-crtl/printf-engine.md) | `__vprinter` 與三個輸出函式、表格驅動的解析、格式寫錯時的行為、浮點轉換為什麼是連結時才接上；附五個模型實跑的格式化與 `scanf` 行為規格 |
| [near heap 與 far heap：兩套配置器，一條與堆疊的邊界](docs/10-borland-crtl/heap.md) | 兩套配置器的區塊頭與單位、first fit 與漫遊指標、切割從尾端切、`realloc` 為什麼一律搬家、near heap 與堆疊之間的 512 bytes、far heap 怎麼向 DOS 要空間；附五個模型實跑的行為規格 |
| [conio：一個結構、三個原語、兩條路](docs/10-borland-crtl/conio-screen.md) | `_video` 結構、`__cputn`／`__scroll`／`__screenio` 三個原語、直接寫顯示記憶體與走 BIOS 的岔路、CGA 雪花的偵測條件、兩套座標系；附五個模型實跑的行為規格 |
| [BCC 2.0 產生的程式碼在反組譯裡長什麼樣](docs/60-re-fingerprints/bcc20-codegen.md) | 函式進出、暫存器變數、呼叫與清堆疊、`switch` 的跳躍表與值表搜尋、各記憶體模型的資料存取、立即值編碼、浮點修正；從產生碼判斷編譯選項 |
| [從執行檔判斷 runtime 廠牌、版本與記憶體模型](docs/60-re-fingerprints/identify-vendor-and-model.md) | 壓縮檢查、TLINK 的檔頭標記、C0 進入點與字串、用簽章推記憶體模型與出貨版本、沒有簽章時從產生碼判斷 |
| [一套 CRT 原始碼怎麼變成三十幾個 .LIB](docs/20-msvc-crt/library-combination.md) | Visual C++ 1.0 的 16 位元 CRT：模型 × 浮點方式 × 環境三個維度、安裝時才合併的元件庫、模型無關的 `LIBH` 怎麼做到、與 Borland 的取捨對照（**沒有對拍**，限於建置檔所寫） |
| [DMX 的五份封存：版本號、建置設定與呼叫慣例是三件事](docs/30-dmx/version-history.md) | 五個目錄名不是五個版本：功能版本幾乎沒動，差異在建置檔位與參數傳遞方式；出貨的庫併了第三方 AWE32 支援，樹裡沒有那份原始碼；`fmfix` 同時改了 OPL3 偵測與 FM 搶聲；遊戲資料裡的 GENMIDI、DMXGUS、MUS 是這套庫的格式（**只讀原始碼**，版權標示機密與專有） |
| [DSMI：一份組語核心，三種語言接得上](docs/40-dsmi/interface-split.md) | 核心是一份組語原始碼、用組譯期符號編出 C 版與 Pascal 版兩套目的碼；CDI 這張裝置表在架構中央，MCP、GUS、無聲各是一個裝置；封存裡哪些檔根本不是原始碼（**只讀原始碼**，授權禁止逆向該套件） |
| [在 dosgolem 裡用 Borland C++ 2.0 編譯](docs/70-toolchain/bcc20-on-dosgolem.md) | 從安裝磁片映像到可重跑的命令列編譯；BGI 俄羅斯方塊範例在瀏覽器裡玩；dosgolem 為 BCC 補的七條規格 |

後續篇目、各階段目標與完成條件見 [`docs/goal/`](docs/goal/README.md)；進度看 issues。

## 工具與範例

| 路徑 | 內容 |
|---|---|
| `tools/build_index.py` | 從各篇前置欄位產生 `kb-index.json`，並檢查欄位、id 唯一與 `related` 連結 |
| `tools/gen_signatures.py` | 從你自己的 OMF 函式庫產生遮罩過修正位置的短位元組簽章，並拿它掃執行檔、對 map 檔核對 |
| `signatures/borland-crtl-2.0/` | BC++ 2.0 DOS 函式庫的簽章與準確度說明（正反對照、未對拍模組） |
| `tools/bcpp20/install.sh` | 從你自己的 BC++ 2.0 安裝磁片映像裝出 BORLANDC 目錄（容器內執行） |
| `tools/bcpp20/bcc.sh` | 在 [dosgolem](https://github.com/wicanr2/dosgolem) 裡執行 BCC、TASM、TLINK 等工具 |
| `examples/tetris/` | BGI 俄羅斯方塊：自己寫的範例程式與編譯、遊玩腳本 |
| `examples/helpers/` | 編譯器 helper 的邊界值測試程式與預期輸出 |
| `examples/stdio/` | 檔案 I/O 邊界行為的測試程式與預期輸出 |
| `examples/printf/` | 格式化與 `scanf` 邊界行為的測試程式與預期輸出 |
| `examples/heap/` | heap 配置策略的測試程式與預期輸出 |
| `examples/conio/` | 文字畫面行為的測試程式、預期輸出與預期畫面 |
| `examples/codegen/` | 各種語法構造在五個模型與各選項下的產生碼測試程式與預期輸出 |
| `examples/startup/` | `#pragma startup`／`exit` 與 `atexit` 執行順序的測試程式 |

## 邊界

- **本 repo 不含任何原始碼。** 上述函式庫的授權都禁止散布原始碼，這裡只有自己寫的說明、
  虛擬碼、圖與統計。要讀原文請自行取得合法來源。`examples/` 的程式是本 repo 自己寫的；
  工具鏈腳本不附任何 Borland 檔案，要用你自己的安裝磁片。
- 位元組簽章只收 Borland 與 Microsoft 函式庫，且只收遮罩過位址的短樣式。
- 推論等級分「已證實／強推論／假說」；沒有一手來源支持的內容標「待查證」。

## 授權

文件與圖採 [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)。
Borland、Microsoft、Visual C++ 等名稱為各自權利人的商標；
被研究的函式庫原始碼版權屬各自權利人，不在本授權範圍內。
