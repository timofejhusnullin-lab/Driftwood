#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

using APF = juce::AudioParameterFloat;
using APC = juce::AudioParameterChoice;
using API = juce::AudioParameterInt;

namespace
{
juce::String waveName (float v)
{
    static const char* names[] = { "Sine", "Tri", "Saw", "Square" };
    const int i = juce::jlimit (0, 3, (int) std::round (v));
    if (std::abs (v - (float) i) < 0.04f) return names[i];
    const int lo = juce::jlimit (0, 2, (int) v);
    return juce::String (names[lo]) + ">" + names[lo + 1] + " " + juce::String ((int) ((v - (float) lo) * 100.0f)) + "%";
}

std::unique_ptr<APF> fp (const char* id, const char* name, juce::NormalisableRange<float> r, float def,
                         std::function<juce::String (float, int)> toText)
{
    return std::make_unique<APF> (juce::ParameterID { id, 1 }, name, r, def,
                                  juce::AudioParameterFloatAttributes().withStringFromValueFunction (std::move (toText)));
}

juce::NormalisableRange<float> skewed (float lo, float hi, float centre)
{
    juce::NormalisableRange<float> r (lo, hi); r.setSkewForCentre (centre); return r;
}

auto pct   = [] (float v, int) { return juce::String ((int) std::round (v * 100.0f)) + "%"; };
auto bipct = [] (float v, int) { return (v > 0 ? "+" : "") + juce::String ((int) std::round (v * 100.0f)) + "%"; };
auto secs  = [] (float v, int) { return v < 1.0f ? juce::String ((int) std::round (v * 1000.0f)) + " ms" : juce::String (v, 2) + " s"; };
auto hz    = [] (float v, int) { return v < 1000.0f ? juce::String ((int) std::round (v)) + " Hz" : juce::String (v / 1000.0f, 2) + " kHz"; };
}

