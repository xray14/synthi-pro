#include "PluginEditor.h"

using namespace sp;
using APVTS = juce::AudioProcessorValueTreeState;

bool Lang::ru = false;

namespace
{
const juce::Colour cBg (0xff15181c), cPanel (0xff21272d), cPanel2 (0xff2a3138), cLine (0xff3b454e), cInk (0xffe9e4d6), cDim (0xff8f99a1);
const juce::Colour cAmber (0xfff0b24a), cTeal (0xff4fc3b0), cRed (0xffe2685f), cBlue (0xff6aa9e0), cGreen (0xff8fd46a), cViolet (0xffb490e6), cPink (0xffdc6fb4);

juce::Font fnt (float h, bool bold = false)
{
   #if JUCE_MAJOR_VERSION >= 8
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
   #else
    return juce::Font (h, bold ? juce::Font::bold : juce::Font::plain);
   #endif
}

juce::String fullName (const juce::String& id)
{
    const int i = paramIndex (id.toStdString());
    if (i < 0) return id;
    const auto& d = paramDefs()[(size_t) i];
    return juce::String::fromUTF8 (Lang::ru ? d.ru.c_str() : d.en.c_str());
}

float plainValue (juce::RangedAudioParameter* p) { return p != nullptr ? p->convertFrom0to1 (p->getValue()) : 0.0f; }
void setPlainValue (juce::RangedAudioParameter* p, float v) { if (p != nullptr) p->setValueNotifyingHost (p->convertTo0to1 (v)); }

//======================================================================================================
class SpLook : public juce::LookAndFeel_V4
{
public:
    SpLook()
    {
        setColour (juce::ResizableWindow::backgroundColourId, cBg);
        setColour (juce::Label::textColourId, cInk);
        setColour (juce::ComboBox::backgroundColourId, cPanel2);
        setColour (juce::ComboBox::outlineColourId, cLine);
        setColour (juce::ComboBox::textColourId, cInk);
        setColour (juce::ComboBox::arrowColourId, cDim);
        setColour (juce::PopupMenu::backgroundColourId, cPanel2);
        setColour (juce::PopupMenu::textColourId, cInk);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, cAmber);
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
        setColour (juce::TextButton::buttonColourId, cPanel2);
        setColour (juce::TextButton::buttonOnColourId, cAmber);
        setColour (juce::TextButton::textColourOffId, cInk);
        setColour (juce::TextButton::textColourOnId, juce::Colours::black);
        setColour (juce::TextEditor::backgroundColourId, cBg);
        setColour (juce::TextEditor::textColourId, cInk);
        setColour (juce::TextEditor::outlineColourId, cLine);
        setColour (juce::ListBox::backgroundColourId, cBg);
        setColour (juce::ListBox::outlineColourId, cLine);
        setColour (juce::Slider::backgroundColourId, cBg);
        setColour (juce::Slider::trackColourId, cAmber);
        setColour (juce::Slider::thumbColourId, cInk);
        setColour (juce::BubbleComponent::backgroundColourId, cPanel2);
        setColour (juce::TooltipWindow::backgroundColourId, cPanel2);
        setColour (juce::TooltipWindow::textColourId, cInk);
        setColour (juce::AlertWindow::backgroundColourId, cPanel);
        setColour (juce::AlertWindow::textColourId, cInk);
        setColour (juce::ScrollBar::thumbColourId, cLine);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s) override
    {
        const auto col = s.findColour (juce::Slider::rotarySliderFillColourId);
        const float size = (float) juce::jmin (w, h) - 6.0f, cx = (float) x + (float) w * 0.5f, cy = (float) y + (float) h * 0.5f, r = size * 0.5f;
        const float ang = a0 + pos * (a1 - a0);
        juce::Path track; track.addCentredArc (cx, cy, r + 1.5f, r + 1.5f, 0.0f, a0, a1, true);
        g.setColour (cBg); g.strokePath (track, juce::PathStrokeType (2.5f));
        juce::Path val; val.addCentredArc (cx, cy, r + 1.5f, r + 1.5f, 0.0f, s.getMinimum() < 0.0 && s.getMaximum() > 0.0 ? (a0 + a1) * 0.5f : a0, ang, true);
        g.setColour (col); g.strokePath (val, juce::PathStrokeType (2.5f));
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5a646c), cx - r * 0.4f, cy - r * 0.5f, juce::Colour (0xff272d33), cx + r * 0.5f, cy + r * 0.7f, true));
        g.fillEllipse (cx - r + 3.0f, cy - r + 3.0f, (r - 3.0f) * 2.0f, (r - 3.0f) * 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f)); g.drawEllipse (cx - r + 3.0f, cy - r + 3.0f, (r - 3.0f) * 2.0f, (r - 3.0f) * 2.0f, 1.0f);
        juce::Path ptr; ptr.addRoundedRectangle (-1.4f, -(r - 4.0f), 2.8f, (r - 4.0f) * 0.62f, 1.2f);
        g.setColour (cInk); g.fillPath (ptr, juce::AffineTransform::rotation (ang).translated (cx, cy));
    }
};

//======================================================================================================
// Матрица коммутации: строки — источники, столбцы — приёмники, в каждой точке — глубина (-1..+1).
class MatrixView : public juce::Component
{
public:
    enum { LW = 84, TH = 70, C = 20 };
    std::function<void (const juce::String&)> onStatus;

    explicit MatrixView (SynthiProProcessor& p)
    {
        const auto& d = paramDefs();
        for (int i = 0; i < NSRC * NDST; ++i)
            par[i] = p.apvts.getParameter (juce::String (d[(size_t) (P_Mx0 + i)].id));
        setSize (LW + NDST * C + 4, TH + NSRC * C + 4);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff171c20));
        static const juce::Colour srcCol[NSRC] = { cAmber, cAmber, cAmber, juce::Colour (0xffc8ccd0), cRed, cTeal, cTeal, cPink, cGreen, cGreen, cGreen, cInk, cInk, cBlue, cBlue, cViolet };
        static const juce::Colour dstCol[NDST] = { cAmber, cAmber, cAmber, cAmber, cAmber, cAmber, cInk, cInk, cInk, cInk, cInk, cBlue, cBlue, cBlue, cGreen, cGreen };
        for (int r = 0; r < NSRC; ++r) if ((r / 2) % 2 == 0) { g.setColour (juce::Colour (0xff1d2429)); g.fillRect (0, TH + r * C, getWidth(), C); }
        for (int c = 0; c < NDST; ++c) if ((c / 3) % 2 == 1) { g.setColour (juce::Colours::white.withAlpha (0.03f)); g.fillRect (LW + c * C, 0, C, getHeight()); }
        if (hr >= 0)
        {
            g.setColour (cAmber.withAlpha (0.16f));
            g.fillRect (0, TH + hr * C, getWidth(), C); g.fillRect (LW + hc * C, 0, C, getHeight());
        }
        g.setFont (fnt (10.5f));
        const auto sn = srcNames (Lang::ru), dn = dstNames (Lang::ru);
        for (int r = 0; r < NSRC; ++r)
        {
            g.setColour (r == hr ? juce::Colours::white : srcCol[r]);
            g.drawText (juce::String::fromUTF8 (sn[r]), 2, TH + r * C, LW - 7, C, juce::Justification::centredRight);
        }
        for (int c = 0; c < NDST; ++c)
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, (float) (LW + c * C + C / 2), (float) (TH - 4)));
            g.setColour (c == hc ? juce::Colours::white : dstCol[c]);
            g.drawText (juce::String::fromUTF8 (dn[c]), LW + c * C + C / 2, TH - 4 - C / 2, TH - 6, C, juce::Justification::centredLeft);
        }
        for (int r = 0; r < NSRC; ++r)
            for (int c = 0; c < NDST; ++c)
            {
                const float v = plainValue (par[r * NDST + c]), x = (float) (LW + c * C) + C * 0.5f, y = (float) (TH + r * C) + C * 0.5f;
                if (std::abs (v) > 0.004f)
                {
                    const float rad = 3.0f + 5.5f * std::sqrt (std::abs (v));
                    g.setColour (v > 0 ? cAmber : cBlue); g.fillEllipse (x - rad, y - rad, rad * 2, rad * 2);
                    g.setColour (juce::Colours::black); g.drawEllipse (x - rad, y - rad, rad * 2, rad * 2, 1.0f);
                }
                else { g.setColour (juce::Colour (0xff0a0d0f)); g.fillEllipse (x - 2.2f, y - 2.2f, 4.4f, 4.4f); }
            }
    }

    void mouseMove (const juce::MouseEvent& e) override { setHover (e); }
    void mouseExit (const juce::MouseEvent&) override { hr = hc = -1; repaint(); if (onStatus) onStatus ({}); }

    void mouseDown (const juce::MouseEvent& e) override
    {
        setHover (e); moved = false; active = hr >= 0;
        if (! active) return;
        auto* p = par[hr * NDST + hc];
        startVal = plainValue (p); wasSet = std::abs (startVal) > 0.004f;
        p->beginChangeGesture();
        if (e.mods.isPopupMenu()) { setPlainValue (p, 0.0f); removeOnly = true; }
        else { removeOnly = false; if (! wasSet) { startVal = 0.5f; setPlainValue (p, 0.5f); } }
        report(); repaint();
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! active || removeOnly) return;
        if (std::abs (e.getDistanceFromDragStartY()) > 3) moved = true;
        if (! moved) return;
        float v = juce::jlimit (-1.0f, 1.0f, startVal - (float) e.getDistanceFromDragStartY() / 110.0f);
        if (std::abs (v) < 0.02f) v = 0.0f;
        setPlainValue (par[hr * NDST + hc], v); report(); repaint();
    }
    void mouseUp (const juce::MouseEvent&) override
    {
        if (! active) return;
        auto* p = par[hr * NDST + hc];
        if (! moved && wasSet && ! removeOnly) setPlainValue (p, 0.0f);       // щелчок по установленному штырьку снимает его
        p->endChangeGesture(); active = false; report(); repaint();
    }
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        setHover (e); if (hr < 0) return;
        auto* p = par[hr * NDST + hc];
        float v = juce::jlimit (-1.0f, 1.0f, plainValue (p) + (w.deltaY > 0 ? 0.05f : -0.05f));
        if (std::abs (v) < 0.02f) v = 0.0f;
        p->beginChangeGesture(); setPlainValue (p, v); p->endChangeGesture(); report(); repaint();
    }

    int activeRoutes() const { int n = 0; for (auto* p : par) if (std::abs (plainValue (p)) > 0.004f) ++n; return n; }
    void clearAll() { for (auto* p : par) if (std::abs (plainValue (p)) > 0.004f) setPlainValue (p, 0.0f); repaint(); }

