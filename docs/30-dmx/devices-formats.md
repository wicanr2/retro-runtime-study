---
id: dmx/devices-formats
title: DMX 的裝置與格式：七驅動、出貨雙軌與混音六入口
libraries: [dmx]
goals: [craft, re]
evidence: 已證實
triggers:
  - 想知道 DMX 支援的七種卡各由哪支檔驅動、表裡填什麼
  - 封存裡有多支同名驅動（fm3812／fmopl2、codec／clabs），想知道出貨用哪支
  - 遊戲附 .dsp 取樣檔，想知道檔頭長什麼樣
  - 看到 fmfix 補丁，想知道它到底修了什麼、能不能直接蓋掉 3.4a
symbols: [FmMus, clWav, mvWav, Gf1Mus, Gf1Wav, MpuMus, AweMus, AL_Detect, SB_Detect, FmBoardInstalled, AWE_DetectCard, SB_SetCard, dmxMix8bitStereo, dmx16to8bit, dmxClipSeperate, WAV_SAMPLE, GSS_PATCH, FMT_WAV_SAMPLE, DMXGUS, pawe32, wcc386p, DOS4G]
related: [dmx/api-core, dmx/version-history]
---

# DMX 的裝置與格式：七驅動、出貨雙軌與混音六入口

一句話：DMX 3.4a（DOS 遊戲音效庫，
架構見前篇）要餵七種卡：
每家驅動填一張表，
其中四家（SB／FM／MPU／AWE）另有
`Detect` 探測加 `SetCard` 定址、
PAS 與 GUS 只有 `Detect`
（GUS 以 `SetMap` 餵配置）；
呼叫端先探測定址、
再 `DMX_Init` 按固定順序試各表 `Init`；
封存裡與出貨同名的備份五件
（舊版與實驗分支），
另有測試工具檔未進庫，
雙軌＝進庫 20 件對未進庫，
以 `dmx.mif` 清單為準才不會讀錯邊；
取樣三家靠 DMA 多頁環加軟體混音，
SFX 四格式首字是順編 enum，
獨立 `midi2mus` 與庫內嵌版同源異本；
`fmfix` 偵測更嚴但搶聲更簡，
不是單向升級。

## 結論

驅動規格：每張卡填一張表
（`DMX_MUS` 或 `DMX_WAV`，動詞見前篇）。
SB／FM／MPU／AWE 四家另有 `Detect`
（回 0 表命中，C 慣例 0 即成功）
加 `SetCard`，但簽名各異：
SB 收埠／IRQ／DMA 三元
（`BLASTER` 環境來，讀不到用預設
0x220／7／1），
FM 收等待狀態加音色庫指標，
MPU 與 AWE 只收埠址；
`AL_Detect` 回填等待狀態（初值 6，
寫入後空讀埠的次數）與卡型
（2＝OPL2、3＝OPL3）。
PAS 與 GUS 只有 `Detect`，
無 `SetCard`——
GUS 以 `GF1_SetMap` 餵 INI 映像。
順序鐵律：呼叫端先 `Detect`、
再 `SetCard`、最後 `DMX_Init`
（三支測試程式皆此序，
`AWE_Detect` 註解明寫先於 `DMX_Init`）；
`DMX_Init` 內零次調 `Detect`，
只按 GUS→PAS→AWE→SB→MPU→AdLib→PCS
順序試各表 `Init`。

七家分工（括號是發聲原理）：
`FmMus`（FM 合成，OPL2／3 晶片現場算波形）、
`clWav`（Creative Labs 即 SoundBlaster 系，
播錄好的取樣）、
`mvWav`（MediaVision PAS，取樣）、
`Gf1Mus／Gf1Wav`（Gravis Ultrasound，
自帶記憶體，雙表全包）、
`MpuMus`（MPU-401，MIDI 介面，
轉手送指令給外接音源）、
`AweMus`（AWE32，自帶音色庫的取樣合成卡）、
PCS（PC 喇叭，無表直調，見前篇）。