juce::AudioProcessorValueTreeState::ParameterLayout DriftwoodProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto wave = [] (float v, int) { return waveName (v); };

    // Oscillators
    p.push_back (fp ("osc1Wave", "Osc1 Wave", { 0.0f, 3.0f }, 2.0f, wave));
    p.push_back (fp ("osc1Pw", "Pulse Width", { 0.05f, 0.95f }, 0.5f, pct));
    p.push_back (std::make_unique<API> (juce::ParameterID { "osc1Oct", 1 }, "Osc1 Octave", -2, 2, 0));
    p.push_back (fp ("osc2Wave", "Osc2 Wave", { 0.0f, 3.0f }, 2.0f, wave));
    p.push_back (std::make_unique<API> (juce::ParameterID { "osc2Oct", 1 }, "Osc2 Octave", -2, 2, 0));
    p.push_back (std::make_unique<API> (juce::ParameterID { "osc2Semi", 1 }, "Osc2 Semi", -12, 12, 0));
    p.push_back (fp ("osc2Fine", "Osc2 Fine", { -50.0f, 50.0f }, 7.0f, [] (float v, int) { return juce::String (v, 1) + " ct"; }));
    p.push_back (fp ("osc2Xmod", "X-Mod", { 0.0f, 1.0f }, 0.0f, pct));
    p.push_back (fp ("oscMix", "Osc Mix", { 0.0f, 1.0f }, 0.5f, [] (float v, int) { return juce::String ((int) ((1 - v) * 100)) + "/" + juce::String ((int) (v * 100)); }));
    p.push_back (fp ("sub", "Sub", { 0.0f, 1.0f }, 0.0f, pct));
    p.push_back (fp ("noise", "Noise", { 0.0f, 1.0f }, 0.0f, pct));
    p.push_back (fp ("drift", "Drift", { 0.0f, 1.0f }, 0.3f, pct));

    // Filter
    p.push_back (std::make_unique<APC> (juce::ParameterID { "filterType", 1 }, "Filter Type",
                                        juce::StringArray { "Ladder", "MS LP", "MS BP", "MS HP" }, 0));
    p.push_back (fp ("cutoff", "Cutoff", skewed (20.0f, 18000.0f, 900.0f), 2000.0f, hz));
    p.push_back (fp ("reso", "Resonance", { 0.0f, 1.0f }, 0.2f, pct));
    p.push_back (fp ("drive", "Drive", { 0.0f, 1.0f }, 0.2f, pct));
    p.push_back (fp ("fEnv", "Env Amount", { -1.0f, 1.0f }, 0.4f, bipct));
    p.push_back (fp ("keyTrack", "Key Track", { 0.0f, 1.0f }, 0.5f, pct));
    p.push_back (fp ("velFilter", "Vel > Cutoff", { 0.0f, 1.0f }, 0.3f, pct));

    // Envelopes
    auto envRange = skewed (0.001f, 10.0f, 0.4f);
    p.push_back (fp ("fA", "F.Attack",  envRange, 0.005f, secs));
    p.push_back (fp ("fD", "F.Decay",   envRange, 0.4f,   secs));
    p.push_back (fp ("fS", "F.Sustain", { 0.0f, 1.0f }, 0.3f, pct));
    p.push_back (fp ("fR", "F.Release", envRange, 0.4f,   secs));
    p.push_back (fp ("aA", "Attack",  envRange, 0.005f, secs));
    p.push_back (fp ("aD", "Decay",   envRange, 0.6f,   secs));
    p.push_back (fp ("aS", "Sustain", { 0.0f, 1.0f }, 0.8f, pct));
    p.push_back (fp ("aR", "Release", envRange, 0.4f,   secs));
    p.push_back (fp ("velAmp", "Vel > Amp", { 0.0f, 1.0f }, 0.5f, pct));

    // LFO
    p.push_back (std::make_unique<APC> (juce::ParameterID { "lfoShape", 1 }, "LFO Shape",
                                        juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H", "Drift" }, 0));
    p.push_back (fp ("lfoRate", "LFO Rate", skewed (0.05f, 20.0f, 3.0f), 5.0f, [] (float v, int) { return juce::String (v, 2) + " Hz"; }));
    p.push_back (fp ("lfoDelay", "LFO Delay", { 0.0f, 4.0f }, 0.0f, secs));
    p.push_back (fp ("lfoPitch", "LFO > Pitch", { 0.0f, 1.0f }, 0.0f, pct));
    p.push_back (fp ("lfoFilter", "LFO > Filter", { 0.0f, 1.0f }, 0.0f, pct));
    p.push_back (fp ("lfoPw", "LFO > PW", { 0.0f, 1.0f }, 0.0f, pct));
    p.push_back (fp ("lfoAmp", "LFO > Amp", { 0.0f, 1.0f }, 0.0f, pct));

    // Voice
    p.push_back (std::make_unique<APC> (juce::ParameterID { "voiceMode", 1 }, "Voice Mode",
                                        juce::StringArray { "Poly", "Mono", "Legato" }, 0));
    p.push_back (fp ("glide", "Glide", skewed (0.0f, 2.0f, 0.25f), 0.0f, secs));
    p.push_back (std::make_unique<API> (juce::ParameterID { "bendRange", 1 }, "Bend Range", 1, 12, 2));
    p.push_back (fp ("wheelVib", "Wheel > Vibrato", { 0.0f, 1.0f }, 0.3f, pct));

    // FX
    p.push_back (std::make_unique<APC> (juce::ParameterID { "chorusMode", 1 }, "Chorus",
                                        juce::StringArray { "Off", "I", "II", "I+II" }, 0));
    p.push_back (fp ("tapeWow", "Wow/Flutter", { 0.0f, 1.0f }, 0.0f, pct));
    p.push_back (fp ("tapeSat", "Tape Sat", { 0.0f, 1.0f }, 0.0f, pct));
    p.push_back (fp ("tapeAge", "Tape Age", { 0.0f, 1.0f }, 0.0f, pct));
    p.push_back (fp ("crush", "Lo-Fi", { 0.0f, 1.0f }, 0.0f, pct));
    p.push_back (std::make_unique<APC> (juce::ParameterID { "delaySync", 1 }, "Delay Sync",
                                        juce::StringArray { "Free", "1/4", "1/8.", "1/8", "1/4T", "1/16" }, 0));
    p.push_back (fp ("delayTime", "Delay Time", skewed (10.0f, 1500.0f, 300.0f), 350.0f, [] (float v, int) { return juce::String ((int) v) + " ms"; }));
    p.push_back (fp ("delayFb", "Delay Feedback", { 0.0f, 0.95f }, 0.35f, pct));
    p.push_back (fp ("delayMix", "Delay Mix", { 0.0f, 1.0f }, 0.0f, pct));
    p.push_back (fp ("revSize", "Reverb Size", { 0.0f, 1.0f }, 0.5f, pct));
    p.push_back (fp ("revMix", "Reverb Mix", { 0.0f, 1.0f }, 0.0f, pct));
    p.push_back (fp ("master", "Master", { -36.0f, 6.0f }, -4.0f, [] (float v, int) { return juce::String (v, 1) + " dB"; }));

    return { p.begin(), p.end() };
}

DriftwoodProcessor::DriftwoodProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DRIFTWOOD", createLayout())
{
    heldNotes.reserve (128);
    lfoRng.seed (4242u);
}

bool DriftwoodProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void DriftwoodProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    for (int i = 0; i < maxVoices; ++i) { voices[i].prepare (sampleRate, i); voices[i].kill(); }
    chorus.prepare (sampleRate); tape.prepare (sampleRate); echo.prepare (sampleRate);
    reverb.setSampleRate (sampleRate); reverb.reset();
    lfoBuf.assign ((size_t) std::max (1, samplesPerBlock), 0.0f);
    heldNotes.clear();
    sustainedNotes.fill (false);
}

