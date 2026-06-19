# PCMFlowDevice Specification

## 1. Scope

PCMFlowDevice contains device-specific helpers for PCMFlow applications.

In scope:

- Output-device queue and buffer lifetime helpers.
- Board-specific helpers that adapt PCM frames to hardware APIs.
- Application-side diagnostics for real hardware playback.

Out of scope:

- Network transport. Use PCMFlowUDP.
- Codec encode/decode. Use codec sibling libraries.
- PCM generation, conversion, WAV/MP3/FLAC, and generic ring buffers. Use PCMFlow.

## 2. Dependency Boundaries

`PCMFlowDevice.h` is generic and must not include M5Unified.

M5-specific APIs are exposed from `PCMFlowDeviceM5.h` and may include
M5Unified. The initial library release declares M5Unified as a package
dependency because M5 speaker playback is the first shipped helper.

## 3. M5SpeakerBufferedPlayer

`M5SpeakerBufferedPlayer<MaxPlayFrames, AudioBuffers>` is a header-only helper
for mono PCM16 playback through `M5.Speaker.playRaw()`.

### Responsibilities

- Own `AudioBuffers` rotating PCM buffers.
- Keep buffers alive after `playRaw()` returns.
- Collect incoming frames until the initial prebuffer target is reached.
- After playback starts, collect frames until the normal chunk target is reached.
- Retry `playRaw()` with `delay(1)` while it returns false.
- Expose application-side counters.

### Format

The initial implementation accepts:

- PCM16
- mono
- caller-specified sample rate

The initial implementation intentionally rejects stereo and non-PCM16 formats in `begin()`.

### Profiles

Profiles are expressed in milliseconds.

| Profile | Initial prebuffer | Chunk |
|---|---:|---:|
| Low latency | 40 ms | 20 ms |
| Balanced | 40 ms | 40 ms |
| Stable | 80 ms | 40 ms |

Applications may pass custom profiles, for example `{160, 80}`.

The template parameter `MaxPlayFrames` must be large enough for both the
initial prebuffer and normal chunk after conversion from milliseconds to frames.
If not, `begin()` returns false.

## 4. Data Paths

Two write paths are supported.

`writeFrames(const int16_t*, size_t)` copies caller-owned PCM frames into the
helper-owned rotating buffer. It is the normal path when decoded PCM already
exists in a separate buffer.

`writableData()` + `commitFrames()` lets a decoder write directly into the
current helper buffer. The caller must not write more than `writableFrames()`.

## 5. Counters

| Counter | Meaning |
|---|---|
| `chunks()` | Number of chunks accepted by `playRaw()` |
| `waits()` | Number of retries after `playRaw()` returned false |
| `gapRisks()` | Application-side late-submit risk count |
| `overflowDrops()` | Frames not accepted because helper storage was full |
| `fillFrames()` | Frames currently accumulated for the next chunk |
| `lastSubmitMs()` | `millis()` at the last successful chunk submit |

`gapRisks()` is not a hardware underrun counter. It compares the elapsed time
between successful chunk submits with the theoretical duration of the previous
chunk plus a small slack value.

## 6. Core2 Manual-Test Findings

Manual testing with M5Stack Core2 showed:

- 20 ms speaker chunks can produce audible gaps even when transport counters
  are clean.
- G.722 / L16 16 kHz mono is stable enough with the stable profile.
- G.711 / PCMU decodes to 8 kHz PCM, but Core2 speaker playback was more stable
  when PCM was upsampled to 16 kHz before `writeFrames()`.
- Opus 48 kHz can decode on Core2 but requires larger task stack and much
  larger playback buffering. Treat it as experimental on classic ESP32 boards.
