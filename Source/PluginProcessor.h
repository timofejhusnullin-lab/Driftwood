#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include "Voice.h"
#include "Effects.h"

class DriftwoodProcessor : public juce::AudioProcessor
{
public:
    DriftwoodProcessor();
    ~DriftwoodProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Driftwood"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    // GUI feedback
    static constexpr int scopeSize = 1024;
    std::array<float, scopeSize> scope {};
    std::atomic<int> scopeWrite { 0 };
    std::atomic<float> lfoDisplay { 0.0f };
    std::atomic<int> activeVoices { 0 };
    std::atomic<int> programChangedFlag { 0 };

private:
    static constexpr int maxVoices = 8;
    void readParams();
    void handleMidi (const juce::MidiMessage& m);
    void startNote (int note, float vel);
    void stopNote (int note);
    void renderVoices (float* L, float* R, int n);
    float lfoTick();

    dw::Voice voices[maxVoices];
    dw::SynthParams sp;
    double sr = 44100.0;
    uint64_t noteCounter = 0;
    float lastNotePitch = -1.0f;
    std::vector<int> heldNotes;   // mono note stack
    bool sustainPedal = false;
    std::array<bool, 128> sustainedNotes {};
    float bend = 0.0f, modWheel = 0.0f;

    // LFO
    int lfoShape = 0; float lfoRate = 5, lfoPhase = 0, lfoSH = 0, lfoSHPrev = 0; dw::Rng lfoRng;
    std::vector<float> lfoBuf;

    // FX
    dw::Crusher crusher; dw::Chorus chorus; dw::Tape tape; dw::Echo echo;
    juce::Reverb reverb; dw::DCBlock dcL, dcR;
    float masterGainSmoothed = 0.5f;

    int voiceMode = 0;
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DriftwoodProcessor)
};
