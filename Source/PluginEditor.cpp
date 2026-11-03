#include "PluginEditor.h"
#include "Presets.h"

static constexpr int kWidth = 1080, kHeight = 720, kCheek = 26, kTop = 72;

// ============================================================================
ParamControl::ParamControl (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& text,
                            Kind k, juce::Colour accent, const juce::String& tip)
    : kind (k)
{
    label.setText (text, juce::dontSendNotification);
    label.setFont (vintageFont (11.5f));
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, Palette::ink);
    label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (label);

    if (k == Selector)
    {
        if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (id)))
            combo.addItemList (p->choices, 1);
        combo.setTooltip (tip);
        addAndMakeVisible (combo);
        cAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, id, combo);
        return;
    }

    if (k == Fader)
    {
        slider.setSliderStyle (juce::Slider::LinearVertical);
        slider.setColour (juce::Slider::trackColourId, accent);
    }
    else
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.22f, juce::MathConstants<float>::pi * 2.78f, true);
        slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    }
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setMouseDragSensitivity (k == BigKnob ? 320 : 220);
    slider.setTooltip (tip);
    addAndMakeVisible (slider);
    sAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);

    if (auto* p = s.getParameter (id))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
}

void ParamControl::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromBottom (16));
    if (kind == Selector)
    {
        combo.setBounds (r.withSizeKeepingCentre (juce::jmin (r.getWidth() - 6, 96), 24));
        return;
    }
    if (kind == Fader) r = r.reduced (0, 8);
    else if (kind == Knob) r = r.withSizeKeepingCentre (juce::jmin (r.getWidth(), r.getHeight(), 64), juce::jmin (r.getWidth(), r.getHeight(), 64));
    slider.setBounds (r);
}

// ============================================================================
void Scope::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Palette::screen);
    g.fillRoundedRectangle (r, 5.0f);

    // grid
    g.setColour (Palette::amber.withAlpha (0.08f));
    for (int i = 1; i < 8; ++i) g.drawVerticalLine ((int) (r.getX() + r.getWidth() * (float) i / 8.0f), r.getY() + 3, r.getBottom() - 3);
    for (int i = 1; i < 4; ++i) g.drawHorizontalLine ((int) (r.getY() + r.getHeight() * (float) i / 4.0f), r.getX() + 3, r.getRight() - 3);

    // find a rising zero crossing for a stable trace
    const int N = DriftwoodProcessor::scopeSize;
    const int w = proc.scopeWrite.load();
    const int span = 400;
    int start = (w - span - 300 + 2 * N) % N;
    for (int k = 0; k < 300; ++k)
    {
        const int i0 = (start + k) % N, i1 = (start + k + 1) % N;
        if (proc.scope[(size_t) i0] <= 0.0f && proc.scope[(size_t) i1] > 0.0f) { start = i1; break; }
    }

    juce::Path p;
    const float mid = r.getCentreY(), amp = r.getHeight() * 0.42f;
    for (int k = 0; k < span; ++k)
    {
        const float v = juce::jlimit (-1.0f, 1.0f, proc.scope[(size_t) ((start + k) % N)] * 1.6f);
        const float x = r.getX() + 4 + (r.getWidth() - 8) * (float) k / (float) (span - 1);
        if (k == 0) p.startNewSubPath (x, mid - v * amp); else p.lineTo (x, mid - v * amp);
    }
    g.setColour (Palette::amber.withAlpha (0.18f));
    g.strokePath (p, juce::PathStrokeType (4.0f));
    g.setColour (Palette::amber);
    g.strokePath (p, juce::PathStrokeType (1.3f));

    // glass
    juce::ColourGradient glass (juce::Colours::white.withAlpha (0.10f), r.getX(), r.getY(), juce::Colours::transparentWhite, r.getX(), r.getCentreY(), false);
    g.setGradientFill (glass);
    g.fillRoundedRectangle (r.reduced (2.0f), 4.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (r, 5.0f, 2.0f);
}

// ============================================================================
ParamControl* DriftwoodEditor::add (const juce::String& id, const juce::String& text, ParamControl::Kind k,
                                    juce::Colour c, const char* tipUtf8, float weight)
{
    auto* pc = controls.add (new ParamControl (proc.apvts, id, text, k, c, juce::String::fromUTF8 (tipUtf8)));
    pc->weight = weight;
    addAndMakeVisible (pc);
    return pc;
}

