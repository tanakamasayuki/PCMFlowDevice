# PCMFlowDevice Design Brief

[日本語](device_library_brief.ja.md)

PCMFlowDevice exists to keep device-specific audio behavior out of transport
and codec libraries.

## Boundary

- PCMFlow: generic PCM abstractions and processing.
- PCMFlowUDP: UDP transports such as RAW, VBAN, and RTP.
- PCMFlowG711 / PCMFlowG722 / PCMFlowOpus: codec encode/decode.
- PCMFlowDevice: hardware API helpers such as M5Unified speaker playback.

The first shipped helper is `M5SpeakerBufferedPlayer`.

## Initial Scope

The initial release focuses on M5Unified speaker playback because
`M5.Speaker.playRaw()` queues audio asynchronously and requires the caller to
keep PCM buffers alive after the function returns.

The helper owns rotating buffers and exposes prebuffer/chunk profiles. It does
not own network sockets, RTP/VBAN parsing, or codec decode state.

## Core2 Findings

Manual testing from PCMFlowUDP showed:

- 16 kHz G.722 / L16 playback is stable enough with 80/40 ms buffering.
- 8 kHz G.711 playback is more stable when upsampled to 16 kHz before speaker
  output.
- 48 kHz Opus playback on classic ESP32 class boards is experimental. It needs
  larger loop stack and playback buffering, and DRAM limits prevent adding much
  more margin.

## Future Work

- M5 microphone source helper.
- Generic I2S helpers.
- Optional PCMSource integration helpers such as `pumpFrom(PCMSource&)`.