出貨以 `dmx.mif` 為準，共 20 件：
核心十件（`dmxapi／musapi／wavapi／pcsapi／
gssapi／sfxapi／tsmapi／dspapi／dmxdisk／dpmiapi`）
加驅動六件（`fmopl2／clabs／pas／mpu401／
gravis／awe32`）加底層四件
（`mixers` 混音、`realint` 實模式跳板、
`pcdma` DMA、`pcints` 中斷）。
與出貨同名的備份五件：
`fm3812`（舊 FM，未進；
OPL3 段被 `#if 0` 關閉，編出永遠 9 聲部，
也無立體聲／平衡／搶聲計分）、
`codec`（CODEC 取樣卡，
定同名 `clWav／SB_Detect／SB_SetCard` 三符號，
與 `clabs` 連結期互斥——
同名即符號衝突，出貨只留一支）、
`qsound`（實驗混音，
同名六入口、`Interleave` 內核改單流）、
`pcint`（舊中斷層，被 `pcints` 取代）、
`hpmixers`（另一套混音，
只有混四入口、無削波兩入口，
非等價替換）。
另有測試工具檔未進庫
（`playmus／playwav／test／testmix／t／tohex`、
`pztimer` 等），
讀到同名檔先查 `mif`，
這是讀 DMX 源碼的第一條紀律。

FM 驅動三段式：存在測試先用 Timer-1 溢出法
（遮罩計時器、讀狀態、啟動 T1、延遲再讀，
狀態埠高三位元 0 變 `0xC0` 才算有卡——
因果：計時器會跑＝晶片活著；
源碼不讀 ID 而用此法），
再判 OPL3（`DMXOPTION` 的 `USE_OPL3` 位元是總開關，
沒設永遠 9 聲部；
開了再讀基底埠，讀回 0 才升 18 聲部——
聲部＝同時發聲數，OPL2 是 9、OPL3 是 18），
運轉期靠復音分配（活躍／非活躍兩列、
音色庫）與搶聲計分
（`Rate＝Attack＋Release`，
由音色的起音衰減與延音釋放參數算出，
聲部不夠時挑分數最低、
且舊聲部通道號不小於新音符的通道號者犧牲，
只掃活躍數減三——
留三給誰源碼無註解，
以比較式推或為保底高通道，純猜測）。
（`InstNotes[175]` 宣告了但全版零讀寫，
疑為殘留死陣列；
`Channel 15→2` 特判算而不用，
比較式仍用原通道號，疑為 bug，
`fmfix` 刪的正是這段死碼。）
`Port 388h` 寫死，
延遲初值等待 6、索引 6、資料 24
（空讀埠次數），
可經 `AL_SetCard` 改等待狀態、
`FmInit` 按等待數重算
（索引＝等待、資料＝等待乘 4）。
立體聲段：`Stereo[]` 初始全 `0x30`
（暫存器置中值）、
輸入平衡 0..127 分三段
（48 以下、96 以上、中間，
輸出三值 `0x20／0x10／0x30`，
左右對應待 datasheet 確認）；
未進出貨的 `fm3812` 無此段。

取樣三家共享同一套混音後端：
DMA 多頁環（4K 緩衝切頁，
頁大小 128／256／512、
頁數 32／16／8，
視 11k／22k 與單聲／立體聲而定，
`PageMixed` 記混到哪頁；
DMA 即卡自己從記憶體抓波形，
頁面跟不上就斷音）加
`dspapi` 虛聲道層
（`gVirtChan[]`：每路取樣的波形位置／
音量／取樣率轉換商餘／相位，
混完一頁才交給卡；
`gPanTables` 平衡表）。
混音本體是六個組語入口
（`dmxClipInterleave／Seperate／Mix8bitExpStereo／
Stereo／Mono／16to8bit`，
`Seperate` 拼法原文如此）：
先混（多路 16 位元累加、
單聲道／立體聲／擴展立體聲三味）再削
（`clip.inc` 查表壓回 8 位元：
削波＝超過範圍夾住，
查表無分支、時間可預期），
`.386` 平坦模式（32 位元保護模式組語）、
`tasm` 組譯、`REPT` 迴圈展開。
`Interleave` 出貨版是雙流交錯
（一次處理左右兩聲道、各查表、合存），
qsound 版是單流（一次一個取樣，
仍查表、無第二聲道路徑）——
同名不同實，建置只用出貨版。

