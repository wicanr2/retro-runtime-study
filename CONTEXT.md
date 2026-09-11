# 術語與來源代號

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
