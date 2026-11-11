#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FactoryPresets.h"

using namespace sp;

//======================================================================================================
static juce::String formatValue (float v, const juce::String& unit)
{
    const float a = std::abs (v);
    juce::String s = a >= 100.0f ? juce::String (juce::roundToInt (v)) : (a >= 10.0f ? juce::String (v, 1) : (a >= 1.0f ? juce::String (v, 2) : juce::String (v, 3)));
    return unit.isEmpty() ? s : s + " " + unit;
}

juce::AudioProcessorValueTreeState::ParameterLayout SynthiProProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (const auto& d : paramDefs())
    {
        const juce::ParameterID pid { juce::String (d.id), 1 };
        const juce::String name = U8 (d.en.c_str());
        const juce::String unit = U8 (d.unit.c_str());

        switch (d.type)
        {
            case PT::Bool:
                layout.add (std::make_unique<juce::AudioParameterBool> (pid, name, d.def > 0.5f));
                break;
            case PT::Choice:
                layout.add (std::make_unique<juce::AudioParameterChoice> (pid, name, juce::StringArray::fromTokens (juce::String (d.choices), "|", ""), (int) d.def));
                break;
            case PT::Int:
                layout.add (std::make_unique<juce::AudioParameterInt> (pid, name, (int) d.min, (int) d.max, (int) d.def));
                break;
            case PT::Float:
            case PT::Exp:
            case PT::Log:
            {
                juce::NormalisableRange<float> range (d.min, d.max);
                if (d.type == PT::Exp)
                    range = juce::NormalisableRange<float> (d.min, d.max, 0.0f, 1.0f / 3.0f);
                else if (d.type == PT::Log)
                    range = juce::NormalisableRange<float> (d.min, d.max,
                        [] (float s, float e, float n) { return s * std::pow (e / s, n); },
                        [] (float s, float e, float v) { return std::log (juce::jmax (v, s) / s) / std::log (e / s); },
                        [] (float s, float e, float v) { return juce::jlimit (s, e, v); });

                auto attr = juce::AudioParameterFloatAttributes()
                                .withStringFromValueFunction ([unit] (float v, int) { return formatValue (v, unit); })
                                .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); });
                layout.add (std::make_unique<juce::AudioParameterFloat> (pid, name, range, d.def, attr));
                break;
            }
        }
    }
    return layout;
}

//======================================================================================================
SynthiProProcessor::SynthiProProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "SynthiProState", createLayout()),
      presets (*this)
{
    const auto& defs = paramDefs();
    for (int i = 0; i < P_COUNT; ++i)
    {
        raw[i] = apvts.getRawParameterValue (juce::String (defs[(size_t) i].id));
        jassert (raw[i] != nullptr);
    }
    presets.rescan();
    if (! presets.list().empty())
    {
        presets.load (0);
        setUiProp ("preset", presets.list()[0].name);
    }
}

void SynthiProProcessor::prepareToPlay (double sampleRate, int)
{
    for (int i = 0; i < P_COUNT; ++i)
        engine.setParam (i, raw[i]->load());
    engine.prepare (sampleRate);
    keyboardState.reset();
}

bool SynthiProProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void SynthiProProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())                engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())          engine.noteOff (m.getNoteNumber());
    else if (m.isPitchWheel())       engine.pitchBend ((float) (m.getPitchWheelValue() - 8192) / 8192.0f);
    else if (m.isChannelPressure())  engine.aftertouch ((float) m.getChannelPressureValue() / 127.0f);
    else if (m.isAftertouch())       engine.aftertouch ((float) m.getAfterTouchValue() / 127.0f);
    else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.allNotesOff();
    else if (m.isController())
    {
        const int cc = m.getControllerNumber();
        const float v = (float) m.getControllerValue() / 127.0f;
        if (cc == 1)        engine.modWheel (v);
        else if (cc == 64)  engine.sustain (v >= 0.5f);
    }
}

void SynthiProProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || n <= 0) { buffer.clear(); return; }

    keyboardState.processNextMidiBuffer (midi, 0, n, true);       // экранная клавиатура -> MIDI

    for (int i = 0; i < P_COUNT; ++i)
        engine.setParam (i, raw[i]->load (std::memory_order_relaxed));

    double bpm = 0.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                bpm = *b;
    engine.setHostTempo (bpm);

    if (panicFlag.exchange (false)) engine.panic();
    const float ub = uiBend.load(), um = uiMod.load();
    if (ub != lastUiBend) { lastUiBend = ub; engine.pitchBend (ub); }
    if (um != lastUiMod)  { lastUiMod = um;  engine.modWheel (um); }

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);
    int done = 0;
    for (const auto meta : midi)
    {
        const int t = juce::jlimit (0, n, meta.samplePosition);
        if (t > done) { engine.process (L + done, R + done, t - done); done = t; }
        handleMidi (meta.getMessage());
    }
    if (done < n) engine.process (L + done, R + done, n - done);

    for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, n);
}