GUS 是全庫最大一支（4196 行）：
四組記憶體庫（`gf1_bank[4]`，
卡上記憶體分四庫、逐庫記基址用量，
載音色逐庫找洞）、
聲道遮罩與年齡戳
（`voice_mask` 記誰空、
年齡戳取最小者分配，搶聲搶最老的）、
音色映射（`gf1_patch` 加程式號加交叉表，
MIDI 號到卡上音色兩次翻譯）、
疊音 32 乘 4（一音符疊四層樣本）、
配置外置 `DMXGUS.INI`
（卡上記憶體多大決定載哪些樣本，
見版本篇的樣本分級表）、
16 通道 MIDI 狀態。
一家填兩張表（`Gf1Mus` 加 `Gf1Wav`），
歌曲取樣通吃。
MPU-401 是純源碼驅動中最短：
切 UART 直寫模式（命令 `0x3F`，
把 MIDI 介面當串列埠塞位元組）後，
命令等 ACK（`0xFE`）、
資料輪詢 `NOT_READY`
（`MPU_TIMEOUT` 32000 輪放棄），
忙等直寫、無佇列結構。
AWE32 分兩層：
DSP 層在源碼內（`AWE_DetectCard`：
埠加 6 重置、讀回 `0xAA`、
`0xE1` 取版本、主版號小於 4 拒絕；
`AWE_Detect` 掃 0x220／240／260），
EMU8000 層（`awe32Detect／InitHardware`、
MIDI 函式、SoundFont 物件）在
`lib／pawe32.lib`（二進位 OMF 庫，
只有機器碼沒有 `.c`）。
前篇未知項修正：
DSP 探測有答案，EMU 層永久無答案。
三家對比：4196／719／466 行加一座二進位庫——
同樣的表，背後厚度完全不同。

格式分兩群。
歌曲是 `MUS`：檔頭 `MUS\x1A` 四位元組加
音樂長度／偏移／主次通道數／音色數，
後接精簡事件流（格式定位見版本篇）。
（`\x1A` 是位元組 `0x1A`，
DOS 文字檔結尾字元，
原文簽名字段寫 `M U S Eof`，
`Eof` 即指它。）
音效是 SFX 四格式，首字 `FMT_*` 是 enum 順編
（`FMT_PCS_WAVE＝0、FMT_GSS_WAVE＝1、
FMT_GSS_PATCH＝2、FMT_WAV_SAMPLE＝3`，
正是前篇路由鍵；
`PCS_WAVE` 等去前綴名是 struct 名，
不要跟 enum 搞混）：
`PCS_WAVE` 首字加波長、
`GSS_WAVE` 首字加波長加 Bank／Patch、
`GSS_PATCH` 首字加 Bank／Patch／Note／
140 tick 為單位的長度加保留填充字、
`WAV_SAMPLE` 首字加取樣率（11025 到 44100）
加位元組長（長度欄是 `DWORD`，
他欄皆 `WORD`）。
路由實例：讀到首字 3 就走 `WAV_SAMPLE` 分支，
前篇的 `switch (HIWORD)` 即判此值。
`WAV_LoadPatch` 讀 `*.dsp`
（測試程式實證 `drum／racecar／medium` 三檔，
`.dsp` 即裝 `WAV_SAMPLE` 的檔），
`open／read／malloc` 直讀，
播歌註冊一樣吃記憶體指標——
`dmxdisk` 快取只有 GUS 驅動在用。

`midi2mus` 獨立工具（1272 行）與庫內嵌版同源異本：
`TRACK／VOICE` 結構、`MAX_TRACK_SIZE 65000` 同構，
獨立版多 `PatchesUsed[189]`
（追蹤用過的音色與 Bank，
供後續流程參考，強推論），
內嵌版整段沒有——
改一邊另一邊不會自動跟，
對讀時先認清手上是哪一本。

