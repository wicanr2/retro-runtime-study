# 檔案 I/O 行為測試程式

`STDIO.C` 把 BC++ 2.0 的檔案 I/O 邊界行為跑出來：文字模式的 CR／LF 轉換與 `Ctrl-Z`、`ftell`／`fseek`、
緩衝策略、附加與更新模式、`ungetc`、`fgets`、錯誤碼、低階代號層。結果寫到 `OUT.TXT`。
說明見[檔案 I/O：FILE、緩衝區、文字模式與錯誤碼](../../docs/10-borland-crtl/stdio-file-io.md)。

## 需要

- 你自己的 Borland C++ 2.0，先用 `tools/bcpp20/install.sh` 裝好（本 repo 不含任何 Borland 檔案）
- Docker
- dosgolem 的 `bcc20-toolchain` 分支

## 執行

```sh
cd examples/stdio
export BCPP=~/bcpp20 DOSGOLEM=~/dosgolem
./run.sh          # small 模型
./run.sh h        # huge 模型
```

`run.sh` 編譯、在 dosgolem 執行，並和預期輸出比對：near 資料模型（small、medium）對 `expected.txt`，
far 資料模型（compact、large、huge）對 `expected-far.txt`。兩個檔只差第一行的 `sizeof(FILE)`（16 對 20）。

測試程式會在 dosgolem 的暫存層建立 `T1.TXT`～`TC.TXT` 等測試檔，不會動到來源目錄。

## 輸出的每一行

每行是「標籤 三個數字」或「標籤 一串十六進位位元組」。

| 標籤 | 測的是 |
|---|---|
| `BUFSIZ` | `BUFSIZ`、`FOPEN_MAX`、`sizeof(FILE)` |
| `textwrite`、`binwrite` | 文字與二進位模式寫 `"a\nb\n"` 之後，檔案裡的位元組 |
| `textread`、`textread.bytes` | 文字模式讀 `x\r\ny\rz\r\n`：拿到幾個位元組、`feof`、`ftell` |
| `ctrlz`、`ctrlz.bytes`、`ctrlz.bin` | 文字模式遇到 `1Ah` 停住；二進位模式照讀 |
| `ftell.text` | 文字模式寫入後的 `ftell`（`fflush` 前後）與檔案長度 |
| `seekgap` | `fseek` 越過檔尾再寫，中間的洞 |
| `readpasteof`、`eofflag`、`clearerr` | 讀到檔尾之後的 `ftell`、`feof`、`ferror` |
| `fullbuf`、`nobuf`、`smallbuf` | 三種緩衝方式下，資料什麼時候真的進檔案 |
| `append`、`update` | `"ab"` 與 `"r+b"` 模式的寫入位置 |
| `ungetc` | 推回一個字元前後的 `ftell` |
| `fgets1`–`fgets3` | 有換行的一行、檔尾沒換行的一行、檔尾再讀一次 |
| `appendpos` | `"ab"` 開檔後的 `ftell` 與檔案長度 |
| `noent`、`badmode`、`open.noent`、`read.badfd` | 四種失敗路徑的回傳值、`errno`、`_doserrno` |
| `dup.pos`、`fileno` | 代號層：`dup` 共用位置、`fileno`／`isatty`／`eof` |
| `fmode` | `_fmode` 的預設值是不是文字模式 |
