# PCMFlowDevice

[English](README.md)

PCMFlow ファミリー向けの device I/O helper ライブラリです。

PCMFlowDevice は、ボード固有の audio API の癖を transport / codec
ライブラリから分離します。初期版では M5Unified speaker 再生用の
`M5SpeakerBufferedPlayer` を提供します。

## なぜ必要か

`M5.Speaker.playRaw()` は非同期再生です。`playRaw()` に渡した PCM buffer は、
関数が return した後も有効でなければなりません。stack buffer や packet buffer
をすぐ再利用すると、RTP/VBAN の受信 counter が正常でも、無音・音の破損・短い
途切れが起きることがあります。

`M5SpeakerBufferedPlayer` は rotating PCM buffer を所有し、packet 単位の書き込みを
speaker 向け chunk にまとめ、初回 prebuffer、`playRaw()` retry、実機切り分け用の
application-side diagnostics を提供します。

Transport は [PCMFlowUDP](https://github.com/tanakamasayuki/PCMFlowUDP)、
codec encode/decode は PCMFlowG711 / PCMFlowG722 / PCMFlowOpus などの兄弟
ライブラリが担当します。

## ヘッダ

```cpp
#include <PCMFlowDevice.h>    // 汎用 profile helper
#include <PCMFlowDeviceM5.h>  // M5Unified speaker helper
```

`PCMFlowDevice.h` は M5Unified を include しません。M5 固有 API は
`PCMFlowDeviceM5.h` から公開します。

## 基本例

```cpp
#include <M5Unified.h>
#include <PCMFlowDeviceM5.h>

static constexpr uint32_t kSampleRate = 16000;
static constexpr size_t kMaxPlayFrames = (kSampleRate * 80u) / 1000u;

M5SpeakerBufferedPlayer<kMaxPlayFrames> player;

void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Speaker.begin();

    player.begin({kSampleRate, 1, 16},
                 M5SpeakerBufferedPlayer<kMaxPlayFrames>::stableProfile());
}

void onPcmFrames(const int16_t *pcm, size_t frames)
{
    player.writeFrames(pcm, frames);
}
```

## Profile

| Profile | 初回 prebuffer | 通常 chunk | 用途 |
|---|---:|---:|---|
| `lowLatencyProfile()` | 40 ms | 20 ms | 低遅延寄りの実験用 |
| `balancedProfile()` | 40 ms | 40 ms | 単純な実機 speaker 再生の default 候補 |
| `stableProfile()` | 80 ms | 40 ms | Core2/CoreS3 の安定寄り再生 |

独自 profile は aggregate 初期化で渡せます。

```cpp
player.begin({48000, 1, 16}, {160, 80});
```

## Diagnostics

`chunks()`、`waits()`、`gapRisks()`、`overflowDrops()`、`fillFrames()`、
`lastSubmitMs()` は application-side の観測値です。M5Unified 内部の真の underrun
counter ではありません。

## Core2 手動テストからの知見

- RTP/G.722 16 kHz mono は `stableProfile()` で採用候補。
- RTP/PCMU / G.711 8 kHz は、decode 後に 16 kHz へ upsample して speaker へ渡す方が
  Core2 では安定しました。
- RTP/Opus 48 kHz は Core2 で decode 可能ですが、loop stack と大きな playback
  buffer が必要です。classic ESP32 class の実機では experimental 扱いを推奨します。

## License

MIT。[LICENSE](LICENSE) 参照。