`fmfix` 改兩處，方向相反：
偵測更嚴（基底埠讀回 0 且第二埠 `0x38A` 讀回
`0xFF` 才認 OPL3，並附四行真值表註解，
取代 3.4a 被註解自陳會在 SB Pro II 誤判的舊判式），
搶聲更簡（整套 `Rate` 計分、
`Channel 15→2` 死碼特判、
減三範圍一併拿掉，
改掃全部活躍聲部、挑通道號最大者）。
一嚴一簡（淨效果偵測修好、搶聲退步，
強推論：優劣未經量測），
所以 `fmfix` 不能直接蓋掉 3.4a
（要拿偵測修正得連搶聲一起退回去，
或自行把嚴偵測移植過去——
兩處分屬不同函式，結構分離，
但移植未經編譯驗證）。
兩支同日、`$Log` 皆空，
先後只能從目錄名與 3.4a 的註解自承推
（強推論，見版本篇）。
（`fmfix` 內容物分屬 `api／fmopl2.c` 與
`example／test.exe`。）

工具鏈（makefile 實證）：
`wcc386p`（Watcom C，編 32 位元 DOS
保護模式程式）加 `tasm`（組譯器）加
`wlib`（建庫），跑在 DOS／4GW 上
（一種 DOS extender，
讓 32 位元程式跑在 DOS 下，
`DOS4G=QUIET` 關 banner，
細節見後篇）。
出貨旗 `-oxt -4r`
（極致優化加暫存器呼叫：
參數盡量放暫存器傳）、
結構 `-zp1`（一位元組緊排；
`WAV_LoadPatch` 以 `sizeof` 讀檔直入 struct，
佈局即格式，緊排實證相關）、
`%version＝debug／beta／production` 三檔
（分別帶 `-dDEBUG／-dBETA／-dPROD`）。
`mixers.obj` 顯式相依 `clip.inc`
（改表重組，依賴寫死）；
`test.exe` 連的是上層 `..\lib\dmx.lib`
（編出的庫先拷到上層再連測試，
不是就地連）。
`#ifndef PROD` 除錯碼
（前篇的邊框色與追蹤）
就是這套開關留下的。

## 根本問題：七種卡要七種形狀，收斂點在哪

1993 年的遊戲音效是硬體叢林，
每種卡要的資料形狀都不同：
FM 合成卡吃暫存器命令、
取樣卡吃連續波形（DMA 送）、
MIDI 介面卡吃串列位元組、
GUS 自帶記憶體（先載後播）、
PC 喇叭只有開關（查表定頻）。
DMX 的收斂分三層：
裝置層收斂成兩種動詞（播歌曲、播取樣），
混音層收斂成一條管線（虛聲道混成 16 位元、
削波成 8 位元、DMA 送卡），
格式層收斂成首字路由（四種音效資料一個入口）。
每層的代價都寫在源碼裡：
驅動要自帶探測（當年無隨插即用），
混音要手寫組語（當年無作業系統混音器），
格式要自帶魔術字（當年無登錄檔查副檔名）。

## 推導

### 三件套與出貨雙軌

新支援一張卡至少要填一張表
（八個或五個函式指標）。
SB／FM／MPU／AWE 另有 `Detect` 加 `SetCard`，
簽名各異，不可互套；
PAS 與 GUS 只有 `Detect`，
GUS 以 `SetMap` 餵配置代替定址。
呼叫順序是測試程式鐵證：
先 `Detect` 取參數（SB 例：
`SB_Detect` 回填埠／IRQ／DMA／版本）、
再 `SetCard` 定入、
`TSM_Install` 裝時鐘、
最後 `DMX_Init` 試各表 `Init`——
探測定址永遠走在 `Init` 前面
（`AWE_Detect` 註解明寫先於 `DMX_Init`）。

