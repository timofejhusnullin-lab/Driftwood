#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace Palette
{
    const juce::Colour cream     { 0xffeadfc6 };
    const juce::Colour creamDark { 0xffd8caa9 };
    const juce::Colour ink       { 0xff33281f };
    const juce::Colour inkSoft   { 0xff6b5a48 };
    const juce::Colour charcoal  { 0xff1e1a17 };
    const juce::Colour orange    { 0xffd5602a };
    const juce::Colour mustard   { 0xffdfa12a };
    const juce::Colour teal      { 0xff2e7c76 };
    const juce::Colour brick     { 0xffa93c2c };
    const juce::Colour olive     { 0xff6f7a36 };
    const juce::Colour amber     { 0xffffb347 };
    const juce::Colour screen    { 0xff1a1410 };
}

inline juce::Font vintageFont (float h, bool bold = true)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}

class VintageLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VintageLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, Palette::ink);
        setColour (juce::BubbleComponent::backgroundColourId, Palette::charcoal);
        setColour (juce::BubbleComponent::outlineColourId, Palette::amber);
        setColour (juce::TooltipWindow::backgroundColourId, Palette::charcoal);
        setColour (juce::TooltipWindow::textColourId, Palette::cream);
        setColour (juce::TooltipWindow::outlineColourId, Palette::amber);
        setColour (juce::PopupMenu::backgroundColourId, Palette::charcoal);
        setColour (juce::PopupMenu::textColourId, Palette::cream);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, Palette::orange);
        setColour (juce::PopupMenu::highlightedTextColourId, Palette::charcoal);
        setColour (juce::ComboBox::textColourId, Palette::amber);
        setColour (juce::ComboBox::arrowColourId, Palette::amber);
        setColour (juce::Label::textColourId, Palette::ink);
        setColour (juce::TextButton::buttonColourId, Palette::charcoal);
        setColour (juce::TextButton::textColourOffId, Palette::cream);
    }

    // ------------------------------------------------------------------ knobs
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAng, float endAng, juce::Slider& s) override
    {
        const auto accent = s.findColour (juce::Slider::rotarySliderFillColourId);
        auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
        const float size = juce::jmin (area.getWidth(), area.getHeight());
        auto r = area.withSizeKeepingCentre (size, size);
        const auto c = r.getCentre();
        const float radius = size * 0.5f;

        // tick marks
        g.setColour (Palette::inkSoft);
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAng + (endAng - startAng) * (float) i / 10.0f;
            const float r1 = radius * 0.86f, r2 = radius * (i % 5 == 0 ? 1.0f : 0.95f);
            g.drawLine (c.x + r1 * std::sin (a), c.y - r1 * std::cos (a), c.x + r2 * std::sin (a), c.y - r2 * std::cos (a), i % 5 == 0 ? 1.6f : 1.0f);
        }

        // value arc
        {
            juce::Path arc;
            const float ar = radius * 0.80f;
            const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
            const float from = bipolar ? (startAng + endAng) * 0.5f : startAng;
            const float to = startAng + pos * (endAng - startAng);
            arc.addCentredArc (c.x, c.y, ar, ar, 0.0f, juce::jmin (from, to), juce::jmax (from, to), true);
            g.setColour (accent.withAlpha (0.9f));
            g.strokePath (arc, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // knob body with drop shadow
        const float kr = radius * 0.68f;
        auto body = juce::Rectangle<float> (kr * 2, kr * 2).withCentre (c);
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillEllipse (body.translated (1.5f, 2.5f).expanded (1.0f));

        // skirt (ridged)
        juce::ColourGradient skirt (juce::Colour (0xff3a3430), c.x, body.getY(), juce::Colour (0xff0d0b0a), c.x, body.getBottom(), false);
        g.setGradientFill (skirt);
        g.fillEllipse (body);
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        for (int i = 0; i < 24; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / 24.0f;
            g.drawLine (c.x + kr * 0.78f * std::sin (a), c.y - kr * 0.78f * std::cos (a), c.x + kr * std::sin (a), c.y - kr * std::cos (a), 1.0f);
        }

        // metal cap
        const float cr = kr * 0.62f;
        auto cap = juce::Rectangle<float> (cr * 2, cr * 2).withCentre (c);
        juce::ColourGradient metal (juce::Colour (0xffe9e4da), cap.getX(), cap.getY(), juce::Colour (0xff8a8378), cap.getRight(), cap.getBottom(), false);
        g.setGradientFill (metal);
        g.fillEllipse (cap);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawEllipse (cap, 0.8f);

        // pointer
        const float a = startAng + pos * (endAng - startAng);
        juce::Path ptr;
        ptr.addRoundedRectangle (-1.6f, -kr * 0.98f, 3.2f, kr * 0.55f, 1.4f);
        g.setColour (Palette::cream);
        g.fillPath (ptr, juce::AffineTransform::rotation (a).translated (c.x, c.y));
        juce::Path dot;
        dot.addEllipse (-1.6f, -cr * 0.8f, 3.2f, 3.2f);
        g.setColour (accent.darker (0.2f));
        g.fillPath (dot, juce::AffineTransform::rotation (a).translated (c.x, c.y));
    }

    // ------------------------------------------------------------------ faders
    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos, float, float,
                           juce::Slider::SliderStyle style, juce::Slider& s) override
    {
        if (style != juce::Slider::LinearVertical)
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos, 0, 0, style, s);
            return;
        }
        const auto accent = s.findColour (juce::Slider::trackColourId);
        auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
        const float cx = area.getCentreX();

        // scale ticks
        g.setColour (Palette::inkSoft);
        for (int i = 0; i <= 10; ++i)
        {
            const float ty = area.getY() + area.getHeight() * (float) i / 10.0f;
            const float len = (i % 5 == 0) ? 7.0f : 4.0f;
            g.drawLine (cx - 9.0f - len, ty, cx - 9.0f, ty, 1.0f);
            g.drawLine (cx + 9.0f, ty, cx + 9.0f + len, ty, 1.0f);
        }

        // slot
        auto slot = juce::Rectangle<float> (5.0f, area.getHeight() + 6.0f).withCentre ({ cx, area.getCentreY() });
        g.setColour (juce::Colour (0xff120f0d));
        g.fillRoundedRectangle (slot, 2.5f);
        auto filled = slot.withTop (sliderPos);
        g.setColour (accent.withAlpha (0.75f));
        g.fillRoundedRectangle (filled.reduced (1.2f, 0.0f), 2.0f);

        // cap
        auto capR = juce::Rectangle<float> (26.0f, 14.0f).withCentre ({ cx, sliderPos });
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.fillRoundedRectangle (capR.translated (1.0f, 2.0f), 2.0f);
        juce::ColourGradient cg (juce::Colour (0xff3d3632), cx, capR.getY(), juce::Colour (0xff0f0d0c), cx, capR.getBottom(), false);
        g.setGradientFill (cg);
        g.fillRoundedRectangle (capR, 2.0f);
        g.setColour (Palette::cream);
        g.fillRect (capR.withSizeKeepingCentre (capR.getWidth() - 4.0f, 2.0f));
    }

    // ------------------------------------------------------------------ combos
    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&) override
    {
        auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
        g.setColour (Palette::screen);
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (r, 3.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.45f).reduced (2.0f, 1.0f), 2.0f);
        juce::Path arrow;
        const float ax = (float) w - 11.0f, ay = (float) h * 0.5f;
        arrow.addTriangle (ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
        g.setColour (Palette::amber.withAlpha (0.8f));
        g.fillPath (arrow);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return vintageFont (13.0f); }
    juce::Font getPopupMenuFont() override { return vintageFont (14.0f); }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (4, 1, box.getWidth() - 18, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
        label.setJustificationType (juce::Justification::centred);
    }

    // ------------------------------------------------------------------ buttons
    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (down ? Palette::orange : (over ? juce::Colour (0xff3a332e) : Palette::charcoal));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (Palette::amber.withAlpha (0.5f));
        g.drawRoundedRectangle (r, 3.0f, 1.0f);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override { return vintageFont (14.0f); }

    void drawBubble (juce::Graphics& g, juce::BubbleComponent&, const juce::Point<float>&, const juce::Rectangle<float>& body) override
    {
        g.setColour (Palette::charcoal);
        g.fillRoundedRectangle (body, 3.0f);
        g.setColour (Palette::amber);
        g.drawRoundedRectangle (body, 3.0f, 1.0f);
    }
    juce::Font getSliderPopupFont (juce::Slider&) override { return vintageFont (13.0f); }
    int getSliderPopupPlacement (juce::Slider&) override { return juce::BubbleComponent::above; }
};
