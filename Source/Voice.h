#pragma once
#include "DSP.h"

namespace dw
{
// Snapshot of all sound parameters, read once per block
struct SynthParams
{
    float osc1Wave = 2, osc1Pw = 0.5f, osc1Oct = 0;
    float osc2Wave = 2, osc2Oct = 0, osc2Semi = 0, osc2Fine = 7, osc2Xmod = 0;
    float oscMix = 0.5f, sub = 0, noise = 0, drift = 0.3f;
    int   filterType = 0;
    float cutoff = 2000, reso = 0.2f, drive = 0.2f, fEnv = 0.4f, keyTrack = 0.5f, velFilter = 0.3f;
    float fA = 0.005f, fD = 0.4f, fS = 0.3f, fR = 0.4f;
    float aA = 0.005f, aD = 0.6f, aS = 0.8f, aR = 0.4f, velAmp = 0.5f;
    float lfoDelay = 0, lfoPitch = 0, lfoFilter = 0, lfoPw = 0, lfoAmp = 0;
    float glide = 0, wheelVib = 0.3f;
    // performance
    float bendSemis = 0, modWheel = 0;
};

class Voice
{
public:
    void prepare (double sampleRate, int index)
    {
        sr = (float) sampleRate;
        ampEnv.prepare (sampleRate); filtEnv.prepare (sampleRate);
        driftA.prepare (sampleRate, 1234u + (uint32_t) index * 7919u);
        driftB.prepare (sampleRate, 4321u + (uint32_t) index * 104729u);
        rng.seed (777u + (uint32_t) index * 31337u);
        slop1 = rng.bipolar(); slop2 = rng.bipolar(); slopF = rng.bipolar();
        pan = (index % 2 == 0 ? -1.0f : 1.0f) * (0.08f + 0.04f * (float) (index / 2));
        ladder.reset(); ms.reset();
    }

    bool isActive() const { return ampEnv.isActive(); }
    bool isGateOn() const { return gate; }
    int  getNote() const   { return note; }
    uint64_t age = 0;

    void noteOn (int midiNote, float vel, float glideFromNote, bool retrigger, uint64_t stamp)
    {
        const bool wasActive = isActive();
        note = midiNote; velocity = vel; gate = true; age = stamp;
        targetPitch = (float) midiNote;
        if (glideFromNote >= 0.0f) currentPitch = wasActive ? currentPitch : glideFromNote;
        else currentPitch = targetPitch;

        if (retrigger || ! wasActive)
        {
            ampEnv.gateOn(); filtEnv.gateOn();
            lfoFade = 0.0f;
            if (! wasActive)
            {
                // free-running analog oscillators: random start phase, clean filter
                osc1.reset (rng.unipolar()); osc2.reset (rng.unipolar()); subPhase = 0.0f;
                ladder.reset(); ms.reset();
            }
        }
    }

    void legatoTo (int midiNote)  { note = midiNote; targetPitch = (float) midiNote; }
    void noteOff() { gate = false; ampEnv.gateOff(); filtEnv.gateOff(); }
    void kill() { gate = false; ampEnv.kill(); filtEnv.kill(); }

