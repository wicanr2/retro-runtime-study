---
id: borland-crtl/iostream
title: iostream：兩套 stream、自有格式化引擎、與啟動鏈的接點
libraries: [borland-crtl-2.0]
goals: [craft, re]
evidence: 已證實
triggers:
  - remake 或反組譯碰到 `cout`／`<<`／`ios::` 這類符號，想知道這一版 iostream 的實際行為
  - 混用 printf 與 cout 輸出順序亂掉，想知道為什麼、怎麼解
  - 想知道 cin/cout 這些全域物件是什麼時候建構的、程式裡什麼時候開始能用
  - 浮點用 `<<` 輸出結果怪（例如 0.0001 印成 0），想確認是函式庫的規則不是自己的 bug
  - 想把舊碼的 `stream.h`（舊式 stream）跟 `iostream.h` 分清楚
symbols: [ios, streambuf, filebuf, istream, ostream, iostream, withassign, cin, cout, cerr, clog, endl, flush, dec, hex, oct, setw, setprecision, setiosflags, width, precision, fill, tie, ipfx, opfx, osfx, sync_with_stdio, unitbuf, showbase, showpos, showpoint, internal, Iostream_init, Iostream_delete, oldstrm, STREAM.H, IOSTREAM.H, FSTREAM.H]
related: [borland-crtl/stdio-file-io, borland-crtl/startup-and-exit, borland-crtl/printf-engine, borland-crtl/rand-time]
---

# iostream：兩套 stream、自有格式化引擎、與啟動鏈的接點

## 結論

這一版 RTL 帶**兩套** stream：新式 iostream（`iostream.h`；191 個模組在主庫裡）與
舊式 stream（`stream.h`；8 個模組獨立成 `oldstrm<模型>.lib`，格式化直接靠 stdio 的
`sprintf`）。新式的格式化引擎完全自帶，緩衝也與 stdio 各自為政——
混用 `printf` 與 `cout` 會亂序，`sync_with_stdio()` 才接回同一條路。
全域物件 `cin`/`cout`/`cerr`/`clog` 由啟動鏈的 `Iostream_init`（優先序 16）建立，
早於使用者全域物件的建構（32），所以全域物件的建構式裡 `cout` 已經可用。

<p align="center"><img src="../../img/iostream-classes.svg" width="900" alt="左：ios／streambuf／istream／ostream／iostream／withassign 類別階層與 filebuf；右：Iostream_init 到使用者建構的順序、filebuf 緩衝、tie、sync_with_stdio 與 cerr/cout 的 unitbuf 規則"></p>

## 根本問題

iostream 是 1990 年從 Turbo C++ 1.0 的 RTL 移植進來的（模組檔頭的版權行可證），
它繼承了 AT&T iostream 的設計，卻要塞進 DOS 的世界：

1. **DOS 沒有「流」的概念**——只有 handle 與 `FILE*` 兩層（見[檔案 I/O](stdio-file-io.md)）。
   filebuf 選了**低階 handle**（`F_stdin`＝0、`F_stdout`＝1、`F_stderr`＝2），
   自己做緩衝；同樣的 `FILE*` 緩衝在 stdio 裡還有一份。
2. **C 與 C++ 的 I/O 要能並存**——但 1991 年沒有人想清楚怎麼同步，所以預設
   「各緩各的」，把同步做成一個顯式呼叫（`sync_with_stdio`）。
3. **C++ 的全域物件需要建構時機**——啟動鏈的 `_INIT_` 表本來就存在
   （見[啟動與結束鏈](startup-and-exit.md)），iostream 把自己掛在優先序 16，
   使用者全域物件在 32。
4. **記憶體以 KB 計**——191 個模組做成「用不到就連不進來」的函式庫模組，
   而不是一個一定要拉進來的執行期系統。

## 推導

### 類別地圖

（階層與資料流見上圖左半。）

