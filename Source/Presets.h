#pragma once
#include <vector>
#include <utility>

// Factory presets: only values that differ from defaults are listed (plain units).
struct FactoryPreset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values;
};

inline const std::vector<FactoryPreset>& getFactoryPresets()
{
    static const std::vector<FactoryPreset> presets =
    {
        { "Init", {} },

        { "Wonky microLead", {
            {"osc1Wave",2.6f},{"osc2Wave",3},{"osc2Fine",9},{"oscMix",0.4f},{"drift",0.55f},
            {"filterType",1},{"cutoff",1400},{"reso",0.55f},{"drive",0.45f},{"fEnv",0.35f},
            {"fA",0.002f},{"fD",0.35f},{"fS",0.35f},{"fR",0.3f},
            {"aA",0.004f},{"aD",0.5f},{"aS",0.9f},{"aR",0.25f},
            {"lfoShape",0},{"lfoRate",5.6f},{"lfoDelay",0.6f},{"lfoPitch",0.22f},
            {"voiceMode",2},{"glide",0.12f},{"wheelVib",0.5f},
            {"tapeWow",0.25f},{"tapeSat",0.3f},{"delaySync",2},{"delayFb",0.35f},{"delayMix",0.18f},{"revMix",0.12f} } },

        { "Kaleido Keys", {
            {"osc2Fine",11},{"drift",0.35f},{"cutoff",700},{"reso",0.25f},{"fEnv",0.5f},
            {"fA",0.003f},{"fD",0.55f},{"fS",0.25f},{"fR",0.45f},
            {"aA",0.003f},{"aD",1.2f},{"aS",0.55f},{"aR",0.5f},{"velFilter",0.5f},
            {"chorusMode",1},{"tapeWow",0.15f},{"revMix",0.2f},{"revSize",0.55f} } },

        { "Juno Cloud Pad", {
            {"osc1Wave",3},{"lfoPw",0.55f},{"lfoRate",0.6f},{"lfoShape",1},{"osc2Fine",-8},{"oscMix",0.35f},{"sub",0.25f},
            {"cutoff",1800},{"reso",0.15f},{"fEnv",0.15f},{"fA",1.2f},{"fD",2},{"fS",0.6f},{"fR",2},
            {"aA",0.9f},{"aD",2},{"aS",0.85f},{"aR",2.2f},
            {"chorusMode",1},{"revMix",0.3f},{"revSize",0.8f},{"tapeAge",0.15f} } },

        { "Squelch Bass", {
            {"master",1},
            {"voiceMode",1},{"glide",0.06f},{"osc1Oct",-1},{"osc2Wave",3},{"osc2Oct",-1},{"osc2Fine",4},{"oscMix",0.35f},{"sub",0.5f},
            {"cutoff",220},{"reso",0.72f},{"drive",0.6f},{"fEnv",0.62f},{"keyTrack",0.3f},
            {"fA",0.001f},{"fD",0.22f},{"fS",0.05f},{"fR",0.15f},
            {"aA",0.002f},{"aD",0.4f},{"aS",0.75f},{"aR",0.1f},{"velFilter",0.6f},{"tapeSat",0.35f},{"drift",0.2f} } },

        { "Tape Rhodes", {
            {"osc1Wave",0.6f},{"osc2Wave",0},{"osc2Oct",1},{"osc2Fine",3},{"oscMix",0.25f},{"osc2Xmod",0.18f},
            {"filterType",1},{"cutoff",2600},{"reso",0.05f},{"keyTrack",0.8f},{"fEnv",0.2f},{"fA",0.001f},{"fD",0.8f},{"fS",0.3f},
            {"aA",0.002f},{"aD",2.2f},{"aS",0.35f},{"aR",0.6f},{"velAmp",0.7f},{"velFilter",0.7f},
            {"lfoRate",4.5f},{"lfoAmp",0.22f},{"tapeWow",0.35f},{"tapeSat",0.2f},{"tapeAge",0.3f},{"revMix",0.18f} } },

        { "Funk Clav", {
            {"master",0},
            {"osc1Wave",3},{"osc1Pw",0.18f},{"oscMix",0},{"filterType",2},{"cutoff",1500},{"reso",0.45f},{"fEnv",0.45f},
            {"fA",0.001f},{"fD",0.18f},{"fS",0.15f},{"fR",0.1f},
            {"aA",0.001f},{"aD",0.35f},{"aS",0.25f},{"aR",0.08f},{"velFilter",0.8f},{"velAmp",0.7f},{"keyTrack",0.7f},
            {"drive",0.35f},{"crush",0.15f},{"tapeSat",0.25f} } },

        { "Woozy Bells", {
            {"osc1Wave",0},{"osc2Wave",0},{"osc2Oct",1},{"osc2Semi",7},{"osc2Xmod",0.55f},{"oscMix",0.6f},
            {"filterType",1},{"cutoff",6000},{"reso",0},{"fEnv",0},
            {"aA",0.001f},{"aD",1.8f},{"aS",0},{"aR",1.5f},
            {"tapeWow",0.45f},{"delaySync",3},{"delayMix",0.25f},{"delayFb",0.45f},{"revMix",0.3f},{"revSize",0.7f},{"drift",0.4f} } },

        { "Sub Wobbler", {
            {"master",2},
            {"voiceMode",1},{"osc1Wave",1},{"osc1Oct",-1},{"sub",0.7f},{"oscMix",0},
            {"cutoff",300},{"reso",0.5f},{"drive",0.5f},{"lfoRate",2.2f},{"lfoFilter",0.45f},{"fEnv",0.1f},
            {"aA",0.005f},{"aS",1},{"aR",0.15f},{"glide",0.08f} } },

        { "Dusty Strings", {
            {"osc2Fine",14},{"drift",0.5f},{"cutoff",2400},{"reso",0.1f},{"fEnv",0.1f},{"fA",0.6f},
            {"aA",0.7f},{"aD",1.5f},{"aS",0.9f},{"aR",1.8f},
            {"chorusMode",3},{"tapeWow",0.3f},{"tapeAge",0.55f},{"revMix",0.25f},{"lfoShape",1},{"lfoRate",0.3f},{"lfoFilter",0.12f} } },

        { "Hyperjazz Pluck", {
            {"master",1},
            {"osc1Wave",2.3f},{"osc2Wave",3},{"osc2Oct",1},{"osc2Fine",5},{"oscMix",0.3f},
            {"filterType",1},{"cutoff",500},{"reso",0.4f},{"fEnv",0.7f},{"fA",0.001f},{"fD",0.16f},{"fS",0},{"fR",0.2f},
            {"aA",0.001f},{"aD",0.45f},{"aS",0},{"aR",0.3f},{"velFilter",0.5f},
            {"delaySync",2},{"delayFb",0.4f},{"delayMix",0.3f},{"revMix",0.15f},{"chorusMode",2} } },

        { "Glass Sine Lead", {
            {"master",-2},
            {"osc1Wave",0.25f},{"osc2Wave",0},{"osc2Oct",1},{"oscMix",0.15f},{"filterType",1},{"cutoff",5000},{"reso",0},
            {"voiceMode",2},{"glide",0.18f},{"lfoRate",5.2f},{"lfoDelay",0.4f},{"lfoPitch",0.2f},{"wheelVib",0.6f},
            {"aA",0.01f},{"aS",1},{"aR",0.3f},{"tapeWow",0.2f},{"delayMix",0.2f},{"delaySync",1},{"delayFb",0.3f},{"revMix",0.22f} } },

        { "MS Screamer", {
            {"voiceMode",2},{"osc2Fine",-12},{"filterType",1},{"cutoff",900},{"reso",0.9f},{"drive",0.8f},{"fEnv",0.4f},
            {"fD",0.6f},{"fS",0.4f},{"lfoShape",4},{"lfoRate",6},{"lfoFilter",0.25f},{"glide",0.05f},{"aS",1},{"tapeSat",0.4f} } },

        { "Lo-Fi Organ", {
            {"osc1Wave",3},{"osc2Wave",0},{"osc2Oct",1},{"oscMix",0.4f},{"sub",0.5f},{"cutoff",3200},{"reso",0.05f},{"fEnv",0},
            {"aA",0.004f},{"aD",0.1f},{"aS",1},{"aR",0.08f},{"chorusMode",3},{"crush",0.35f},{"tapeWow",0.2f},{"tapeAge",0.4f} } },

        { "Rubber Bass", {
            {"master",1},
            {"voiceMode",1},{"glide",0.1f},{"osc1Wave",1.2f},{"osc1Oct",-1},{"sub",0.55f},{"osc2Wave",3},{"osc2Oct",-1},{"osc1Pw",0.3f},{"oscMix",0.2f},
            {"filterType",1},{"cutoff",380},{"reso",0.35f},{"fEnv",0.4f},{"fD",0.3f},{"fS",0.2f},{"drive",0.3f},{"aS",0.9f},{"aR",0.1f},{"drift",0.25f} } },

        { "Drunk Choir", {
            {"master",-1},
            {"osc1Wave",1.5f},{"osc2Wave",1.5f},{"osc2Fine",18},{"noise",0.08f},{"filterType",2},{"cutoff",1100},{"reso",0.55f},
            {"lfoShape",5},{"lfoRate",1.2f},{"lfoFilter",0.35f},{"lfoPitch",0.08f},
            {"aA",0.5f},{"aD",1},{"aS",0.8f},{"aR",1.4f},{"chorusMode",1},{"revMix",0.35f},{"revSize",0.85f},{"tapeWow",0.4f} } },

        { "Broken Cassette", {
            {"osc2Fine",10},{"drift",0.6f},{"cutoff",1600},{"reso",0.2f},{"fEnv",0.25f},{"fD",0.8f},
            {"aA",0.01f},{"aD",1.5f},{"aS",0.7f},{"aR",0.7f},
            {"tapeWow",0.8f},{"tapeAge",0.7f},{"tapeSat",0.4f},{"crush",0.1f},{"chorusMode",1},{"revMix",0.18f} } },
    };
    return presets;
}
