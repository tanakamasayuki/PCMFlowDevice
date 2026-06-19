# PCMFlowDevice 仕様

## 1. スコープ

PCMFlowDevice は PCMFlow application 向けの device 固有 helper を提供します。

対象:

- 出力 device の queue と buffer lifetime 管理。
- PCM frame を hardware API へ渡す board-specific helper。
- 実機再生の application-side diagnostics。

対象外:

- network transport。PCMFlowUDP を使います。
- codec encode/decode。codec 兄弟ライブラリを使います。
- PCM 生成、変換、WAV/MP3/FLAC、汎用 ring buffer。PCMFlow を使います。

## 2. 依存境界

`PCMFlowDevice.h` は汎用ヘッダで、M5Unified を include しません。

M5 固有 API は `PCMFlowDeviceM5.h` から公開し、ここでは M5Unified に依存します。
初期リリースでは M5 speaker helper を提供対象にするため、パッケージ依存として
M5Unified を宣言します。

## 3. M5SpeakerBufferedPlayer

`M5SpeakerBufferedPlayer<MaxPlayFrames, AudioBuffers>` は
`M5.Speaker.playRaw()` 用の mono PCM16 playback helper です。header-only です。

### 責務

- `AudioBuffers` 個の rotating PCM buffer を所有する。
- `playRaw()` return 後も buffer lifetime を保証する。
- 初回 prebuffer 分たまるまで再生開始しない。
- 再生開始後は通常 chunk 分たまったら投入する。
- `playRaw()` が false の間、`delay(1)` 付きで retry する。
- application-side counter を公開する。

### Format

初期実装は以下だけを受け付けます。

- PCM16
- mono
- caller-specified sample rate

stereo と non-PCM16 は `begin()` で拒否します。

### Profile

Profile は milliseconds で表します。

| Profile | 初回 prebuffer | 通常 chunk |
|---|---:|---:|
| Low latency | 40 ms | 20 ms |
| Balanced | 40 ms | 40 ms |
| Stable | 80 ms | 40 ms |

`{160, 80}` のような custom profile も渡せます。

template parameter の `MaxPlayFrames` は、ms から frame に換算した初回 prebuffer と
通常 chunk の両方を保持できる必要があります。不足する場合、`begin()` は false を返します。

## 4. Data Path

2 つの書き込み経路があります。

`writeFrames(const int16_t*, size_t)` は caller-owned PCM frames を helper-owned
rotating buffer へコピーします。decode 後の PCM が別 buffer にある場合の通常経路です。

`writableData()` + `commitFrames()` は decoder が helper の current buffer に直接
書くための経路です。caller は `writableFrames()` を超えて書いてはいけません。

## 5. Counter

| Counter | 意味 |
|---|---|
| `chunks()` | `playRaw()` が受け付けた chunk 数 |
| `waits()` | `playRaw()` false 後の retry 回数 |
| `gapRisks()` | application-side の late-submit risk 数 |
| `overflowDrops()` | helper storage 不足で受け付けなかった frames |
| `fillFrames()` | 次 chunk として蓄積済みの frames |
| `lastSubmitMs()` | 最後に chunk 投入に成功した `millis()` |

`gapRisks()` は hardware underrun counter ではありません。前回 chunk の理論再生時間に
小さな slack を足した時間より、次の投入が遅れた回数です。

## 6. Core2 手動テストの知見

M5Stack Core2 での手動テストでは以下が見えています。

- 20 ms speaker chunk は、transport counter が正常でも聴感上の gap が出る場合がある。
- G.722 / L16 16 kHz mono は stable profile で採用候補。
- G.711 / PCMU は 8 kHz PCM に decode されるが、Core2 speaker では 16 kHz へ
  upsample してから `writeFrames()` する方が安定した。
- Opus 48 kHz は Core2 で decode 可能だが、大きめの task stack と playback buffer が
  必要。classic ESP32 board では experimental 扱いを推奨する。