private:
    void setHover (const juce::MouseEvent& e)
    {
        if (active) return;
        const int c = (e.x - LW) / C, r = (e.y - TH) / C;
        const bool in = e.x >= LW && e.y >= TH && c >= 0 && c < NDST && r >= 0 && r < NSRC;
        const int nr = in ? r : -1, nc = in ? c : -1;
        if (nr != hr || nc != hc) { hr = nr; hc = nc; repaint(); report(); }
    }
    void report()
    {
        if (! onStatus) return;
        if (hr < 0) { onStatus ({}); return; }
        const float v = plainValue (par[hr * NDST + hc]);
        juce::String s = juce::String::fromUTF8 (srcNames (Lang::ru)[hr]) + juce::String::fromUTF8 ("  \xe2\x86\x92  ") + juce::String::fromUTF8 (dstNames (Lang::ru)[hc]);
        if (std::abs (v) > 0.004f) s << "   " << (v > 0 ? "+" : "") << juce::roundToInt (v * 100.0f) << " %";
        onStatus (s);
    }

    juce::RangedAudioParameter* par[NSRC * NDST] = {};
    int hr = -1, hc = -1; bool moved = false, active = false, wasSet = false, removeOnly = false; float startVal = 0;
};

//======================================================================================================
// Шаги секвенсора: столбик — смещение ноты в полутонах, квадрат под ним — гейт.
class SeqView : public juce::Component
{
public:
    explicit SeqView (SynthiProProcessor& p) : proc (p)
    {
        const auto& d = paramDefs();
        for (int i = 0; i < NSTEP; ++i)
        {
            note[i] = p.apvts.getParameter (juce::String (d[(size_t) (P_SeqN0 + i)].id));
            gate[i] = p.apvts.getParameter (juce::String (d[(size_t) (P_SeqG0 + i)].id));
        }
        len = p.apvts.getParameter ("seq_len");
    }
    void paint (juce::Graphics& g) override
    {
        const float w = (float) getWidth() / NSTEP, gh = 18.0f, bh = (float) getHeight() - gh - 4.0f, mid = bh * 0.5f;
        const int n = juce::roundToInt (plainValue (len)), cur = proc.engine.uiSeqStep.load();
        g.fillAll (juce::Colour (0xff12161a));
        g.setColour (cLine); g.fillRect (0.0f, mid, (float) getWidth(), 1.0f);
        g.setFont (fnt (9.5f));
        for (int i = 0; i < NSTEP; ++i)
        {
            const float x = (float) i * w; const bool on = plainValue (gate[i]) > 0.5f; const int v = juce::roundToInt (plainValue (note[i]));
            const float al = i < n ? 1.0f : 0.25f;          // шаги за пределами длины последовательности приглушены
            if (i == cur) { g.setColour (juce::Colours::white.withAlpha (0.12f)); g.fillRect (x, 0.0f, w, (float) getHeight()); }
            if (i % 4 == 0) { g.setColour (cLine); g.fillRect (x, 0.0f, 1.0f, bh); }
            const float h = (float) v / 24.0f * (mid - 2.0f);
            g.setColour ((on ? cViolet : cLine).withMultipliedAlpha (al));
            if (v >= 0) g.fillRect (x + 2.0f, mid - h - 1.5f, w - 4.0f, h + 3.0f); else g.fillRect (x + 2.0f, mid - 1.5f, w - 4.0f, -h + 3.0f);
            g.setColour (cInk.withMultipliedAlpha (al)); g.drawText (juce::String (v), juce::Rectangle<float> (x, v >= 0 ? mid + 3.0f : mid - 14.0f, w, 11.0f), juce::Justification::centred);
            g.setColour ((on ? cRed : juce::Colour (0xff0c0f12)).withMultipliedAlpha (al)); g.fillRoundedRectangle (x + 2.0f, bh + 3.0f, w - 4.0f, gh - 2.0f, 3.0f);
        }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int i = juce::jlimit (0, NSTEP - 1, (int) ((float) e.x / ((float) getWidth() / NSTEP)));
        gateMode = e.y > getHeight() - 22;
        if (gateMode) { gateVal = plainValue (gate[i]) > 0.5f ? 0.0f : 1.0f; setPlainValue (gate[i], gateVal); repaint(); }
        else edit (e);
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (gateMode) { const int i = juce::jlimit (0, NSTEP - 1, (int) ((float) e.x / ((float) getWidth() / NSTEP))); setPlainValue (gate[i], gateVal); repaint(); }
        else edit (e);
    }
private:
    void edit (const juce::MouseEvent& e)
    {
        const int i = juce::jlimit (0, NSTEP - 1, (int) ((float) e.x / ((float) getWidth() / NSTEP)));
        const float bh = (float) getHeight() - 22.0f, mid = bh * 0.5f;
        setPlainValue (note[i], (float) juce::jlimit (-24, 24, juce::roundToInt ((mid - (float) e.y) / (mid - 2.0f) * 24.0f)));
        repaint();
    }
    SynthiProProcessor& proc;
    juce::RangedAudioParameter* note[NSTEP] = {}; juce::RangedAudioParameter* gate[NSTEP] = {}; juce::RangedAudioParameter* len = nullptr;
    bool gateMode = false; float gateVal = 0;
};

//======================================================================================================
class MeterView : public juce::Component
{
public:
    float l = 0, r = 0;
    void paint (juce::Graphics& g) override
    {
        const float w = (float) getWidth() * 0.5f - 2.0f, h = (float) getHeight();
        for (int c = 0; c < 2; ++c)
        {
            const float x = c ? w + 4.0f : 0.0f, v = c ? r : l;
            const float db = juce::Decibels::gainToDecibels (v, -60.0f), fill = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f) * h;
            g.setColour (juce::Colour (0xff0a0d0f)); g.fillRoundedRectangle (x, 0, w, h, 2.0f);
            g.setGradientFill (juce::ColourGradient (cGreen, 0, h, cRed, 0, 0, false));
            g.fillRoundedRectangle (x, h - fill, w, fill, 2.0f);
            g.setColour (cBg); for (int k = 1; k < 10; ++k) g.fillRect (x, h * (float) k / 10.0f, w, 1.0f);
        }
    }
};

//======================================================================================================
// Браузер пресетов: поиск по названию, фильтр по категории, избранное.
class PresetBrowser : public juce::Component, private juce::ListBoxModel
{
public:
    std::function<void()> onLoaded;

    explicit PresetBrowser (SynthiProProcessor& p) : proc (p)
    {
        addAndMakeVisible (search); addAndMakeVisible (cat); addAndMakeVisible (fav); addAndMakeVisible (del); addAndMakeVisible (list);
        search.setTextToShowWhenEmpty (TR ("Search by name...", "Поиск по названию..."), cDim);
        search.onTextChange = [this] { refresh(); };
        cat.onChange = [this] { refresh(); };
        fav.setButtonText (juce::String::fromUTF8 ("\xe2\x98\x85 ") + TR ("Favorites", "Избранное")); fav.setClickingTogglesState (true); fav.onClick = [this] { refresh(); };
        del.setButtonText (TR ("Delete", "Удалить")); del.setTooltip (TR ("Delete the selected user preset", "Удалить выбранный пользовательский пресет"));
        del.onClick = [this]
        {
            const int row = list.getSelectedRow(); if (row < 0 || row >= (int) rows.size()) return;
            proc.presets.deleteUser (rows[(size_t) row]); rebuildCategories(); refresh();
        };
        list.setModel (this); list.setRowHeight (22); list.setOutlineThickness (1);
        rebuildCategories(); refresh();
    }
    void rebuildCategories()
    {
        const auto keep = cat.getText();
        cat.clear (juce::dontSendNotification);
        cat.addItem (TR ("All categories", "Все категории"), 1);
        int id = 2; for (const auto& c : proc.presets.categories()) cat.addItem (c, id++);
        int sel = 1; for (int i = 0; i < cat.getNumItems(); ++i) if (cat.getItemText (i) == keep) sel = cat.getItemId (i);
        cat.setSelectedId (sel, juce::dontSendNotification);
    }
    void refresh()
    {
        rows.clear();
        const auto q = search.getText().trim().toLowerCase(); const bool allCats = cat.getSelectedId() <= 1; const auto c = cat.getText(); const bool favOnly = fav.getToggleState();
        const auto& all = proc.presets.list();
        for (int i = 0; i < (int) all.size(); ++i)
        {
            const auto& p = all[(size_t) i];
            if (! allCats && p.category != c) continue;
            if (q.isNotEmpty() && ! p.name.toLowerCase().contains (q)) continue;
            if (favOnly && ! proc.presets.isFavorite (p.name)) continue;
            rows.push_back (i);
        }
        loadedRow = -2;                                   // пока список перестраивается, выбор строки не должен перезагружать пресет
        list.deselectAllRows(); list.updateContent(); list.repaint();
        const auto curName = proc.getUiProp ("preset", {});
        int found = -1;
        for (int r = 0; r < (int) rows.size(); ++r) if (all[(size_t) rows[(size_t) r]].name == curName) { found = r; break; }
        if (found >= 0) { loadedRow = found; list.selectRow (found, false, true); } else loadedRow = -1;
    }
    void resized() override
    {
        auto a = getLocalBounds().reduced (6);
        auto top = a.removeFromTop (24); search.setBounds (top.removeFromLeft (top.getWidth() / 2 - 3)); top.removeFromLeft (6); cat.setBounds (top);
        a.removeFromTop (5);
        auto row = a.removeFromTop (22); fav.setBounds (row.removeFromLeft (130)); del.setBounds (row.removeFromRight (90));
        a.removeFromTop (5); list.setBounds (a);
    }
    void paint (juce::Graphics& g) override { g.setColour (cPanel); g.fillRoundedRectangle (getLocalBounds().toFloat(), 5.0f); g.setColour (cLine); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 5.0f, 1.0f); }

