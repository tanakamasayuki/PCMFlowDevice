#ifndef PCMFLOWDEVICE_M5SPEAKERBUFFEREDPLAYER_H
#define PCMFLOWDEVICE_M5SPEAKERBUFFEREDPLAYER_H

#include <Arduino.h>
#include <M5Unified.h>
#include <PCMFormat.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "PCMFlowDevice.h"

// M5Unified's Speaker.playRaw() queues audio asynchronously, so callers must
// keep the PCM buffer alive after playRaw() returns. This helper owns a small
// rotating buffer set, groups packet-sized writes into speaker-sized chunks,
// and exposes application-side counters useful for tuning.
template <size_t MaxPlayFrames, size_t AudioBuffers = 3>
class M5SpeakerBufferedPlayer
{
public:
    using Profile = pcmflowdevice::PlaybackProfile;

    static Profile lowLatencyProfile() { return pcmflowdevice::lowLatencyProfile(); }
    static Profile balancedProfile() { return pcmflowdevice::balancedProfile(); }
    static Profile stableProfile() { return pcmflowdevice::stableProfile(); }

    bool begin(const PCMFormat &format, Profile profile = balancedProfile())
    {
        if (!format.isValid() || format.bitsPerSample != 16 || format.channels != 1)
        {
            ready_ = false;
            return false;
        }

        format_ = format;
        initialFrames_ = framesForMs(profile.initialPrebufferMs);
        chunkFrames_ = framesForMs(profile.chunkMs);
        if (initialFrames_ == 0 || chunkFrames_ == 0 ||
            initialFrames_ > MaxPlayFrames || chunkFrames_ > MaxPlayFrames)
        {
            ready_ = false;
            return false;
        }

        reset();
        ready_ = true;
        return true;
    }

    void reset()
    {
        M5.Speaker.stop();
        fillFrames_ = 0;
        audioIndex_ = 0;
        started_ = false;
        chunks_ = 0;
        waits_ = 0;
        gapRisks_ = 0;
        overflowDrops_ = 0;
        lastSubmitMs_ = 0;
        lastDurationMs_ = 0;
    }

    void stop()
    {
        M5.Speaker.stop();
        fillFrames_ = 0;
        started_ = false;
    }

    int16_t *writableData()
    {
        return audio_[audioIndex_] + fillFrames_;
    }

    size_t writableFrames() const
    {
        const size_t target = targetFrames();
        return (target > fillFrames_) ? (target - fillFrames_) : 0;
    }

    size_t commitFrames(size_t frameCount)
    {
        if (!ready_ || frameCount == 0)
            return 0;

        const size_t room = writableFrames();
        const size_t accepted = (frameCount < room) ? frameCount : room;
        if (accepted < frameCount)
            ++overflowDrops_;

        fillFrames_ += accepted;
        if (fillFrames_ >= targetFrames())
            submitAudio(fillFrames_);
        return accepted;
    }

    size_t writeFrames(const int16_t *frames, size_t frameCount)
    {
        if (!ready_ || frames == nullptr || frameCount == 0)
            return 0;

        size_t written = 0;
        while (written < frameCount)
        {
            const size_t room = writableFrames();
            if (room == 0)
            {
                ++overflowDrops_;
                break;
            }
            size_t take = frameCount - written;
            if (take > room)
                take = room;
            memcpy(writableData(), frames + written, take * sizeof(int16_t));
            commitFrames(take);
            written += take;
        }
        return written;
    }

    bool flush()
    {
        if (!ready_ || fillFrames_ == 0)
            return false;
        submitAudio(fillFrames_);
        return true;
    }

    bool isReady() const { return ready_; }
    bool hasStarted() const { return started_; }
    size_t fillFrames() const { return fillFrames_; }
    size_t targetFrames() const { return started_ ? chunkFrames_ : initialFrames_; }
    size_t initialFrames() const { return initialFrames_; }
    size_t chunkFrames() const { return chunkFrames_; }
    uint32_t chunks() const { return chunks_; }
    uint32_t waits() const { return waits_; }
    uint32_t gapRisks() const { return gapRisks_; }
    uint32_t overflowDrops() const { return overflowDrops_; }
    uint32_t drops() const { return overflowDrops_; }
    unsigned long lastSubmitMs() const { return lastSubmitMs_; }

private:
    size_t framesForMs(uint16_t ms) const
    {
        return (static_cast<size_t>(format_.sampleRate) * ms) / 1000u;
    }

    void submitAudio(size_t frames)
    {
        const unsigned long now = millis();
        if (started_ && lastSubmitMs_ != 0 &&
            now - lastSubmitMs_ > lastDurationMs_ + kGapSlackMs)
        {
            ++gapRisks_;
        }

        while (!M5.Speaker.playRaw(audio_[audioIndex_], frames,
                                   format_.sampleRate, false,
                                   format_.channels, 0, false))
        {
            if (started_)
                ++waits_;
            delay(1);
        }

        lastSubmitMs_ = millis();
        lastDurationMs_ = static_cast<uint32_t>((frames * 1000u) / format_.sampleRate);
        ++chunks_;
        audioIndex_ = (audioIndex_ + 1) % AudioBuffers;
        fillFrames_ = 0;
        started_ = true;
    }

    static constexpr uint32_t kGapSlackMs = 5;

    PCMFormat format_{};
    bool ready_ = false;
    bool started_ = false;
    int16_t audio_[AudioBuffers][MaxPlayFrames] = {};
    size_t audioIndex_ = 0;
    size_t fillFrames_ = 0;
    size_t initialFrames_ = 0;
    size_t chunkFrames_ = 0;
    uint32_t chunks_ = 0;
    uint32_t waits_ = 0;
    uint32_t gapRisks_ = 0;
    uint32_t overflowDrops_ = 0;
    unsigned long lastSubmitMs_ = 0;
    uint32_t lastDurationMs_ = 0;
};

#endif // PCMFLOWDEVICE_M5SPEAKERBUFFEREDPLAYER_H
