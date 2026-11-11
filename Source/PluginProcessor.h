// SYNTHI PRO — плагин: связывает звуковой движок с хостом (VST3 / Standalone), хранит состояние и пресеты.
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "dsp/Engine.h"

inline juce::String U8 (const char* s) { return juce::String::fromUTF8 (s); }

struct PresetInfo
{
    juce::String name, category;
    bool factory = true;
    int factoryIndex = -1;
    juce::File file;
};

class SynthiProProcessor;

// Библиотека пресетов: заводские (встроены в бинарник) и пользовательские (файлы в «Документы/SYNTHI PRO/Presets»).
class PresetManager
{
public:
    explicit PresetManager (SynthiProProcessor& p);

    void rescan();
    const std::vector<PresetInfo>& list() const { return all; }
    juce::StringArray categories() const;
    void load (int index);
    bool saveUser (const juce::String& name, const juce::String& category);
    bool deleteUser (int index);
    void randomPatch();
    void initPatch();

    bool isFavorite (const juce::String& name) const { return favorites.contains (name); }
    void toggleFavorite (const juce::String& name);
    int indexOf (const juce::String& name) const;

    static juce::File userDir();
    static juce::StringArray factoryCategories();

private:
    void setPlain (const juce::String& id, float value);
    void resetAll();
    void loadFavorites();
    void saveFavorites();

    SynthiProProcessor& proc;
    std::vector<PresetInfo> all;
    juce::StringArray favorites;
};

class SynthiProProcessor : public juce::AudioProcessor
{
public:
    SynthiProProcessor();
    ~SynthiProProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SYNTHI PRO"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Настройки интерфейса хранятся вместе с состоянием плагина (и в проекте DAW, и в автономной версии).
    juce::String getUiProp (const juce::String& key, const juce::String& def) const;
    void setUiProp (const juce::String& key, const juce::String& value);

    juce::AudioProcessorValueTreeState apvts;
    sp::Engine engine;
    juce::MidiKeyboardState keyboardState;
    PresetManager presets;

    std::atomic<float> uiBend { 0.0f }, uiMod { 0.0f };
    std::atomic<bool> panicFlag { false };
    std::atomic<int> stateVersion { 0 };              // увеличивается при загрузке состояния — редактор пересобирает интерфейс

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void handleMidi (const juce::MidiMessage& m);

    std::atomic<float>* raw[sp::P_COUNT] = {};
    float lastUiBend = 0.0f, lastUiMod = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthiProProcessor)
};
