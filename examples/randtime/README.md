# 亂數與時間測試程式

四支程式驗 Borland C++ 2.0 runtime 的 `rand` 家族與 `time` 家族。
說明見[亂數與時間](../../docs/10-borland-crtl/rand-time.md)。

## 需要

- 你自己的 Borland C++ 2.0，先用 `tools/bcpp20/install.sh` 裝好（本 repo 不含任何 Borland 檔案）
- Docker
- dosgolem 的 `bcc20-toolchain` 分支

## 執行

```sh
cd examples/randtime
export BCPP=~/bcpp20 DOSGOLEM=~/dosgolem
./run.sh        # small 模型
./run.sh h      # huge 模型
```

五個記憶體模型（s/c/m/l/h）的輸出全部相同。

## 程式與驗的行為

| 程式 | 輸出 | 驗什麼 |
|---|---|---|
| `RANDTEST.C` | `RAND.TXT` | `rand` 的序列（種子 0/1/2/32767/65535 與預設）、`RAND_MAX`、`random()` 巨集的縮放除法、重設種子得到同一串 |
| `TIMETEST.C` | `TIME.TXT` | `dostounix` 固定輸入、`TZ` 剖析（`putenv` 驅動，含壞格式退回預設）、`ctime`/`localtime`/`gmtime`、`strftime` 全部轉換、`mktime` 的邊界（年份 69/139、2038 溢位、`sec=75`/`hour=25`/`mday=0`/`mday=32` 的正規化、非閏年 2 月 29 正規化成 3 月 1） |
| `DOTIME.C` | `DAY.TXT` | 讀鐘那一側：`getdate`（dosgolem 固定 1993-01-01）、`time`/`ftime` 的界內檢查、`clock` 的 tick 單位與 `CLK_TCK`、`getftime`/`setftime`（dosgolem 的 57h「設定」不落地，只驗服務成功與欄位範圍）、`setdate` 讀回 |
| `MKFEB29.C` | `FEB.TXT` | **`mktime` 對閏年 2 月 29 無限迴圈**：對照組（非閏年 2 月 29、閏年 2 月 30 都正常正規化）先印出來，最後呼叫 `mktime(1992-02-29)`——`run.sh` 用步數上限偵測「沒有結束」，五個模型都掛在那裡 |

expected 檔（`expected-rand.txt`、`expected-time.txt`、`expected-day.txt`）的數值
是由一份**照 RTL 原始碼語意獨立寫的參考模型**算出來的，不是抄程式輸出；
`RAND.TXT`/`TIME.TXT` 與它逐位元組相同，代表「讀原始碼得到的規格」等於「出貨函式庫的行為」。

## 已知的輸出限制

- `DOTIME.C` 的時鐘值會隨（決定性的）執行推進，所以只印 PASS 旗標不印原值；
  `getdate` 與 `setdate` 讀回值印原值，因為 dosgolem 的 `AH=2Ah` 固定回 1993-01-01、
  `AH=2Bh`/`AH=2Dh` 未實作（`setdate` 讀回不變）。
- `FEB.TXT` 的最後一行是 `calling mktime(1992-02-29)...`——之後程式不會回來，
  這正是要驗的行為。