封存不是出貨，`dmx.mif` 才是：
`dmx.mif` 是出貨清單檔，
20 件進庫；
與出貨同名的備份五件各有來歷可推：
`fm3812` 是舊 FM
（行數差 49、diff 180 行，
OPL3 段被 `#if 0` 關閉，
立體聲整套沒有）、
`codec` 是另一條取樣卡路線
（三符號與 `clabs` 衝突，只能二擇一）、
`qsound` 是實驗混音（來歷待查）、
`pcint` 被 `pcints` 取代（差一個 `s`，
`dmx.h` 的 include 跟著改）、
`hpmixers` 是另一套混音
（缺削波兩入口，用途待查）。
測試工具檔（`playmus／playwav／test／
testmix／t／tohex`、`pztimer` 等）
本來就不進庫，另算一群。
雙軌＝進庫對未進庫，
讀到同名檔先查 `mif`。

### FM：存在測試、開關、搶聲

FM 驅動（`FmBoardInstalled` 存在測試、
`FmInit` 初始化）三段式層層加碼。
存在測試用 Timer-1 溢出法：
遮罩兩個計時器、清中斷、讀狀態（`s1`）、
設 T1 鎖存 `0xFF`、啟動 T1、
空轉延遲（80 微秒以上等溢出）、再讀狀態（`s2`）——
高三位元從 0 變 `0xC0` 才算有卡。
判式裡的 `0xE0` 遮罩與 `0xC0` 期望值寫死在碼裡，
錯一邊都不認。

OPL3 判定分兩閘：
`USE_OPL3` 位元是總開關（環境字串來，
沒設永遠 9 聲部），
開了再讀基底埠（讀回 0 才升 18 聲部、
型別記 3）。
3.4a 的第二閘太鬆，
註解自陳在某些 SB Pro II 上誤判
（原話帶髒字，此處不引）——
`fmfix` 的存在理由就是這一閘。

運轉期是復音分配加搶聲：
活躍／非活躍兩列記聲部歸屬、
音色庫供載入。
聲部不夠時算分搶：
`Rate` 是起音速率加釋放速率
（各由音色參數的高／低半位元組反算，
15 減之——包絡越慢分越高），
掃活躍列（留三個不掃，原因待查）、
限定舊聲部通道號不小於新音符的通道號、
挑分數最低者犧牲。
`Channel 15→2` 特判算而不用
（比較式仍用原通道號），
是死碼，不是機制。
立體聲段：`Stereo[]` 初始全 `0x30`
（暫存器置中值，注意不是輸入刻度 48）、
輸入平衡 0..127 分三段輸出三值
（左右對應待 datasheet 確認），
未進出貨的 `fm3812` 無此段。

圖為結構主線
（僅列動詞槽，`Init／DeInit` 略；
18 聲部需 `-opl3` 開關；
格式盒餵前篇兩層，
`fmfix` 作用於 FM 盒，
`Seperate` 拼法原文如此，
上有文字註明，箭頭略）：

<p align="center"><img src="../../img/dmx-devices-formats.svg" width="640" alt="DMX 裝置與格式：七驅動填兩張表，取樣三家經混音管線，SFX 四格式首字路由"></p>

### 取樣：DMA 多頁環與虛聲道

取樣卡不能像 FM 那樣一個音符一個命令，
得連續餵波形。
做法是 DMA 多頁環：
4K 緩衝切頁，頁大小向 DSP 查詢
（11k 單聲 128、22k 單聲或 11k 立體聲 256、
22k 立體聲 512；
頁數反比 32／16／8），
卡播當頁、CPU 混後頁，
`PageMixed` 記混到哪頁——
頁面跟不上就斷音。
每路取樣是一個虛聲道（`gVirtChan[]`）：
記波形位置、音量指標、
取樣率轉換的商餘累加、相位與 QSound 參數，
混完一頁才交給卡；
平衡另查 `gPanTables`。
三家取樣驅動（`clabs／pas／gravis` 的 WAV 半）
共享這套後端，
差別只在怎麼把混好的頁送上各自的卡。

### 混音六入口