private:
    int getNumRows() override { return (int) rows.size(); }
    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override
    {
        if (row < 0 || row >= (int) rows.size()) return;
        const auto& p = proc.presets.list()[(size_t) rows[(size_t) row]];
        if (selected) { g.setColour (cAmber.withAlpha (0.25f)); g.fillRect (0, 0, w, h); }
        const bool f = proc.presets.isFavorite (p.name);
        g.setFont (fnt (14.0f)); g.setColour (f ? cAmber : cLine); g.drawText (juce::String::fromUTF8 (f ? "\xe2\x98\x85" : "\xe2\x98\x86"), 4, 0, 18, h, juce::Justification::centred);
        g.setFont (fnt (13.0f)); g.setColour (cInk); g.drawText (p.name, 28, 0, w - 190, h, juce::Justification::centredLeft);
        g.setFont (fnt (11.0f)); g.setColour (p.factory ? cDim : cTeal); g.drawText (p.category, w - 160, 0, 154, h, juce::Justification::centredRight);
    }
    void listBoxItemClicked (int row, const juce::MouseEvent& e) override
    {
        if (row < 0 || row >= (int) rows.size()) return;
        if (e.x < 26) { proc.presets.toggleFavorite (proc.presets.list()[(size_t) rows[(size_t) row]].name); if (fav.getToggleState()) refresh(); else list.repaintRow (row); }
    }
    void selectedRowsChanged (int row) override
    {
        if (loadedRow == -2 || row < 0 || row >= (int) rows.size() || row == loadedRow) return;
        loadedRow = row; proc.presets.load (rows[(size_t) row]);
        if (onLoaded) onLoaded();
    }

    SynthiProProcessor& proc;
    juce::TextEditor search; juce::ComboBox cat; juce::TextButton fav, del; juce::ListBox list;
    std::vector<int> rows; int loadedRow = -1;
};
//======================================================================================================
// Логотип: скошенный треугольник — стальные левая и нижняя грани, оранжевая правая.
// Та же геометрия используется генератором иконок tools/make_icons.ps1.
void logoFace (juce::Graphics& g, juce::Rectangle<float> b, std::initializer_list<float> u, juce::uint32 c1, juce::uint32 c2, bool vertical)
{
    juce::Path p; int i = 0; float px = 0;
    for (float v : u)
    {
        if (i % 2 == 0) px = v;
        else { const float x = b.getX() + px * b.getWidth(), y = b.getY() + v * b.getHeight(); if (i == 1) p.startNewSubPath (x, y); else p.lineTo (x, y); }
        ++i;
    }
    p.closeSubPath();
    g.setGradientFill (juce::ColourGradient (juce::Colour (c1), b.getX(), b.getY(), juce::Colour (c2), vertical ? b.getX() : b.getRight(), b.getBottom(), false));
    g.fillPath (p);
}
void drawLogoMark (juce::Graphics& g, juce::Rectangle<float> b)
{
    logoFace (g, b, { 0.04f, 0.90f, 0.50f, 0.06f, 0.50f, 0.25f, 0.17f, 0.82f }, 0xfff4f5f6, 0xff8a9096, false);
    logoFace (g, b, { 0.17f, 0.82f, 0.50f, 0.25f, 0.50f, 0.44f, 0.30f, 0.74f }, 0xff565c62, 0xff24282c, false);
    logoFace (g, b, { 0.50f, 0.06f, 0.96f, 0.90f, 0.83f, 0.82f, 0.50f, 0.25f }, 0xffffb03a, 0xffe86a00, true);
    logoFace (g, b, { 0.50f, 0.25f, 0.83f, 0.82f, 0.70f, 0.74f, 0.50f, 0.44f }, 0xffb85400, 0xff6a2e00, true);
    logoFace (g, b, { 0.04f, 0.90f, 0.17f, 0.82f, 0.83f, 0.82f, 0.96f, 0.90f }, 0xffd4d7da, 0xff7c8288, true);
    logoFace (g, b, { 0.17f, 0.82f, 0.30f, 0.74f, 0.70f, 0.74f, 0.83f, 0.82f }, 0xff6a7076, 0xff3a3f44, true);
}
juce::Path logoOutline (juce::Rectangle<float> b)
{
    juce::Path p;
    p.startNewSubPath (b.getX() + 0.04f * b.getWidth(), b.getY() + 0.90f * b.getHeight());
    p.lineTo (b.getX() + 0.50f * b.getWidth(), b.getY() + 0.06f * b.getHeight());
    p.lineTo (b.getX() + 0.96f * b.getWidth(), b.getY() + 0.90f * b.getHeight());
    p.closeSubPath();
    return p;
}

// Горизонтальный логотип в верхней панели. Состояния: обычное, наведение (оранжевый контур), нажатие (оранжевая подложка).
// По щелчку по логотипу пробегает блик — как по лезвию ножа на свету, — после чего вызывается onClick.
class LogoButton : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    std::function<void()> onClick;

    LogoButton() { setMouseCursor (juce::MouseCursor::PointingHandCursor); setRepaintsOnMouseActivity (true); }

    void flash() { if (isTimerRunning()) return; t0 = juce::Time::getMillisecondCounterHiRes(); shine = 0.0f; startTimerHz (60); }

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat().reduced (1.0f);
        const bool over = isMouseOver(), down = isMouseButtonDown();
        g.setGradientFill (juce::ColourGradient (juce::Colour (down ? 0xffc0620e : 0xff2b3036), 0, r.getY(), juce::Colour (down ? 0xff5a2a04 : 0xff15181b), 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, 7.0f);
        if (over && ! down) { g.setColour (cAmber.withAlpha (0.22f)); g.drawRoundedRectangle (r.reduced (1.5f), 6.0f, 3.0f); }
        g.setColour (over || down ? cAmber : juce::Colour (0xff4a5158)); g.drawRoundedRectangle (r, 7.0f, over || down ? 1.6f : 1.0f);

        const auto mark = juce::Rectangle<float> (r.getX() + 6.0f, r.getY() + 2.0f, r.getHeight() - 4.0f, r.getHeight() - 4.0f);
        drawLogoMark (g, mark);
        const float tx = mark.getRight() + 8.0f;
        g.setFont (fnt (18.5f, true));
        g.setGradientFill (juce::ColourGradient (juce::Colours::white, 0, r.getY() + 9.0f, juce::Colour (0xff9aa0a6), 0, r.getBottom() - 9.0f, false));
        g.drawText ("SYNTHI", juce::Rectangle<float> (tx - 2.0f, r.getY(), 74.0f, r.getHeight()), juce::Justification::centredLeft);
        g.setGradientFill (juce::ColourGradient (juce::Colour (down ? 0xffffe0b0 : 0xffffb03a), 0, r.getY() + 9.0f, juce::Colour (down ? 0xffffc070 : 0xffe86a00), 0, r.getBottom() - 9.0f, false));
        g.drawText ("PRO", juce::Rectangle<float> (tx + 72.0f, r.getY(), 46.0f, r.getHeight()), juce::Justification::centredLeft);

        if (shine < 0.0f) return;
        const float e = shine * shine * (3.0f - 2.0f * shine), skew = r.getHeight() * 0.55f;
        const float x = r.getX() - 50.0f + e * (r.getWidth() + 100.0f);
        auto band = [&] (float w, float alpha)
        {
            juce::Path p; p.addQuadrilateral (x - w, r.getBottom(), x - w + skew, r.getY(), x + w + skew, r.getY(), x + w, r.getBottom());
            juce::ColourGradient gr (juce::Colours::white.withAlpha (0.0f), x - w + skew * 0.5f, 0, juce::Colours::white.withAlpha (0.0f), x + w + skew * 0.5f, 0, false);
            gr.addColour (0.5, juce::Colours::white.withAlpha (alpha));
            g.setGradientFill (gr); g.fillPath (p);
        };
        juce::Graphics::ScopedSaveState s1 (g);
        juce::Path clip; clip.addRoundedRectangle (r, 7.0f); g.reduceClipRegion (clip);
        band (30.0f, 0.28f); band (7.0f, 0.55f);
        {
            juce::Graphics::ScopedSaveState s2 (g);
            g.reduceClipRegion (logoOutline (mark));
            band (16.0f, 1.0f);
        }
        // звезда-вспышка на вершине треугольника в момент, когда блик проходит через неё
        const float ax = mark.getX() + 0.5f * mark.getWidth(), ay = mark.getY() + 0.10f * mark.getHeight();
        const float k = juce::jmax (0.0f, 1.0f - std::abs (x + skew * 0.8f - ax) / 34.0f);
        if (k > 0.0f)
        {
            g.setColour (juce::Colours::white.withAlpha (k));
            const float len = 13.0f * k;
            g.drawLine (ax - len, ay, ax + len, ay, 1.4f); g.drawLine (ax, ay - len, ax, ay + len, 1.4f);
            g.drawLine (ax - len * 0.45f, ay - len * 0.45f, ax + len * 0.45f, ay + len * 0.45f, 0.9f); g.drawLine (ax - len * 0.45f, ay + len * 0.45f, ax + len * 0.45f, ay - len * 0.45f, 0.9f);
            g.fillEllipse (ax - 2.2f * k, ay - 2.2f * k, 4.4f * k, 4.4f * k);
        }
    }

    void mouseUp (const juce::MouseEvent& e) override { if (getLocalBounds().contains (e.getPosition())) flash(); }

private:
    void timerCallback() override
    {
        shine = (float) ((juce::Time::getMillisecondCounterHiRes() - t0) / 520.0);
        if (shine >= 1.0f) { stopTimer(); shine = -1.0f; repaint(); if (onClick) onClick(); return; }
        repaint();
    }
    float shine = -1.0f; double t0 = 0;
};

