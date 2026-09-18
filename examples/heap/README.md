# heap 行為測試程式

`HEAP.C` 把 Borland C++ 2.0（以下簡稱 BC++ 2.0）的 heap 配置策略跑出來：區塊大小的進位規則、
切割方向、合併、`realloc` 的搬家與就地縮小、`calloc`、失敗時的 `errno`、`heapcheck`／`heapwalk`、
far heap 的段落對齊與 `farcoreleft`。結果寫到 `OUT.TXT`。
說明見[near heap 與 far heap](../../docs/10-borland-crtl/heap.md)。

位址本身隨載入位置而變，所以程式只印**差值、是否相等、回傳值**這些跨執行穩定的東西。

## 需要

- 你自己的 Borland C++ 2.0，先用 `tools/bcpp20/install.sh` 裝好（本 repo 不含任何 Borland 檔案）
- Docker
- dosgolem 的 `bcc20-toolchain` 分支

## 執行

```sh
cd examples/heap
export BCPP=~/bcpp20 DOSGOLEM=~/dosgolem
./run.sh          # small 模型
./run.sh h        # huge 模型
```

`run.sh` 編譯、在 dosgolem 執行，並和預期輸出比對：near 資料模型（small、medium）對 `expected.txt`，
far 資料模型（compact、large、huge）對 `expected-far.txt`。

**兩份預期輸出差很多，因為換了配置器**：near 資料模型的 `malloc` 走 near heap（單位是 byte、
區塊頭 4 bytes），far 資料模型走的是 far heap（單位是段落 16 bytes、區塊頭 4 bytes、free 頭 10 bytes）。
所以同樣 `malloc(1)`，near 模型相鄰兩塊差 8 bytes、far 模型差 16 bytes。

## 輸出的每一行

| 標籤 | 測的是 |
|---|---|
| `ptr` | `sizeof(void *)`、`sizeof(void far *)` |
| `layout.1`、`layout.8`、`layout.9`、`layout.10` | 連續配置同樣大小時相鄰兩塊的距離＝資料區＋區塊頭，看得出進位規則與最小區塊 |
| `zero` | `malloc(0)` 回不回 NULL |
| `split` | 放掉中間一塊、再要一塊小的：新區塊落在空洞的哪一端 |
| `exact` | 放掉一塊再要同樣大小，會不會拿回同一個位址 |
| `coalesce` | 實體相鄰的兩塊都放掉之後，再要一塊吃得下合併結果的：位址落在哪 |
| `realloc.grow`、`realloc.shrink` | 指標變不變、內容留不留 |
| `realloc.tail` | 縮小之後吐出來的空間能不能再配出去 |
| `realloc.zero`、`realloc.null` | `realloc(p, 0)` 與 `realloc(NULL, n)` |
| `calloc` | 有沒有清零 |
| `toobig` | 要不到的量：回傳值與 `errno` |
| `calloc.overflow` | `calloc(30000, 4)` 的乘積溢位 |
| `heapcheck` | `heapcheck()`、對用中區塊與已放掉區塊各做一次 `heapchecknode()` |
| `heapwalk` | 走完整個 heap 的用中／自由區塊數與結束碼 |
| `far.malloc`、`far.gap` | `farmalloc` 回傳位址的位移（區塊頭大小）與 `farcoreleft` 的減少量 |
| `far.restore`、`far.one` | `farfree` 之後可用量回不回升、再配置會不會重用 |
| `far.walk0`…、`far.walk` | `farheapwalk` 走出來的區塊序列（相對段、大小、用中與否）與結束碼 |
| `coreleft` | `coreleft()` 有沒有隨配置變少、放掉後回來、是不是 16 的倍數 |

## 量測上的兩個坑

- **far 指標不能直接相減**。`(char *)hi - (char *)lo` 對 far 指標只算位移，跨段就得出 0。
  程式在 far 資料模型下把段與位移攤成線性位址再相減。
- **far 區塊的距離不要拿 `FP_SEG` 相減或 huge 指標算術去量**，會得到看起來自洽、其實錯誤的數字；
  可靠的來源是 `farheapwalk` 走出來的序列。

## 已知限制

`far.restore` 這類數字與 heap 當下的狀態有關（前面的測試在 far heap 裡留下了什麼），
所以兩份預期輸出在這幾行不同，且**不能當成通用規格**——通用的部分寫在文章的行為規格一節。
