---
id: dmx/api-core
title: DMX 的 API 核心：四播放器、兩張表與一個軟體時鐘
libraries: [dmx]
goals: [craft, re]
evidence: 已證實
triggers:
  - 要 remake 用 DMX 的遊戲，想知道 DMX_Init 到底配了什麼、順序是什麼
  - SFX handle 的高字是什麼，想知道音效播到一半怎麼路由
  - 遊戲播 MIDI 檔卻沒看到轉換工具，想知道轉換藏在哪
  - DOS 沒有執行緒，想知道多首歌加淡入淡出怎麼做排程
symbols: [DMX_Init, DMX_DeInit, TSM_Install, TSM_Remove, MUS_RegisterSong, MUS_ChainSong, SFX_PlayPatch, TSM_NewService, DMX_MUS, DMX_WAV, HIWORD, FMT_PCS_WAVE, FMT_GSS_WAVE, FMT_GSS_PATCH, FMT_WAV_SAMPLE, Midi2Mus, ServicePlayer, ServiceFader, TraceQue]
related: [dmx/version-history, dsmi/interface-split]
---

# DMX 的 API 核心：四播放器、兩張表與一個軟體時鐘

一句話：DMX 3.4a（Digital Expressions 的 DOS 音效函式庫，
遊戲連結它來播音樂與音效；來歷見前篇）的 `DMX_Init`
按固定順序探測七種音效硬體
（MUS／WAV 後配不覆先配，GSS 有破口見後），
配出四個播放器
（歌曲、GSS 音色——GSS 是音色資料格式，
全稱兩處寫法分歧見後——取樣、PC 喇叭）
與兩張函式指標表；
音效 handle 的高字回填格式，後續操作全按此路由；
歌曲層有雙槽加鏈結（播完接下一首）加淡入淡出，
內嵌 MIDI→MUS 轉換器，
排程靠自寫 TSM（Timer Service Manager）軟體定時多工器
（DOS 無執行緒，以 tick 回呼模擬多工）。

## 結論

初始化是「讀環境、逐裝置配對、回硬體旗」三步：
`DMXOPTION` 環境字串開 `-opl3／-phase` 選項
（前者 FM 合成相關，後者取樣混音層讀此旗；
細節屬驅動層），
按 `AHW_*` 裝置旗（`AHW_` 即 audio hardware，
各裝置佔一位元，另有全選 `AHW_ANY`）
逐一調各驅動 `Init()`，
`MUS_Init／GSS_Init／WAV_Init` 把命中的驅動表配給播放器
（`fMus→MUS／fSfx→GSS／fWav→WAV` 三旗防重配），
最後回 `HardWare` 旗（`INT`，原文大寫 W；
位元意義同 `AHW_*`）。
探測順序固定：GUS（Gravis Ultrasound 取樣卡）→
MediaVision（MV，取樣卡）→AWE32（自帶音色卡）→
SoundBlaster（SB，取樣卡）→MPU-401（MIDI 介面）→
AdLib（FM 合成卡）→PC 喇叭（PCSPK）——
先命中的卡住位置；
MUS／WAV 由旗保證不覆蓋，
GSS 在 AWE32 先命中時會被後配覆蓋
（AWE32 分支調 `GSS_Init` 卻不設 `fSfx`，
後續分支再次配對覆寫驅動指標——
疑似原廠 bug，強推論）。

四播放器分兩種抽象。
`MUS_` 管歌曲（吃 `DMX_MUS` 表），
`GSS_` 管 GSS 音色（借同一張 `DMX_MUS` 表——
`MUS` 表的動詞恰是送音符事件需要的動詞，
同一驅動實例服務兩種上層而不衝突：
兩邊都只是排隊送事件，無獨佔狀態），
`WAV_` 管取樣（吃 `DMX_WAV` 表），
`PCS_` 直調 PC 喇叭（無表：
喇叭靠開關頻率發聲，由 140Hz TSM 服務逐 tick 餵，
SFX 層直調即可）。
音色與取樣之別：取樣是 PCM 波形（錄好的聲音），
音色是合成器參數（現場算出聲音）；
歌曲則是指令序列（幾時發哪個音）。
`SFX_PlayPatch` 看 patch（一塊音效資料，
首字是格式碼）首字 `FMT_*` 分派——
四值（`PCS_WAVE／GSS_WAVE／GSS_PATCH／WAV_SAMPLE`）
收斂三家（GSS 佔兩值），
handle 高字回填格式
（`HIWORD` 是自寫巨集，取 32 位元整數高 16 位；
低字才是索引，高字閒置順手存標籤——
省查表結構，代價是 handle 變魔法數字；
後續查播、設位、停播全按此分派，
唯 `SetOrigin` 對 PCS 無動作、GSS 版丟音高）。
兩張表是裝置抽象層：
`DMX_MUS`（`Init／DeInit／NoteOn／NoteOff／
PitchBend／Command／Enter／Leave`）
與 `DMX_WAV`（`Init／DeInit／PlayMode／
StopMode／Callback`），
各驅動填一張即插即用。