// ---------------------------------------------------------------------------
void DriftwoodProcessor::readParams()
{
    auto v = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    sp.osc1Wave = v ("osc1Wave"); sp.osc1Pw = v ("osc1Pw"); sp.osc1Oct = v ("osc1Oct");
    sp.osc2Wave = v ("osc2Wave"); sp.osc2Oct = v ("osc2Oct"); sp.osc2Semi = v ("osc2Semi");
    sp.osc2Fine = v ("osc2Fine"); sp.osc2Xmod = v ("osc2Xmod");
    sp.oscMix = v ("oscMix"); sp.sub = v ("sub"); sp.noise = v ("noise"); sp.drift = v ("drift");
    sp.filterType = (int) v ("filterType");
    sp.cutoff = v ("cutoff"); sp.reso = v ("reso"); sp.drive = v ("drive"); sp.fEnv = v ("fEnv");
    sp.keyTrack = v ("keyTrack"); sp.velFilter = v ("velFilter");
    sp.fA = v ("fA"); sp.fD = v ("fD"); sp.fS = v ("fS"); sp.fR = v ("fR");
    sp.aA = std::max (0.001f, v ("aA")); sp.aD = v ("aD"); sp.aS = v ("aS"); sp.aR = std::max (0.005f, v ("aR"));
    sp.velAmp = v ("velAmp");
    sp.lfoDelay = v ("lfoDelay"); sp.lfoPitch = v ("lfoPitch"); sp.lfoFilter = v ("lfoFilter");
    sp.lfoPw = v ("lfoPw"); sp.lfoAmp = v ("lfoAmp");
    sp.glide = v ("glide"); sp.wheelVib = v ("wheelVib");
    sp.bendSemis = bend * v ("bendRange");
    sp.modWheel = modWheel;
    lfoShape = (int) v ("lfoShape"); lfoRate = v ("lfoRate");

    const int newMode = (int) v ("voiceMode");
    if (newMode != voiceMode)
    {
        for (auto& vc : voices) vc.noteOff();
        heldNotes.clear();
        voiceMode = newMode;
    }
}

float DriftwoodProcessor::lfoTick()
{
    lfoPhase += lfoRate / (float) sr;
    if (lfoPhase >= 1.0f)
    {
        lfoPhase -= 1.0f;
        lfoSHPrev = lfoSH; lfoSH = lfoRng.bipolar();
    }
    const float p = lfoPhase;
    switch (lfoShape)
    {
        case 0:  return std::sin (dw::kTwoPi * p);
        case 1:  return 1.0f - 4.0f * std::abs (p - 0.5f);
        case 2:  return 1.0f - 2.0f * p;
        case 3:  return p < 0.5f ? 1.0f : -1.0f;
        case 4:  return lfoSH;
        default: { const float t = 0.5f - 0.5f * std::cos (dw::kPi * p); return lfoSHPrev + (lfoSH - lfoSHPrev) * t; }
    }
}

