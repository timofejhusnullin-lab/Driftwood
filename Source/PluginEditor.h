#pragma once
#include "PluginProcessor.h"
#include "VintageLookAndFeel.h"

// One labelled control bound to a parameter: knob, vertical fader or selector.
class ParamControl : public juce::Component
{
public:
    enum Kind { Knob, BigKnob, Fader, Selector };

    ParamControl (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& text,
                  Kind k, juce::Colour accent, const juce::String& tip);
    void resized() override;

    Kind kind;
    float weight = 1.0f;

private:
    juce::Slider slider;
    juce::ComboBox combo;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cAtt;
};

class Scope : public juce::Component
{
public:
    explicit Scope (DriftwoodProcessor& p) : proc (p) {}
    void paint (juce::Graphics& g) override;
private:
    DriftwoodProcessor& proc;
};

struct Section
{
    juce::String title;
    juce::Colour colour;
    std::vector<ParamControl*> items;
    juce::Rectangle<int> bounds;
};

class DriftwoodEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DriftwoodEditor (DriftwoodProcessor&);
    ~DriftwoodEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    ParamControl* add (const juce::String& id, const juce::String& text, ParamControl::Kind k, juce::Colour c, const char* tipUtf8, float weight = 1.0f);
    void layoutRow (std::vector<Section*> row, juce::Rectangle<int> area);
    void refreshPresetBox();
    void drawBackground (juce::Graphics&);
    juce::Image background;

    DriftwoodProcessor& proc;
    VintageLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    juce::OwnedArray<ParamControl> controls;
    std::vector<std::unique_ptr<Section>> sections;
    std::vector<std::vector<Section*>> rows;

    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };
    Scope scope;
    float lfoLed = 0.0f;
    int voicesLit = 0;
    int lastProgramFlag = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DriftwoodEditor)
};
