// Offline test: renders every preset to WAV, checks for NaN / clipping, snapshots the GUI.
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Source/PluginProcessor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

static void addNote (juce::MidiBuffer& m, int note, double start, double len, float vel, double sr)
{
    m.addEvent (juce::MidiMessage::noteOn (1, note, vel), (int) (start * sr));
    m.addEvent (juce::MidiMessage::noteOff (1, note), (int) ((start + len) * sr));
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const double sr = 48000.0; const int block = 256;
    const juce::File outDir (argc > 1 ? argv[1] : "renders");
    outDir.createDirectory();

    std::unique_ptr<DriftwoodProcessor> proc (dynamic_cast<DriftwoodProcessor*> (createPluginFilter()));
    proc->setPlayConfigDetails (0, 2, sr, block);
    bool ok = true;

    for (int prog = 0; prog < proc->getNumPrograms(); ++prog)
    {
        proc->setCurrentProgram (prog);
        proc->prepareToPlay (sr, block);
        const bool mono = proc->apvts.getRawParameterValue ("voiceMode")->load() > 0.5f;

        const double total = 6.0;
        juce::MidiBuffer all;
        if (mono)
        {   // little bass/lead line
            const int seq[] = { 48, 51, 55, 58, 60, 58, 55, 53 };
            for (int i = 0; i < 8; ++i) addNote (all, seq[i] + (proc->getProgramName (prog).contains ("Bass") || proc->getProgramName (prog).contains ("Wobbler") ? -12 : 12), i * 0.38, 0.42, 0.6f + 0.05f * (float) (i % 3), sr);
        }
        else
        {   // jazzy voicings: Dm9 -> G13 -> Cmaj9
            const int c1[] = { 50, 53, 57, 60, 64 }, c2[] = { 43, 53, 57, 59, 64 }, c3[] = { 48, 52, 55, 59, 62 };
            for (int n : c1) addNote (all, n, 0.0, 1.4, 0.75f, sr);
            for (int n : c2) addNote (all, n, 1.6, 1.4, 0.7f, sr);
            for (int n : c3) addNote (all, n, 3.2, 1.6, 0.8f, sr);
        }

        const int totalSamples = (int) (total * sr);
        juce::AudioBuffer<float> out (2, totalSamples);
        juce::AudioBuffer<float> buf (2, block);
        float peak = 0; double sumSq = 0; bool nan = false;
        for (int pos = 0; pos < totalSamples; pos += block)
        {
            const int n = juce::jmin (block, totalSamples - pos);
            buf.setSize (2, n, false, false, true);
            juce::MidiBuffer mb;
            for (const auto meta : all)
                if (meta.samplePosition >= pos && meta.samplePosition < pos + n)
                    mb.addEvent (meta.getMessage(), meta.samplePosition - pos);
            proc->processBlock (buf, mb);
            for (int c = 0; c < 2; ++c)
            {
                out.copyFrom (c, pos, buf, c, 0, n);
                for (int i = 0; i < n; ++i)
                {
                    const float v = buf.getSample (c, i);
                    if (! std::isfinite (v)) nan = true;
                    peak = juce::jmax (peak, std::abs (v)); sumSq += v * v;
                }
            }
        }
        const double rmsDb = 10.0 * std::log10 (sumSq / (2.0 * totalSamples) + 1e-12);
        std::printf ("%-2d %-18s peak %.3f  rms %6.1f dB %s\n", prog, proc->getProgramName (prog).toRawUTF8(), peak, rmsDb, nan ? "NaN!!" : "");
        if (nan || peak < 0.01f) ok = false;

        auto file = outDir.getChildFile (juce::String (prog).paddedLeft ('0', 2) + "_" + proc->getProgramName (prog).replaceCharacter (' ', '_') + ".wav");
        file.deleteFile();
        juce::WavAudioFormat wav;
        if (auto stream = file.createOutputStream())
            if (auto* w = wav.createWriterFor (stream.get(), sr, 2, 24, {}, 0))
            {
                stream.release();
                std::unique_ptr<juce::AudioFormatWriter> writer (w);
                writer->writeFromAudioSampleBuffer (out, 0, totalSamples);
            }
    }

    // state round-trip
    {
        proc->setCurrentProgram (4);
        juce::MemoryBlock mb; proc->getStateInformation (mb);
        proc->setCurrentProgram (0);
        proc->setStateInformation (mb.getData(), (int) mb.getSize());
        const float c = proc->apvts.getRawParameterValue ("cutoff")->load();
        std::printf ("state restore cutoff=%.1f (expect 220) program=%d\n", c, proc->getCurrentProgram());
    }

    // GUI snapshot
    if (argc > 2)
    {
        proc->setCurrentProgram (1);
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc->createEditor());
        // put something on the scope
        juce::AudioBuffer<float> b (2, 1024); juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::noteOn (1, 57, 0.8f), 0);
        proc->prepareToPlay (sr, 1024); proc->processBlock (b, mb); juce::MidiBuffer e; proc->processBlock (b, e);
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
        juce::File png (argv[2]); png.deleteFile();
        juce::PNGImageFormat fmt;
        if (auto s = png.createOutputStream()) fmt.writeImageToStream (img, *s);
        std::printf ("GUI snapshot %dx%d\n", img.getWidth(), img.getHeight());
    }
    return ok ? 0 : 1;
}