// ---------------------------------------------------------------------------
void DriftwoodProcessor::startNote (int note, float vel)
{
    const float glideFrom = (sp.glide > 0.001f && lastNotePitch >= 0.0f) ? lastNotePitch : -1.0f;
    sustainedNotes[(size_t) note] = false;

    if (voiceMode == 0)
    {
        dw::Voice* target = nullptr;
        for (auto& v : voices) if (v.isActive() && v.getNote() == note) { target = &v; break; }
        if (! target) for (auto& v : voices) if (! v.isActive()) { target = &v; break; }
        if (! target)
        {
            uint64_t oldest = UINT64_MAX;
            for (auto& v : voices) if (! v.isGateOn() && v.age < oldest) { oldest = v.age; target = &v; }
        }
        if (! target)
        {
            uint64_t oldest = UINT64_MAX;
            for (auto& v : voices) if (v.age < oldest) { oldest = v.age; target = &v; }
        }
        target->noteOn (note, vel, glideFrom, true, ++noteCounter);
    }
    else
    {
        heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
        heldNotes.push_back (note);
        auto& v = voices[0];
        if (voiceMode == 2 && v.isActive() && v.isGateOn())
            v.legatoTo (note);
        else
            v.noteOn (note, vel, glideFrom, true, ++noteCounter);
    }
    lastNotePitch = (float) note;
}

void DriftwoodProcessor::stopNote (int note)
{
    if (voiceMode == 0)
    {
        for (auto& v : voices) if (v.isGateOn() && v.getNote() == note) v.noteOff();
        return;
    }

    heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
    auto& v = voices[0];
    if (v.getNote() != note) return;
    if (! heldNotes.empty())
    {
        const int next = heldNotes.back();
        if (voiceMode == 2) v.legatoTo (next);
        else v.noteOn (next, 0.8f, sp.glide > 0.001f ? (float) note : -1.0f, true, ++noteCounter);
        lastNotePitch = (float) next;
    }
    else v.noteOff();
}

void DriftwoodProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
        startNote (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())
    {
        if (sustainPedal) sustainedNotes[(size_t) m.getNoteNumber()] = true;
        else stopNote (m.getNoteNumber());
    }
    else if (m.isPitchWheel())
    {
        bend = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
        sp.bendSemis = bend * apvts.getRawParameterValue ("bendRange")->load();
    }
    else if (m.isController())
    {
        if (m.getControllerNumber() == 1) { modWheel = (float) m.getControllerValue() / 127.0f; sp.modWheel = modWheel; }
        else if (m.getControllerNumber() == 64)
        {
            sustainPedal = m.getControllerValue() >= 64;
            if (! sustainPedal)
                for (int n = 0; n < 128; ++n)
                    if (sustainedNotes[(size_t) n]) { sustainedNotes[(size_t) n] = false; stopNote (n); }
        }
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        for (auto& v : voices) v.noteOff();
        heldNotes.clear(); sustainedNotes.fill (false);
    }
}

void DriftwoodProcessor::renderVoices (float* L, float* R, int n)
{
    if ((int) lfoBuf.size() < n) lfoBuf.resize ((size_t) n);
    for (int i = 0; i < n; ++i) lfoBuf[(size_t) i] = lfoTick();
    for (auto& v : voices) v.render (L, R, n, sp, lfoBuf.data());
}

