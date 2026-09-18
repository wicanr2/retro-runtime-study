# printf／scanf 行為測試程式

`PRINTF.C` 把 BC++ 2.0 格式化家族的邊界行為跑出來：寬度與精度、旗標、Borland 的 `%Np`／`%Fp`、
格式寫錯時的輸出、`%n`、超過內部緩衝區的長輸出、浮點的進位規則，以及 `sscanf` 的回傳值與各種轉換。
結果寫到 `OUT.TXT`。說明見 [printf 家族：一個引擎、三個出口，與浮點的連結開關](../../docs/10-borland-crtl/printf-engine.md)。

## 需要

- 你自己的 Borland C++ 2.0，先用 `tools/bcpp20/install.sh` 裝好（本 repo 不含任何 Borland 檔案）
- Docker
- dosgolem 的 `bcc20-toolchain` 分支

## 執行

```sh
cd examples/printf
export BCPP=~/bcpp20 DOSGOLEM=~/dosgolem
./run.sh          # small 模型
./run.sh h        # huge 模型
```

`run.sh` 編譯、在 dosgolem 執行，並和預期輸出比對：near 資料模型（small、medium）對 `expected.txt`，
far 資料模型（compact、large、huge）對 `expected-far.txt`。**兩個檔只差 `p.default` 那一行**——
`%p` 的預設輸出跟著資料指標寬度走，near 資料印四個字元、far 資料印九個（`段:位移`）。
其餘每一行五個模型完全相同。

## 輸出怎麼讀

多數行是「標籤 `|`格式化結果`|` 回傳值」，兩個直線之間就是 `sprintf` 寫出來的東西，方便看出空白與長度。
少數行（`p.default`、`n`、`wide200`、`scan.*`）改印數值，因為要看的是長度或回傳值。

| 標籤 | 測的是 |
|---|---|
| `d`、`d.width`、`d.prec`、`d.sign` | 整數的寬度、靠左、補零、精度、正負號旗標 |
| `u.x.o`、`alt`、`long` | 無號、十六進位、八進位；`#` 旗標；`long` 修飾 |
| `star`、`negstar` | 寬度與精度由引數給；負的寬度等於靠左對齊 |
| `s`、`s.prec`、`s.null` | 字串的寬度與精度；空指標印 `(null)` |
| `c`、`c.zero`、`pct` | 字元轉換；`%c` 印 0；`%%` 與不認得的 `%5%` |
| `p.far`、`p.near`、`p.default` | Borland 的 `%Fp`（`段:位移`）、`%Np`（四位十六進位）與預設的 `%p` |
| `bad.q`、`bad.end`、`bad.width` | 格式寫錯：從那個 `%` 開始當一般文字輸出，沒有錯誤回傳 |
| `n` | 兩個 `%n` 拿到的已輸出長度，與整體回傳值 |
| `wide200`、`wide120` | 輸出超過引擎內部 80 bytes 緩衝區 |
| `f`、`f.prec`、`e`、`g`、`zero`、`neg`、`big` | 浮點的預設精度、指數位數、`%g` 的去尾 |
| `round` | `%.0f` 對 0.5、1.5、2.5、3.5：**四捨六入五成雙** |
| `scan.d`、`scan.skip`、`scan.width` | `sscanf` 的基本轉換、跳空白、寬度切分 |
| `scan.fail`、`scan.empty` | 型別不合回 0；輸入是空的回 −1（EOF） |
| `scan.radix`、`scan.long`、`scan.f` | `%x` 認 `0x` 前綴、`%o`、`%ld`、`%lf` |
| `scan.s`、`scan.c`、`scan.set` | `%5s`、`%c` 不跳空白、`%[abc]` 字元集合 |
| `scan.star`、`scan.lit`、`scan.litfail`、`scan.n` | `%*d` 不計入回傳值；格式字串裡的字面字元；`%n` |

浮點那幾行需要程式裡真的有浮點運算，否則連結器不會把轉換函式接上去——這正是文章裡
`_CVTSEG` 向量那一節在講的事。

## NOFLOAT.C：沒有浮點運算的那一版

同目錄的 `NOFLOAT.C` 用 `%f` 印一個從檔案讀進來的 `double`，程式本身沒有任何浮點運算。
它是文章裡「`_CVTSEG` 長度 0」那一列的來源：連結器沒有理由把浮點轉換或模擬器拉進來，
向量因此是空的。編法與 `PRINTF.C` 相同（`tools/bcpp20/bcc.sh`），執行前要先在暫存層放一個 8 bytes 的 `VAL.BIN`。

## 已知限制

`-f87` 編出來的版本在 dosgolem 下的浮點結果與軟體模擬不同（dosgolem 沒有完整的 x87），
那是執行環境的差別，不是編譯器或函式庫的差別。預設不加 `-f87`。