`ios`（虛擬基礎類別：多重繼承路徑共用同一份基底子物件）持有狀態
（goodbit/eofbit/failbit/badbit/hardfail）與格式化旗標——skipws（輸入跳空白）、
left/right/internal（對齊）、dec/oct/hex（基底）、showbase/showpoint/uppercase/showpos
（前綴與補位）、scientific/fixed（浮點形式）、unitbuf（每筆即刷）、stdio——
行為在下文各段展開；另有 `precision`／`width`／`fill`／`tie` 四個格式化欄位。
`streambuf` 是純緩衝抽象——讀側（get 區）與寫側（put 區）各三個位置指標
（起點、目前位置、終點），加上可覆寫的 overflow/underflow/seekoff 原語；`istream`/`ostream` 在其上做格式化，`iostream` 同時繼承兩者，
`*_withassign` 加上 `operator=`（預設串流就是靠它換 streambuf）。
`filebuf`（`fstream.h`）把 streambuf 接到低階 handle；`stdiobuf`／`stdiostream`
（stdiostr.h）接到 stdio 的 `FILE*`。

預設值（`ios::init`）：`flags`＝skipws、`precision`＝**0**、`width`＝0、`fill`＝空格、
`tie`＝0、狀態＝good。

### 預設串流與啟動鏈

`Iostream_init`（`#pragma startup`，優先序 **16**）做四件事：`new` 三個 filebuf
（分別綁 handle 0/1/2）；用 placement new（在已配置的位址上呼叫建構式）建構四個全域 withassign 物件
（`cin`/`cout`/`cerr`/`clog`，定義在同一模組）；把串流接上 buf
（`cin = stdin_filebuf` …，`cerr` 與 `clog` 同接 handle 2）；
設 tie 與 unitbuf——`cin.tie(&cout)`、`clog.tie(&cout)`、`cerr.tie(&cout)`、
`cerr.setf(unitbuf)`，**且 stdout 是主控台時 `cout` 也 unitbuf**。
`Iostream_delete`（結束鏈，優先序 16）收掉三個 filebuf。

使用者自己的全域物件建構被編譯器掛在優先序 **32**——優先序數字小的先執行，所以晚於 16，
所以「全域物件的建構式裡 `cout` 已可用」是有保證的設計，不是巧合
（實測：`examples/iostream/CTOR.CPP`）。

### 輸出的完整路徑

`cout << x` 的每一步：

1. `opfx()`：失敗狀態就整筆放棄；有 tie 就先刷掉 tie 的串流
   （讓提示字串先出現再讀輸入，這是 cin 綁 cout 的理由——反向亦然）。
2. 轉換：整數用自己的 todec/tooct/tohex（基底由 dec/oct/hex 旗標決定，
   showbase 加 0x/0X/0 前綴，showpos 只對十進位加號，uppercase 影響十六進位字母）；
   浮點用自己的 AT&T 血統 IEEE 轉換（normalize/round/flt_out/eout，
   Inf/NaN 有專屬字串 `[+Infinity]`/`[NotANumber]`）。
3. `outstr` 墊墊：`width(0)` 取值**並歸零**——width 是一次性的；
   不足的位數用 `fill` 補（預設右靠＝左補；internal 在前綴之後補；left 右補）。
4. `osfx()`：unitbuf 旗標開就 flush。

**浮點的規則**。先定義 exp＝這個數以科學記號表示時的指數
（`floor(log10|x|)`；3.5 的 exp 是 0、12345.6789 是 4、0.0001 是 −4）。
`precision()`≤0 時視為 6。然後：

- 旗標有 fixed：固定小數，小數位數＝**exp＋precision**，之後剝尾零與尾點。
- 旗標有 scientific，或 exp<−4，或 precision<exp：科學記號，
  小數點後 **precision** 位（有效位數＝precision＋1），指數至少兩位帶正負號。
- 都沒設（預設）：上面兩條擇一——「exp<−4 或 precision<exp」走科學，
  否則走固定小數。

逐步算例（precision 預設視為 6）：`0.0001` 的 exp=−4，不走科學；
小數位數＝−4＋6＝2 → `0.00` → 剝尾零 → **印成 `0`**。`0.001` 的 exp=−3，
小數 3 位 → `0.001`。`12345.6789` 的 exp=4，小數 10 位 → 全數印出。
`3.5` 的 exp=0，小數 6 位 → `3.500000` → 剝尾零 → `3.5`。
precision=2 時 `12345.0`：precision(2)<exp(4) → 科學 → `1.23e+04`。
**預設下 10⁻⁴ 量級的數會整個崩成 `0`**——對 remake 是真正的行為差異。

### 輸入