// ---------------------------------------------------------------------------
void DriftwoodProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();
    readParams();

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    std::vector<float> tmpR;
    if (R == nullptr) { tmpR.assign ((size_t) numSamples, 0.0f); R = tmpR.data(); }

    // sample-accurate MIDI: render up to each event
    int pos = 0;
    for (const auto meta : midi)
    {
        const int evPos = juce::jlimit (0, numSamples, meta.samplePosition);
        if (evPos > pos) { renderVoices (L + pos, R + pos, evPos - pos); pos = evPos; }
        handleMidi (meta.getMessage());
    }
    if (pos < numSamples) renderVoices (L + pos, R + pos, numSamples - pos);

    lfoDisplay.store (lfoBuf.empty() ? 0.0f : lfoBuf[0]);
    int act = 0; for (auto& v : voices) act += v.isActive() ? 1 : 0;
    activeVoices.store (act);

    // ---- FX ----
    auto v = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    const int chorusMode = (int) v ("chorusMode");
    const float wow = v ("tapeWow"), sat = v ("tapeSat"), age = v ("tapeAge"), crush = v ("crush");
    const float fb = v ("delayFb"), dMix = v ("delayMix");
    float dTime = v ("delayTime");
    const int sync = (int) v ("delaySync");
    if (sync > 0)
    {
        double bpm = 120.0;
        if (auto* ph = getPlayHead())
            if (auto pos2 = ph->getPosition())
                if (auto b = pos2->getBpm()) bpm = *b;
        static const float beats[] = { 1.0f, 1.0f, 0.75f, 0.5f, 2.0f / 3.0f, 0.25f };
        dTime = juce::jlimit (10.0f, 2000.0f, (float) (beats[sync] * 60000.0 / bpm));
    }

    juce::Reverb::Parameters rp;
    const float revMix = v ("revMix");
    rp.roomSize = 0.3f + 0.68f * v ("revSize"); rp.damping = 0.55f; rp.width = 1.0f;
    rp.wetLevel = revMix * 0.4f; rp.dryLevel = 0.5f * (1.0f - 0.35f * revMix);
    reverb.setParameters (rp);

    const float masterTarget = juce::Decibels::decibelsToGain (v ("master"));

    for (int i = 0; i < numSamples; ++i)
    {
        float l = L[i] * 0.55f, r = R[i] * 0.55f;
        crusher.process (l, r, crush);
        chorus.process (l, r, chorusMode);
        tape.process (l, r, wow, sat, age);
        echo.process (l, r, dTime, fb, dMix);
        L[i] = l; R[i] = r;
    }

    if (revMix > 0.001f) reverb.processStereo (L, R, numSamples);

    int w = scopeWrite.load();
    for (int i = 0; i < numSamples; ++i)
    {
        masterGainSmoothed += (masterTarget - masterGainSmoothed) * 0.002f;
        float l = dcL.process (L[i]) * masterGainSmoothed;
        float r = dcR.process (R[i]) * masterGainSmoothed;
        // gentle output soft-clip so nothing ever explodes
        l = std::abs (l) > 0.8f ? std::copysign (0.8f + 0.2f * dw::softClip ((std::abs (l) - 0.8f) * 5.0f), l) : l;
        r = std::abs (r) > 0.8f ? std::copysign (0.8f + 0.2f * dw::softClip ((std::abs (r) - 0.8f) * 5.0f), r) : r;
        L[i] = l; R[i] = r;
        scope[(size_t) w] = 0.5f * (l + r); w = (w + 1) % scopeSize;
    }
    scopeWrite.store (w);

    if (buffer.getNumChannels() == 1) {} // mono host: L already holds signal
}

// ---------------------------------------------------------------------------
int DriftwoodProcessor::getNumPrograms() { return (int) getFactoryPresets().size(); }

const juce::String DriftwoodProcessor::getProgramName (int index)
{
    const auto& ps = getFactoryPresets();
    return juce::isPositiveAndBelow (index, (int) ps.size()) ? juce::String (ps[(size_t) index].name) : juce::String();
}

void DriftwoodProcessor::setCurrentProgram (int index)
{
    const auto& ps = getFactoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) ps.size())) return;
    currentProgram = index;

    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    for (const auto& kv : ps[(size_t) index].values)
        if (auto* rp = apvts.getParameter (kv.first))
            rp->setValueNotifyingHost (rp->convertTo0to1 (kv.second));

    programChangedFlag.fetch_add (1);
}

void DriftwoodProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void DriftwoodProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = tree.getProperty ("program", 0);
            apvts.replaceState (tree);
            programChangedFlag.fetch_add (1);
        }
}

juce::AudioProcessorEditor* DriftwoodProcessor::createEditor() { return new DriftwoodEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DriftwoodProcessor(); }