六個入口分兩步：混、削。
混有三味：
`Mono`（多路累加成單聲道）、
`Stereo`（左右分開累加）、
`ExpStereo`（擴展立體聲）；
`16to8bit` 是位元轉換。
削有兩式：
`Interleave`（左右交錯存成雙聲道流）、
`Seperate`（分開存，拼法原文如此）。
削波不用 `if` 夾取，
用 `clip.inc` 查表（16 位元值當索引、
一查即得 8 位元飽和值，
128 行全表，建置相依明寫在 makefile 裡）——
查表省掉分支，迴圈時間可預期；
混音迴圈最怕時間不確定的分支。
qsound 版同名同入口，
`Interleave` 改單流（一次一個取樣），
其餘待讀（範圍外）。

### GUS、MPU、AWE：大、中、小

GUS 驅動是全庫最大（4196 行，
第二名 `musapi` 才 2169 行），
因為 GUS 把事做全了：
卡上記憶體當四庫管（逐庫記基址與用量，
載音色逐庫找洞）、
聲道分配用遮罩加年齡戳
（`voice_mask` 記誰空、年齡戳取最小者、
用後加一，搶聲搶最老的）、
音色映射三層（`gf1_patch` 加程式號加交叉表，
MIDI 號到卡上音色兩次翻譯）、
疊音 32 乘 4（一音符疊四層樣本）、
配置外置 `DMXGUS.INI`
（卡上記憶體多大決定載哪些樣本，
見版本篇的樣本分級表）。
一家填兩張表（`Gf1Mus` 加 `Gf1Wav`），
歌曲取樣通吃。

MPU-401 是純源碼驅動中最短：
切 UART 直寫模式（命令 `0x3F`）、
之後命令等 ACK（`0xFE`）、
資料忙等 `NOT_READY`
（`MPU_TIMEOUT` 32000 輪放棄），
直寫、無佇列結構。
AWE32 分兩層：
DSP 層在源碼內
（`AWE_DetectCard`：埠加 6 重置、
讀回 `0xAA`、`0xE1` 取版本、
主版號小於 4 拒絕；
`AWE_Detect` 掃三埠），
EMU8000 層在 `lib／pawe32.lib`
（二進位 OMF 庫，無源碼）。
三家對比是 craft 展品：
同樣的表，背後可以是四千行、七百行、
或四百行加一座二進位庫。

### 格式：MUS 檔頭與 SFX 四首字

`MUS` 檔頭是 `DMX_HEADER`：
`MUS\x1A` 四位元組簽名、
音樂長度、音樂偏移、
主／次通道數、音色數、保留字。
事件流接在偏移之後（見版本篇定位）。

SFX 四格式的設計是一句話：
首字 enum 路由，後面各表各的。
`PCS_WAVE` 最短（首字加波長，
波形是頻率表索引序列——
逐位元組查 `pcFreq` 餵 timer，
見前篇）；
`GSS_WAVE` 加 Bank／Patch
（波形配音色號）；
`GSS_PATCH` 無波形（Bank／Patch／Note／
140 tick 長度加保留填充，純指令）；
`WAV_SAMPLE` 加取樣率與位元組長
（11025 到 44100，後接原始波形；
長度欄是全表唯一的 `DWORD`）。
`*.dsp` 檔就是裝 `WAV_SAMPLE` 的容器，
`WAV_LoadPatch` 以 `open／read／malloc` 直讀，
註冊播歌也傳記憶體指標——
`dmxdisk` 快取只有 GUS 驅動在用。

### 轉換器同源異本

`midi2mus／midi2mus.c`（1272 行，獨立命令列工具）
與 `musapi.c` 的 `Midi2Mus`（庫內嵌）同源：
`TRACK／VOICE` 結構同構、
`MAX_TRACK_SIZE 65000` 同值、
塊解析與三處理（事件／Meta／SysEx）同流程。
異本在 `PatchesUsed[189]`：
獨立版追蹤用過的音色與 Bank，
內嵌版整段沒有。
兩邊並存、改一邊另一邊不會跟，
對讀時先認清手上是哪一本。