歌曲層是雙槽狀態機：
`SongStatus[2]`（動作旗、現播歌、鏈結歌），
雙槽對稱、無現用／備用之分：
交接是殺舊翻槽（`MUS_Index ^= 1`）、先殺後接，
舊首播完或淡到零才殺、鏈發新首，無疊播；
兩槽就夠：同時只需要現播加下一首，
`Song[]` 註冊表（`max_songs` 上限，
handle 從 1 起算，`ChainTo=0` 表無鏈結，
`ChainSong(h,-1)` 清鏈結），
`RegisterSong／UnregisterSong／PlaySong／StopSong／
PauseSong／ResumeSong／ChainSong／FadeToNextSong／
FadeInSong／FadeOutSong／RampVolume／QrySongPlaying／
GetMasterVolume／SetMasterVolume` 全家。
鏈結分兩種：自鏈（鏈到自己）原槽重置重播
即單曲循環，跨鏈殺舊歌翻槽接播；
淡入淡出是 `ServiceFader`（音量漸變服務，
50Hz 到點跑，向 TSM 註冊）：
逐 tick 趨動各活動歌曲音量向目標靠攏，
與主音量取最小後下驅動；
播歌主迴路是另一服務 `ServicePlayer`
（播歌加鏈結切換）。
`Midi2Mus` 內嵌轉換器吃標準 MIDI 生 `MUS`：
塊解析、多軌按 delta-time 併成一條時間線、
`MidiEvent／Meta／SysEx` 三處理、
寫記憶體緩衝（回傳 `MUS` 指標，不碰磁碟），
全程 `setjmp／longjmp` 錯誤出口。
除錯兩件套：非出貨（`PROD` 是出貨版編譯旗）版
以 VGA 邊框色標執行區間——
無除錯器時最便宜的示波器
（`#ifndef PROD` 加執行期 `DmxDebug` 旗雙重開關），
64K 線性追蹤緩衝（滿則 `Q-OVERFLOW` 標記；
`DMXTRACE` 環境開追蹤，`TOFILE` 位元
決定落 `DMX.LOG` 還是走螢幕）落檔。
（`TraceQue` 少 e 是原文拼法。）

TSM 是軟體定時多工器：
`Service[]` 加 `ServiceOrder[]` 優先佇列
（按優先級與頻率排序插入），
主 tick 計數、分頻商餘累加器
（例每 3.5 tick 跑一次：
每 tick 累加 10，除以 35 取商跑、餘留輪——
整數累加做出分數頻率）、
`volatile` 存取鎖宣告
（中斷與主程式共享變數，禁編譯器優化；
但源碼內無上鎖協議，疑殘留）；
`TSM_CorrectTime` 從 RTC（電池時鐘，
`int 1Ah AH=2`）讀回 BCD 時間、
`bcd2bin` 解碼、換算 tick 寫 `0x46C` 系統時鐘
（記憶體裡的時間變數；
PIT 是可程式 tick 產生器，
重設它後系統時鐘漂移故需校時；
由 `TSM_Remove` 調，測試程式 `atexit` 登記）；
啟停服務都關中斷保護臨界區後改序。
`TSM_Install` 由呼叫端在 `DMX_Init` 之前調
（頻率不得低於 18Hz；
未安裝則建服務回 -1，`MUS_Init` 失敗）——
時鐘誰裝誰拆，`DMX_Init` 不管。

## 根本問題：沒有音效 API，也沒有執行緒

1993 年的 DOS 兩樣都沒有。
遊戲要出聲，得自己認七種卡、自己餵三種資料
（歌曲、音色、取樣），還得在只有一個硬體時鐘的機器上
同時做播放、淡入淡出、鏈結。
DMX 的回答是三層：
裝置抽象（兩張表把七種卡收斂成兩種動詞）、
路由（handle 高字讓上層不用記音效走哪條路）、
排程（TSM 把「每 N tick 做一件事」
變成可註冊的服務）。
每層都只用 C 加一點組語，不依賴作業系統。

