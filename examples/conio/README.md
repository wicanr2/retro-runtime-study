# conio 行為測試程式

`CONIO.C` 把 Borland C++ 2.0（以下簡稱 BC++ 2.0）的文字畫面行為跑出來：視窗與游標、
`cputs` 對控制字元的處理、換行與捲動、屬性位元、區塊搬移、`_wscroll` 關掉之後的行為。
座標與屬性寫到 `OUT.TXT`，畫面本身交給 dosgolem 的文字畫面 dump。
說明見 [conio：一個結構、三個原語、兩條路](../../docs/10-borland-crtl/conio-screen.md)。

## 需要

- 你自己的 Borland C++ 2.0，先用 `tools/bcpp20/install.sh` 裝好（本 repo 不含任何 Borland 檔案）
- Docker
- dosgolem 的 `bcc20-toolchain` 分支

## 執行

```sh
cd examples/conio
export BCPP=~/bcpp20 DOSGOLEM=~/dosgolem
./run.sh          # small 模型
./run.sh h        # huge 模型
```

比對兩樣東西：`OUT.TXT` 對 `expected.txt`，以及 dosgolem 印出的文字畫面對 `expected-screen.txt`。
**五個記憶體模型的結果完全相同**——conio 的行為跟資料指標寬度無關，所以只有一份預期輸出。

## 輸出的每一行

| 標籤 | 測的是 |
|---|---|
| `directvideo` | `directvideo` 與 `_wscroll` 的預設值 |
| `screen`、`win.full`、`attr.init` | `gettextinfo` 拿到的螢幕大小、初始視窗與初始屬性 |
| `win.set`、`win.cursor` | `window()` 之後視窗邊界與游標位置 |
| `goto.in`、`goto.out` | 視窗座標的 `gotoxy`，以及越界時被忽略 |
| `win.bad` | 無效的 `window()` 被整個忽略 |
| `cputs`、`cell.A`、`cell.B` | 寫兩個字元之後的游標與畫面內容 |
| `lf`、`cell.Z` | `\n` 只換列不回欄 |
| `cr`、`bs.left` | `\r` 回視窗左界；`\b` 不退出視窗 |
| `wrap` | 寫滿一列折到下一列 |
| `scroll.pos`、`scroll.top`、`scroll.win` | 在視窗最後一列寫滿觸發捲動，捲的是視窗不是整個螢幕 |
| `attr.fgbg`、`attr.blink`、`attr.clrscr` | 屬性位元的組法；`clrscr` 用目前屬性 |
| `gettext`、`puttext`、`movetext` | 區塊操作（絕對座標）與緩衝區內容 |
| `gettext.rows`、`movetext.overlap` | 多列讀取時第二列從哪裡開始；往下搬一塊會與自己重疊的區域 |
| `bounds.get`、`bounds.move`、`bounds.put` | 越界座標：前兩者回 0 不動作，`puttext` 回 1 而且真的寫下去 |
| `nowrap` | `_wscroll = 0` 時寫超過視窗寬度會發生什麼 |
| `biospath`、`biosclr` | 把 `directvideo` 關掉改走 BIOS，同樣的動作結果一不一樣 |

`cell.*` 那幾行印的是「字元碼 屬性」，從 `gettext` 取回來。

## 已知限制

- **走 BIOS 那條路只測了寫字元與清除**（結果與直接寫顯示記憶體相同）。
  清除走的是「捲 0 列」，那條路不論 `directvideo` 為何都是 BIOS；真正沒測的是 **BIOS 版的捲 1 列**與**BIOS 版的區塊搬移**。
- **捲動的畫面結果與函式庫版本有關**：`SCROLL` 是唯一原始碼與 1991-08 出貨庫對不上的模組，
  1991-04 版捲完之後空出來的那一列會殘留舊內容。這裡的 `expected-screen.txt` 是用 **1991-08 版**跑出來的。
- **`attr.init` 是 0，那是 dosgolem 的畫面初始狀態**，真機上啟動時通常不是 0。
  這一行記錄的是「初始屬性來自畫面上游標那格」這個機制，不是某個固定值。
- 雪花那條路在模擬器裡無從驗證（沒有 CGA 的狀態埠行為）。
