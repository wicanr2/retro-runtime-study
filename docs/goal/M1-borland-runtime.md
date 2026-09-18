# M1 Borland runtime 分析

## 目標

把 Borland C++ 2.0 的執行時期函式庫寫到「不看原始碼也能重做」的程度。分兩批：

- **第一批（已完成）**：證明原始碼能代表出貨的函式庫，並寫完三個每支程式都會用到的機制——
  一份原始碼怎麼編出五個記憶體模型、編譯器自動插入的 helper、啟動與結束鏈。
- **第二批（本輪）**：遊戲真正呼叫的那些函式——檔案 I/O 與緩衝、`printf` 家族、heap、conio 與文字畫面。
  每篇要給得出行為規格：同樣的輸入，remake 照規格做會得到同樣的輸出與錯誤碼。

## 從第一批帶過來的前提

| 前提 | 對第二批的影響 |
|---|---|
| C 函式庫與 iostream 共 491 個模組 × 五個模型（2,455 次）、far 版 31 個、啟動碼 20 個目的檔與出貨版逐模組相同 | 本輪要讀的模組全部在對拍範圍內，結論可以標「已證實（對拍）」 |
| 數學函式庫的 `STRTOD`、`FPERR`、`_MATHERR`、`SCANTOD`、`EFCVT`、`GCVT` 重現不出來 | `printf` 的浮點格式化與 `scanf` 的字串轉數字會碰到這幾個模組，引用時只能標「已證實（原文）」 |
| 兩版出貨的 C 函式庫只有 `SCROLL` 不同，行為差異已實跑確認 | conio 那篇直接引用，不必重做 |
| 工具鏈：`tools/bcpp20/bcc.sh`、dosgolem `bcc20-toolchain` 分支（含 `int 10h` 文字模式、檔案代號、決定性時鐘） | 範例程式可以編譯、實跑、比對輸出 |
| M2 的產生碼特徵與 `signatures/borland-crtl-2.0/dos.json` | 每篇的「在執行檔裡怎麼認」直接用簽章裡的符號名，不必重新歸納 |
| 範例程式的既有形狀：`examples/<主題>/` 內含 `.C`、`run.sh`、`expected.txt`、`README.md` | 沿用，不另立格式 |

## 工作項目（第二批）

每個主題一組：私有研究筆記 → 公開文章（含 SVG 與範例程式）。A–D 已登記在兩邊的 `worklist.json` 與 GitHub issue（milestone「M1 Borland runtime 分析」）。

| 主題 | 私有研究筆記（issue） | 公開文章與範例（issue） | 要回答的問題 |
|---|---|---|---|
| A 檔案 I/O 與緩衝 | `borland-stdio-file-io`（私有 #22） | [#9](https://github.com/wicanr2/retro-runtime-study/issues/9)：`docs/10-borland-crtl/stdio-file-io.md`、`examples/stdio/` | `FILE` 的欄位與緩衝策略（什麼時候全緩衝、行緩衝、不緩衝）；文字模式的 CR／LF 與 `Ctrl-Z`；`fopen` 模式字串的解析；低階代號層（`open`／`read`／`write`／`lseek`）與 `FILE` 的關係；`errno`、`_doserrno`、`perror` 的對應；`fseek`／`ftell` 在文字模式的邊界 |
| B `printf` 家族 | `borland-printf-engine`（私有 #23） | [#10](https://github.com/wicanr2/retro-runtime-study/issues/10)：`docs/10-borland-crtl/printf-engine.md`、`examples/printf/` | 格式化引擎怎麼被 `printf`／`fprintf`／`sprintf`／`cprintf` 共用；支援哪些轉換與旗標、寬度與精度的邊界；浮點格式化為什麼要另外連結（`floating point formats not linked` 從哪來）；`scanf` 家族的輸入規則與回傳值 |
| C heap | `borland-heap`（私有 #24） | [#11](https://github.com/wicanr2/retro-runtime-study/issues/11)：`docs/10-borland-crtl/heap.md`、`examples/heap/` | near heap 與 far heap 兩套配置器的資料結構與差異；`malloc`／`free`／`realloc`／`calloc`／`coreleft` 的行為與失敗條件；`brk`／`sbrk` 與堆疊的邊界；各記憶體模型下 heap 放在哪裡；與 DOS 記憶體配置（`allocmem`）的關係 |
| D conio 與文字畫面 | `borland-conio-screen`（私有 #25） | [#12](https://github.com/wicanr2/retro-runtime-study/issues/12)：`docs/10-borland-crtl/conio-screen.md`、`examples/conio/` | `directvideo` 決定直接寫 `B800` 還是走 BIOS；`window`／`gotoxy`／`wherex` 的座標與邊界；`gettext`／`puttext`／`movetext`；捲動與清除（含 1991-04 與 1991-08 版的差異）；`getch`／`getche`／`kbhit` 的鍵盤行為與擴充鍵 |
| E（備選）亂數與時間 | `borland-rand-time`（未登記） | `docs/10-borland-crtl/rand-time.md`、`examples/randtime/` | `rand`／`srand` 的序列公式與 `RAND_MAX`；`time`／`clock`／`ftime`／`getdate` 讀哪些 DOS 服務；remake 要重現同一串亂數時該怎麼做 |

