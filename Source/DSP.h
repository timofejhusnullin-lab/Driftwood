#pragma once
// Driftwood — core DSP building blocks (header-only, no JUCE dependency)
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <vector>

namespace dw
{
constexpr float kPi    = 3.14159265358979f;
constexpr float kTwoPi = 6.28318530717959f;

inline float clampf (float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }

// cheap tanh-like soft clipper (Pade approx, accurate enough for saturation)
inline float softClip (float x)
{
    x = clampf (x, -3.0f, 3.0f);
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

inline float noteToHz (float note) { return 440.0f * std::exp2 ((note - 69.0f) / 12.0f); }

// ---------------------------------------------------------------------------
struct Rng
{
    uint32_t s = 0x12345678u;
    void seed (uint32_t v) { s = v ? v : 0x9e3779b9u; }
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float bipolar() { return (float) (next() >> 8) * (2.0f / 16777216.0f) - 1.0f; }  // -1..1
    float unipolar() { return (float) (next() >> 8) * (1.0f / 16777216.0f); }       // 0..1
};

// Slow random wander — the "analog drift" source
struct Drift
{
    float value = 0, target = 0, coef = 0, counter = 0, period = 1;
    Rng rng;
    void prepare (double sr, uint32_t seed) { rng.seed (seed); period = (float) sr * 0.35f; coef = 1.0f - std::exp (-1.0f / ((float) sr * 0.25f)); counter = 0; }
    float tick()
    {
        if (--counter <= 0) { target = rng.bipolar(); counter = period * (0.5f + rng.unipolar()); }
        value += (target - value) * coef;
        return value;
    }
};

// ---------------------------------------------------------------------------
inline float polyBlep (float t, float dt)
{
    if (t < dt)        { t /= dt;            return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}

// Band-limited oscillator with continuous morph: 0 sine -> 1 tri -> 2 saw -> 3 square/pulse
struct Osc
{
    float phase = 0.0f;
    void reset (float p) { phase = p; }

    static float shape (int idx, float ph, float dt, float pw)
    {
        switch (idx)
        {
            case 0:  return std::sin (kTwoPi * ph);
            case 1:  { float t = ph < 0.5f ? (4.0f * ph - 1.0f) : (3.0f - 4.0f * ph); return t; }
            case 2:  return (2.0f * ph - 1.0f) - polyBlep (ph, dt);
            default:
            {
                float v = ph < pw ? 1.0f : -1.0f;
                v += polyBlep (ph, dt);
                float t2 = ph - pw; if (t2 < 0.0f) t2 += 1.0f;
                v -= polyBlep (t2, dt);
                return v - (2.0f * pw - 1.0f); // remove DC of asymmetric pulse
            }
        }
    }

    float process (float dt, float wave, float pw)
    {
        dt = clampf (dt, 0.0f, 0.45f);
        const int   i0 = std::min (2, (int) wave);
        const float fr = clampf (wave - (float) i0, 0.0f, 1.0f);
        float out;
        if (wave >= 2.999f)      out = shape (3, phase, dt, pw);
        else if (fr < 0.001f)    out = shape (i0, phase, dt, pw);
        else                     out = shape (i0, phase, dt, pw) * (1.0f - fr) + shape (i0 + 1, phase, dt, pw) * fr;
        phase += dt;
        if (phase >= 1.0f) phase -= 1.0f;
        return out;
    }
};

// ---------------------------------------------------------------------------
// 4-pole transistor-ladder style lowpass (ZDF, tanh on input & feedback)
struct LadderFilter
{
    float s[4] {};
    void reset() { for (auto& v : s) v = 0.0f; }

    float process (float in, float g, float reso, float drive)
    {
        const float G  = g / (1.0f + g);
        const float b  = 1.0f / (1.0f + g);
        const float k  = reso * 4.1f;
        const float S  = G * G * G * b * s[0] + G * G * b * s[1] + G * b * s[2] + b * s[3];
        const float G4 = G * G * G * G;
        const float u0 = in * (1.0f + drive * 3.0f) * (1.0f + reso * 0.6f);   // gain comp for reso bass loss
        const float y4 = (G4 * u0 + S) / (1.0f + k * G4);
        float x = softClip (u0 - k * softClip (y4));

        for (int i = 0; i < 4; ++i)
        {
            const float v = (x - s[i]) * G;
            const float y = v + s[i];
            s[i] = y + v;
            x = y;
        }
        return x / (1.0f + drive * 1.2f);
    }
};

// 2-pole state-variable filter with saturating feedback (MS-20-ish screamer)
struct MSFilter
{
    float s1 = 0, s2 = 0;
    void reset() { s1 = s2 = 0; }

    // mode: 0 LP, 1 BP, 2 HP
    float process (float in, float g, float reso, float drive, int mode)
    {
        const float R  = 1.0f - 0.985f * reso;
        in = softClip (in * (1.0f + drive * 4.0f)) * 0.8f;
        const float hp = (in - (2.0f * R + g) * s1 - s2) / (1.0f + 2.0f * R * g + g * g);
        const float bp = g * hp + s1;
        s1 = softClip (g * hp + bp);          // saturating integrator => aggressive but stable resonance
        const float lp = g * bp + s2;
        s2 = g * bp + lp;
        s2 = clampf (s2, -4.0f, 4.0f);
        switch (mode)
        {
            case 1:  return bp * (1.0f + reso);
            case 2:  return hp;
            default: return lp;
        }
    }
};

// ---------------------------------------------------------------------------
// Analog-style RC envelope (overshooting attack, exponential decay/release).
struct Envelope
{
    enum Stage { Idle, Attack, Decay, Sustain, Release };
    Stage stage = Idle;
    float value = 0.0f;
    float sr = 44100.0f;

    void prepare (double s) { sr = (float) s; }
    void gateOn()  { stage = Attack; }
    void gateOff() { if (stage != Idle) stage = Release; }
    void kill()    { stage = Idle; value = 0.0f; }
    bool isActive() const { return stage != Idle; }

    float coef (float seconds) const { return 1.0f - std::exp (-1.0f / std::max (1.0f, seconds * sr)); }

    float process (float a, float d, float s, float r)
    {
        switch (stage)
        {
            case Attack:
                value += (1.25f - value) * coef (a * 0.6f);
                if (value >= 1.0f) { value = 1.0f; stage = Decay; }
                break;
            case Decay:
                value += (s - value) * coef (d * 0.3f);
                if (std::abs (value - s) < 1.0e-4f) stage = Sustain;
                break;
            case Sustain:
                value += (s - value) * 0.001f;
                break;
            case Release:
                value += (0.0f - value) * coef (r * 0.3f);
                if (value < 1.0e-5f) { value = 0.0f; stage = Idle; }
                break;
            default: break;
        }
        return value;
    }
};

// ---------------------------------------------------------------------------
struct OnePole
{
    float z = 0, a = 1;
    void setCutoff (float hz, float sr) { a = 1.0f - std::exp (-kTwoPi * clampf (hz, 10.0f, sr * 0.49f) / sr); }
    float lp (float x) { z += a * (x - z); return z; }
    float hp (float x) { return x - lp (x); }
};

struct DelayLine
{
    std::vector<float> buf; int w = 0;
    void prepare (int size) { buf.assign ((size_t) std::max (4, size), 0.0f); w = 0; }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); }
    void push (float x) { buf[(size_t) w] = x; if (++w >= (int) buf.size()) w = 0; }
    // fractional read, delay in samples (>=1)
    float read (float d) const
    {
        const int n = (int) buf.size();
        d = clampf (d, 1.0f, (float) n - 3.0f);
        float rp = (float) w - d; while (rp < 0) rp += (float) n;
        const int i0 = (int) rp; const float fr = rp - (float) i0;
        const int i1 = (i0 + 1) % n;
        return buf[(size_t) i0] + (buf[(size_t) i1] - buf[(size_t) i0]) * fr;
    }
};
} // namespace dw