格式化的讀取運算子（extractor，`>>`）走 `ipfx(0)`：清 gcount、檢查狀態、
**先刷 tie 的串流**、skipws 開就跳空白。整數讀取可有正負號；
hex/oct 旗標強制基底；都沒設時用 **C 字面值語意**（前導 `0x` 十六進位、
前導 `0` 八進位、其餘十進位）。`short`/`int` 統一以 long 讀入再截回
（原始模組的註解明說是為了省碼）。無格式的 `get`/`read` 走 `ipfx(1)`，
不跳空白。實測：空 stdin 上 `cin >> x` → failbit 設定、eofbit 沒設、
`x` 的值不被更動。

### 與 stdio 的兩層關係

（見上圖右半。）

預設：iostream 的 filebuf（緩衝大小是函式庫內部常數 1024；版面＝回看區 4 bytes＋寫區，
  緩衝極小時回看 1——原始碼層閱讀）與 stdio 的
`FILE` 緩衝**各自獨立**——同一個 handle 兩份緩衝，交錯輸出的順序由「誰先 flush」決定
（實測：`cout` 的內容因 `sync_with_stdio` 裡的顯式 flush 先落地，
`printf` 的內容反而後出現）。`ios::sync_with_stdio()`（原始碼裡有 done 旗標，確實只生效一次）把 cin/cout/clog
的 streambuf 換成包 `FILE*` 的 stdiobuf，**並把 cout/clog 設成 unitbuf**——
之後輸出與 stdio 同路同序。實測還發現：呼叫 `sync_with_stdio` 的程式在結束時
會印出 Borland 的 **"Null pointer assignment"**——啟動鏈在資料段開頭放了哨兵位元組、
  結束時比對，被改寫就印這行（代表程式曾透過空指標寫入）——
觸發與否取決於記憶體模型的 DGROUP 版面（s、m 觸發；c、l、h 不觸發），
五個模型都記錄在 `examples/iostream` 的執行報告裡。

### 建置取捨

新式 191 個模組進主庫（`IOSTREAM.RSP`）——用不到 iostream 的程式一個位元組都不付；
舊式 stream 的 8 個核心模組獨立成 `oldstrm<模型>.lib`（`OLDSTRM.RSP`）——
要寫 `stream.h` 的舊碼必須自己多連這個庫。來源包裡另有兩個「孤兒檔」
（`STSTATIC.CPP`、`FSBSTAT.CPP`）：內容與 `STDTR.CPP`／`FSBDTR.CPP` 重複
（ios 靜態常數與 `filebuf::openprot` 的定義），被建置清單刻意排除以免重複符號；
出貨庫裡這些定義確實存在於 STDTR／FSBDTR 模組（從出貨 `.LIB` 的 PUBDEF 掃出——OMF 記錄裡的「公開符號定義」）。

## 在執行檔裡怎麼認

- **C++ mangling**：`@類別@名稱$q參數`。實物樣本：`@ostream@$blsh$ql`
  （`operator<<(long)`）、`@ostream@$blsh$qg`（double）、`@istream@$brsh$qrl*`
  （`operator>>(long&)`；遠資料模型 compact/large/huge 下引用帶 `*` 尾碼）、
  `@istream_withassign@$basg$qp9streambuf`（`operator=(streambuf*)`）。
  型別碼：`v`/`i`/`l`/`g`/`c` 基本型、`p` 指標、`r` 引用、`q` 參數表、
  `$bctr`/`$bdtr`/`$blsh`/`$brsh`/`$basg`＝建構/解構/`<<`/`>>`/`=`、
  `N字元數類名`（`3ios`、`9streambuf`）。詳細表見
  [BCC 2.0 的 C++ 產生碼指紋](../60-re-fingerprints/bcc20-cxx-codegen.md)。
- **全域物件**：map 檔裡 `_cin`/`_cout`/`_cerr`/`_clog`（DGROUP 的物件）與
  `Iostream_init`/`Iostream_delete`；`_INIT_` 表裡優先序 16 的紀錄。
- **呼叫形狀**：`cout <<` 展開成一連串 `push`＋`call @ostream@…`；
  操作子（`endl` 等）以「函式指標」身分出現在參數列，對應
  `$blsh$qpqr3ios$r3ios` 這種簽名。