//======================================================================================================
// Окно «О программе»: автор, контакт, лицензия свободного ПО, сведения о сторонних компонентах.
class AboutPanel : public juce::Component
{
public:
    AboutPanel()
        : mail ("Dinar899@gmail.com", juce::URL ("mailto:Dinar899@gmail.com")),
          lic ("https://www.gnu.org/licenses/agpl-3.0.html", juce::URL ("https://www.gnu.org/licenses/agpl-3.0.html")),
          repo ("https://github.com/xray14/synthi-pro", juce::URL ("https://github.com/xray14/synthi-pro"))
    {
        addAndMakeVisible (mail); addAndMakeVisible (lic); addAndMakeVisible (repo); addAndMakeVisible (close);
        repo.setColour (juce::HyperlinkButton::textColourId, cAmber); repo.setJustificationType (juce::Justification::centredLeft);
        repo.setTooltip (TR ("Open the source code repository", "Открыть репозиторий с исходным кодом"));
        mail.setColour (juce::HyperlinkButton::textColourId, cAmber); lic.setColour (juce::HyperlinkButton::textColourId, cTeal);
        mail.setJustificationType (juce::Justification::centredLeft); lic.setJustificationType (juce::Justification::centredLeft);
        mail.setTooltip (TR ("Write an e-mail to the author", "Написать автору письмо"));
        close.setButtonText (TR ("Close", "Закрыть")); close.onClick = [this] { setVisible (false); };
    }
    juce::Rectangle<int> card() const { return juce::Rectangle<int> (0, 0, 620, 500).withCentre (getLocalBounds().getCentre()); }
    void resized() override
    {
        const auto c = card();
        mail.setBounds (c.getX() + 150, c.getY() + 166, 300, 20);
        repo.setBounds (c.getX() + 150, c.getY() + 336, 400, 20);
        lic.setBounds (c.getX() + 150, c.getY() + 358, 400, 20);
        close.setBounds (c.getCentreX() - 60, c.getBottom() - 42, 120, 28);
    }
    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black.withAlpha (0.72f));
        const auto c = card(); const auto cf = c.toFloat(); const int x = c.getX() + 30, w = c.getWidth() - 60;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2b3036), 0, cf.getY(), juce::Colour (0xff15181b), 0, cf.getBottom(), false)); g.fillRoundedRectangle (cf, 12.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffb9bec3), cf.getX(), cf.getY(), cAmber, cf.getRight(), cf.getBottom(), false)); g.drawRoundedRectangle (cf.reduced (1.0f), 12.0f, 2.0f);
        drawLogoMark (g, juce::Rectangle<float> ((float) x, cf.getY() + 22.0f, 92.0f, 92.0f));
        g.setFont (fnt (32.0f, true));
        g.setGradientFill (juce::ColourGradient (juce::Colours::white, 0, cf.getY() + 30.0f, juce::Colour (0xff9aa0a6), 0, cf.getY() + 62.0f, false)); g.drawText ("SYNTHI", x + 110, c.getY() + 26, 124, 40, juce::Justification::centredLeft);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffffb03a), 0, cf.getY() + 30.0f, juce::Colour (0xffe86a00), 0, cf.getY() + 62.0f, false)); g.drawText ("PRO", x + 234, c.getY() + 26, 90, 40, juce::Justification::centredLeft);
        g.setFont (fnt (13.5f)); g.setColour (cInk); g.drawText (TR ("Version 1.1.0  -  matrix modular synthesizer", "Версия 1.1.0  -  матричный модульный синтезатор"), x + 110, c.getY() + 68, w - 110, 18, juce::Justification::centredLeft);
        g.setColour (cDim); g.drawText ("Standalone / VST3  -  Windows 10/11 x64", x + 110, c.getY() + 88, w - 110, 18, juce::Justification::centredLeft);
        g.setColour (cLine); g.fillRect (x, c.getY() + 126, w, 1); g.fillRect (x, c.getY() + 198, w, 1);
        g.setColour (cDim); g.drawText (TR ("Author:", "Автор:"), x, c.getY() + 140, 116, 20, juce::Justification::centredLeft); g.drawText (TR ("Contact:", "Связь с автором:"), x, c.getY() + 166, 116, 20, juce::Justification::centredLeft);
        g.setFont (fnt (14.5f, true)); g.setColour (cInk); g.drawText ("Khafizov Dinar", x + 120, c.getY() + 140, 300, 20, juce::Justification::centredLeft);
        g.setColour (cAmber); g.drawText (TR ("FREE SOFTWARE", "СВОБОДНОЕ ПРОГРАММНОЕ ОБЕСПЕЧЕНИЕ"), x, c.getY() + 208, w, 20, juce::Justification::centredLeft);
        g.setFont (fnt (12.5f)); g.setColour (cInk);
        g.drawFittedText (TR ("Copyright (C) 2026 Khafizov Dinar.\n"
                              "SYNTHI PRO is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License, "
                              "version 3, as published by the Free Software Foundation.\n"
                              "This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of "
                              "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.",
                             "Copyright (C) 2026 Khafizov Dinar.\n"
                              "SYNTHI PRO - свободное программное обеспечение: вы можете распространять и изменять его на условиях Стандартной общественной "
                              "лицензии GNU Affero (GNU AGPL) версии 3, опубликованной Фондом свободного программного обеспечения.\n"
                              "Программа распространяется в надежде, что она будет полезной, но БЕЗ КАКИХ-ЛИБО ГАРАНТИЙ, в том числе без подразумеваемых "
                              "гарантий ТОВАРНОГО СОСТОЯНИЯ и ПРИГОДНОСТИ ДЛЯ КОНКРЕТНОЙ ЦЕЛИ."),
                         x, c.getY() + 232, w, 100, juce::Justification::topLeft, 12, 1.0f);
        g.setColour (cDim);
        g.drawText (TR ("Source code:", "Исходный код:"), x, c.getY() + 336, 116, 20, juce::Justification::centredLeft);
        g.drawText (TR ("License text:", "Текст лицензии:"), x, c.getY() + 358, 116, 20, juce::Justification::centredLeft);
        g.setFont (fnt (11.0f)); g.setColour (cDim);
        g.drawFittedText (TR ("Built with the JUCE 8 framework (AGPLv3). VST is a registered trademark of Steinberg Media Technologies GmbH. "
                              "ASIO is a trademark and software of Steinberg Media Technologies GmbH. "
                              "SYNTHI PRO is an independent project and is not affiliated with or endorsed by EMS or Arturia.",
                              "Создано с использованием фреймворка JUCE 8 (AGPLv3). VST - зарегистрированный товарный знак Steinberg Media Technologies GmbH. "
                              "ASIO - товарный знак и программное обеспечение Steinberg Media Technologies GmbH. "
                              "SYNTHI PRO - независимый проект, не связанный с компаниями EMS и Arturia и не одобренный ими."),
                          x, c.getY() + 388, w, 60, juce::Justification::topLeft, 5, 1.0f);
    }
    void mouseDown (const juce::MouseEvent& e) override { if (! card().contains (e.getPosition())) setVisible (false); }

private:
    juce::HyperlinkButton mail, lic, repo; juce::TextButton close;
};
//======================================================================================================
// Регуляторы с поддержкой MIDI Learn: правый щелчок не меняет значение, а вызывает onRight.
struct LearnSlider : public juce::Slider
{
    using juce::Slider::Slider;
    std::function<void()> onRight;
    void mouseDown (const juce::MouseEvent& e) override { right = e.mods.isPopupMenu(); if (right) { if (onRight) onRight(); } else juce::Slider::mouseDown (e); }
    void mouseDrag (const juce::MouseEvent& e) override { if (! right) juce::Slider::mouseDrag (e); }
    void mouseUp (const juce::MouseEvent& e) override { if (! right) juce::Slider::mouseUp (e); right = false; }
    bool right = false;
};
struct LearnButton : public juce::TextButton
{
    using juce::TextButton::TextButton;
    std::function<void()> onRight;
    void mouseDown (const juce::MouseEvent& e) override { right = e.mods.isPopupMenu(); if (right) { if (onRight) onRight(); } else juce::TextButton::mouseDown (e); }
    void mouseDrag (const juce::MouseEvent& e) override { if (! right) juce::TextButton::mouseDrag (e); }
    void mouseUp (const juce::MouseEvent& e) override { if (! right) juce::TextButton::mouseUp (e); right = false; }
    bool right = false;
};
struct LearnCombo : public juce::ComboBox
{
    std::function<void()> onRight;
    void mouseDown (const juce::MouseEvent& e) override { right = e.mods.isPopupMenu(); if (right) { if (onRight) onRight(); } else juce::ComboBox::mouseDown (e); }
    void mouseDrag (const juce::MouseEvent& e) override { if (! right) juce::ComboBox::mouseDrag (e); }
    void mouseUp (const juce::MouseEvent& e) override { if (! right) juce::ComboBox::mouseUp (e); right = false; }
    bool right = false;
};
} // namespace

//======================================================================================================
class SynthiProEditor::Content : public juce::Component
{
public:
    enum { KS = 40, CW = 50, RowH = 91, Top = 52 };

