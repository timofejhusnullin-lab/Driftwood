#pragma once
#include "DSP.h"

namespace dw
{
// Lo-fi: sample-rate reduction + bit reduction
struct Crusher
{
    float holdL = 0, holdR = 0, counter = 0;
    void process (float& l, float& r, float amt)
    {
        if (amt <= 0.001f) return;
        const float step = 1.0f + amt * amt * 24.0f;
        if ((counter += 1.0f) >= step) { counter -= step; holdL = l; holdR = r; }
        const float levels = std::exp2 (12.0f - amt * 7.5f);
        l = std::round (holdL * levels) / levels;
        r = std::round (holdR * levels) / levels;
    }
};

// Juno-style BBD chorus (modes I, II, I+II)
struct Chorus
{
    DelayLine dl; float sr = 44100, phase = 0; OnePole tone;
    void prepare (double s) { sr = (float) s; dl.prepare ((int) (s * 0.03)); tone.setCutoff (7500.0f, sr); phase = 0; }
    void clear() { dl.clear(); }
    void process (float& l, float& r, int mode)
    {
        if (mode == 0) return;
        float rate, depthMs, centreMs;
        switch (mode)
        {
            case 1:  rate = 0.513f; depthMs = 1.85f; centreMs = 3.5f; break;
            case 2:  rate = 0.863f; depthMs = 1.85f; centreMs = 3.5f; break;
            default: rate = 9.75f;  depthMs = 0.25f; centreMs = 3.2f; break;
        }
        phase += rate / sr; if (phase >= 1.0f) phase -= 1.0f;
        const float tri = 4.0f * std::abs (phase - 0.5f) - 1.0f;
        const float mono = tone.lp (0.5f * (l + r));
        dl.push (softClip (mono * 1.1f) * 0.91f);
        const float msToS = sr * 0.001f;
        const float wetL = dl.read ((centreMs + depthMs * tri) * msToS);
        const float wetR = dl.read ((centreMs - depthMs * tri) * msToS);
        l = 0.72f * l + 0.72f * wetL;
        r = 0.72f * r + 0.72f * wetR;
    }
};

// Tape: wow & flutter pitch wobble, saturation, "age" (dull top + hiss)
struct Tape
{
    DelayLine dlL, dlR; float sr = 44100, wowPh = 0, flPh = 0; Drift wander; Rng rng;
    OnePole lpL, lpR, hsL, hsR; float lastAge = -1;
    void prepare (double s)
    {
        sr = (float) s; dlL.prepare ((int) (s * 0.05)); dlR.prepare ((int) (s * 0.05));
        wander.prepare (s, 99u); rng.seed (2024u);
        lastAge = -1;
    }
    void clear() { dlL.clear(); dlR.clear(); }
    void process (float& l, float& r, float wow, float sat, float age)
    {
        if (age != lastAge) { lpL.setCutoff (18000.0f * std::exp2 (-age * 2.6f), sr); lpR.a = lpL.a; hsL.setCutoff (30.0f + age * 120.0f, sr); hsR.a = hsL.a; lastAge = age; }

        if (sat > 0.001f)
        {
            const float d = 1.0f + sat * 5.0f, comp = 1.0f / softClip (d * 0.5f) * 0.5f;
            l = softClip (l * d) * comp; r = softClip (r * d) * comp;
        }
        if (wow > 0.001f)
        {
            wowPh += 0.55f / sr; if (wowPh >= 1) wowPh -= 1;
            flPh  += 6.8f / sr;  if (flPh >= 1) flPh -= 1;
            const float w = wander.tick();
            const float modMs = wow * (2.2f * std::sin (kTwoPi * wowPh) + 1.6f * w) + wow * 0.12f * std::sin (kTwoPi * flPh);
            const float d = (6.0f + modMs) * sr * 0.001f;
            dlL.push (l); dlR.push (r);
            l = dlL.read (d); r = dlR.read (d);
        }
        if (age > 0.001f)
        {
            l = lpL.lp (l); r = lpR.lp (r);
            l = hsL.hp (l); r = hsR.hp (r);
            const float hiss = age * age * 0.004f;
            l += rng.bipolar() * hiss; r += rng.bipolar() * hiss;
        }
    }
};

// Tape-echo style stereo delay with darkening feedback
struct Echo
{
    DelayLine dlL, dlR; OnePole fbL, fbR, hpL, hpR; float sr = 44100, smoothedSamples = 0;
    void prepare (double s)
    {
        sr = (float) s; dlL.prepare ((int) (s * 2.2)); dlR.prepare ((int) (s * 2.2));
        fbL.setCutoff (4200.0f, sr); fbR.a = fbL.a; hpL.setCutoff (120.0f, sr); hpR.a = hpL.a; smoothedSamples = 0;
    }
    void clear() { dlL.clear(); dlR.clear(); }
    void process (float& l, float& r, float timeMs, float fb, float mix)
    {
        const float target = timeMs * 0.001f * sr;
        if (smoothedSamples <= 0) smoothedSamples = target;
        smoothedSamples += (target - smoothedSamples) * 0.0004f;   // tape-like pitch glide when time changes
        if (mix <= 0.001f) { dlL.push (0); dlR.push (0); return; }
        const float yL = dlL.read (smoothedSamples);
        const float yR = dlR.read (smoothedSamples);
        // ping-pong: left feeds right, input into left
        const float inMono = 0.5f * (l + r);
        dlL.push (softClip (inMono + hpL.hp (fbL.lp (yR)) * fb));
        dlR.push (softClip (hpR.hp (fbR.lp (yL)) * fb));
        l += yL * mix; r += yR * mix;
    }
};

struct DCBlock
{
    float x1 = 0, y1 = 0;
    float process (float x) { const float y = x - x1 + 0.9995f * y1; x1 = x; y1 = y; return y; }
};
} // namespace dw
