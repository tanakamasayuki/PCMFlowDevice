# PCMFlowDevice 設計メモ

[English](device_library_brief.md)

PCMFlowDevice は、device 固有の audio 挙動を transport / codec ライブラリから分離するためのライブラリです。

## 境界

- PCMFlow: 汎用 PCM 抽象と PCM 処理。
- PCMFlowUDP: RAW / VBAN / RTP などの UDP transport。
- PCMFlowG711 / PCMFlowG722 / PCMFlowOpus: codec encode/decode。
- PCMFlowDevice: M5Unified speaker 再生などの hardware API helper。

最初に提供する helper は `M5SpeakerBufferedPlayer` です。

## 初期スコープ

初期版は M5Unified speaker 再生に絞ります。`M5.Speaker.playRaw()` は非同期 queue に audio を投入する API であり、呼び出し側は関数 return 後も PCM buffer を有効に保つ必要があるためです。

helper は rotating buffer を所有し、prebuffer/chunk profile を提供します。network socket、RTP/VBAN parse、codec decode state は持ちません。

## Core2 での知見

PCMFlowUDP の手動テストでは以下が見えています。

- 16 kHz G.722 / L16 再生は 80/40 ms buffer で十分安定する候補。
- 8 kHz G.711 再生は、speaker 出力前に 16 kHz へ upsample した方が安定する。
- 48 kHz Opus 再生は classic ESP32 class board では experimental。大きめの loop stack と playback buffer が必要で、DRAM 制約により追加できる margin は小さい。

## 今後

- M5 microphone source helper。
- 汎用 I2S helper。
- `pumpFrom(PCMSource&)` のような optional PCMSource 統合 helper。