## 推導

### 初始化：讀環境、配對、回旗

`DMX_Init` 取四參：
tick 速率（歌曲量化率）、最大歌數、
音樂裝置旗、音效裝置旗。
先讀 `DMXOPTION` 環境字串：
出現 `-opl3` 開 OPL3（FM 合成晶片，
FM 驅動見此旗才走擴充路徑）、
`-phase` 開相位選項（取樣混音層讀此旗）——
選項走環境不走參數，
同一支執行檔換環境即換行為。
再按固定順序逐裝置試 `Init()`：
命中且該層還沒配才配對——
mus 層看 `fMus`、sfx 層看 `fSfx`、wav 看 `fWav`，
同一裝置的兩層各自獨立判斷；
「後配不覆先配」是三個旗保證的，
不是順序本身保證的：
例 SB 先命中配了 `fMus`，
後面 MPU 再命中時見 `fMus` 已設而跳過；
若無此旗，後配將覆寫驅動指標、
先配的卡直接失聯。
音樂與音效可配不同卡
（兩組裝置旗各自獨立），
回傳的 `HardWare` 旗告訴呼叫端配中了誰。
例外：AWE32 分支不設 `fSfx`
（破口見結論）。

### 四播放器與兩張表

播放器是上層概念，表是下層契約。
`MUS_` 播歌曲：註冊、播、停、暫停、鏈結、淡變，
全家十五六個函式，全吃 `DMX_MUS` 表。
`GSS_` 播 GSS 音色：`GSS_WAVE／GSS_PATCH` 雙格式，
全域播放狀態機逐 tick 餵同一張 `DMX_MUS` 表——
一表兩吃，驅動不用知道上層是誰。
（GSS 全稱兩處寫法並存：
檔頭寫 General Sound Set、
格式註解寫 General Sound System，
以何者為準待查。）
`WAV_` 播取樣：`WAV_PlayMode` 定聲道數與取樣率
（簽名寫聲道 1..4，
測試程式實際傳 8——註解與實用分歧，
位元深度在此 API 根本不出現；
測試用 22050Hz）、
載 patch、播，吃 `DMX_WAV` 表。
`PCS_` 播 PC 喇叭：直調，無表——
喇叭只有開／關（靠 128 元頻率表查頻、
寫 timer2 定方波頻率發聲），
由 140Hz TSM 服務逐 tick 餵，
SFX 層直調即可。
GSS 與 PCS 皆單音：同時只播一個，
新播覆蓋舊播（PCS 另有優先級搶佔門：
優先級不夠高直接拒播）。
圖為結構主線
（Midi2Mus 內轉與 FCB 磁碟層略，見正文）：

<p align="center"><img src="../../img/dmx-api-core.svg" width="640" alt="DMX API 結構主線：配對、分派、播放器、驅動表，TSM 管時間"></p>

### 路由：handle 高字

`SFX_PlayPatch` 取 patch、音高、`x`、
音量等六參
（`x` 是左右平衡，註解明寫；
測試程式稱之 `panpot`），
看 patch 首字 `FMT_*` 分派
（四值收斂三家，見結論）。
回傳的 handle 高字回填格式：
後續查播、設發聲位、停播全 `switch (HIWORD(handle))`——
呼叫端只存一個整數，路由資訊藏在裡面。
機制說破：實際索引只需要低 16 位，
高 16 位閒置，順手拿來存格式標籤——
省了查表結構，代價是 handle 變魔法數字。
邊界：`SetOrigin` 對 PCS 無動作
（`switch` 裡無此 case，靜默跳過），
GSS 版只收 `x` 與音量、丟音高，
`flags` 只進 WAV 家。

### 歌曲層：雙槽、鏈結、淡變