    Content (SynthiProEditor& ed, SynthiProProcessor& p)
        : editor (ed), proc (p), matrix (p), seq (p), browser (p), kb (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
    {
        easy = proc.getUiProp ("mode", "pro") == "easy";
        setSize (BaseW, BaseH);
        buildTopBar();
        buildEffects();
        buildKeyboard();
        if (easy) buildEasy(); else buildPro();
        browser.onLoaded = [this] { updatePresetName(); };
        updatePresetName();
    }

    //--------------------------------------------------------------------------------------------------
    void paint (juce::Graphics& g) override
    {
        g.fillAll (cBg);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0e1113), 0, 0, juce::Colour (0xff1a1f24), 0, 46, false)); g.fillRect (0, 0, BaseW, 46);
        g.setColour (cAmber); g.fillRect (0, 46, BaseW, 2);
        for (const auto& pn : panels)
        {
            g.setColour (cPanel); g.fillRoundedRectangle (pn.r.toFloat(), 4.0f);
            g.setColour (pn.c); g.fillRoundedRectangle ((float) pn.r.getX(), (float) pn.r.getY(), (float) pn.r.getWidth(), 3.0f, 1.5f);
            g.setColour (cLine); g.drawRoundedRectangle (pn.r.toFloat().reduced (0.5f), 4.0f, 1.0f);
            g.setFont (fnt (10.5f, true)); g.setColour (pn.c); g.drawText (pn.title, pn.r.getX() + 7, pn.r.getY() + 4, pn.r.getWidth() - 10, 13, juce::Justification::centredLeft);
        }
        g.setColour (juce::Colour (0xff0e1113)); g.fillRect (0, 690, BaseW, 110);
    }

    void tick()
    {
        const float pl = proc.engine.peakL.exchange (0.0f), pr = proc.engine.peakR.exchange (0.0f);
        meter.l = juce::jmax (pl, meter.l * 0.86f); meter.r = juce::jmax (pr, meter.r * 0.86f); meter.repaint();
        if (matrix.isShowing()) matrix.repaint();
        if (seq.isShowing()) seq.repaint();
        const int v = proc.engine.uiVoices.load();
        if (v != shownVoices) { shownVoices = v; voicesLabel.setText (TR ("Voices: ", "Голоса: ") + juce::String (v), juce::dontSendNotification); }
        if (++slow >= 10) { slow = 0; updatePresetName(); if (! easy) routesLabel.setText (TR ("Active routes: ", "Активных маршрутов: ") + juce::String (matrix.activeRoutes()), juce::dontSendNotification); }

        // экранные колёса повторяют движения колёс MIDI-клавиатуры
        const int hs = proc.hwSeq.load();
        if (hs != seenHw)
        {
            seenHw = hs;
            if (bendSl != nullptr && ! bendSl->isMouseButtonDown()) bendSl->setValue (proc.hwBend.load(), juce::dontSendNotification);
            if (modSl != nullptr && ! modSl->isMouseButtonDown()) modSl->setValue (proc.hwMod.load(), juce::dontSendNotification);
        }

        // MIDI Learn: мигание ожидающего регулятора и сообщение о созданной привязке
        const int ls = proc.linkSeq.load();
        if (ls != seenLink)
        {
            seenLink = ls;
            const int p = proc.lastLinkParam.load();
            if (p >= 0) { linkMsg = TR ("Linked: ", "Привязано: ") + fullName (juce::String (paramDefs()[(size_t) p].id)) + juce::String::fromUTF8 ("  \xe2\x86\x90  CC ") + juce::String (proc.lastLinkCC.load()); linkTicks = 75; }
        }
        const int lp = proc.learnParam.load();
        if (lp >= 0 || linkTicks > 0 || lp != lastLearn) repaint();
        lastLearn = lp;
        if (linkTicks > 0) --linkTicks;
    }

    void paintOverChildren (juce::Graphics& g) override
    {
        if (about != nullptr && about->isVisible()) return;
        const int lp = proc.learnParam.load();
        for (const auto& t : learnTargets)
        {
            if (! t.second->isShowing()) continue;
            const auto b = getLocalArea (t.second, t.second->getLocalBounds()).toFloat();
            if (t.first == lp)
            {
                const float pulse = 0.5f + 0.5f * std::sin ((float) (juce::Time::getMillisecondCounterHiRes() * 0.012));
                g.setColour (juce::Colour (0xffff8a80).withAlpha (0.20f + 0.50f * pulse)); g.fillRoundedRectangle (b.expanded (3.0f), 7.0f);
                g.setColour (juce::Colour (0xffff5a4d).withAlpha (0.6f + 0.4f * pulse)); g.drawRoundedRectangle (b.expanded (3.0f), 7.0f, 1.6f);
            }
            else if (proc.ccForParam (t.first) >= 0)        // метка «привязан к контроллеру»
            {
                g.setColour (juce::Colour (0xffff8a80)); g.fillEllipse (b.getRight() - 5.0f, b.getY() - 1.0f, 6.0f, 6.0f);
                g.setColour (juce::Colours::black); g.drawEllipse (b.getRight() - 5.0f, b.getY() - 1.0f, 6.0f, 6.0f, 0.8f);
            }
        }
        juce::String msg;
        if (lp >= 0) msg = TR ("MIDI Learn: move a knob, fader or button on your controller  (right-click to cancel)", "MIDI Learn: подвигайте ручку, фейдер или кнопку на контроллере  (правый щелчок - отмена)");
        else if (linkTicks > 0) msg = linkMsg;
        if (msg.isNotEmpty())
        {
            const auto r = juce::Rectangle<float> (190.0f, 52.0f, 836.0f, 30.0f);
            g.setColour (juce::Colour (0xf0140f0e)); g.fillRoundedRectangle (r, 6.0f);
            g.setColour (juce::Colour (0xffff8a80)); g.drawRoundedRectangle (r, 6.0f, 1.4f);
            g.setFont (fnt (13.5f, true)); g.drawText (msg, r.reduced (10.0f, 0.0f), juce::Justification::centred);
        }
    }

    juce::MidiKeyboardComponent& keyboard() { return kb; }
    void demo (int what) { if (what == 1) toggleBrowser(); else if (what == 2) showAbout(); }   // для съёмки экранов