DriftwoodEditor::DriftwoodEditor (DriftwoodProcessor& p)
    : AudioProcessorEditor (&p), proc (p), scope (p)
{
    setLookAndFeel (&lnf);
    using K = ParamControl::Kind;

    auto section = [this] (const char* title, juce::Colour col, std::vector<ParamControl*> items)
    {
        auto s = std::make_unique<Section>();
        s->title = title; s->colour = col; s->items = std::move (items);
        sections.push_back (std::move (s));
        return sections.back().get();
    };

    const auto O = Palette::orange, M = Palette::mustard, T = Palette::teal, B = Palette::brick, L = Palette::olive;

    auto* osc1 = section ("OSC 1", O, {
        add ("osc1Wave", "WAVE",   K::Knob, O, u8"Форма волны: Sine → Tri → Saw → Square. Плавный морфинг между формами."),
        add ("osc1Pw",   "WIDTH",  K::Knob, O, u8"Ширина импульса (для Square). Узкий импульс = носовой, клавесинный звук."),
        add ("osc1Oct",  "OCTAVE", K::Knob, O, u8"Октава всего синта (оба осциллятора и саб).") });

    auto* osc2 = section ("OSC 2", M, {
        add ("osc2Wave", "WAVE",   K::Knob, M, u8"Форма волны второго осциллятора."),
        add ("osc2Oct",  "OCTAVE", K::Knob, M, u8"Октава OSC 2 относительно OSC 1."),
        add ("osc2Semi", "SEMI",   K::Knob, M, u8"Сдвиг в полутонах: +7 = квинта, +12 = октава. Даёт интервалы и аккордовый звук."),
        add ("osc2Fine", "DETUNE", K::Knob, M, u8"Расстройка в центах. 5–15 = тёплое биение как у винтажных синтов."),
        add ("osc2Xmod", "X-MOD",  K::Knob, M, u8"Кросс-модуляция: OSC 1 модулирует частоту OSC 2. Колокольчики, металл, рычание.") });

    auto* mixer = section ("MIXER", L, {
        add ("oscMix", "OSC 1/2", K::Knob, L, u8"Баланс OSC 1 ↔ OSC 2."),
        add ("sub",    "SUB",     K::Knob, L, u8"Саб-осциллятор: квадрат на октаву ниже OSC 1. Плотность низа."),
        add ("noise",  "NOISE",   K::Knob, L, u8"Белый шум — дыхание, перкуссивность."),
        add ("drift",  "DRIFT",   K::Knob, L, u8"Аналоговая нестабильность: плавающий строй и фильтр каждого голоса. Главный «живой» параметр.") });

    auto* filter = section ("FILTER", B, {
        add ("filterType", "TYPE",   K::Selector, B, u8"Ladder — мягкий 24 дБ, жирный (Moog/Juno). MS — агрессивный 12 дБ, кричащий (MS-20): LP / BP / HP.", 1.25f),
        add ("cutoff",     "CUTOFF", K::BigKnob,  B, u8"Частота среза. Главная ручка тембра.", 1.5f),
        add ("reso",       "RESO",   K::Knob,     B, u8"Резонанс. Выше 85% фильтр начинает свистеть/самовозбуждаться."),
        add ("drive",      "DRIVE",  K::Knob,     B, u8"Перегруз на входе фильтра — грязь и плотность."),
        add ("fEnv",       "ENV",    K::Knob,     B, u8"Глубина огибающей фильтра (+/−). Основа «квакающих» плаков и басов."),
        add ("keyTrack",   "KEY",    K::Knob,     B, u8"Слежение за клавиатурой: верхние ноты ярче."),
        add ("velFilter",  "VEL",    K::Knob,     B, u8"Чем сильнее удар по клавише — тем ярче звук.") });

    auto* fenv = section ("FILTER ENV", B, {
        add ("fA", "A", K::Fader, B, u8"Атака огибающей фильтра."),
        add ("fD", "D", K::Fader, B, u8"Спад огибающей фильтра. Короткий = «пик», длинный = «вау»."),
        add ("fS", "S", K::Fader, B, u8"Уровень удержания огибающей фильтра."),
        add ("fR", "R", K::Fader, B, u8"Затухание огибающей фильтра после отпускания.") });

    auto* aenv = section ("AMP ENV", T, {
        add ("aA", "A", K::Fader, T, u8"Атака громкости. Больше = мягкое нарастание (пэды)."),
        add ("aD", "D", K::Fader, T, u8"Спад громкости."),
        add ("aS", "S", K::Fader, T, u8"Уровень громкости пока нота удерживается. 0 = плак/перкуссия."),
        add ("aR", "R", K::Fader, T, u8"Хвост после отпускания клавиши."),
        add ("velAmp", "VEL", K::Knob, T, u8"Чувствительность громкости к силе нажатия.") });

    auto* lfo = section ("LFO", M, {
        add ("lfoShape",  "SHAPE",  K::Selector, M, u8"Форма LFO. S&H — случайные ступеньки, Drift — плавный случайный дрейф.", 1.25f),
        add ("lfoRate",   "RATE",   K::Knob, M, u8"Скорость LFO."),
        add ("lfoDelay",  "DELAY",  K::Knob, M, u8"Задержка LFO: модуляция плавно появляется после нажатия ноты (классическое отложенное вибрато)."),
        add ("lfoPitch",  "PITCH",  K::Knob, M, u8"LFO → высота тона (вибрато / вобл)."),
        add ("lfoFilter", "FILTER", K::Knob, M, u8"LFO → срез фильтра (вау-вау, wobble)."),
        add ("lfoPw",     "PWM",    K::Knob, M, u8"LFO → ширина импульса. Классический хоровой Juno-звук на Square."),
        add ("lfoAmp",    "TREM",   K::Knob, M, u8"LFO → громкость (тремоло как у электропиано).") });

    auto* voice = section ("VOICE", L, {
        add ("voiceMode", "MODE",  K::Selector, L, u8"Poly — аккорды. Mono — одна нота, перезапуск огибающих. Legato — связные ноты без перезапуска (для глайдов).", 1.25f),
        add ("glide",     "GLIDE", K::Knob, L, u8"Портаменто — скольжение между нотами."),
        add ("bendRange", "BEND",  K::Knob, L, u8"Диапазон pitch bend в полутонах."),
        add ("wheelVib",  "WHEEL", K::Knob, L, u8"Сколько вибрато добавляет колесо модуляции (CC1).") });

    auto* chorus = section ("CHORUS", T, {
        add ("chorusMode", "MODE", K::Selector, T, u8"BBD-хорус в стиле Juno: I — мягкий, II — глубже, I+II — быстрая дрожь.", 1.25f) });

    auto* tapeS = section ("TAPE", O, {
        add ("tapeWow", "WOW",  K::Knob, O, u8"Wow & flutter — плавающая высота как у старой кассеты."),
        add ("tapeSat", "SAT",  K::Knob, O, u8"Ленточная сатурация — тепло и компрессия."),
        add ("tapeAge", "AGE",  K::Knob, O, u8"«Возраст» ленты: глухие верха + шипение.") });

    auto* lofi = section ("LO-FI", B, {
        add ("crush", "CRUSH", K::Knob, B, u8"Понижение битности и частоты дискретизации — сэмплерная грязь.") });

    auto* delay = section ("ECHO", M, {
        add ("delaySync", "SYNC", K::Selector, M, u8"Синхронизация с темпом проекта. Free — время задаётся ручкой TIME.", 1.15f),
        add ("delayTime", "TIME", K::Knob, M, u8"Время задержки (в режиме Free)."),
        add ("delayFb",   "FDBK", K::Knob, M, u8"Обратная связь — количество повторов."),
        add ("delayMix",  "MIX",  K::Knob, M, u8"Громкость эха.") });

    auto* rev = section ("REVERB", T, {
        add ("revSize", "SIZE", K::Knob, T, u8"Размер пространства."),
        add ("revMix",  "MIX",  K::Knob, T, u8"Громкость реверберации.") });

    auto* out = section ("OUT", L, {
        add ("master", "VOLUME", K::Knob, L, u8"Общая громкость.") });

    rows = { { osc1, osc2, mixer }, { filter, fenv, aenv }, { lfo, voice }, { chorus, tapeS, lofi, delay, rev, out } };

    for (auto* c : controls)
        for (auto* child : c->getChildren())
            if (auto* sl = dynamic_cast<juce::Slider*> (child))
                sl->setPopupDisplayEnabled (true, true, this);

    // preset bar
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (juce::String (i + 1).paddedLeft ('0', 2) + "  " + proc.getProgramName (i), i + 1);
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedItemIndex();
        if (idx >= 0 && idx != proc.getCurrentProgram()) proc.setCurrentProgram (idx);
    };
    prevBtn.onClick = [this] { const int n = proc.getNumPrograms(); proc.setCurrentProgram ((proc.getCurrentProgram() - 1 + n) % n); };
    nextBtn.onClick = [this] { const int n = proc.getNumPrograms(); proc.setCurrentProgram ((proc.getCurrentProgram() + 1) % n); };
    presetBox.setTooltip (juce::String::fromUTF8 (u8"Заводские пресеты. Также доступны в меню пресетов FL Studio."));
    addAndMakeVisible (presetBox);
    addAndMakeVisible (prevBtn);
    addAndMakeVisible (nextBtn);
    addAndMakeVisible (scope);
    refreshPresetBox();

    setSize (kWidth, kHeight);
    startTimerHz (30);
}