    // lfo = per-sample global LFO buffer (-1..1)
    void render (float* L, float* R, int n, const SynthParams& p, const float* lfo)
    {
        if (! isActive()) return;

        const float glideCoef = p.glide > 0.001f ? 1.0f - std::exp (-1.0f / (p.glide * 0.35f * sr)) : 1.0f;
        const float lfoFadeInc = p.lfoDelay > 0.001f ? 1.0f / (p.lfoDelay * sr) : 1.0f;
        const float lfoPitchSemis = p.lfoPitch * p.lfoPitch * 12.0f;
        const float vibSemis = p.modWheel * p.wheelVib * 0.7f;
        const float baseCutOct = std::log2 (std::max (20.0f, p.cutoff));
        const float velGain = 1.0f - p.velAmp + p.velAmp * velocity;
        const float velCut  = p.velFilter * (velocity - 0.6f) * 3.0f;
        const float driftAmt = p.drift;
        const float panL = std::sqrt (0.5f * (1.0f - pan)), panR = std::sqrt (0.5f * (1.0f + pan));
        const float octave1 = 12.0f * p.osc1Oct;
        const float octave2 = 12.0f * p.osc2Oct + p.osc2Semi + p.osc2Fine * 0.01f;
        const float invSr = 1.0f / sr;
        const float nyq = sr * 0.45f;

        for (int i = 0; i < n; ++i)
        {
            currentPitch += (targetPitch - currentPitch) * glideCoef;
            lfoFade = std::min (1.0f, lfoFade + lfoFadeInc);
            const float lfoV = lfo[i] * lfoFade;

            const float dA = driftA.tick(), dB = driftB.tick();
            const float driftSemisA = driftAmt * (dA * 0.18f + slop1 * 0.06f);
            const float driftSemisB = driftAmt * (dB * 0.18f + slop2 * 0.06f);

            const float pitchMod = p.bendSemis + lfoV * lfoPitchSemis + lfo[i] * vibSemis;
            const float base = currentPitch + pitchMod;

            const float f1 = noteToHz (base + octave1 + driftSemisA);
            const float f2 = noteToHz (base + octave1 + octave2 + driftSemisB);

            const float pw = clampf (p.osc1Pw + lfoV * p.lfoPw * 0.42f, 0.04f, 0.96f);
            const float o1 = osc1.process (f1 * invSr, p.osc1Wave, pw);

            float dt2 = f2 * invSr;
            if (p.osc2Xmod > 0.0f) dt2 *= std::max (0.0f, 1.0f + p.osc2Xmod * 2.5f * o1);
            const float o2 = osc2.process (dt2, p.osc2Wave, pw);

            // sub oscillator: square one octave below osc1
            const float dts = 0.5f * f1 * invSr;
            float sub = subPhase < 0.5f ? 1.0f : -1.0f;
            sub += polyBlep (subPhase, dts);
            { float t2 = subPhase - 0.5f; if (t2 < 0) t2 += 1.0f; sub -= polyBlep (t2, dts); }
            subPhase += dts; if (subPhase >= 1.0f) subPhase -= 1.0f;

            float x = o1 * (1.0f - p.oscMix) + o2 * p.oscMix + sub * p.sub * 0.8f + rng.bipolar() * p.noise * 0.6f;
            x *= 0.7f;

            // envelopes
            const float fe = filtEnv.process (p.fA, p.fD, p.fS, p.fR);
            const float ae = ampEnv.process (p.aA, p.aD, p.aS, p.aR);

            // cutoff in octaves
            float cutOct = baseCutOct
                         + p.fEnv * fe * 7.0f
                         + p.keyTrack * (currentPitch - 60.0f) / 12.0f
                         + lfoV * p.lfoFilter * 3.0f
                         + velCut
                         + driftAmt * (dA * 0.12f + slopF * 0.08f);
            const float fc = clampf (std::exp2 (cutOct), 18.0f, nyq);
            const float g = std::tan (kPi * fc * invSr);

            float y;
            if (p.filterType == 0) y = ladder.process (x, g, p.reso, p.drive);
            else                   y = ms.process (x, g, p.reso, p.drive, p.filterType - 1);

            float amp = ae * velGain;
            amp *= 1.0f - p.lfoAmp * (0.5f - 0.5f * lfoV);
            y *= amp;

            L[i] += y * panL;
            R[i] += y * panR;
        }
    }

private:
    float sr = 44100.0f;
    int note = -1;
    float velocity = 1.0f;
    bool gate = false;
    float currentPitch = 60, targetPitch = 60;
    float lfoFade = 0;
    float subPhase = 0;
    float slop1 = 0, slop2 = 0, slopF = 0, pan = 0;
    Osc osc1, osc2;
    LadderFilter ladder;
    MSFilter ms;
    Envelope ampEnv, filtEnv;
    Drift driftA, driftB;
    Rng rng;
};
} // namespace dw