private:
    struct PanelDef { juce::Rectangle<int> r; juce::String title; juce::Colour c; };

    //--------------------------------------------------------------------------------------------------
    template <typename T, typename... A> T* own (A&&... a) { auto* c = new T (std::forward<A> (a)...); owned.add (c); addAndMakeVisible (c); return c; }

    void panel (int x, int y, int w, int h, const juce::String& title, juce::Colour c) { panels.push_back ({ { x, y, w, h }, title, c }); }

    // Регистрация регулятора для MIDI Learn. Правый щелчок: свободный регулятор сразу ждёт контроллер,
    // у привязанного открывается меню «назначить другой / снять назначение».
    template <typename T> void learnable (T* comp, const char* id)
    {
        const int idx = paramIndex (juce::String (id).toStdString());
        if (idx < 0) return;
        learnTargets.push_back ({ idx, comp });
        comp->onRight = [this, idx, comp] { learnClick (idx, comp); };
    }
    void learnClick (int idx, juce::Component* comp)
    {
        if (proc.learnParam.load() == idx) { proc.learnParam.store (-1); return; }        // повторный щелчок — отмена
        const int cc = proc.ccForParam (idx);
        if (cc < 0) { proc.learnParam.store (idx); return; }
        proc.learnParam.store (-1);
        juce::PopupMenu m;
        m.addItem (1, TR ("Assigned to MIDI controller CC ", "Назначен на MIDI-контроллер CC ") + juce::String (cc), false);
        m.addItem (2, TR ("Assign another controller", "Назначить другой контроллер"));
        m.addItem (3, TR ("Remove assignment", "Снять назначение"));
        juce::Component::SafePointer<Content> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (comp), [safe, idx] (int r)
        {
            if (safe == nullptr) return;
            if (r == 2) safe->proc.learnParam.store (idx);
            else if (r == 3) { safe->proc.clearMidiLink (idx); safe->repaint(); }
        });
    }

    juce::Slider* knob (const char* id, const juce::String& label, int x, int y, juce::Colour c, int size = KS, int cell = CW)
    {
        auto* s = own<LearnSlider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
        learnable (s, id);
        s->setBounds (x + (cell - size) / 2, y, size, size);
        s->setColour (juce::Slider::rotarySliderFillColourId, c);
        s->setPopupDisplayEnabled (true, false, &editor);
        s->setTooltip (fullName (id));
        sa.add (new APVTS::SliderAttachment (proc.apvts, id, *s));
        auto* l = own<juce::Label> (juce::String(), label);
        l->setFont (fnt (size > 50 ? 12.5f : 10.0f)); l->setJustificationType (juce::Justification::centred); l->setColour (juce::Label::textColourId, cDim);
        l->setBounds (x - 6, y + size - 1, cell + 12, 14); l->setInterceptsMouseClicks (false, false);
        return s;
    }
    juce::ComboBox* combo (const char* id, int x, int y, int w)
    {
        auto* c = own<LearnCombo>();
        learnable (c, id);
        if (auto* ch = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id))) c->addItemList (ch->choices, 1);
        c->setBounds (x, y, w, 22); c->setTooltip (fullName (id));
        ca.add (new APVTS::ComboBoxAttachment (proc.apvts, id, *c));
        return c;
    }
    juce::TextButton* toggle (const char* id, const juce::String& text, int x, int y, int w, int h, juce::Colour c)
    {
        auto* b = own<LearnButton> (text);
        learnable (b, id);
        b->setClickingTogglesState (true); b->setColour (juce::TextButton::buttonOnColourId, c); b->setBounds (x, y, w, h); b->setTooltip (fullName (id));
        ba.add (new APVTS::ButtonAttachment (proc.apvts, id, *b));
        return b;
    }
    juce::TextButton* button (const juce::String& text, int x, int y, int w, int h, std::function<void()> f, const juce::String& tip = {})
    {
        auto* b = own<juce::TextButton> (text);
        b->setBounds (x, y, w, h); b->onClick = std::move (f); if (tip.isNotEmpty()) b->setTooltip (tip);
        return b;
    }

    //--------------------------------------------------------------------------------------------------
    void showAbout()
    {
        if (about == nullptr) { about = std::make_unique<AboutPanel>(); addChildComponent (*about); about->setBounds (getLocalBounds()); }
        about->setVisible (true); about->toFront (true);
    }

    void buildTopBar()
    {
        addAndMakeVisible (logo); logo.setBounds (5, 3, 180, 41);
        logo.setTooltip (TR ("About SYNTHI PRO", "О программе SYNTHI PRO"));
        logo.onClick = [this] { showAbout(); };
        button ("<", 190, 11, 24, 26, [this] { stepPreset (-1); }, TR ("Previous preset", "Предыдущий пресет"));
        presetBtn = button ({}, 217, 11, 290, 26, [this] { toggleBrowser(); }, TR ("Open the preset browser", "Открыть браузер пресетов"));
        presetBtn->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff0a1210)); presetBtn->setColour (juce::TextButton::textColourOffId, cTeal);
        button (">", 510, 11, 24, 26, [this] { stepPreset (1); }, TR ("Next preset", "Следующий пресет"));
        button (TR ("Save", "Сохранить"), 544, 11, 84, 26, [this] { savePreset(); }, TR ("Save the current patch as a user preset", "Сохранить текущий патч как пользовательский пресет"));
        button (TR ("Load", "Загрузить"), 632, 11, 84, 26, [this] { toggleBrowser(); }, TR ("Browse presets: search, categories, favorites", "Браузер пресетов: поиск, категории, избранное"));
        button (TR ("Random", "Случайный"), 720, 11, 88, 26, [this] { proc.presets.randomPatch(); updatePresetName(); }, TR ("Random patch with musically sensible limits", "Случайный патч с музыкально осмысленными ограничениями"));
        button ("Init", 812, 11, 46, 26, [this] { proc.presets.initPatch(); updatePresetName(); }, TR ("Reset all parameters to defaults", "Сбросить все параметры"));
        auto* mode = button (easy ? "EASY" : "PRO", 872, 11, 70, 26, [this] { proc.setUiProp ("mode", easy ? "pro" : "easy"); editor.requestRebuild(); },
                             TR ("Switch between EASY and PRO modes", "Переключить режим EASY / PRO"));
        mode->setColour (juce::TextButton::buttonColourId, easy ? cTeal : cAmber); mode->setColour (juce::TextButton::textColourOffId, juce::Colours::black);
        button (Lang::ru ? "RU" : "EN", 948, 11, 44, 26, [this] { Lang::ru = ! Lang::ru; proc.setUiProp ("lang", Lang::ru ? "ru" : "en"); editor.requestRebuild(); },
                TR ("Interface language", "Язык интерфейса"));
        settingsBtn = button (TR ("Settings", "Настройки"), 998, 11, 96, 26, [this] { showSettings(); });
        addAndMakeVisible (voicesLabel); voicesLabel.setBounds (1100, 11, 170, 26); voicesLabel.setFont (fnt (12.0f)); voicesLabel.setColour (juce::Label::textColourId, cDim);
        voicesLabel.setJustificationType (juce::Justification::centredRight);
    }

    void buildEffects()
    {
        const int x0 = 1034, w = 242; int y = Top;
        auto fxBlock = [&] (const char* onId, const juce::String& name, juce::Colour c, std::initializer_list<std::pair<const char*, juce::String>> knobs, int h = 79)
        {
            panels.push_back ({ { x0, y, w, h - 3 }, {}, c });
            toggle (onId, name, x0 + 6, y + 6, 96, 18, c);
            int kx = x0 + 6;
            for (const auto& k : knobs) { knob (k.first, k.second, kx, y + 27, c, 34, 46); kx += 46; }
            y += h;
        };
        fxBlock ("dist_on", "DISTORTION", cRed, { { "dist_drive", "Drive" }, { "dist_tone", "Tone" }, { "dist_mix", "Mix" } });
        fxBlock ("sat_on", "SATURATION", cAmber, { { "sat_drive", "Drive" }, { "sat_mix", "Mix" } });
        fxBlock ("pha_on", "PHASER", cViolet, { { "pha_rate", "Rate" }, { "pha_depth", "Depth" }, { "pha_fb", "Feedback" }, { "pha_mix", "Mix" } });
        fxBlock ("cho_on", "CHORUS", cTeal, { { "cho_rate", "Rate" }, { "cho_depth", "Depth" }, { "cho_mix", "Mix" } });
        fxBlock ("dly_on", "DELAY", cBlue, { { "dly_time", "Time" }, { "dly_fb", "Feedback" }, { "dly_tone", "Tone" }, { "dly_mix", "Mix" } });
        toggle ("dly_pp", "PING-PONG", x0 + 108, y - 79 + 6, 84, 18, cBlue);
        fxBlock ("rev_on", "REVERB", cGreen, { { "rev_size", "Size" }, { "rev_damp", "Damp" }, { "rev_mix", "Mix" } });
        panel (x0, y, w, 686 - y, TR ("OUTPUT", "ВЫХОД"), cInk);
        knob ("width", TR ("Width", "Ширина"), x0 + 8, y + 24, cInk, 44, 60);
        knob ("master_vol", TR ("Master", "Громкость"), x0 + 76, y + 20, cAmber, 56, 76);
        addAndMakeVisible (meter); meter.setBounds (x0 + 176, y + 20, 48, 686 - y - 30);
    }

    void buildKeyboard()
    {
        auto wheel = [this] (int x, const juce::String& tip, double lo, double hi, bool spring, std::atomic<float>& target)
        {
            auto* s = own<juce::Slider> (juce::Slider::LinearVertical, juce::Slider::NoTextBox);
            s->setRange (lo, hi); s->setValue (0.0, juce::dontSendNotification); s->setBounds (x, 696, 26, 86); s->setTooltip (tip);
            s->onValueChange = [s, &target] { target.store ((float) s->getValue()); };
            if (spring) s->onDragEnd = [s] { s->setValue (0.0, juce::sendNotificationSync); };
            (spring ? bendSl : modSl) = s;
            auto* l = own<juce::Label> (juce::String(), spring ? "BEND" : "MOD"); l->setFont (fnt (9.0f)); l->setColour (juce::Label::textColourId, cDim);
            l->setJustificationType (juce::Justification::centred); l->setBounds (x - 6, 782, 38, 12); l->setInterceptsMouseClicks (false, false);
        };
        wheel (8, "Pitch Bend", -1.0, 1.0, true, proc.uiBend);
        wheel (40, "Modulation Wheel", 0.0, 1.0, false, proc.uiMod);

        addAndMakeVisible (kb);
        kb.setVelocity (0.8f, true);                 // Velocity зависит от высоты щелчка по клавише
        kb.setOctaveForMiddleC (4);
        kb.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, cAmber);
        kb.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, cAmber.withAlpha (0.3f));
        kbKeys = juce::jlimit (49, 88, proc.getUiProp ("keys", "88").getIntValue());
        kbOct = juce::jlimit (1, 7, proc.getUiProp ("kboct", "4").getIntValue());
        applyKeyboardRange();

        octLabel.setFont (fnt (11.0f)); octLabel.setJustificationType (juce::Justification::centred); octLabel.setColour (juce::Label::textColourId, cInk);
        addAndMakeVisible (octLabel); octLabel.setBounds (72, 696, 70, 16);
        button ("-", 74, 714, 32, 20, [this] { kbOct = juce::jmax (1, kbOct - 1); applyKeyboardRange(); }, TR ("Computer keyboard: octave down", "Клавиатура компьютера: октава вниз"));
        button ("+", 108, 714, 32, 20, [this] { kbOct = juce::jmin (7, kbOct + 1); applyKeyboardRange(); }, TR ("Computer keyboard: octave up", "Клавиатура компьютера: октава вверх"));
        rangeBtn = button ({}, 74, 740, 66, 20, [this] { kbKeys = kbKeys == 88 ? 61 : (kbKeys == 61 ? 49 : 88); applyKeyboardRange(); }, TR ("Keyboard range: 88 / 61 / 49 keys", "Диапазон клавиатуры: 88 / 61 / 49 клавиш"));
        button ("Panic", 74, 766, 66, 20, [this] { proc.panicFlag = true; proc.keyboardState.allNotesOff (0); }, TR ("Stop all notes", "Снять все ноты"));
        applyKeyboardRange();
    }

    void applyKeyboardRange()
    {
        const int lo = kbKeys == 88 ? 21 : 36, hi = kbKeys == 88 ? 108 : (kbKeys == 61 ? 96 : 84), white = kbKeys == 88 ? 52 : (kbKeys == 61 ? 36 : 29);
        kb.setAvailableRange (lo, hi); kb.setKeyWidth (1126.0f / (float) white); kb.setLowestVisibleKey (lo);
        kb.setKeyPressBaseOctave (kbOct); kb.setBounds (148, 696, 1126, 98);
        octLabel.setText (TR ("Keys: C", "Клав.: C") + juce::String (kbOct - 1), juce::dontSendNotification);
        if (rangeBtn != nullptr) rangeBtn->setButtonText (juce::String (kbKeys) + TR (" keys", " клав."));
        proc.setUiProp ("keys", juce::String (kbKeys)); proc.setUiProp ("kboct", juce::String (kbOct));
    }

    //--------------------------------------------------------------------------------------------------
    void buildPro()
    {
        int y = Top;
        for (int i = 1; i <= 3; ++i)
        {
            const auto o = "o" + juce::String (i) + "_";
            panel (6, y, 452, RowH - 4, "VCO " + juce::String (i), cAmber);
            auto id = [&] (const char* s) { ids.add (o + s); return ids.getReference (ids.size() - 1).toRawUTF8(); };
            combo (id ("wave"), 12, y + 30, 84);
            knob (id ("oct"), TR ("Range", "Диапазон"), 100, y + 22, cAmber); knob (id ("semi"), TR ("Semi", "Полутон"), 150, y + 22, cAmber);
            knob (id ("fine"), TR ("Detune", "Расстр."), 200, y + 22, cAmber); knob (id ("pw"), "PW", 250, y + 22, cAmber);
            knob (id ("fm"), "FM", 300, y + 22, cRed); knob (id ("level"), TR ("Level", "Уровень"), 350, y + 22, cInk);
            toggle (id ("key"), "KEY", 404, y + 20, 48, 18, cAmber);
            if (i == 2) toggle ("o2_sync", "SYNC", 404, y + 41, 48, 18, cRed);
            if (i == 3) { toggle ("o3_sync", "SYNC", 404, y + 41, 48, 18, cRed); toggle ("o3_low", "LOW", 404, y + 62, 48, 18, cTeal); }
            y += RowH;
        }
        panel (462, Top, 144, RowH - 4, TR ("NOISE", "ШУМ"), juce::Colour (0xffc8ccd0));
        knob ("noise_color", TR ("Colour", "Окраска"), 472, Top + 22, juce::Colour (0xffc8ccd0)); knob ("noise_level", TR ("Level", "Уровень"), 536, Top + 22, cInk);
        panel (462, Top + RowH, 144, RowH - 4, "RING MOD", cRed);
        knob ("ring_level", TR ("Level", "Уровень"), 504, Top + RowH + 22, cInk);
        panel (462, Top + 2 * RowH, 144, RowH - 4, TR ("GLIDE / DRIFT", "ГЛАЙД / ДРЕЙФ"), cTeal);
        knob ("glide", TR ("Glide", "Глайд"), 472, Top + 2 * RowH + 22, cTeal); knob ("drift", TR ("Drift", "Дрейф"), 536, Top + 2 * RowH + 22, cTeal);

        panel (6, y, 396, RowH - 4, TR ("FILTER (VCF)", "ФИЛЬТР (VCF)"), cBlue);
        combo ("f_mode", 12, y + 30, 84);
        knob ("f_cut", "Cutoff", 100, y + 22, cBlue); knob ("f_res", TR ("Reso", "Резон."), 150, y + 22, cBlue); knob ("f_drive", "Drive", 200, y + 22, cRed);
        knob ("f_env", "Env 2", 250, y + 22, cGreen); knob ("f_key", "Key", 300, y + 22, cInk); knob ("f_vel", "Vel", 350, y + 22, cInk);
        panel (406, y, 200, RowH - 4, "VCA", cGreen);
        knob ("a_level", TR ("Level", "Уровень"), 416, y + 22, cInk); knob ("a_pan", TR ("Pan", "Панорама"), 480, y + 22, cInk); knob ("vel_sens", "Vel", 544, y + 22, cInk);
        y += RowH;

        for (int i = 1; i <= 3; ++i)
        {
            const int x = 6 + (i - 1) * 202; const auto e = "e" + juce::String (i) + "_";
            panel (x, y, 198, RowH - 4, TR ("ENVELOPE ", "ОГИБАЮЩАЯ ") + juce::String (i) + (i == 1 ? " > VCA" : (i == 2 ? " > VCF" : "")), cGreen);
            auto id = [&] (const char* s) { ids.add (e + s); return ids.getReference (ids.size() - 1).toRawUTF8(); };
            knob (id ("a"), "A", x + 3, y + 22, cGreen, KS, 48); knob (id ("d"), "D", x + 51, y + 22, cGreen, KS, 48);
            knob (id ("s"), "S", x + 99, y + 22, cGreen, KS, 48); knob (id ("r"), "R", x + 147, y + 22, cGreen, KS, 48);
        }
        y += RowH;

        for (int i = 1; i <= 2; ++i)
        {
            const int x = 6 + (i - 1) * 220; const auto l = "l" + juce::String (i) + "_";
            panel (x, y, 216, RowH - 4, "LFO " + juce::String (i), cTeal);
            auto id = [&] (const char* s) { ids.add (l + s); return ids.getReference (ids.size() - 1).toRawUTF8(); };
            combo (id ("wave"), x + 6, y + 30, 80);
            knob (id ("rate"), TR ("Rate", "Частота"), x + 88, y + 22, cTeal, KS, 46); knob (id ("fade"), TR ("Fade", "Нараст."), x + 134, y + 22, cTeal, KS, 46);
            toggle (id ("retrig"), "KEY", x + 180, y + 30, 32, 20, cTeal);
        }
        panel (446, y, 160, RowH - 4, "SAMPLE & HOLD", cPink);
        combo ("sh_src", 450, y + 30, 72);
        knob ("sh_rate", TR ("Rate", "Частота"), 522, y + 22, cPink, KS, 42); knob ("sh_lag", TR ("Lag", "Сглаж."), 563, y + 22, cPink, KS, 42);
        y += RowH;

        panel (6, y, 600, RowH - 4, TR ("VOICES / GLOBAL", "ГОЛОСА / ОБЩЕЕ"), cInk);
        combo ("voice_mode", 12, y + 30, 84);
        knob ("voices", TR ("Voices", "Голоса"), 100, y + 22, cInk); knob ("bend_range", "Bend", 150, y + 22, cInk); knob ("master_tune", TR ("Tune", "Строй"), 200, y + 22, cInk);
        knob ("mod_vib", TR ("Mod>Vib", "Мод>Вибр"), 250, y + 22, cTeal);
        addAndMakeVisible (status); status.setBounds (310, y + 22, 288, 22); status.setFont (fnt (13.0f)); status.setColour (juce::Label::textColourId, cTeal);
        status.setColour (juce::Label::backgroundColourId, juce::Colour (0xff0a1210)); status.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (routesLabel); routesLabel.setBounds (310, y + 48, 156, 18); routesLabel.setFont (fnt (11.0f)); routesLabel.setColour (juce::Label::textColourId, cDim);
        button (TR ("Clear matrix", "Очистить матрицу"), 468, y + 47, 130, 20, [this] { matrix.clearAll(); });

        panel (610, Top, 416, 404, TR ("PATCH MATRIX  -  click: pin on/off, drag up/down: depth", "МАТРИЦА  -  щелчок: штырёк, тянуть вверх/вниз: глубина"), cAmber);
        addAndMakeVisible (matrix); matrix.setTopLeftPosition (614, Top + 18);
        matrix.onStatus = [this] (const juce::String& s) { status.setText (s, juce::dontSendNotification); };

        const int sy = Top + 408;
        panel (610, sy, 416, 686 - sy, TR ("STEP SEQUENCER", "СЕКВЕНСОР"), cViolet);
        toggle ("seq_on", "ON", 618, sy + 30, 40, 22, cViolet);
        combo ("seq_rate", 664, sy + 30, 70);
        knob ("seq_bpm", TR ("Tempo", "Темп"), 744, sy + 20, cViolet, 36, 50); knob ("seq_gate", TR ("Gate", "Гейт"), 800, sy + 20, cViolet, 36, 50);
        knob ("seq_len", TR ("Steps", "Шаги"), 856, sy + 20, cViolet, 36, 50);
        auto* hint = own<juce::Label> (juce::String(), TR ("Hold a key: it sets\nthe root note", "Удерживайте клавишу:\nона задаёт тонику"));
        hint->setFont (fnt (10.0f)); hint->setColour (juce::Label::textColourId, cDim); hint->setBounds (910, sy + 20, 112, 40); hint->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (seq); seq.setBounds (616, sy + 74, 404, 686 - sy - 80);

        addChildComponent (browser); browser.setBounds (610, Top, 416, 634);
    }

    void buildEasy()
    {
        addAndMakeVisible (browser); browser.setBounds (6, Top, 430, 634);
        panel (442, Top, 584, 634, TR ("EASY MODE  -  the essential controls", "EASY MODE  -  главные регуляторы"), cTeal);
        struct M { const char* id; const char* en; const char* ru; juce::Colour c; };
        const M macros[12] = {
            { "f_cut", "Cutoff", "Срез", cBlue }, { "f_res", "Resonance", "Резонанс", cBlue }, { "f_env", "Filter Env", "Огиб. фильтра", cGreen }, { "f_drive", "Drive", "Перегруз", cRed },
            { "e1_a", "Attack", "Атака", cGreen }, { "e1_d", "Decay", "Спад", cGreen }, { "e1_s", "Sustain", "Поддержка", cGreen }, { "e1_r", "Release", "Затухание", cGreen },
            { "glide", "Glide", "Глайд", cTeal }, { "l1_rate", "LFO Rate", "Частота LFO", cTeal }, { "noise_level", "Noise", "Шум", cInk }, { "drift", "Drift", "Дрейф", cAmber } };
        for (int i = 0; i < 12; ++i)
            knob (macros[i].id, TR (macros[i].en, macros[i].ru), 462 + (i % 4) * 140, Top + 60 + (i / 4) * 150, macros[i].c, 84, 124);
        auto* info = own<juce::Label> (juce::String(), TR ("Pick a preset on the left, shape it with these twelve knobs, add effects on the right.\nSwitch to PRO for all modules, the patch matrix and the sequencer.",
                                                         "Выберите пресет слева, настройте тембр двенадцатью ручками, добавьте эффекты справа.\nВ режиме PRO доступны все модули, матрица коммутации и секвенсор."));
        info->setFont (fnt (13.0f)); info->setColour (juce::Label::textColourId, cDim); info->setJustificationType (juce::Justification::centred);
        info->setBounds (452, Top + 530, 564, 60); info->setInterceptsMouseClicks (false, false);
        combo ("voice_mode", 462, Top + 500, 110);
    }

    //--------------------------------------------------------------------------------------------------
    void updatePresetName()
    {
        const auto n = proc.getUiProp ("preset", "Init");
        if (presetBtn != nullptr && presetBtn->getButtonText() != n) presetBtn->setButtonText (n);
    }
    void toggleBrowser()
    {
        if (easy) return;
        const bool show = ! browser.isVisible();
        if (show) { browser.rebuildCategories(); browser.refresh(); browser.toFront (false); }
        browser.setVisible (show); matrix.setVisible (! show); seq.setVisible (! show);
    }
    void stepPreset (int dir)
    {
        const auto& all = proc.presets.list(); if (all.empty()) return;
        int i = proc.presets.indexOf (proc.getUiProp ("preset", {}));
        i = i < 0 ? 0 : (i + dir + (int) all.size()) % (int) all.size();
        proc.presets.load (i); updatePresetName(); browser.refresh();
    }
    void savePreset()
    {
        auto* w = new juce::AlertWindow (TR ("Save preset", "Сохранить пресет"), TR ("Preset name and category:", "Название и категория пресета:"), juce::MessageBoxIconType::NoIcon);
        w->addTextEditor ("name", proc.getUiProp ("preset", "My Preset"));
        auto cats = proc.presets.categories(); cats.addIfNotAlreadyThere ("User");
        w->addComboBox ("cat", cats);
        if (auto* cb = w->getComboBoxComponent ("cat")) cb->setSelectedItemIndex (juce::jmax (0, cats.indexOf ("User")));
        w->addButton (TR ("Save", "Сохранить"), 1, juce::KeyPress (juce::KeyPress::returnKey));
        w->addButton (TR ("Cancel", "Отмена"), 0, juce::KeyPress (juce::KeyPress::escapeKey));
        juce::Component::SafePointer<Content> safe (this);
        w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int result)
        {
            if (result != 1 || safe == nullptr) return;
            const auto name = w->getTextEditorContents ("name");
            auto* cb = w->getComboBoxComponent ("cat");
            const bool ok = safe->proc.presets.saveUser (name, cb != nullptr ? cb->getText() : juce::String ("User"));
            safe->status.setText (ok ? TR ("Preset saved: ", "Пресет сохранён: ") + name : TR ("Could not save the preset", "Не удалось сохранить пресет"), juce::dontSendNotification);
            safe->browser.rebuildCategories(); safe->browser.refresh(); safe->updatePresetName();
        }), true);
    }
    void showSettings()
    {
        juce::PopupMenu m, scale;
        const double cur = (double) editor.getWidth() / BaseW;
        for (int pc : { 75, 100, 125, 150, 175, 200 }) scale.addItem (100 + pc, juce::String (pc) + " %", true, std::abs (cur * 100.0 - pc) < 2.0);
        m.addItem (6, TR ("About SYNTHI PRO...", "О программе SYNTHI PRO..."));
        m.addSeparator();
        m.addSubMenu (TR ("Interface scale", "Масштаб интерфейса"), scale);
        m.addItem (2, TR ("Open user presets folder", "Открыть папку пользовательских пресетов"));
        m.addItem (3, TR ("Stop all notes (Panic)", "Снять все ноты (Panic)"));
        m.addSeparator();
        m.addItem (8, TR ("MIDI Learn: right-click any knob, list or switch, then move a control on your MIDI controller", "MIDI Learn: правый щелчок по ручке, списку или переключателю, затем подвигайте регулятор на MIDI-контроллере"), false);
        m.addItem (7, TR ("Remove all MIDI assignments (", "Снять все MIDI-назначения (") + juce::String (proc.numMidiLinks()) + ")", proc.numMidiLinks() > 0);
        m.addSeparator();
        m.addItem (4, TR ("Audio device, sample rate, buffer size and MIDI inputs: button \"Options\" in the window title bar (standalone) or the DAW settings (VST3)",
                          "Аудиоустройство, частота дискретизации, размер буфера и MIDI-входы: кнопка «Options» в заголовке окна (автономная версия) или настройки DAW (VST3)"), false);
        m.addItem (5, "SYNTHI PRO 1.1.0  -  " + juce::String (kNumParams()) + TR (" parameters", " параметров"), false);
        juce::Component::SafePointer<Content> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (settingsBtn), [safe] (int r)
        {
            if (safe == nullptr) return;
            if (r > 100) safe->editor.setScale ((r - 100) / 100.0);
            else if (r == 2) { auto d = PresetManager::userDir(); d.createDirectory(); d.revealToUser(); }
            else if (r == 3) { safe->proc.panicFlag = true; safe->proc.keyboardState.allNotesOff (0); }
            else if (r == 7) { safe->proc.clearAllMidiLinks(); safe->repaint(); }
            else if (r == 6) safe->logo.flash();                 // блик на логотипе, затем окно «О программе»
        });
    }
    static int kNumParams() { return (int) P_COUNT; }

    SynthiProEditor& editor;
    SynthiProProcessor& proc;
    bool easy = false;
    std::vector<PanelDef> panels;
    juce::StringArray ids;                                   // хранит составные идентификаторы параметров
    juce::OwnedArray<juce::Component> owned;
    MatrixView matrix; SeqView seq; PresetBrowser browser; MeterView meter;
    LogoButton logo; std::unique_ptr<AboutPanel> about;
    juce::MidiKeyboardComponent kb;
    juce::Label status, routesLabel, voicesLabel, octLabel;
    juce::TextButton* presetBtn = nullptr; juce::TextButton* settingsBtn = nullptr; juce::TextButton* rangeBtn = nullptr;
    int kbKeys = 88, kbOct = 4, shownVoices = -1, slow = 0;
    std::vector<std::pair<int, juce::Component*>> learnTargets;      // параметр -> регулятор (для MIDI Learn)
    juce::Slider* bendSl = nullptr; juce::Slider* modSl = nullptr;
    int seenHw = 0, seenLink = 0, lastLearn = -1, linkTicks = 0; juce::String linkMsg;
    // Привязки параметров объявлены последними: уничтожаются раньше компонентов, к которым привязаны.
    juce::OwnedArray<APVTS::SliderAttachment> sa;
    juce::OwnedArray<APVTS::ComboBoxAttachment> ca;
    juce::OwnedArray<APVTS::ButtonAttachment> ba;
};