DriftwoodEditor::~DriftwoodEditor() { setLookAndFeel (nullptr); }

void DriftwoodEditor::refreshPresetBox()
{
    presetBox.setSelectedItemIndex (proc.getCurrentProgram(), juce::dontSendNotification);
    lastProgramFlag = proc.programChangedFlag.load();
}

void DriftwoodEditor::timerCallback()
{
    if (proc.programChangedFlag.load() != lastProgramFlag) refreshPresetBox();
    lfoLed = proc.lfoDisplay.load();
    voicesLit = proc.activeVoices.load();
    scope.repaint();
    repaint (juce::Rectangle<int> (690, 44, 90, 20));
}

// ============================================================================
void DriftwoodEditor::layoutRow (std::vector<Section*> row, juce::Rectangle<int> area)
{
    const int gap = 10;
    float totalW = 0;
    for (auto* s : row) for (auto* c : s->items) totalW += c->weight;
    const float sectionPad = 12.0f;
    const float usable = (float) area.getWidth() - gap * (float) (row.size() - 1) - sectionPad * (float) row.size();
    const float unit = usable / totalW;

    int x = area.getX();
    for (auto* s : row)
    {
        float w = sectionPad;
        for (auto* c : s->items) w += c->weight * unit;
        s->bounds = { x, area.getY(), (int) std::round (w), area.getHeight() };
        auto inner = s->bounds.reduced (6, 0).withTrimmedTop (26).withTrimmedBottom (6);
        float cx = (float) inner.getX();
        for (auto* c : s->items)
        {
            const float cw = c->weight * unit;
            c->setBounds ((int) cx, inner.getY(), (int) cw, inner.getHeight());
            cx += cw;
        }
        x += s->bounds.getWidth() + gap;
    }
}

