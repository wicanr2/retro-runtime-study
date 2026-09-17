# M3 其他函式庫

## 目標

把 M1 的做法延伸到另外三份來源，並補上 M1、M2 暫時留白的 Microsoft 對照：

- **Microsoft Visual C++ 1.0 CRT**（16 位元，MS-DOS 與 Windows 3.x）：取得工具鏈，弄清楚元件庫怎麼組合成最終的 `.LIB`，
  之後才能對拍，並和 Borland 的做法並列比較。
- **DMX**（Paul J. Radek，DOS 音效卡驅動與音樂播放；二手來源記載 Doom、Heretic、Hexen 等遊戲使用）：整理各版本的修改。
- **DSMI**（Otto Chrons，DOS 模組音樂混音）：整理 C、組語、Pascal 三套介面的分工。

## 為什麼放在最後

- Borland 的原始碼、工具鏈、對拍流程都已備齊，先做完一家，方法確定後再套到下一家，成本最低。
- VC++ 1.0 工具鏈還沒取得，沒有工具鏈就無法對拍，結論最高只能到「已證實（原文）」。
- DMX、DSMI 受授權限制，只能做原始碼層級的研究，不做位元組簽章；對逆向讀者的直接幫助比 CRT 小。

## 工作項目

### 研究項目（不公開）

| worklist id | 內容 | 依賴 |
|---|---|---|
| `toolchain-msvc10` | 取得 VC++ 1.0 的 `CL`、`MASM`、`NMAKE` 與原廠 16 位元 `.LIB`，記下來歷與 SHA-256，在 dosgolem 執行 | 合法來源 |
| `msvc10-library-combination` | 元件庫怎麼組合成最終 `.LIB`：記憶體模型 × 浮點方式 × DOS／Windows | 無（讀建置檔即可開始） |
| `dmx-version-diff` | DMX 3.3b 到 3.4a 改了什麼，對照原始檔檔頭的 PVCS 修訂紀錄 | 無 |
| `dsmi-interface-split` | DSMI 的 C、組語、Pascal 三套介面怎麼分工、共用哪些核心 | 無 |
| `dmx-provenance` | DMX 原始碼封存的流出來源與目前權利人 | 一手來源 |

### 公開產出

M3 還沒有公開 issue。研究項目完成後，依預定目錄登記文章：

| 預定目錄 | 內容 |
|---|---|
| `docs/20-msvc-crt/` | VC++ 1.0／2.0 CRT |
| `docs/30-dmx/` | DMX |
| `docs/40-dsmi/` | DSMI |
| `docs/50-cross-vendor/` | Borland 與 Microsoft 同一功能的做法比較 |

另外回頭補：R4 啟動與結束鏈的 Microsoft 對照（[#3](https://github.com/wicanr2/retro-runtime-study/issues/3)），
判斷廠牌文章的 VC++ 1.0 判準（[#6](https://github.com/wicanr2/retro-runtime-study/issues/6)）。

## 建議順序

1. `msvc10-library-combination` 與 `toolchain-msvc10` 並行：前者只需要讀建置檔，後者要找來源。
2. 工具鏈到位後，照 M1 的流程對拍 VC++ 1.0 CRT，再補 R4 與判斷文章的 Microsoft 部分。
3. `dmx-version-diff`、`dsmi-interface-split` 可以隨時穿插。
4. `dmx-provenance` 查不到一手來源時，維持「待查證」並回報，不擋其他項目。

## 完成條件

- 上表研究項目完成；`dmx-provenance` 可以用「已窮盡可取得的來源、仍待查證」結案。
- VC++ 1.0、DMX、DSMI 各至少登記一篇文章的 issue，並寫明依據哪個研究項目。
- #3 與 #6 的 Microsoft 段落不再是未知，或寫明為什麼仍無法確認。

## 風險與未知

- **VC++ 1.0 工具鏈可能取得不到合法來源。** 取得不到時，VC++ 1.0 的文章只能寫到「已證實（原文）」，
  #6 的 Microsoft 判準只能依原始碼推論，不做位元組簽章。
- **dosgolem 不一定支援 VC++ 1.0 的工具。** 缺的功能依 DOSBox-X 原始碼補進 dosgolem，先寫規格再實作。
- **DSMI 的授權禁止逆向。** 任何研究都只讀原始碼，不反組譯 DSMI 編出的程式。
- **DMX 的權利狀態不明。** 文章只寫機制，法律上的判斷交給使用者。