### fmfix：雙向差

`fmfix` 內容物分屬 `api／fmopl2.c` 與
`example／test.exe`，
與 3.4a 差 56 行（diff 行數）。
偵測改嚴：
舊判式（開關開且基底埠讀回 0）之外，
加第二埠 `0x38A` 讀回 `0xFF`，
並附四行真值表註解
（`0x388＝0x06` 是 OPL2、
`0x388＝0x00` 且 `0x38A＝0xAA` 是 OPL2、
`0x388＝0x00` 且 `0x38A＝0xFF` 才是 OPL3）。
搶聲改簡：
`Rate` 計分、死碼特判、減三範圍一併拿掉，
改掃全部活躍聲部、挑通道號最大者犧牲。
一嚴一簡，淨效果是偵測修好、搶聲退步
（強推論，未經量測）——
所以 `fmfix` 不能直接蓋掉 3.4a，
要拿偵測修正得連搶聲一起退回去
（或自行把嚴偵測移植到 3.4a 版上，
兩處分屬不同函式、結構分離，
但移植未經編譯驗證）。

### 工具鏈：Watcom 平坦三件套

makefile 是 Watcom 式（`!ifeq`、`%watcom`、
`.symbolic` 全是 Watcom 記號）：
編譯 `wcc386p`（386 保護模式 C）、
組譯 `tasm`、建庫 `wlib`，
跑在 DOS／4GW 上（`DOS4G=QUIET` 關 banner）。
出貨旗 `-oxt -4r`（極致優化加暫存器呼叫）、
結構 `-zp1`（一位元組緊排，
讀檔直入 struct 的佈局基礎）、
`%version` 三檔（`debug／beta／production`，
分帶 `-dDEBUG／-dBETA／-dPROD`）。
`mixers.obj` 顯式相依 `clip.inc`
（改表重組，依賴寫死）；
`test.exe` 連的是上層 `..\lib\dmx.lib`
（編出的庫先拷到上層再連測試，
不是就地連）。

## 在執行檔裡怎麼認

本批無簽章檔（授權：不做位元組簽章）。
手認認五樣（例括號內）：
一，`MUS\x1A` 檔頭
（遊戲資料檔裡找此四位元組字串）；
二，`switch` 首字 `0／1／2／3` 的函式
（`FMT_*` enum 路由，`case` 四值連號，
例讀到 3 走 WAV 分支）；
三，`clip` 查表削波
（unrolled 重複的 `mov al,clip[eax]` 序列，
16 位轉 8 位，源碼用 `REPT` 產生、
執行檔內無 `REPT` 本體）；
四，`DMXGUS` 字串（GUS 驅動獨有，
`.INI` 配置名，
二進位裡見此字串即連了 GUS 驅動）；
五，`USE_OPL3` 開關讀取
（`DMXOPTION` 環境字串加 `-opl3`，
初始化段，例 `-opl3` 出現即 FM 走擴充路徑）。

## 證據、方法與未知

全為源碼層（R98 筆記加本篇核實），
範圍是 `dmx34a/api` 驅動檔、
`mixers／qsound／pztimer／realint`、
`midi2mus／midi2mus.c`、
`fmfix／api／fmopl2.c`、
`dmxsfx.h／dmxmus.h／dmxwav.h`、
`makefile／dmx.mif`、
測試程式 `playmus／playwav／test.c`；
只讀原始碼，無編譯實跑。
出貨／備份以 `dmx.mif` 為準。
此處「已證實」指源碼白紙黑字、
不是跑過，強推論另標。
行數為去 `\r` 後換行數。

未知：AWE32 的 EMU8000 層實作
（二進位牆，永久無答案；
DSP 層已有答案）；
`qsound` 與 `hpmixers` 的來歷與用途
（未進出貨，待查）；
`fmfix` 與 3.4a 的先後（強推論，
`$Log` 皆空）；
搶聲減三範圍的理由（源碼無註解）；
`codec` 路線為何被棄（源碼無說法）。