// SYNTHI PRO — редактор (интерфейс). Базовая раскладка 1280x800, масштабируется вместе с окном.
#pragma once
#include "PluginProcessor.h"

struct Lang { static bool ru; };
inline juce::String TR (const char* en, const char* ru) { return Lang::ru ? juce::String::fromUTF8 (ru) : juce::String (en); }

class SynthiProEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    enum { BaseW = 1280, BaseH = 800 };

    explicit SynthiProEditor (SynthiProProcessor&);
    ~SynthiProEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void requestRebuild() { needRebuild = true; }       // пересборка интерфейса (смена языка / режима) — выполняется из таймера
    void setScale (double s);

    SynthiProProcessor& proc;

private:
    void timerCallback() override;
    void rebuild();

    class Content;
    std::unique_ptr<juce::LookAndFeel_V4> look;
    std::unique_ptr<juce::TooltipWindow> tooltip;
    std::unique_ptr<Content> content;
    bool needRebuild = false, focusGrabbed = false;
    int seenState = 0;

    // Съёмка экранов для документации: если задана переменная окружения SYNTHIPRO_SHOTS=<папка>,
    // редактор сохраняет туда снимки интерфейса в разных состояниях, восстанавливает состояние и закрывает программу.
    void shotTick();
    void saveShot (const juce::String& name);
    void shotLog (const juce::String& line);
    juce::String shotMap;
    juce::String shotDir;
    int shotStep = 0, shotWait = 0;
    juce::MemoryBlock shotSaved;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthiProEditor)
};