void DriftwoodEditor::resized()
{
    auto panel = juce::Rectangle<int> (kCheek + 12, kTop + 10, kWidth - 2 * kCheek - 24, kHeight - kTop - 20);
    const int heights[] = { 138, 178, 138, 138 };
    const int gap = (panel.getHeight() - (heights[0] + heights[1] + heights[2] + heights[3])) / 3;
    int y = panel.getY();
    for (size_t i = 0; i < rows.size(); ++i)
    {
        layoutRow (rows[i], { panel.getX(), y, panel.getWidth(), heights[i] });
        y += heights[i] + gap;
    }

    prevBtn.setBounds (430, 16, 28, 26);
    presetBox.setBounds (462, 16, 228, 26);
    nextBtn.setBounds (694, 16, 28, 26);
    scope.setBounds (kWidth - kCheek - 262, 9, 250, 54);
}

// ============================================================================
static void drawScrew (juce::Graphics& g, float x, float y)
{
    juce::ColourGradient sg (juce::Colour (0xffd9d4ca), x - 4, y - 4, juce::Colour (0xff6d675e), x + 4, y + 4, false);
    g.setGradientFill (sg);
    g.fillEllipse (x - 4.5f, y - 4.5f, 9.0f, 9.0f);
    g.setColour (juce::Colour (0xff3a352f));
    g.drawLine (x - 3.0f, y + 1.5f, x + 3.0f, y - 1.5f, 1.3f);
}