juce::AudioProcessorEditor* SynthiProProcessor::createEditor() { return new SynthiProEditor (*this); }

//======================================================================================================
void SynthiProProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SynthiProProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            ++stateVersion;
        }
}

juce::String SynthiProProcessor::getUiProp (const juce::String& key, const juce::String& def) const
{
    return apvts.state.getProperty (juce::Identifier ("ui_" + key), def).toString();
}

void SynthiProProcessor::setUiProp (const juce::String& key, const juce::String& value)
{
    apvts.state.setProperty (juce::Identifier ("ui_" + key), value, nullptr);
}

//======================================================================================================
PresetManager::PresetManager (SynthiProProcessor& p) : proc (p) { loadFavorites(); }

juce::File PresetManager::userDir()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("SYNTHI PRO").getChildFile ("Presets");
}

juce::StringArray PresetManager::factoryCategories()
{
    juce::StringArray c;
    for (int i = 0; i < kNumFactoryPresets; ++i)
        c.addIfNotAlreadyThere (kFactoryPresets[i].category);
    return c;
}

juce::StringArray PresetManager::categories() const
{
    auto c = factoryCategories();
    for (const auto& p : all)
        c.addIfNotAlreadyThere (p.category);
    return c;
}

void PresetManager::rescan()
{
    all.clear();
    for (int i = 0; i < kNumFactoryPresets; ++i)
    {
        PresetInfo p;
        p.name = kFactoryPresets[i].name; p.category = kFactoryPresets[i].category; p.factory = true; p.factoryIndex = i;
        all.push_back (p);
    }
    const auto dir = userDir();
    if (dir.isDirectory())
    {
        auto files = dir.findChildFiles (juce::File::findFiles, false, "*.synthipreset");
        files.sort();
        for (const auto& f : files)
            if (auto xml = juce::XmlDocument::parse (f))
                if (xml->hasTagName ("SynthiProPreset"))
                {
                    PresetInfo p;
                    p.name = xml->getStringAttribute ("name", f.getFileNameWithoutExtension());
                    p.category = xml->getStringAttribute ("category", "User");
                    p.factory = false; p.file = f;
                    all.push_back (p);
                }
    }
}

int PresetManager::indexOf (const juce::String& name) const
{
    for (int i = 0; i < (int) all.size(); ++i)
        if (all[(size_t) i].name == name) return i;
    return -1;
}

