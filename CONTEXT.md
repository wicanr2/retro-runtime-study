# 術語與來源代號

另見文末「前置欄位的值」：文章 YAML 各欄位可以填什麼。

## 來源代號

文章前置欄位的 `libraries` 與正文的版本標示一律用下列代號。

| 代號 | 指的是 | 不要跟它混用的 |
|---|---|---|
| `borland-crtl-2.0` | Borland C++ 2.0 附的 Runtime Library Source（1991） | Turbo C 各版、BC++ 3.x 以後的 RTL |
| `msvc-1.0-crt` | Visual C++ 1.0 的 16 位元 CRT 原始碼（1993） | MSC 5／6／7、VC++ 1.5 |
| `msvc-2.0-crt` | Visual C++ 2.0 的 Win32 CRT 原始碼（1994）；本封存有 x86 與 Alpha 兩包（RISC 版另有 MIPS，不在封存內） | 16 位元 CRT |
| `dmx` | DMX 音效函式庫；版本另外標（3.3b、3.3d、3.3gs、3.4a、3.7） | 用 DMX 的遊戲自帶的 `.LIB` 版本 |
| `dsmi` | DSMI（Digital Sound and Music Interface） | 以 DSMI 為基礎的 DMP 播放器 |

## 術語

| 術語 | 意思 |
|---|---|
| 記憶體模型（memory model） | 16 位元 x86 編譯器決定程式碼指標與資料指標是 16 位元（near）還是 32 位元段:位移（far）的一組設定；small／compact／medium／large／huge 各一種組合，函式庫要各編一份 |
| near／far 指標 | near 只存段內位移（2 bytes），far 另外帶段值（4 bytes）；呼叫與存取的指令、速度、可定址範圍都不同 |
| `LPROG`／`LDATA` | Borland RTL 內部用的兩個條件編譯開關：程式碼指標是否為 far、資料指標是否為 far。同一份原始碼靠它們切出各記憶體模型的版本 |
| `.CAS` | Borland RTL 的「C 加內嵌組語」原始檔，編譯時要經過 TASM |
| TASM | Turbo Assembler，Borland 的組譯器 |
| TLIB／`.RSP` | Borland 的 library 管理工具，與它讀的回應檔（列出要收進 `.LIB` 的模組） |
| CRT | C runtime，C 程式執行時需要的函式庫與啟動碼 |
| helper（編譯器輔助函式） | 編譯器遇到硬體沒有直接指令的運算（32 位元乘除、結構整塊複製、堆疊檢查）時自動插入的呼叫，原始程式碼裡看不到它 |
| prolog／epilog | 函式開頭建立堆疊框、結尾拆掉它的固定指令序列；記憶體模型與 Windows 相容需求會讓它長得不一樣 |
| 8087 emulator | 沒有數學輔助處理器的機器上，用軟體模擬浮點指令的執行時元件 |
| 對拍（oracle） | 用原廠工具鏈重新編譯，拿產物驗證文章的說法 |
| DOS extender | 讓 DOS 程式在 32 位元保護模式下執行、需要時再切回真實模式呼叫 DOS 的執行時元件 |
| DPMI | DOS Protected Mode Interface，保護模式程式向主機要記憶體、掛中斷、呼叫真實模式程式碼的標準介面 |
| flat model | 32 位元下所有段都從 0 開始、涵蓋整個位址空間，程式只看到一個平面位址 |
| `CMACROS.INC` | Microsoft 16 位元組語用的巨集套件，依記憶體模型展開函式進出與參數存取 |
| 組合函式庫 | Visual C++ 1.0 安裝時把 C 函式庫、浮點庫、C++ 函式庫等元件庫合併成的單一 `.LIB`，檔名編碼記憶體模型、浮點方式與執行環境 |
| 真實模式（real mode） | 8086 原生的執行方式：段值直接乘 16 當位址，沒有記憶體保護，只能用 1 MB；DOS 與 BIOS 的服務都假設處理器在這個模式 |
| 保護模式（protected mode） | 80286 起的執行方式；80386 起可用 32 位元位址存取數 MB 以上的記憶體。DOS 程式要進保護模式得靠 DOS extender，呼叫 DOS 時再切回真實模式 |
| `int 21h` | DOS 的系統呼叫入口：程式把功能編號放進暫存器後執行這個軟體中斷，開檔、讀寫、配置記憶體都經過它 |
| DS／SS | 資料段暫存器與堆疊段暫存器。16 位元 DOS 程式的一般配置下兩者指向同一段，所以 near 指標指全域變數或指堆疊上的區域變數都行得通 |
| `les`／`lds` | 一次從記憶體載入 far 指標（段＋位移）的指令；反組譯時大量出現代表資料指標是 far |
| Win32s | 讓部分 Win32 程式在 16 位元 Windows 3.1 上執行的相容層 |
| Alpha | DEC 的 64 位元 RISC 處理器架構，當時 Windows NT 支援的平台之一 |
| dosgolem | 無頭、決定性的 DOS 執行器：以執行的指令數當時鐘，同樣輸入每次得到同樣結果；本 repo 用它執行原廠工具鏈 |
| `dosrun` | dosgolem 的命令列執行器（`cmd/run` 編出的執行檔），跑完印出結束碼、開過的檔、主控台輸出 |
| webplay | dosgolem 上的互動外殼（`cmd/webplay`），以接近真實時間執行一支 DOS 程式，畫面與按鍵經瀏覽器往返；不是對拍工具 |
| 暫存層（scratch） | dosgolem 讓 DOS 程式的寫入落到另一個目錄、來源目錄保持唯讀的機制 |
| BGI | Borland Graphics Interface，BC++ 附的繪圖庫（`graphics.h`、`GRAPHICS.LIB`）；顯示卡相關的部分是執行時載入的 `.BGI` 驅動檔 |
| mode 12h | VGA 的 640×480、16 色圖形模式；每個像素的 4 個位元分散在 4 個位元平面 |
| DAC | VGA 把色號轉成實際 RGB 的調色盤硬體；每格 R、G、B 各 6 位元 |
| DTA | Disk Transfer Area，DOS 的 FindFirst／FindNext 寫搜尋結果的緩衝區 |
| EXEC | `int 21h AX=4B00h`，由一支程式載入並執行另一支程式（BCC 用它叫 TLINK、TASM） |

## 前置欄位的值

| 欄位 | 可填的值 |
|---|---|
| `libraries` | 本檔「來源代號」一節的代號 |
| `goals` | `craft`：當年的工程手法與設計理由；`re`：在執行檔裡辨識；`oracle`：已經用原廠工具鏈編譯對拍 |
| `evidence` | 全篇最弱的一級：`已證實`（原文直接寫明或對拍）、`強推論`、`假說` |
| `triggers` | 什麼情境該讀這篇，寫成讀者當下會遇到的狀況 |
| `symbols` | 讀者可能拿去 grep 的名稱：函式、巨集、檔名、指令助記符、格式標記 |
| `related` | 其他文章的 `id`；只填已經存在的篇，還沒寫的放在 `PLAN.md` |
