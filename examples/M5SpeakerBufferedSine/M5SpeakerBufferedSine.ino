#include <math.h>

#include <M5Unified.h>
#include <PCMFlowDeviceM5.h>

static constexpr uint32_t kSampleRate = 16000;
static constexpr uint32_t kToneHz = 1000;
static constexpr size_t kBlockFrames = (kSampleRate * 20u) / 1000u;
static constexpr size_t kMaxPlayFrames = (kSampleRate * 80u) / 1000u;

M5SpeakerBufferedPlayer<kMaxPlayFrames> g_player;
static int16_t g_block[kBlockFrames] = {};
static uint32_t g_phase = 0;
static unsigned long g_lastStatsMs = 0;

static void fillSine()
{
    for (size_t i = 0; i < kBlockFrames; ++i)
    {
        const float t = static_cast<float>(g_phase) / static_cast<float>(kSampleRate);
        g_block[i] = static_cast<int16_t>(sinf(2.0f * PI * kToneHz * t) * 6000.0f);
        ++g_phase;
        if (g_phase >= kSampleRate)
            g_phase = 0;
    }
}

void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);
    Serial.begin(115200);

    M5.Display.setTextSize(2);
    M5.Display.setCursor(0, 0);
    M5.Display.println("PCMFlowDevice");
    M5.Display.println("Buffered sine");

    M5.Speaker.begin();
    M5.Speaker.setVolume(128);

    if (!g_player.begin({kSampleRate, 1, 16},
                        M5SpeakerBufferedPlayer<kMaxPlayFrames>::stableProfile()))
    {
        Serial.println("FAIL player-begin");
        M5.Display.println("Player failed");
        return;
    }

    Serial.print("PCMFlowDevice ");
    Serial.print(PCMFLOWDEVICE_VERSION_STR);
    Serial.println(" M5SpeakerBufferedSine ready");
}

void loop()
{
    M5.update();
    fillSine();
    g_player.writeFrames(g_block, kBlockFrames);

    const unsigned long now = millis();
    if (now - g_lastStatsMs >= 1000)
    {
        g_lastStatsMs = now;
        Serial.print("SINE chunks=");
        Serial.print(g_player.chunks());
        Serial.print(" waits=");
        Serial.print(g_player.waits());
        Serial.print(" gaps=");
        Serial.print(g_player.gapRisks());
        Serial.print(" drops=");
        Serial.println(g_player.overflowDrops());
    }
}
