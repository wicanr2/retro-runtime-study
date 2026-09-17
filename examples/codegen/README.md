# BCC 2.0 產生碼測試程式

`CODEGEN.C` 有 33 個 `cg_` 函式，每個只示範一種語法構造（函式進出、暫存器變數、`switch`、呼叫、全域與 far 資料、
long 運算、立即值運算、浮點），`main` 呼叫它們並把結果寫到 `OUT.TXT`。
說明見 [BCC 2.0 產生的程式碼在反組譯裡長什麼樣](../../docs/60-re-fingerprints/bcc20-codegen.md)。

## 需要

- 你自己的 Borland C++ 2.0，先用 `tools/bcpp20/install.sh` 裝好（本 repo 不含任何 Borland 檔案）
- Docker
- dosgolem 的 `bcc20-toolchain` 分支

## 執行

```sh
cd examples/codegen
export BCPP=~/bcpp20 DOSGOLEM=~/dosgolem
./run.sh                # small 模型、不加選項
./run.sh l -O -G -Z     # large 模型、加最佳化選項
./run.sh h -1           # huge 模型、80186 指令
```

`run.sh` 做三件事：以 `-S` 輸出組語清單 `out/codegen.asm`、正常編譯連結出 `out/codegen.obj` 與 `out/codegen.exe`、
在 dosgolem 執行並把 `OUT.TXT` 與 `expected.txt` 比對。

五個記憶體模型搭配不加選項、`-O`、`-G`、`-Z`、`-O -G`、`-O -G -Z`、`-r-`、`-k`、`-k-`、`-1` 的輸出都與 `expected.txt` 相同。
`-f87` 編出的程式直接執行 8087 指令，dosgolem 沒有完整的 x87，`float` 那一行會不同。

## 看產生碼

- **看指令**：讀 `out/codegen.asm`，每個函式前面有原始碼行的註解。
- **看位元組**：把 `out/codegen.obj` 交給反組譯工具。`-S` 清單交給 TASM 組出的位元組與 BCC 直接產生的有兩處不同
  （`and`／`or`／`xor` 的立即值寬度、浮點轉整數前的 `nop`），位元組特徵要以 `codegen.obj` 為準。

## 輸出的每一行

| 行首 | 內容 |
|---|---|
| `frame` | 兩個參數相加；有區域變數；300 bytes 的區域陣列 |
| `loop` | `for` 迴圈加總 0–9；計算字串長度；`do … while` 加總 4–1 |
| `dense` | 連續 case 0–5 的 `switch`：0、5、6（default） |
| `sparse` | 不連續 case 的 `switch`：−7、1000、5000、2（default） |
| `two` | 兩個 case 的 `switch`：3、9、4（default） |
| `swlong` | `long` 的 `switch`：3、70000、65537（default） |
| `call` | 兩次呼叫相加；五個參數；pascal 函式；同模組的 far 函式 |
| `long` | `long` 的加減與位元運算；兩次有號比較 |
| `data` | 全域陣列；明寫 `far` 的全域陣列；結構成員 |
| `ptr` | 經 far 指標取字元 |
| `math` | 乘、除、取餘常數（−37 與 37）；無號除以 16 加右移 3；`char` 加 `unsigned char` |
| `cond` | `?:` 取大值兩次；`&&`／`||` 兩次 |
| `imm` | 暫存器與全域變數上的 `and`／`or`／`xor`／`+=`／`-=` 小常數 |
| `float` | `(1.25 × 4 + 0.5) × 10` 轉 `long` |
| `string` | 回傳字串常數 |
