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

## 文章

| 篇 | 內容 |
|---|---|
| [五份函式庫的全景](docs/00-overview/era-and-libraries.md) | 指標寬度、浮點、作業系統三個約束怎麼決定函式庫的份數與寫法；兩家 16 位元 CRT 的做法對照；各函式庫出現在哪些遊戲 |

後續篇目與進度見 `PLAN.md`。

## 邊界

- **本 repo 不含任何原始碼。** 上述函式庫的授權都禁止散布原始碼，這裡只有自己寫的說明、
  虛擬碼、圖與統計。要讀原文請自行取得合法來源。
- 位元組簽章只收 Borland 與 Microsoft 函式庫，且只收遮罩過位址的短樣式。
- 推論等級分「已證實／強推論／假說」；沒有一手來源支持的內容標「待查證」。

## 授權

文件與圖採 [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)。
Borland、Microsoft、Visual C++ 等名稱為各自權利人的商標；
被研究的函式庫原始碼版權屬各自權利人，不在本授權範圍內。
