# M2 BCC 反組譯

## 目標

累積「BCC 2.0 編出來的程式碼在反組譯裡長什麼樣」的知識，做到讀者拿到一支 Borland C++ 2.0 編譯的 DOS 程式時，能：

1. 認出各記憶體模型下編譯器產生的程式碼（函式開頭與結尾、暫存器變數、`switch` 跳躍表、helper 呼叫點）；
2. 用位元組簽章自動標出函式庫函式，並知道簽章的命中率與誤判率；
3. 判斷程式用的記憶體模型與 runtime 廠牌，說得出依據是哪些指令特徵。

同時收尾 M1 留下的三件事：回寫 helper 判準知識庫、讓 dosgolem 能執行 conio 程式、把測試補齊到五個記憶體模型。

## 從 M1 帶過來的前提

| 前提 | 對 M2 的影響 |
|---|---|
| C 函式庫、far 版函式、啟動碼與出貨版逐模組相同（2,455＋310＋20） | 這些目的碼可以直接當簽章來源，標「已證實（對拍）」 |
| 數學函式庫 huge 模型要加 `-zTDATA` 才相同；`STRTOD`、`FPERR`、`_MATHERR`、`SCANTOD` 無法重現，`EFCVT`、`GCVT` 只有關掉暫存器變數才對得上 | 簽章一律從**出貨 `.LIB` 拆出的模組**產生，不從自己重編的目的碼產生；這幾個模組在說明檔裡註明「未對拍」 |
| Windows 版函式庫的 `E87TRANS`、`FPINIT` 無法重現 | 同上 |
| 兩套 BC++ 2.0 的 `BCC.EXE`、`TASM.EXE` 逐位元組相同 | 產生碼特徵只有一個編譯器版本要記錄 |
| 已有工具：`omflib.py`（拆 `.LIB`）、`omfdiff.py`（OMF 語意比對）、`oracle-*.sh`（批次重編比對）、`tools/ida/ida.sh`（IDA 9.4 headless）、公開 repo 的 `tools/bcpp20/bcc.sh` 與 `examples/*/run.sh` | 不重寫，擴充使用 |
| 自寫測試程式目前只跑過 small、large、huge | R3 的 `-N` 形式、R2 的部分辨識規則在 compact、medium 仍標「推論」 |

## 工作項目

每一項開工前先在私有或公開 repo 的 `worklist.json` 登記、開對應的 GitHub issue（milestone「M2 BCC 反組譯」）；
下表已有 issue 編號的直接沿用。

### A. 產生碼特徵

| worklist id | 內容 | 完成條件 |
|---|---|---|
| `re-bcc20-codegen`（私有 #13） | 自寫測試程式涵蓋：函式進出、區域變數、暫存器變數、`switch`（密集與稀疏）、`long` 運算、結構、far／huge 指標、`-O`／`-Z`／`-r-`／`-G` 等主要選項；五個模型各編一次，IDA 反組譯整理差異表 | 研究筆記存在；每條特徵附「在哪個模型、哪個選項下觀察到」 |
| `bcc20-codegen-fingerprints`（公開 #4） | 文章 `docs/60-re-fingerprints/bcc20-codegen.md`；測試程式與 `run.sh` 放 `examples/codegen/` | 文章通過外洩閘門與專家、學生審查；範例在五個模型可重跑 |
| `examples-all-models`（新增，公開） | `examples/helpers`、`examples/startup` 跑滿 s／c／m／l／h；反組譯 compact、medium 的 `-N` 形式；更新 R2、R3、R4 裡「由資料指標寬度推論」的段落 | 三篇文章不再有這類推論字樣，或改寫成實測結果；`expected.txt` 註明五個模型相同或分列 |

### B. 函式庫辨識與簽章

| worklist id | 內容 | 完成條件 |
|---|---|---|
| `re-bcpp20-flirt`（私有 #10） | IDA 內建 FLIRT 對 BCC／TASM／TLINK 辨識率為 0 的原因；先查 IDA 9.4 容器有沒有 FLAIR 工具，沒有就用自寫的 OMF 樣式比對；正對照用 `TLIB.EXE`（FLIRT 已認出 119 個） | 每個假設有驗證結果 |
| `re-bcpp20-rtl-mapping`（私有 #11） | 用出貨 `.LIB` 的模組產生遮罩 relocation 的樣式，掃四支工具，輸出命中位址、函式名、模型、信心；衝突與未命中另列；統計「扣掉 runtime 後剩多少函式」 | `re/bcpp20/rtl-matches.json` 與說明筆記存在 |
| `signatures-borland`（公開 #5） | `tools/gen_signatures.py` 從讀者自己的 `.LIB` 產生 64 bytes 以內、位址與 relocation 已遮罩的樣式；`signatures/borland-crtl-2.0/*.json` 加同名 `.md` | 說明檔列出四支工具的命中數、已知誤判數、未對拍模組清單；工具有正反對照測試 |

### C. BCC 工具本身

| worklist id | 內容 | 完成條件 |
|---|---|---|
| `re-bcpp20-overlay`（私有 #12） | `BCC.EXE` 的 VROOMM overlay（Borland 的執行時載入程式碼區塊機制）結構；IDA 沒涵蓋的 4.2% 是什麼 | 研究筆記存在；未涵蓋部分有解釋或列為未知 |