//======================================================================================================
SynthiProEditor::SynthiProEditor (SynthiProProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    Lang::ru = proc.getUiProp ("lang", juce::SystemStats::getUserLanguage().startsWithIgnoreCase ("ru") ? "ru" : "en") == "ru";   // по умолчанию — язык системы
    look = std::make_unique<SpLook>();
    setLookAndFeel (look.get());
    tooltip = std::make_unique<juce::TooltipWindow> (this, 700);
    seenState = proc.stateVersion.load();
    rebuild();
    setResizable (true, true);
    setResizeLimits (896, 560, 2560, 1600);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) BaseW / (double) BaseH);
    const double s = juce::jlimit (0.7, 2.0, proc.getUiProp ("scale", "1").getDoubleValue());
    setSize (juce::roundToInt (BaseW * s), juce::roundToInt (BaseH * s));
    startTimerHz (30);
    if (juce::JUCEApplicationBase::isStandaloneApp())
    {
        shotDir = juce::SystemStats::getEnvironmentVariable ("SYNTHIPRO_SHOTS", {});
        shotWait = 30;
    }
}

SynthiProEditor::~SynthiProEditor()
{
    proc.learnParam.store (-1);
    stopTimer();
    content.reset();
    tooltip.reset();
    setLookAndFeel (nullptr);
}