void PresetManager::setPlain (const juce::String& id, float value)
{
    if (auto* p = proc.apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void PresetManager::resetAll()
{
    for (const auto& d : paramDefs())
        setPlain (juce::String (d.id), d.def);
}

void PresetManager::initPatch()
{
    proc.panicFlag = true;
    resetAll();
    proc.setUiProp ("preset", "Init");
}

void PresetManager::load (int index)
{
    if (index < 0 || index >= (int) all.size()) return;
    const auto& info = all[(size_t) index];
    proc.panicFlag = true;
    resetAll();

    if (info.factory)
    {
        juce::StringArray items;
        items.addTokens (juce::String (kFactoryPresets[info.factoryIndex].data), ";", "");
        for (const auto& it : items)
            if (it.containsChar ('='))
                setPlain (it.upToFirstOccurrenceOf ("=", false, false), it.fromFirstOccurrenceOf ("=", false, false).getFloatValue());
    }
    else if (auto xml = juce::XmlDocument::parse (info.file))
    {
        for (auto* e : xml->getChildWithTagNameIterator ("p"))
            setPlain (e->getStringAttribute ("id"), (float) e->getDoubleAttribute ("v"));
    }
    proc.setUiProp ("preset", info.name);
}

bool PresetManager::saveUser (const juce::String& name, const juce::String& category)
{
    const auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty()) return false;
    const auto dir = userDir();
    if (! dir.isDirectory() && ! dir.createDirectory().wasOk()) return false;

    juce::XmlElement xml ("SynthiProPreset");
    xml.setAttribute ("name", name.trim());
    xml.setAttribute ("category", category.isEmpty() ? juce::String ("User") : category);
    xml.setAttribute ("version", 1);
    for (const auto& d : paramDefs())
        if (auto* p = proc.apvts.getParameter (juce::String (d.id)))
        {
            auto* e = xml.createNewChildElement ("p");
            e->setAttribute ("id", juce::String (d.id));
            e->setAttribute ("v", (double) p->convertFrom0to1 (p->getValue()));
        }
    if (! xml.writeTo (dir.getChildFile (clean + ".synthipreset"))) return false;
    proc.setUiProp ("preset", name.trim());
    rescan();
    return true;
}

bool PresetManager::deleteUser (int index)
{
    if (index < 0 || index >= (int) all.size() || all[(size_t) index].factory) return false;
    const bool ok = all[(size_t) index].file.moveToTrash();
    rescan();
    return ok;
}

void PresetManager::loadFavorites()
{
    favorites.clear();
    const auto f = userDir().getSiblingFile ("favorites.txt");
    if (f.existsAsFile()) favorites.addLines (f.loadFileAsString());
    favorites.removeEmptyStrings();
}

void PresetManager::saveFavorites()
{
    const auto f = userDir().getSiblingFile ("favorites.txt");
    f.getParentDirectory().createDirectory();
    f.replaceWithText (favorites.joinIntoString ("\n"));
}

void PresetManager::toggleFavorite (const juce::String& name)
{
    if (favorites.contains (name)) favorites.removeString (name); else favorites.add (name);
    saveFavorites();
}

// Случайный патч с музыкально осмысленными ограничениями: выбирается архетип (бас / соло / пэд / эффект),
// строй генераторов берётся только из консонансных интервалов, глубины модуляции и эффекты — в разумных пределах.
void PresetManager::randomPatch()
{
    auto& r = juce::Random::getSystemRandom();
    auto rnd = [&r] (float a, float b) { return a + (b - a) * r.nextFloat(); };
    auto pick = [&r] (std::initializer_list<float> l) { return *(l.begin() + r.nextInt ((int) l.size())); };
    auto chance = [&r] (float p) { return r.nextFloat() < p; };

    proc.panicFlag = true;
    resetAll();
    const int type = r.nextInt (4);               // 0 бас, 1 соло, 2 пэд, 3 эффект/текстура
    static const char* names[4] = { "Random Bass", "Random Lead", "Random Pad", "Random Texture" };

    setPlain ("voice_mode", type == 0 ? 1.0f : (type == 1 ? 2.0f : 0.0f));
    setPlain ("glide", type == 1 ? rnd (0.02f, 0.15f) : 0.0f);
    setPlain ("drift", rnd (0.05f, type >= 2 ? 0.7f : 0.3f));

    const float baseOct = type == 0 ? -2.0f : (type == 2 ? pick ({ -1, 0, 0 }) : pick ({ -1, 0, 0, 1 }));
    for (int i = 1; i <= 3; ++i)
    {
        const auto o = "o" + juce::String (i) + "_";
        setPlain (o + "wave", (float) r.nextInt (5));
        setPlain (o + "oct", juce::jlimit (-3.0f, 3.0f, baseOct + (i == 1 ? 0.0f : pick ({ 0, 0, 0, 1, -1 }))));
        setPlain (o + "semi", i == 1 ? 0.0f : pick ({ 0, 0, 0, 7, 12, -12, 5, 0 }));
        setPlain (o + "fine", i == 1 ? 0.0f : rnd (-14.0f, 14.0f));
        setPlain (o + "pw", rnd (0.15f, 0.85f));
        setPlain (o + "level", i == 1 ? rnd (0.6f, 0.9f) : (chance (i == 2 ? 0.75f : 0.4f) ? rnd (0.3f, 0.8f) : 0.0f));
    }
    if (chance (0.2f)) { setPlain ("o2_sync", 1.0f); setPlain ("o2_semi", pick ({ 7, 12, 19 })); setPlain ("o2_level", 0.8f); }
    if (chance (0.2f)) setPlain ("o1_fm", rnd (0.2f, 0.6f));
    if (chance (0.25f)) setPlain ("ring_level", rnd (0.2f, 0.6f));
    if (chance (type == 3 ? 0.6f : 0.2f)) { setPlain ("noise_level", rnd (0.1f, type == 3 ? 0.7f : 0.3f)); setPlain ("noise_color", r.nextFloat()); }

    setPlain ("f_mode", type == 3 ? (float) r.nextInt (6) : pick ({ 0, 0, 1, 2, 3 }));
    const float cut = type == 0 ? rnd (120.0f, 600.0f) : (type == 2 ? rnd (400.0f, 2500.0f) : rnd (500.0f, 5000.0f));
    setPlain ("f_cut", cut);
    setPlain ("f_res", rnd (0.05f, type == 3 ? 0.85f : 0.65f));
    setPlain ("f_env", type == 2 ? rnd (0.0f, 0.4f) : rnd (0.2f, 0.7f));
    setPlain ("f_key", rnd (0.2f, 0.6f));
    setPlain ("f_drive", chance (0.3f) ? rnd (0.2f, 0.7f) : 0.0f);

    const bool slow = type >= 2;
    setPlain ("e1_a", slow ? rnd (0.3f, 2.0f) : rnd (0.001f, 0.02f));
    setPlain ("e1_d", rnd (0.15f, 1.2f));
    setPlain ("e1_s", type == 0 ? rnd (0.3f, 0.9f) : rnd (0.6f, 1.0f));
    setPlain ("e1_r", slow ? rnd (0.8f, 3.0f) : rnd (0.08f, 0.4f));
    setPlain ("e2_a", slow ? rnd (0.3f, 3.0f) : rnd (0.001f, 0.05f));
    setPlain ("e2_d", rnd (0.1f, 1.5f));
    setPlain ("e2_s", rnd (0.0f, 0.6f));
    setPlain ("e2_r", slow ? rnd (0.8f, 3.0f) : rnd (0.1f, 0.4f));

    setPlain ("l1_wave", pick ({ 0, 0, 1, 5 })); setPlain ("l1_rate", type == 1 ? rnd (4.5f, 6.5f) : rnd (0.1f, 6.0f));
    setPlain ("l2_wave", (float) r.nextInt (6)); setPlain ("l2_rate", rnd (0.05f, slow ? 0.6f : 3.0f));
    setPlain ("sh_rate", rnd (2.0f, 12.0f)); setPlain ("sh_lag", chance (0.5f) ? rnd (0.0f, 0.4f) : 0.0f);

    // Матрица: несколько маршрутов из проверенного набора (источник, приёмник, мин., макс. глубина).
    struct Route { const char* id; float lo, hi; };
    static const Route routes[] = {
        { "mx_lfo1_p1", 0.05f, 0.1f }, { "mx_lfo1_p2", 0.05f, 0.1f }, { "mx_lfo2_cut", 0.2f, 0.6f }, { "mx_lfo2_pw1", 0.2f, 0.5f }, { "mx_lfo2_pw2", 0.2f, 0.5f },
        { "mx_lfo2_pan", 0.3f, 0.8f }, { "mx_sh_cut", 0.2f, 0.5f }, { "mx_sh_pan", 0.3f, 0.8f }, { "mx_env3_cut", 0.2f, 0.6f }, { "mx_env2_p2", 0.1f, 0.4f },
        { "mx_mod_cut", 0.3f, 0.6f }, { "mx_vel_cut", 0.2f, 0.6f }, { "mx_at_cut", 0.3f, 0.6f }, { "mx_lfo1_vca", 0.2f, 0.6f }, { "mx_vco3_cut", 0.1f, 0.35f },
        { "mx_vco2_p1", 0.1f, 0.4f }, { "mx_noise_cut", 0.1f, 0.3f }, { "mx_lfo2_res", 0.2f, 0.4f }, { "mx_key_pan", 0.3f, 0.7f }, { "mx_lfo2_lr", 0.2f, 0.5f }
    };
    const int nRoutes = type == 3 ? 3 + r.nextInt (4) : 2 + r.nextInt (3);
    for (int i = 0; i < nRoutes; ++i)
    {
        const auto& rt = routes[r.nextInt ((int) (sizeof (routes) / sizeof (routes[0])))];
        if (type < 2 && juce::String (rt.id).startsWith ("mx_vco")) continue;       // бас и соло оставляем в строе
        setPlain (rt.id, rnd (rt.lo, rt.hi) * (chance (0.2f) ? -1.0f : 1.0f));
    }

    if (chance (type == 2 ? 0.7f : 0.3f)) { setPlain ("cho_on", 1.0f); setPlain ("cho_mix", rnd (0.3f, 0.6f)); }
    if (chance (0.25f)) { setPlain ("pha_on", 1.0f); setPlain ("pha_rate", rnd (0.05f, 0.6f)); }
    if (chance (type == 0 ? 0.2f : 0.6f)) { setPlain ("dly_on", 1.0f); setPlain ("dly_time", pick ({ 0.25f, 0.33f, 0.375f, 0.5f })); setPlain ("dly_mix", rnd (0.15f, 0.35f)); }
    if (chance (type == 0 ? 0.15f : 0.75f)) { setPlain ("rev_on", 1.0f); setPlain ("rev_size", rnd (0.4f, 0.9f)); setPlain ("rev_mix", rnd (0.15f, type >= 2 ? 0.5f : 0.3f)); }
    if (chance (type == 0 ? 0.5f : 0.2f)) { setPlain ("sat_on", 1.0f); setPlain ("sat_drive", rnd (0.2f, 0.6f)); }
    setPlain ("master_vol", 0.55f);

    proc.setUiProp ("preset", names[type]);
}

//======================================================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SynthiProProcessor(); }