A–D 這一輪要做完；E 只有在 A–D 完成後還有餘裕才開工，或另一輪再排。

## 建議順序

1. **A 檔案 I/O**：`printf` 與 conio 都會用到 `FILE` 與代號層，先做它，後兩篇才不必重複解釋。
2. **B `printf` 家族**：依賴 A 的緩衝與 `FILE` 結論。
3. **C heap**：獨立，可以與 B 並行；`realloc` 的行為規格要實跑。
4. **D conio**：材料最齊（SCROLL 已實跑、dosgolem 已補 `int 10h`），可隨時插入。
5. E 視進度。

## 每一項的做法

- 研究在私有工作區 `~/cht/borland/`，公開文章在本 repo，照兩邊 `CLAUDE.md` 的契約；一行原始碼都不放。
- 分析、編譯、實跑一律在 docker；只清自己建立的 container，不做任何 prune 或 `rmi`。
- 每篇的「給 remake 的行為規格」要有**實跑**支持：範例程式涵蓋邊界值（空字串、超長欄位、配置失敗、視窗邊緣、檔尾），
  在 dosgolem 執行，輸出存成 `expected.txt`，五個記憶體模型都要跑。
- 「在執行檔裡怎麼認」引用 `signatures/borland-crtl-2.0/dos.json` 的符號名與 M2 的產生碼特徵，不重新歸納。
- 結論分四級（已證實／強推論／假說／未知）；引用第一批無法重現的模組時標「已證實（原文）」。
- 公開文章：外洩閘門 → `tools/build_index.py` → 專家與學生兩個審查（`model: sonnet`、唯讀）→ 逐條查證後修正 → 再過閘門推送。
- dosgolem 缺功能時，先照 DOSBox-X 原始碼寫規格（`READY`）再實作，放 `bcc20-toolchain` 分支。
- 某一項的實驗出現「怎麼調都對不上」時，最多換兩輪假設；仍解釋不了就把排除過的假設寫進筆記，另開一條 worklist。
- 每完成一項：`tools/worklist.py` 確認該條已不成立 → commit（`Closes #N`）→ push → 從 worklist 移除。

## 完成條件

- A–D 四組的研究筆記與文章完成，對應 issue 關閉，兩個 repo 的 worklist 沒有這一批的條目。
- 四篇文章都有「給 remake 的行為規格」與「在執行檔裡怎麼認」兩節，且行為規格的每一條都指得出是哪一支範例程式跑出來的。
- `examples/stdio`、`examples/printf`、`examples/heap`、`examples/conio` 在五個記憶體模型可重跑，輸出與 `expected.txt` 相同。
- `README.md` 的文章表、`CONTEXT.md` 的術語、`kb-index.json` 都已更新。

## 風險與未知

- **浮點格式化牽涉到無法重現的模組。** `printf` 的 `%f`／`%e`／`%g` 走 `EFCVT`、`GCVT`，`scanf` 的數字轉換走 `STRTOD`、`SCANTOD`；
  這些模組重編後與出貨版不同，文章對它們只能標「已證實（原文）」，行為規格改以實跑結果為準。
- **dosgolem 的 DOS 服務可能不夠。** 檔案 I/O 的邊界（`lseek` 超過檔尾、唯讀檔案、磁碟滿）與時間服務可能要補規格；
  補到第二個服務仍不夠時停下來評估，不在原項目裡無限延伸。
- **conio 的實跑只能看文字緩衝區。** dosgolem 沒有真實螢幕，`getch` 這類互動行為要靠送鍵腳本；擴充鍵的行為可能測不完整。
- **heap 的行為與 DOS 版本有關。** `coreleft`、`sbrk` 會受可用記憶體影響，範例程式要固定 dosgolem 的記憶體設定，否則輸出不穩定。
- **iostream 不在這一輪。** C++ 串流（`IOSTRM1`、`IOSTRM2`，193 個 `.cpp`）量大且與 C 函式庫交錯，另外排一輪。