同時只播兩首的概念槽：
`SongStatus[2]` 記動作旗、現播歌、鏈結歌，
兩槽對稱、無主副之分——
鏈結目標可預載另一槽待命，
A 播完或淡到零：殺舊、翻槽（`^= 1`）、
有預載直接起播、無預載現場 `StartSong`；
先殺後接，無疊播。
`Song[]` 註冊表按 `max_songs` 上限收歌，
`RegisterSong` 收、`UnregisterSong` 退。
淡入淡出是 `ServiceFader` 服務
（50Hz 到點跑）：
每輪讀各活動歌曲現音量與目標、
趨動靠攏、與主音量取最小下驅動；
`Enter／Leave` 是驅動表的進出函式
（進驅動工作區前調 `Enter`、離開調 `Leave`），
主音量變更偵測是因主音量由呼叫端隨時改、
fader 每輪比對才跟得上——
淡變不是驅動功能，是排程出來的。
播歌主迴路是 `ServicePlayer`
（播歌加鏈結切換，優先級 10 高於 fader 的 9）。
`Midi2Mus` 藏在同一檔：
呼叫端先載檔、餵記憶體指標，
`MThd` 開頭才轉內部 `MUS` 格式再播——
呼叫端看不到轉換（另有獨立工具版，
見前篇；格式定位見前篇，
檔頭 `MUS\x1A` 位元組證據在本篇範圍）。

### 排程：TSM

TSM 只做一件事：到點調函式。
`Service[]` 登記（函式、優先級、頻率），
`ServiceOrder[]` 按序排，
主 tick 到時走一遍，夠點的調。
分頻用商餘累加器（整數除法的餘數留到下輪），
改序關中斷保護臨界區後改
（`PUSH_DISABLE` 存旗關中斷），
存取鎖 `volatile` 宣告
（中斷與主程式共享，禁編譯器優化；
源碼內無上鎖協議，疑殘留）。
`TSM_CorrectTime` 處理時鐘漂移：
重設 PIT 後系統時鐘不准，
從 RTC 讀回 BCD 時間、`bcd2bin` 解碼、
換算 tick 寫回 `0x46C`——
`__386__` 與否兩套寫法
（32 位元與 16 位元呼叫 BIOS 中斷的方式不同，
故 `int386` 軟體中斷包裝或內嵌組語二選一），
照編譯目標切；
由 `TSM_Remove` 調（測試程式 `atexit` 登記）。

### 磁碟與測試序列

`dmxdisk.c` 是單塊快取讀：
`FCB`（自訂檔案控制塊：
快取旗、索引、大小三欄）包一塊緩衝，
大讀直通、小讀先填快取——
本體仍是 `_dos_open／read／close` 封裝，
不是不用系統呼叫；
樹內唯一源碼呼叫端是範圍外的 GUS 驅動
（載 patch 用），
播歌註冊吃記憶體指標，根本不走它
（測試程式讀檔用 `open／read` 直讀）。
測試程式給出最小呼叫序列
（時鐘先裝：兩支測試程式都在 `DMX_Init` 之前
調 `TSM_Install`，否則建服務失敗）：

```text
TSM_Install → DMX_Init → MUS_RegisterSong → MUS_ChainSong(h,h)
  → MUS_PlaySong → … → MUS_StopSong
  → MUS_UnregisterSong → DMX_DeInit → TSM_Remove
WAV_PlayMode → SFX_PlayPatch → … → DMX_DeInit → TSM_Remove
```

`ChainSong(h,h)` 自鏈即單曲循環：
播完接「下一首」，下一首是自己，
故無限重播。

## 在執行檔裡怎麼認

本批無簽章檔（授權：不做位元組簽章）。
手認只認三樣：
一，`DMXOPTION` 環境字串與 `-opl3／-phase`
（初始化讀它的形狀少見）；
二，`HIWORD(handle)` 路由
（連續多個 `switch` 高字的函式，
`case` 常數四值收斂三家）；
三，TSM 服務表
（優先佇列加商餘分頻加 `volatile` 宣告的組合）。
遊戲資料檔裡的 `MUS` 格式是另一條線索：
格式定位（精簡事件流）見前篇，
檔頭 `MUS\x1A` 四位元組證據在本篇範圍
（轉換器寫檔頭處）。

## 證據、方法與未知

全為源碼層（R97 筆記加本篇核實），
範圍是 `dmx34a/api` 八檔
（六核心加 `wavapi／pcsapi`，因正文斷言兩者行為）
加兩支測試程式；
只讀原始碼，無編譯實跑。
函式名與結構名以源碼為準
（`MUS_RegisterSong／ChainSong／PlaySong` 等全名，
筆記縮寫已在本篇展開）。
此處「已證實」指源碼白紙黑字，
非編譯對拍（授權只允許源碼閱讀）。

未知：各驅動 `Init()` 的實際探測行為
（無硬體不驗，見 R98 裝置篇）；
TSM 在真實 tick 速率下的抖動；
GSS 全稱以 Set／System 何者為準；
AWE32 分支不設 `fSfx` 是 bug 還是另有後手
（強推論為 bug，待裝置篇對讀）。
