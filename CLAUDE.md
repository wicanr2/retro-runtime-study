# 這個 repo 怎麼寫

1990 年代 DOS／Win16／早期 Win32 函式庫的研究知識庫：Borland C++ 2.0 runtime、
Microsoft Visual C++ 1.0／2.0 CRT、DMX、DSMI。主要讀者是正在做老遊戲 remake
或逆向的 AI session，其次是人。每篇要讓讀者**不看原始碼**也能回答三件事：
這個函式做什麼、當年為什麼這樣寫、在執行檔裡怎麼認出它。

繼承 `~/.claude/rules/`（身分、文風、執行邊界）與 `~/.claude/rules/00-rules-index.md`；
每輪流程與 SVG 規範照 skill `first-principles-tech-notes`。研究證據與原始碼在私有工作區
`~/cht/borland/`，那邊的 `CLAUDE.md` 是研究契約，本檔只管公開文章怎麼寫。

## [HARD] 不放原文

四份來源的授權都禁止散布原始碼（條文整理在私有工作區 `CLAUDE.md`「授權邊界」）。

- **一行原始碼都不放**，包括「只有幾行」的片段、改了變數名的片段、放在圖裡的片段。
- 可以寫：函式名、檔名、巨集名、helper 符號、參數與回傳值的意義、演算法步驟、
  資料結構的欄位意義、通用的 x86 指令慣用法。
- 要示意就用自己寫的虛擬碼或最小重寫；順序、命名、註解都不能跟原檔一一對應。
- 授權條文、README 這類說明文字可以引用兩行以內，標出處；該行同時加進 `.leak-allow`。
- 不連到私有 repo 的任何路徑（對讀者是 404），出處寫成「Borland C++ 2.0 RTL，`STRLEN.CAS`」。
- **每次 push 前跑外洩閘門**，命中就不准推：

  ```sh
  docker run --rm --network none -u "$(id -u):$(id -g)" \
    -v ~/cht/borland:/p:ro -v "$PWD":/pub:ro retro-runtime-study-tools:1 \
    python /p/tools/leak_check.py /p/vendor /pub
  ```

- **位元組簽章只做 Borland 與 MS**，而且只放短樣式（64 bytes 以內、relocation 與位址遮罩），
  不放完整函式的位元組。DSMI 的授權禁止逆向該套件、DMX 權利狀態不明，這兩份只寫原始碼層級的機制。

## 文章格式

一檔一主題，放 `docs/NN-主題/`。開頭是 YAML 前置欄位，AI 讀者靠它決定要不要讀這篇：

```yaml
---
id: borland-crtl/memory-model-macros      # 全庫唯一，與路徑對應
title: 一份原始碼怎麼編出五個記憶體模型
libraries: [borland-crtl-2.0]             # 見 CONTEXT.md 的來源代號
goals: [craft, re]                        # craft 工程手法／re 逆向辨識／oracle 編譯對拍
evidence: 強推論                          # 全篇最弱的一級：已證實／強推論／假說
triggers:                                 # 什麼情境該讀這篇
  - 反組譯 Borland 編譯的 16 位元程式，同一個函式在不同執行檔裡長得不一樣
symbols: [LDATA, LPROG]                   # 讀者可能拿來 grep 的名稱
related: [borland-crtl/compiler-helpers]
---
```

正文依序：

1. **結論**：兩三句，先講答案。
2. **根本問題**：當年的約束是什麼（分段定址、記憶體大小、有沒有 8087、DOS 還是 Windows）。
3. **推導**：從約束推到設計，說清楚它擋住了什麼（柵欄原則）；Borland 與 MS 做法不同時並列比較。
4. **在執行檔裡怎麼認**（`re` 目標才寫）：辨識特徵、容易誤判的相似物、各版本或各記憶體模型的差異。
5. **給 remake 的行為規格**（有對應行為才寫）：輸入、輸出、邊界條件、錯誤處理，
   寫到「照規格重做就行為一致」為止。
6. **證據與未知**：每條結論的出處（來源名＋檔名＋函式名）與推論等級；沒查到的列成未知。

## 書寫規範

- 繁體中文為正本；程式碼、識別字、檔名、助記符保留原文。
- 標點全形；半形只用在程式碼、行內程式碼、英文原名與英文列舉。
- 術語首次出現當場一句話翻譯，並登進 `CONTEXT.md`。
- 不貼導引式標籤（「先看這段」「白話：」「本文適合⋯」），章節用中性標題。
- 位元組寫 `0x` 十六進位；段:位移寫 `1234:0010`。
- 新結論寫進既有文章時，正文只寫現況；被推翻的舊斷言記在 `PLAN.md` 勘誤段，
  正文最多留一個指標（`~/.claude/rulebook/63`）。

## 證據紀律

- 版本不混用：每張表、每條結論標它屬於哪個來源代號。
- 條件編譯分支要講明：只讀了 large model 的分支，就只對 large model 下結論。
- 編譯對拍過的結論標「已證實（對拍）」；只讀原始碼得出的，最高到「已證實（原文）」，
  而且只限原文直接寫明的事。
- 原廠沒附原始碼的部分（Borland 的 8087 emulator 與 graphics library、VC 2.0 大部分浮點例程）
  只寫介面與呼叫端看得到的行為。

## 目錄

| 路徑 | 放什麼 |
|---|---|
| `README.md` | 用途、閱讀動線、來源清單、邊界宣告 |
| `CONTEXT.md` | 來源代號、術語表 |
| `PLAN.md` | 分輪進度、勘誤段 |
| `worklist.json` | 未完成項（權威）；`tools/worklist.py` 跑 verify |
| `docs/NN-主題/*.md` | 文章 |
| `img/` | SVG；程式執行畫面的截圖可以用 PNG，檔名要在 `.gitignore` 明列例外（驗證用 PNG 走 scratchpad，不進版控） |
| `tools/` | 可公開的腳本；不含、也不下載任何受授權限制的檔案 |
| `examples/` | 自己寫的範例程式；只能引用原廠標頭與函式庫，不能改寫原廠原始碼 |
| `signatures/` | 由工具從對拍產物產生的短位元組樣式（JSON）＋同名 `.md` 的語意說明 |
| `kb-index.json` | 由前置欄位產生的索引，給 AI 讀者路由（尚未建置，見 worklist） |

## Git

- 開工先確認 `git config user.email` 是 `wicanr2@gmail.com`，並看一次 `git log --format=%ae | sort -u`。
- commit message 繁體中文，結尾只留 `Co-Authored-By`，不放 `Claude-Session:`。
- 每輪收尾：外洩閘門 → `tools/worklist.py` → commit → push。