void SynthiProEditor::paint (juce::Graphics& g) { g.fillAll (cBg); }

void SynthiProEditor::resized()
{
    if (content == nullptr) return;
    const float s = (float) getWidth() / (float) BaseW;
    content->setTransform (juce::AffineTransform::scale (s));
    content->setBounds (0, 0, BaseW, BaseH);
    proc.setUiProp ("scale", juce::String (s, 3));
}

void SynthiProEditor::setScale (double s) { setSize (juce::roundToInt (BaseW * s), juce::roundToInt (BaseH * s)); }

void SynthiProEditor::rebuild()
{
    content.reset();
    Lang::ru = proc.getUiProp ("lang", juce::SystemStats::getUserLanguage().startsWithIgnoreCase ("ru") ? "ru" : "en") == "ru";   // по умолчанию — язык системы
    content = std::make_unique<Content> (*this, proc);
    addAndMakeVisible (*content);
    focusGrabbed = false;
    resized();
}

void SynthiProEditor::timerCallback()
{
    const int sv = proc.stateVersion.load();
    if (sv != seenState) { seenState = sv; needRebuild = true; }
    if (needRebuild) { needRebuild = false; rebuild(); }
    if (content == nullptr) return;
    content->tick();
    if (! focusGrabbed && isShowing()) { content->keyboard().grabKeyboardFocus(); focusGrabbed = true; }
    if (shotDir.isNotEmpty()) shotTick();
}

void SynthiProEditor::saveShot (const juce::String& name)
{
    const auto img = content->createComponentSnapshot (content->getLocalBounds(), true, 2.0f);
    const auto file = juce::File (shotDir).getChildFile (name);
    file.deleteFile();
    juce::FileOutputStream out (file);
    if (out.openedOk()) juce::PNGImageFormat().writeImageToStream (img, out);
}

void SynthiProEditor::shotLog (const juce::String& line)
{
    juce::File (shotDir).getChildFile ("midi_test.txt").appendText (line + "\r\n");
}

void SynthiProEditor::shotTick()
{
    if (shotWait > 0) { --shotWait; return; }
    auto setup = [this] (const char* lang, const char* mode, const char* preset)
    {
        proc.setUiProp ("lang", lang); proc.setUiProp ("mode", mode);
        if (preset != nullptr) proc.presets.load (proc.presets.indexOf (preset));
        rebuild();
    };
    switch (shotStep++)
    {
        case 0:  proc.getStateInformation (shotSaved); juce::File (shotDir).createDirectory(); juce::File (shotDir).getChildFile ("midi_test.txt").deleteFile(); setup ("ru", "pro", "Berlin School"); break;
        case 1:  saveShot ("01_pro_ru.png"); content->demo (1); break;
        case 2:  saveShot ("02_presets_ru.png"); setup ("ru", "easy", "Warm Analog Pad"); break;
        case 3:  saveShot ("03_easy_ru.png"); setup ("ru", "pro", "Evolving Organism"); break;
        case 4:  saveShot ("04_matrix_ru.png"); content->demo (2); break;
        case 5:  saveShot ("05_about_ru.png"); setup ("en", "pro", "Acid Line"); break;
        case 6:  saveShot ("06_pro_en.png"); shotMap = proc.midiMapToString(); setup ("ru", "pro", "Solo Saw"); break;
        case 7:  proc.learnParam.store (sp::P_FCut); break;                                  // как правый щелчок по ручке Cutoff
        case 8:  saveShot ("07_midi_learn_ru.png"); proc.debugInjectCC (21, 64); break;       // «повернули ручку CC 21»
        case 9:  shotLog ("after learn: f_cut is linked to CC " + juce::String (proc.ccForParam (sp::P_FCut)) + ", learn mode " + juce::String (proc.learnParam.load()));
                 saveShot ("08_midi_linked_ru.png"); proc.debugInjectCC (21, 127); break;
        case 10: shotLog ("CC21=127 -> f_cut = " + juce::String (proc.apvts.getRawParameterValue ("f_cut")->load(), 1) + " Hz"); proc.debugInjectCC (21, 0); break;
        case 11: shotLog ("CC21=0   -> f_cut = " + juce::String (proc.apvts.getRawParameterValue ("f_cut")->load(), 1) + " Hz"); proc.debugInjectCC (21, 64); break;
        case 12: shotLog ("CC21=64  -> f_cut = " + juce::String (proc.apvts.getRawParameterValue ("f_cut")->load(), 1) + " Hz"); proc.debugInjectCC (1, 100); break;
        case 13: shotLog ("CC1=100  -> hwMod = " + juce::String (proc.hwMod.load(), 3)); saveShot ("09_mod_wheel_follows.png");
                 proc.debugInjectCC (1, 0); proc.midiMapFromString (shotMap); proc.saveMidiMap(); break;
        default:
            shotDir.clear();
            proc.setStateInformation (shotSaved.getData(), (int) shotSaved.getSize());      // вернуть всё как было
            juce::JUCEApplicationBase::quit();
            return;
    }
    shotWait = 20;
}