- 舊式 stream（`stream.h`）的類別名與新式不同，mangled 符號因此不同；
  從類別名即可判定是哪一族。

## 給 remake 的行為規格

以下每一條都有 `examples/iostream` 五個模型的實跑或對拍過的原始碼支持：

- **預設值**：`skipws` 開、precision=0（浮點視為 6）、width=0（一次性）、
  fill=空白、無 tie、狀態 good。`cin.tie(&cout)`、`cerr.tie(&cout)`、
  `clog.tie(&cout)`、cerr 的 unitbuf 恆開；**cout 的 unitbuf 只在 stdout 是主控台
  （isatty(1) 為真）時開**——轉向檔案就是完全緩衝（實測環境的主控台回報為檔案，
  所以觀察到的是關）。
- **整數輸出**：dec/oct/hex 互斥；showbase 前綴 `0x`/`0X`/`0`；showpos 只對十進位；
  uppercase 影響十六進位字母。width 一次性，不足補 fill（預設右靠）。
- **浮點輸出**（exp＝值的科學記號指數；precision≤0 視為 6）：
  fixed → 小數 **exp＋precision** 位；scientific、exp<−4、或 precision<exp →
  `e` 型、小數點後 **precision** 位（有效位數 precision＋1）、指數至少兩位；
  預設兩者擇一後**剝尾零與尾點**。預設下 0.0001 印成 `0`、0.001 印成 `0.001`、
  12345.6789 全數印出。
- **輸入**：`>>` 跳空白（skipws）、整數支援 0x/0 前綴自動基底；失敗設 failbit、
  目標變數不動；空輸入 → failbit 但無 eofbit（dosgolem 觀察）。
  無格式 `get`/`read` 不跳空白。
- **順序**：輸入前的 tie-flush 保證「提示先出現」；unitbuf 保證每筆即刷；
  沒有 sync 之前，printf 與 cout 的相對順序依各自緩衝的 flush 時點，
  程式不該依賴（要依賴就先 `sync_with_stdio()`）。
- **sync 之後**：cout/clog 永遠 unitbuf；與 stdio 同緩衝同序；
  結束時可能印 "Null pointer assignment"（模型相關，見上）。
- **舊式 stream**：概念同名但格式化走 `sprintf`、filebuf 包 `FILE*`；
  需要連 `oldstrm<模型>.lib`。remake 一律用 `iostream.h` 這族。

## 證據與未知

**已證實（對拍）**：本文的機制敘述出自 IOSTRM1＋IOSTRM2 共 193 檔中進主庫的 191 個模組——
五個模型的重編對拍與出貨函式庫逐模組相同（CLIB 2455/2455）；
`STDTR.CPP`/`FSBDTR.CPP` 帶的靜態成員定義也在出貨庫的對應模組裡掃到 PUBDEF。

**已證實（實測）**：格式化（旗標、width 一次性、internal、浮點預設與
`0.0001 → 0`）、全域物件建構順序（建構式裡 cout 可用、解構反向）、
空 stdin 的失敗語意、`sync_with_stdio` 之後的 unitbuf 與空指標哨兵
——`examples/iostream` 三支程式五個模型可重跑，輸出與 expected 檔相同；
主控台交錯觀察記錄在執行報告。

**已證實（原文）**：舊式 stream（`stream.h`）的 8 個模組不在對拍清單，
機制敘述限原始碼層級；`sprintf` 路徑與 `FILE*` 綁定出自該 8 檔。
Inf/NaN 的專屬字串、filebuf 的緩衝常數（1024）與回看版面、
「long 讀入再截回」的整數輸入實作、失敗時不回寫目標變數——
這些出自原始碼層閱讀，實跑未逐項覆蓋。

**外部知識**：「AT&T 血統」「1980 年代的 iostream 慣例」這類定位是背景說明，
本文的結論不依賴它們。

**未知**：

- 浮點 `showpoint` 的逐位規則（補尾零的確切條件）沒有逐 case 驗證。
- `sync_with_stdio` 踩哨兵的寫入指令沒有定位（現象已記錄：模型相關）。
- 舊式 stream 的 extractor/insertor 邊界（與新式的差異清單）沒有整理。
- strstreambuf（`SRB*`）家族未展開；`ios::stdioflush` 靜態的呼叫點沒有追。