void DriftwoodEditor::drawBackground (juce::Graphics& g)
{
    // wood cheeks
    for (int side = 0; side < 2; ++side)
    {
        auto r = juce::Rectangle<float> (side == 0 ? 0.0f : (float) (kWidth - kCheek), 0.0f, (float) kCheek, (float) kHeight);
        juce::ColourGradient wood (juce::Colour (0xff6e3c22), r.getX(), 0, juce::Colour (0xff4a2614), r.getRight(), 0, false);
        wood.addColour (0.5, juce::Colour (0xff86502e));
        g.setGradientFill (wood);
        g.fillRect (r);
        juce::Random rnd (side == 0 ? 11 : 23);
        for (int i = 0; i < 26; ++i)
        {
            const float gx = r.getX() + rnd.nextFloat() * r.getWidth();
            g.setColour (juce::Colour (0xff2c160b).withAlpha (0.18f + 0.2f * rnd.nextFloat()));
            juce::Path grain;
            grain.startNewSubPath (gx, 0);
            for (int yy = 0; yy <= kHeight; yy += 40)
                grain.lineTo (gx + std::sin ((float) yy * 0.013f + (float) i) * 2.2f, (float) yy);
            g.strokePath (grain, juce::PathStrokeType (0.6f + rnd.nextFloat()));
        }
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawVerticalLine (side == 0 ? kCheek - 1 : kWidth - kCheek, 0, (float) kHeight);
    }

    // top bar
    auto top = juce::Rectangle<float> ((float) kCheek, 0, (float) (kWidth - 2 * kCheek), (float) kTop);
    g.setColour (Palette::charcoal);
    g.fillRect (top);
    // retro stripes
    const juce::Colour stripes[] = { Palette::orange, Palette::mustard, Palette::teal };
    for (int i = 0; i < 3; ++i)
    {
        g.setColour (stripes[i]);
        g.fillRect (top.getX(), top.getBottom() - 9.0f + (float) i * 3.0f, top.getWidth(), 3.0f);
    }
    g.setColour (Palette::cream);
    g.setFont (vintageFont (34.0f));
    g.drawText ("driftwood", (int) top.getX() + 22, 6, 260, 44, juce::Justification::centredLeft);
    g.setColour (Palette::amber);
    g.setFont (vintageFont (10.5f));
    g.drawText ("ANALOG DRIFT SYNTHESIZER  /  MODEL DW-1", (int) top.getX() + 24, 44, 320, 14, juce::Justification::centredLeft);

    g.setColour (Palette::cream.withAlpha (0.55f));
    g.setFont (vintageFont (10.0f));
    g.drawText ("PROGRAM", 430, 46, 60, 14, juce::Justification::centredLeft);

    // main cream panel
    auto panel = juce::Rectangle<float> ((float) kCheek, (float) kTop, (float) (kWidth - 2 * kCheek), (float) (kHeight - kTop));
    juce::ColourGradient pg (Palette::cream, 0, panel.getY(), Palette::cream.darker (0.06f), 0, panel.getBottom(), false);
    g.setGradientFill (pg);
    g.fillRect (panel);
    {
        juce::Random rnd (7);
        for (int i = 0; i < 1400; ++i)
        {
            g.setColour (Palette::ink.withAlpha (0.025f + 0.03f * rnd.nextFloat()));
            g.fillRect (panel.getX() + rnd.nextFloat() * panel.getWidth(), panel.getY() + rnd.nextFloat() * panel.getHeight(), 1.0f, 1.0f);
        }
    }
    drawScrew (g, panel.getX() + 9, panel.getY() + 9);
    drawScrew (g, panel.getRight() - 9, panel.getY() + 9);
    drawScrew (g, panel.getX() + 9, panel.getBottom() - 9);
    drawScrew (g, panel.getRight() - 9, panel.getBottom() - 9);

    // sections
    for (auto& s : sections)
    {
        auto r = s->bounds.toFloat();
        g.setColour (Palette::creamDark.withAlpha (0.55f));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (Palette::ink.withAlpha (0.55f));
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.2f);

        auto head = r.removeFromTop (20.0f).reduced (1.0f, 1.0f);
        juce::Path hp;
        hp.addRoundedRectangle (head.getX(), head.getY(), head.getWidth(), head.getHeight(), 5.0f, 5.0f, true, true, false, false);
        g.setColour (s->colour);
        g.fillPath (hp);
        g.setColour (Palette::cream);
        g.setFont (vintageFont (12.0f));
        g.drawText (s->title, head.toNearestInt().withTrimmedLeft (8), juce::Justification::centredLeft);
    }
}

void DriftwoodEditor::paint (juce::Graphics& g)
{
    if (background.isNull() || background.getWidth() != getWidth())
    {
        background = juce::Image (juce::Image::ARGB, getWidth(), getHeight(), true);
        juce::Graphics bg (background);
        drawBackground (bg);
    }
    g.drawImageAt (background, 0, 0);

    // LFO lamp + voice LEDs
    {
        g.setFont (vintageFont (10.0f));
        const float lx = 700, ly = 53;
        const float b = 0.5f + 0.5f * lfoLed;
        g.setColour (Palette::brick.withAlpha (0.25f + 0.2f * b));
        g.fillEllipse (lx - 7, ly - 7, 14, 14);
        g.setColour (juce::Colour (0xffff4a2a).withAlpha (0.25f + 0.75f * b));
        g.fillEllipse (lx - 4, ly - 4, 8, 8);
        g.setColour (Palette::cream.withAlpha (0.55f));
        g.drawText ("LFO", (int) lx + 8, (int) ly - 7, 30, 14, juce::Justification::centredLeft);
        for (int i = 0; i < 8; ++i)
        {
            g.setColour (i < voicesLit ? Palette::amber : Palette::amber.withAlpha (0.15f));
            g.fillEllipse (742.0f + (float) i * 4.5f, ly - 1.5f, 3.0f, 3.0f);
        }
    }

}