### D. 判斷文章與索引

| worklist id | 內容 | 完成條件 |
|---|---|---|
| `re-identify-vendor`（公開 #6） | 文章 `docs/60-re-fingerprints/identify-vendor-and-model.md`：Borland 的判準（版權字串、C0 特徵、helper、prolog）與記憶體模型判斷；Microsoft 的部分列為未知，待 M3 | 文章通過審查；讀者能照文章判斷一支 BC++ 2.0 程式的模型 |
| `kb-index`（公開 #7） | `tools/build_index.py` 只掃 `docs/NN-主題/`，產生 `kb-index.json`；缺欄、id 重複、`related` 斷鏈時 exit 1 | 有正反對照測試；`docs/goal/` 不列入、不會讓它失敗 |

### E. M1 收尾

| worklist id | 內容 | 完成條件 |
|---|---|---|
| `helper-kb-writeback`（新增，私有） | 把 M1 的差異表寫進 `~/.claude/knowledge-base/retro/borland-win16-rtl-helpers.md`：`LXMUL@` 最多三次 `mul`、`LDIV@` 家族的共用本體與捷徑、`SPUSH@`／`SCOPY@` 的分辨、`N_` 入口改寫 far 框架、huge 指標 helper 特徵、`-N` 兩種形式；指回公開文章 R3。修改紀錄依全域規則追加到 `~/.claude/WORKLOG.md` | 知識庫不再有「`pop 回位址、sub sp,cx、rep movsw` → `N_SCOPY@`」這類舊寫法；只動這一個知識庫檔與 WORKLOG |
| `dosgolem-conio`（新增，私有＋dosgolem） | 查明 conio 程式在 dosgolem 裡游標不前進、只寫到第 1 列的原因（BIOS 資料區的列數、游標位置、`int 10h` 功能）；依 DOSBox-X 原始碼先寫 dosgolem 規格（`READY`）再實作，放 `bcc20-toolchain` 分支；用 M1 的 SCROLL 測試程式分別連結 4 月與 8 月版 `CS.LIB` 實跑 | SCROLL 筆記的「沒有實跑確認畫面效果」改成實測結果；dosgolem 規格與測試進版控 |

## 建議順序

1. **E 與 A 的前半並行**：`helper-kb-writeback`（半小時內可完成）、`examples-all-models`（只用現有範例）、`dosgolem-conio`（獨立於其他項目）。
2. `re-bcc20-codegen` → `bcc20-codegen-fingerprints`。
3. `re-bcpp20-flirt` → `re-bcpp20-rtl-mapping` → `signatures-borland`。
4. `re-bcpp20-overlay`、`kb-index` 穿插。
5. `re-identify-vendor` 最後，引用 A、B 的結果。

## 每一項的做法

- 研究在私有工作區 `~/cht/borland/`，公開文章在本 repo，照兩邊 `CLAUDE.md` 的契約。
- 分析、編譯、IDA、dosgolem 一律在 docker；docker 只清自己建立的 container，不做任何 prune 或 `rmi`。
- 結論分四級（已證實／強推論／假說／未知）；能實跑就實跑，實跑結果比原文推論優先。
- **公開文章**：外洩閘門 → 專家與學生兩個審查 agent（`model: sonnet`、唯讀、禁止 docker 與寫檔）→ 逐條自己查證後修正 → 再過閘門推送。M1 三篇文章的審查都抓到實際錯誤，這一步不省略。
- 推論在實跑或原文查到反例時，當下改正文、在 `PLAN.md` 勘誤段記一行，不在正文敘述怎麼錯的。
- 某項的對拍或實驗出現「參數怎麼調都對不上」時，最多換兩輪假設；仍解釋不了就把排除過的假設寫進筆記，另開一條 worklist，不在原項目裡無限延伸。
- 每完成一項：`tools/worklist.py` 確認該條已不成立 → commit（`Closes #N`）→ push → 從 worklist 移除該條。

## 完成條件

- 上表所有項目完成，對應 issue 關閉，兩個 repo 的 worklist 沒有 M2 條目。
- 兩篇新文章（產生碼特徵、判斷廠牌與模型）通過審查修訂並推送。
- 簽章說明檔有四支工具的正反對照數字與未對拍模組清單。
- R2、R3、R4 裡「compact、medium 由推論得出」的段落已換成實測。
- helper 判準知識庫已回寫，SCROLL 的畫面效果已實跑確認。

## 風險與未知

- **FLIRT 工具可能不在 IDA 映像裡。** 這時改用自寫的 OMF 樣式比對，文章只描述自己的工具，不宣稱 IDA 簽章檔的格式細節。
- **簽章只涵蓋 BC++ 2.0。** Turbo C 2.0、Borland C++ 3.x 的函式庫不同，沒命中不代表不是 Borland；說明檔寫明適用版本。
- **自寫測試程式涵蓋不了所有寫法。** 產生碼特徵以實測過的構造為限，其他列為未知。
- **dosgolem 的 conio 可能牽涉多個 BIOS 服務。** 若修到第二個服務仍不夠，停下來評估是否改用 DOSBox-X 原始碼逐項比對的方式，先補規格再動手。
- **判斷文章的 Microsoft 部分要等 M3。**
