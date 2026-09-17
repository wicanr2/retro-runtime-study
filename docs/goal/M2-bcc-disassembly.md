# M2 BCC 反組譯

## 目標

從兩個方向累積「BCC 2.0 編出來的程式碼在反組譯裡長什麼樣」的知識：

- **認出 BCC 編出的程式碼**：各記憶體模型的函式開頭與結尾、暫存器變數、`switch` 跳躍表、helper 呼叫點的樣子，
  以及從 M1 對拍產物產生、可以在 IDA 或其他工具裡套用的位元組簽章。
- **拆解 BCC 工具本身**：`BCC.EXE`、`TASM.EXE`、`TLINK.EXE`、`TLIB.EXE` 是 Borland 用自家工具編出來的 DOS 程式，
  是檢驗簽章的現成樣本，也是研究 overlay（執行時才從檔案載入的程式碼區塊）的實例。

最後產出一篇判斷文章：拿到一支 1990 年代的 DOS 或 Windows 執行檔，怎麼判斷它用哪一家的 runtime、哪種記憶體模型。

## 為什麼這樣做

IDA 內建的 FLIRT（用位元組樣式辨識函式庫函式的機制）對四支工具的辨識結果很不一致：
`TLIB.EXE` 約六成函式被認出，另外三支是 0。四支都出自同一家公司、同一個時期，辨識率差這麼多，
表示現成簽章不能直接拿來判斷「這支程式有沒有用 Borland 的函式庫」。
用 M1 已證明與出貨版相同的目的碼自己產生簽章，再拿這四支工具驗證，才知道簽章的命中率與誤判率。

## 工作項目

### 研究項目（不公開）

| worklist id | 內容 | 依賴 |
|---|---|---|
| `re-bcpp20-flirt` | FLIRT 對 BCC／TASM／TLINK 辨識率為 0 的原因；先用自己產生的簽章在 `TLIB.EXE` 做正對照 | M1 的對拍產物 |
| `re-bcpp20-rtl-mapping` | 把四支工具裡的函式對回函式庫原始碼的函式名，統計扣掉函式庫之後還剩多少函式 | `re-bcpp20-flirt` |
| `re-bcpp20-overlay` | `BCC.EXE` 的 overlay 結構，以及 IDA 沒涵蓋到的那 4.2% 是什麼 | 無 |
| `re-bcc20-codegen` | 用自寫測試程式，在五個模型 × 主要最佳化選項下編譯，整理產生碼的差異 | 工具鏈教學（[T1](../70-toolchain/bcc20-on-dosgolem.md)） |

### 公開產出

| Issue | 內容 | 依賴 |
|---|---|---|
| [#4](https://github.com/wicanr2/retro-runtime-study/issues/4) | 文章：BCC 2.0 產生的程式碼在執行檔裡長什麼樣（`docs/60-re-fingerprints/bcc20-codegen.md`），測試程式放 `examples/` | `re-bcc20-codegen` |
| [#5](https://github.com/wicanr2/retro-runtime-study/issues/5) | `tools/gen_signatures.py` 與 `signatures/borland-crtl-2.0/`：從讀者自己的 `.LIB` 產生短樣式，附正對照命中率 | M1 對拍、`re-bcpp20-rtl-mapping` |
| [#6](https://github.com/wicanr2/retro-runtime-study/issues/6) | 文章：從執行檔判斷用的是哪家、哪種記憶體模型的 runtime（`docs/60-re-fingerprints/identify-vendor-and-model.md`） | #4、#5；VC++ 1.0 的部分要等 M3 |
| [#7](https://github.com/wicanr2/retro-runtime-study/issues/7) | `tools/build_index.py` 產生 `kb-index.json`，檢查 id 唯一、`related` 指得到 | 無，隨時可做 |

## 建議順序

1. `re-bcc20-codegen` → #4。只需要 T1 的工具鏈與自寫程式，不依賴其他項目，可以和 M1 後段並行。
2. `re-bcpp20-flirt` → `re-bcpp20-rtl-mapping` → #5。
3. #6 放在最後，要引用 #4 與 #5 的結果。
4. `re-bcpp20-overlay` 與 #7 穿插處理。

## 完成條件

- 上表項目全部完成。
- 簽章有正反對照：對四支工具的命中數、已知不是函式庫函式卻被命中的數量，都寫在簽章的說明檔裡。
- 讀者照 #4、#6 的文章，能判斷一支 BC++ 2.0 編譯的 DOS 程式用的記憶體模型，並說出依據是哪些指令特徵。
- `tools/build_index.py` 只掃文章目錄；`docs/goal/` 這類沒有前置欄位的規劃文件不列入索引，也不會讓它失敗。

## 風險與未知

- **簽章只涵蓋 BC++ 2.0。** 遊戲若用 Turbo C 2.0、Borland C++ 3.x 編譯，函式庫程式碼可能不同；
  簽章說明要寫明適用版本，沒命中不代表「不是 Borland」。
- **自寫測試程式涵蓋不了所有寫法。** 產生碼特徵以實測過的構造為限，沒測過的構造在文章裡列為未知。
- **判斷文章的 Microsoft 部分要等 M3。** 在那之前只寫 Borland 的判準，以及「不是 Borland」時該看哪些線索。
