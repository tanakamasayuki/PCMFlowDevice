# PCMFlowDevice

Device I/O helpers for the PCMFlow family.

PCMFlowDevice keeps board-specific audio behavior out of transport and codec
libraries. The initial release provides `M5SpeakerBufferedPlayer`, a small
helper for M5Unified speaker playback.

## Why This Exists

`M5.Speaker.playRaw()` queues audio asynchronously. The PCM buffer passed to
`playRaw()` must remain alive after the call returns. Reusing stack buffers or
packet buffers too early can produce silence, corrupted audio, or short gaps
even when RTP/VBAN receive counters look clean.

`M5SpeakerBufferedPlayer` owns rotating PCM buffers, groups packet-sized writes
into speaker-sized chunks, handles initial prebuffering, retries when
`playRaw()` returns false, and exposes application-side diagnostics.

Transport stays in [PCMFlowUDP](https://github.com/tanakamasayuki/PCMFlowUDP).
Codec encode/decode stays in codec siblings such as PCMFlowG711, PCMFlowG722,
and PCMFlowOpus.

## Headers

```cpp
#include <PCMFlowDevice.h>    // generic profile helpers
#include <PCMFlowDeviceM5.h>  // M5Unified speaker helper
```

`PCMFlowDevice.h` does not include M5Unified. M5-specific APIs are exposed by
`PCMFlowDeviceM5.h`.

## Basic Use

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

## Profiles

| Profile | Initial prebuffer | Chunk | Notes |
|---|---:|---:|---|
| `lowLatencyProfile()` | 40 ms | 20 ms | For experiments where latency matters most |
| `balancedProfile()` | 40 ms | 40 ms | Default candidate for simple hardware playback |
| `stableProfile()` | 80 ms | 40 ms | Stability-oriented Core2/CoreS3 playback |

Custom profiles can be passed with aggregate initialization:

```cpp
player.begin({48000, 1, 16}, {160, 80});
```

## Diagnostics

`chunks()`, `waits()`, `gapRisks()`, `overflowDrops()`, `fillFrames()`, and
`lastSubmitMs()` are application-side observations. They are not M5Unified
internal underrun counters.

## Notes From Core2 Manual Testing

- RTP/G.722 at 16 kHz mono is a good fit with `stableProfile()`.
- RTP/PCMU / G.711 at 8 kHz was more stable on Core2 when decoded PCM was
  upsampled to 16 kHz before speaker playback.
- RTP/Opus at 48 kHz decodes on Core2, but it needs a larger loop stack and
  much larger playback buffering. Treat it as experimental on classic ESP32
  class devices.

## License

MIT. See [LICENSE](LICENSE).
