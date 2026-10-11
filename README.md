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
| `watcom-6.5` | Watcom C Optimizing Compiler and Tools V6.5 | 1988 | 16 位元 DOS 編譯器、函式庫與工具 |
| `watcom-7.0` | Watcom C 386 V7.0 | 1989 | 第一代 32 位元編譯器；搭 Phar Lap 或 OS/386 的 DOS extender |
| `watcom-8.0` | Watcom C/SQL 386 8.0（1990）的 CNW386 BBS 六包；SKU 依據是編譯器 banner | 1990 | 早期 32 位元；NetWare／Btrieve 市場的開發支援 |
| `watcom-9.01` | Watcom C/C++ 9.01 | 1992 | DOS/4GW 年代的工具鏈；磁片內容為 Watcom 自家壓縮封裝 |
| `watcom-9.5` | Watcom C/C++ 9.5b（1993-05-05） | 1993 | 20 張 1.44 MB 磁片；DOS/NT/OS2/NetWare/AutoCAD；16 位元目標回歸 |

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
| [亂數與時間：一個 LCG、兩個 DOS 服務、寫死的美國日光節約規則](docs/10-borland-crtl/rand-time.md) | `rand` 的 LCG 與參考序列、`random`／`randomize` 其實是巨集、`time` 家族讀的 DOS／BIOS 服務、`TZ` 的剖析與靜默退回、1986 年美國修法寫進函式庫的 DST 分支、`clock` 的 tick 單位、**`mktime` 對閏年 2 月 29 無限迴圈**；附五個模型實跑的行為規格 |
| [iostream：兩套 stream、自有格式化引擎、與啟動鏈的接點](docs/10-borland-crtl/iostream.md) | 新式（iostream.h，191 模組在主庫）與舊式（stream.h，8 模組獨立 oldstrm 庫）兩族；類別地圖與預設值；自有整數／AT&T 浮點轉換（**預設下 0.0001 印成 0**）；width 一次性；cin/cout 的建構時機與 tie；與 stdio 的緩衝分離和 sync_with_stdio；附五個模型實跑的行為規格 |
| [BCC 2.0 產生的程式碼在反組譯裡長什麼樣](docs/60-re-fingerprints/bcc20-codegen.md) | 函式進出、暫存器變數、呼叫與清堆疊、`switch` 的跳躍表與值表搜尋、各記憶體模型的資料存取、立即值編碼、浮點修正；從產生碼判斷編譯選項 |
| [從執行檔判斷 runtime 廠牌、版本與記憶體模型](docs/60-re-fingerprints/identify-vendor-and-model.md) | 壓縮檢查、TLINK 的檔頭標記、C0 進入點與字串、用簽章推記憶體模型與出貨版本、沒有簽章時從產生碼判斷 |
| [BCC 2.0 的 C++ 產生碼指紋：mangling、vtable、靜態建構](docs/60-re-fingerprints/bcc20-cxx-codegen.md) | `@類別@名稱$q參數` 的 mangling 與型別碼表、`$bctr`/`$blsh` 等運算子碼、以類別命名的 vtable 虛擬段與近指標表、`call word ptr [bx+N]` 虛擬呼叫、`@_STCON_$qv` 與 `_INIT_` 優先序 32 |
| [一套 CRT 原始碼怎麼變成三十幾個 .LIB](docs/20-msvc-crt/library-combination.md) | Visual C++ 1.0 的 16 位元 CRT：模型 × 浮點方式 × 環境三個維度、安裝時才合併的元件庫、模型無關的 `LIBH` 怎麼做到、與 Borland 的取捨對照（**沒有對拍**，限於建置檔所寫） |
| [strlen 的兩種長相](docs/20-msvc-crt/strlen-anatomy.md) | 兩家 `strlen` 共用 `repne scasb` 倒數慣用法；差別在 far 飽和分支、NULL 檢查與 `DI` 保存位置；Microsoft 出貨位元組對拍 |
| [strcpy 家族：複製、串接與它們的邊界](docs/20-msvc-crt/strcpy-family.md) | `strcpy／strcat` 兩遍式（量長再整塊搬、奇位址湊對齊）；`strncpy／strncat` 有界版與段偏移 0 的失控複製；原廠 ML 重編對拍 |
| [strcmp 家族：三值比較與大小寫折疊](docs/20-msvc-crt/strcmp-family.md) | `strcmp` 量長加 `repe` 比、`stricmp` 快慢雙路；`strncmp` 重比末位元組、`strnicmp` 無 `repe` 迴圈；兩種作者手筆；原廠 ML 重編對拍 |
| [strchr 家族：搜尋的三種算法與分詞狀態機](docs/20-msvc-crt/strchr-family.md) | 單字元兩遍式與方向旗借還、子串樸素雙層、集合組的 256 位元表、`strtok` 靜態狀態機；原廠 ML 重編對拍 |
| [strlwr 家族：就地改寫的五種手法](docs/20-msvc-crt/strlwr-family.md) | 五支就地改寫：`BX` 游標、無號區間檢查、跳入式迴圈、`cEnd` 三式收尾；原廠 ML 重編對拍 |
| [mem 家族：記憶體塊的複製、比較、搜尋與填寫](docs/20-msvc-crt/mem-family.md) | 一源多件（`MEM_MOVE` 開關、`hmemcpy` 三件）、重疊兩關判定、`memccpy` 尾式；原廠 ML 重編對拍 |
| [locale 樁與未建的寬字串](docs/20-msvc-crt/locale-stubs.md) | C locale 下 `strcoll／strxfrm` 恆等樁、建置旗記載、`WCS` 未建三面夾擊；原廠 CL 重現 |
| [DMX 的五份封存：版本號、建置設定與呼叫慣例是三件事](docs/30-dmx/version-history.md) | 五個目錄名不是五個版本：功能版本幾乎沒動，差異在建置檔位與參數傳遞方式；出貨的庫併了第三方 AWE32 支援，樹裡沒有那份原始碼；`fmfix` 同時改了 OPL3 偵測與 FM 搶聲；遊戲資料裡的 GENMIDI、DMXGUS、MUS 是這套庫的格式（**只讀原始碼**，版權標示機密與專有） |
| [DMX 的 API 核心：四播放器、兩張表與一個軟體時鐘](docs/30-dmx/api-core.md) | 四播放器、兩張函式表、`HIWORD` 路由、MUS 雙槽鏈結淡變、內嵌 `Midi2Mus`、TSM 軟體時鐘（**只讀原始碼**，無簽章） |
| [DMX 的裝置與格式：七驅動、出貨雙軌與混音六入口](docs/30-dmx/devices-formats.md) | 七驅動規格、出貨雙軌（`dmx.mif` 20 對備份 5）、FM 三段式、DMA 多頁環混音六入口、SFX 四格式、`fmfix` 雙向差（**只讀原始碼**，無簽章） |
| [DMX 怎麼跟 DOS extender 相處：DPMI、跳板與看門狗](docs/30-dmx/extender-usage.md) | 實用三組 DPMI 加六個零呼叫包裝器、跳板生平、旗標交換、看門狗補服、DMA 只進不出；勘誤 Watcom 篇（**只讀原始碼**，無簽章） |
| [DSMI：一份組語核心，三種語言接得上](docs/40-dsmi/interface-split.md) | 核心是一份組語原始碼、用組譯期符號編出 C 版與 Pascal 版兩套目的碼；CDI 這張裝置表在架構中央，MCP、GUS、無聲各是一個裝置；封存裡哪些檔根本不是原始碼（**只讀原始碼**，授權禁止逆向該套件） |
| [DSMI 核心與驅動：混音、播放、輸出怎麼串起來](docs/40-dsmi/core-drivers.md) | 載入器→AMP→CDI→MCP→SDI 五層單向資料流；CDI 按聲道範圍註冊三裝置；MCP 單一混音迴圈四分支＋IF16/IF32；AMP 24 個 CMD 效果器；自動偵測只走 GUS→PAS→Aria→SB 四層；LOADM 魔術數字辨認順序（**只讀原始碼**，無反組譯無位元組簽章） |
| [啟動碼：四種進入點、一份源](docs/50-msvc2-crt/startup.md) | 一份 `CRT0` 源、兩個開關編出四種進入點；`main` 只是七步的最後一步 |
| [字串：對齊、位圖、locale 三條線](docs/50-msvc2-crt/string.md) | 窄字元組語熱路徑加 C locale、寬字元素 C、多位元組 73 檔；對齊、位圖、`locale` 三條線 |
| [輸入輸出：兩張表、三建引擎、文字翻譯層](docs/50-msvc2-crt/stdio.md) | `STDIO` 管流、`LOWIO` 管 fd；兩張表、三建引擎、文字翻譯層 |
| [轉換：整數雙引擎、浮點黑箱、字元雙生](docs/50-msvc2-crt/convert.md) | 整數 `xtoa` 雙頭反轉、浮點黑箱、字元分類大小寫雙生 |
| [堆：描述子鏈加 region](docs/50-msvc2-crt/heap.md) | Win32 描述子鏈加 region 分配器：8 位元組描述子、`VirtualAlloc` 取頁 |
| [行程與 DOS：單樞 spawn、誤碼表、碟號協定](docs/50-msvc2-crt/exec.md) | 三十二種 `spawn／exec` 收斂到 `SPAWNVE` 單樞紐；五種等待模式與假 overlay |
| [例外處理：NT 借殼、查表三重、鉤子三件](docs/50-msvc2-crt/eh.md) | 借 NT SEH 的殼裝 C++ 型別查表；`'msc'` 例外碼標籤、三參數、x86／MIPS 同源 Alpha 另樹 |
| [串流：三層塔、格式借道、換底同步](docs/50-msvc2-crt/iostream.md) | 三層塔（`ios` 狀態、中層格式化、底層三出口）、格式借道、換底同步 |
| [時間雜項目錄：時區雙源、locale 六樞、訊號兩制](docs/50-msvc2-crt/timemisc.md) | 時間四式互轉、時區雙源、`locale` 六樞、訊號兩制；`_disable` 體內 `sti` 之謎 |
| [收尾：連結選項、啟動樁、Alpha 差量與四味建置](docs/50-msvc2-crt/alphabuild.md) | 連結開關六件、啟動樁、Alpha 差量；四味建置收尾（2.0 完結） |
| [Watcom C 6.5：1988 年就把參數放進暫存器](docs/80-watcom/watcom65-runtime.md) | 暫存器呼叫慣例與它在符號上的痕跡（尾端底線）、四組 far 指標 helper 為什麼存在、六個記憶體模型的位元旗標與巨集、啟動碼與 Borland C0 的逐項對照；`_r`／`_s` 命名一路延續到 DMX |
| [Watcom C/386 7.0：編譯器賣你，作業系統得跟別人買](docs/80-watcom/watcom386-extenders.md) | 1989 年的 32 位元 DOS 程式怎麼分工：編譯器廠商只賣碼與函式庫，DOS extender 要另外買；連 PSP 都是 extender 約定的選擇器；兩種呼叫慣例的啟動碼差的幾乎只有符號名；16 位元那組指標 helper 到 32 位元全部消失 |
| [DPMI 與 DOS/4GW：32 位元 DOS 程式不必再為每家 extender 編一次](docs/80-watcom/dos4gw-startup.md) | 音效卡中斷在保護模式下的三個難題（DMA 的 1 MB 限制、記憶體鎖定、兩種模式都要能接）與 DPMI 的解法；extender 從自己買到隨編譯器附的三個階段；stub 怎麼找到 `dos4gw.exe` 再把自己交出去，以及 9.01 的啟動碼怎麼在執行時認出自己跑在哪一家 extender 上 |
| [9.01 的啟動多型：一份函式庫、三條啟動路徑、兩層機制](docs/80-watcom/watcom90-startup.md) | DOS 庫與 OS/2 庫逐位元組相同（Family API）；庫內啟動 `CSTART→OS2MAIN→__CMain` 的 `__OS2Main` 之謎；隨附的變體啟動檔（一份原始碼＋條件組譯→三顆 .obj，AutoCAD ADS／ADI 用）；連結期選啟動家族 vs 執行期偵測 extender 的兩層邊界 |
| [8.0 到 9.5：工具鏈補齊、Family API 實驗、與 DOS/4GW 的回歸](docs/80-watcom/watcom80-delta.md) | WLINK／WLIB／WCC386P 自 8.0 起隨附；NetWare NLM 與 Btrieve 市場；clib3r 327→334→369 的家族消長；9.01 Family API 啟動只用一版、9.5 回歸 DOS 直接啟動並加入 DOS/4GW 專屬符號；函式庫容器從裸 OMF 到 WPK 自壓縮 |
| [9.5 的 DOS/4GW 啟動：偵測鏈、三個專屬符號、16 位元回歸](docs/80-watcom/watcom95-dos4gw.md) | 偵測鏈反組譯（AH=30h 帶 `EBX='PHAR'`、高字 4458h/4243h、AX=FF00h 探測）；`__GDAptr`／`__D16Infoseg`／`__x386_zero_base_selector` 解碼；`__Extender` 編號 9.5≠9.01；16 位元庫回歸（413 模組、overlay 支援） |
| [拿到一支 Watcom 編譯的程式，怎麼判斷是哪一代](docs/80-watcom/watcom-lineage.md) | 三代 Watcom（1988／1989／1992）的判斷表：符號形態、helper 的有無（那組 far 指標 helper 是分水嶺）、段名、啟動碼行為、執行檔結構；附各版的證據強度對照 |
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
| `examples/randtime/` | 亂數與時間的測試程式與預期輸出（期望值來自獨立參考模型；含 `mktime` 2 月 29 無限迴圈的重現） |
| `examples/iostream/` | iostream 的格式化、預設串流與 stdio 混用、全域物件建構順序的測試程式與預期輸出 |
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
