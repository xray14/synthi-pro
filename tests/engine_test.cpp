// Автономные тесты движка SYNTHI PRO (без JUCE).
// Сборка:  cl /O2 /EHsc /std:c++17 /utf-8 /I Source tests\engine_test.cpp
#include "dsp/Engine.h"
#if __has_include("FactoryPresets.h")
 #include "FactoryPresets.h"
 #define HAVE_PRESETS 1
#endif
#include <chrono>
#include <cstdio>
#include <map>
#include <memory>
#include <sstream>
#include <vector>

using namespace sp;
static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++failures; std::printf("  FAIL: "); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

struct Stats { double peak = 0, rms = 0; int nan = 0; double freq = 0; };

static std::vector<float> render(Engine& e, double seconds, float sr, std::vector<float>* right = nullptr) {
    const int n = (int) (seconds * sr); std::vector<float> L((size_t) n), R((size_t) n);
    for (int i = 0; i < n; i += 256) e.process(L.data() + i, R.data() + i, std::min(256, n - i));
    if (right) *right = R;
    return L;
}
static Stats analyse(const std::vector<float>& x, float sr) {
    Stats s; double sum = 0; int cross = 0; int first = -1, last = -1; double l1 = 0, l2 = 0, prev = 0; const double lc = 1.0 - std::exp(-6.283185307179586 * 1200.0 / sr);
    for (size_t i = 0; i < x.size(); ++i) {
        const float v = x[i]; if (!(v == v) || std::fabs(v) > 100.0f) { ++s.nan; continue; }
        s.peak = std::max(s.peak, (double) std::fabs(v)); sum += (double) v * v;
        l1 += (v - l1) * lc; l2 += (l1 - l2) * lc;            // частоту считаем по сглаженному сигналу, пропуская начало
        if (i > x.size() / 5 && prev <= 0 && l2 > 0) { if (first < 0) first = (int) i; last = (int) i; ++cross; }
        prev = l2;
    }
    s.rms = std::sqrt(sum / std::max<size_t>(1, x.size()));
    if (cross > 1) s.freq = (cross - 1) * (double) sr / (last - first);
    return s;
}
static void setDefaults(Engine& e) { const auto& d = paramDefs(); for (int i = 0; i < P_COUNT; ++i) e.setParam(i, d[(size_t) i].def); }
static std::unique_ptr<Engine> make(float sr) { auto e = std::make_unique<Engine>(); setDefaults(*e); e->prepare(sr); return e; }

// Доля энергии вне гармоник основной частоты, дБ (оценка алиасинга).
static double aliasDb(const std::vector<float>& x, float sr, double f0) {
    const int N = 16384; if ((int) x.size() < N + 4096) return 0;
    const float* p = x.data() + 4096; double harm = 0, total = 0;
    std::vector<double> w((size_t) N); for (int i = 0; i < N; ++i) { const double q = 6.283185307179586 * i / (N - 1); w[(size_t) i] = 0.35875 - 0.48829 * std::cos(q) + 0.14128 * std::cos(2 * q) - 0.01168 * std::cos(3 * q); }   // Blackman-Harris
    for (int k = 4; k < N / 2; ++k) {
        double re = 0, im = 0; const double a = 6.283185307179586 * k / N;
        for (int i = 0; i < N; i += 1) { const double v = p[i] * w[(size_t) i]; re += v * std::cos(a * i); im -= v * std::sin(a * i); }
        const double e = re * re + im * im, f = k * (double) sr / N, h = f / f0, dist = std::fabs(h - std::round(h)) * f0;
        if (f > 18000.0) break;                                  // считаем только слышимую полосу
        total += e; if (dist < 6.0 * sr / N && std::round(h) >= 1) harm += e;
    }
    return 10.0 * std::log10(std::max(1e-20, (total - harm) / total));
}

int main() {
    const float sr = 48000;
    std::printf("SYNTHI PRO engine test: %d parameters, %d matrix points\n", (int) P_COUNT, (int) (NSRC * NDST));
    CHECK((int) paramDefs().size() == P_COUNT, "param table size %d != %d", (int) paramDefs().size(), (int) P_COUNT);
    CHECK(paramIndex("mx_lfo1_cut") == P_Mx0 + S_LFO1 * NDST + D_CUT, "matrix id mapping");
    CHECK(paramIndex("seq_g1") == P_SeqG0, "seq id mapping");

    std::printf("[1] Tuning and waveforms\n");
    for (int w = 0; w < 5; ++w) {
        auto e = make(sr); e->setParam(P_O1Wave, (float) w); e->setParam(P_FCut, 20000); e->setParam(P_FEnv, 0); e->setParam(P_Drift, 0); e->setParam(P_FRes, 0);
        e->noteOn(69, 0.8f); auto x = render(*e, 1.0, sr); auto s = analyse(x, sr);
        std::printf("  wave %d: freq %.2f Hz peak %.3f rms %.3f\n", w, s.freq, s.peak, s.rms);
        CHECK(std::fabs(s.freq - 440.0) < 0.6, "A4 tuning wave %d: %.2f", w, s.freq);
        CHECK(s.nan == 0 && s.rms > 0.02 && s.peak < 1.0, "level wave %d", w);
    }

    std::printf("[2] Aliasing at high pitch (non-harmonic energy, lower is better)\n");
    for (int w = 2; w <= 3; ++w) {
        auto e = make(sr); e->setParam(P_O1Wave, (float) w); e->setParam(P_FCut, 20000); e->setParam(P_FEnv, 0); e->setParam(P_FKey, 0); e->setParam(P_Drift, 0); e->setParam(P_FRes, 0); e->setParam(P_E1S, 1);
        e->noteOn(96, 1.0f); auto x = render(*e, 1.0, sr); const double f0 = 440.0 * std::pow(2.0, (96 - 69) / 12.0);
        const double a = aliasDb(x, sr, f0);
        std::printf("  wave %d at %.0f Hz: %.1f dB\n", w, f0, a);
        CHECK(a < -55.0, "aliasing wave %d: %.1f dB", w, a);
    }

    std::printf("[3] Filter modes, self-oscillation, sync, FM, ring\n");
    for (int m = 0; m < 6; ++m) {
        auto e = make(sr); e->setParam(P_FMode, (float) m); e->setParam(P_FCut, 1200); e->setParam(P_FRes, 0.7f);
        e->noteOn(48, 0.9f); auto s = analyse(render(*e, 0.8, sr), sr);
        std::printf("  mode %d: peak %.3f rms %.3f\n", m, s.peak, s.rms);
        CHECK(s.nan == 0 && s.rms > 0.005 && s.peak < 1.0, "filter mode %d", m);
    }
    {
        auto e = make(sr); e->setParam(P_O1Level, 0); e->setParam(P_FRes, 1.0f); e->setParam(P_FCut, 1000); e->setParam(P_FEnv, 0); e->setParam(P_FKey, 0); e->setParam(P_E1S, 1);
        e->noteOn(60, 1.0f); auto x = render(*e, 1.5, sr); std::vector<float> tail(x.begin() + 48000, x.end()); auto s = analyse(tail, sr);
        std::printf("  self-oscillation: %.1f Hz (cutoff 1000), rms %.3f\n", s.freq, s.rms);
        CHECK(s.rms > 0.01 && std::fabs(s.freq - 1000.0) < 60.0, "self oscillation %.1f Hz rms %.4f", s.freq, s.rms);
    }
    {
        auto e = make(sr); e->setParam(P_O2Level, 0.8f); e->setParam(P_O1Level, 0); e->setParam(P_O2Sync, 1); e->setParam(P_O2Semi, 19); e->setParam(P_FCut, 20000); e->setParam(P_FEnv, 0); e->setParam(P_Drift, 0);
        e->noteOn(57, 0.8f); auto s = analyse(render(*e, 1.0, sr), sr);
        std::printf("  hard sync: rms %.3f\n", s.rms); CHECK(s.nan == 0 && s.rms > 0.02, "sync");
        auto e2 = make(sr); e2->setParam(P_O1FM, 0.7f); e2->setParam(P_O1Wave, 0); e2->setParam(P_O2Semi, 12); e2->setParam(P_RingLevel, 0.5f); e2->setParam(paramIndex("mx_vco3_p1"), 0.4f);
        e2->noteOn(57, 0.8f); s = analyse(render(*e2, 1.0, sr), sr);
        std::printf("  FM + ring + audio-rate matrix: rms %.3f\n", s.rms); CHECK(s.nan == 0 && s.rms > 0.02, "fm/ring");
    }

    std::printf("[4] Matrix really modulates\n");
    {
        auto a = make(sr), b = make(sr);
        for (auto* e : { a.get(), b.get() }) { e->setParam(P_Drift, 0); e->setParam(P_FEnv, 0); e->setParam(P_L1Rate, 4); }
        b->setParam(paramIndex("mx_lfo1_cut"), 0.8f);
        a->noteOn(48, 0.8f); b->noteOn(48, 0.8f);
        auto xa = render(*a, 1.0, sr), xb = render(*b, 1.0, sr); double diff = 0; for (size_t i = 0; i < xa.size(); ++i) diff += std::fabs(xa[i] - xb[i]);
        std::printf("  LFO1 > Cutoff: mean abs difference %.4f\n", diff / xa.size()); CHECK(diff / xa.size() > 0.003, "matrix has no effect");
        auto c = make(sr); c->setParam(P_Drift, 0); c->setParam(P_FCut, 20000); c->setParam(P_FEnv, 0); c->setParam(P_O1Wave, 0); c->setParam(P_L1Wave, 3); c->setParam(P_L1Rate, 0.02f); c->setParam(P_L1Retrig, 1);
        c->setParam(paramIndex("mx_lfo1_p1"), 0.5f);   // 0.25 * 4 окт = +1 октава на положительной полуволне
        c->noteOn(69, 0.8f); auto s = analyse(render(*c, 0.5, sr), sr);
        std::printf("  LFO1 square > VCO1 pitch at depth 0.5: %.1f Hz (expect 880)\n", s.freq); CHECK(std::fabs(s.freq - 880.0) < 2.0, "pitch mod scale %.1f", s.freq);
    }

    std::printf("[5] Envelope release, voice modes, glide, sequencer\n");
    {
        auto e = make(sr); e->noteOn(60, 0.8f); render(*e, 0.3, sr); e->noteOff(60); auto x = render(*e, 3.0, sr); std::vector<float> tail(x.end() - 9600, x.end());
        auto s = analyse(tail, sr); std::printf("  silence after release: peak %.6f, voices %d\n", s.peak, e->uiVoices.load()); CHECK(s.peak < 1e-4 && e->uiVoices.load() == 0, "release tail");
        auto p = make(sr); for (int n = 0; n < 12; ++n) p->noteOn(48 + n * 3, 0.7f); render(*p, 0.2, sr);
        std::printf("  poly: 12 keys -> %d voices (limit 8)\n", p->uiVoices.load()); CHECK(p->uiVoices.load() == 8, "voice limit");
        auto m = make(sr); m->setParam(P_VoiceMode, 1); m->setParam(P_Glide, 0.5f); m->setParam(P_Drift, 0); m->setParam(P_FCut, 20000); m->setParam(P_FEnv, 0); m->setParam(P_O1Wave, 0); render(*m, 0.05, sr);
        m->noteOn(57, 0.8f); render(*m, 0.5, sr); m->noteOn(69, 0.8f); auto g1 = analyse(render(*m, 0.05, sr), sr); render(*m, 2.0, sr); auto g2 = analyse(render(*m, 0.3, sr), sr);
        std::printf("  mono glide: %.1f Hz just after key, %.1f Hz later; voices %d\n", g1.freq, g2.freq, m->uiVoices.load());
        CHECK(g1.freq > 215 && g1.freq < 330 && std::fabs(g2.freq - 440) < 2 && m->uiVoices.load() == 1, "glide/mono");
        auto q = make(sr); q->setParam(P_SeqOn, 1); q->setParam(P_SeqLen, 4); q->setParam(P_SeqBpm, 120); q->setParam(P_SeqRate, 3); render(*q, 0.05, sr);
        q->noteOn(60, 0.8f); std::map<int, int> seen; for (int i = 0; i < 100; ++i) { render(*q, 0.01, sr); seen[q->uiSeqStep.load()]++; }
        std::printf("  sequencer steps visited in 1 s at 120 BPM 1/16:"); for (auto& kv : seen) std::printf(" %d(x%d)", kv.first, kv.second); std::printf("\n");
        CHECK(seen.size() == 4, "sequencer steps");
        q->noteOff(60); auto t = render(*q, 3.0, sr); std::vector<float> tl(t.end() - 4800, t.end()); CHECK(analyse(tl, sr).peak < 1e-4, "sequencer stops");
    }

    std::printf("[6] Effects change the signal, stay finite, and decay\n");
    {
        const int on[6] = { P_DistOn, P_SatOn, P_ChoOn, P_PhaOn, P_DlyOn, P_RevOn }; const char* nm[6] = { "distortion", "saturation", "chorus", "phaser", "delay", "reverb" };
        auto base = make(sr); base->setParam(P_Drift, 0); base->noteOn(48, 0.8f); auto xb = render(*base, 1.0, sr);
        for (int k = 0; k < 6; ++k) {
            auto e = make(sr); e->setParam(P_Drift, 0); e->setParam(on[k], 1); e->noteOn(48, 0.8f); std::vector<float> r; auto x = render(*e, 1.0, sr, &r);
            double diff = 0; for (size_t i = 0; i < x.size(); ++i) diff += std::fabs(x[i] - xb[i]); auto s = analyse(x, sr);
            e->noteOff(48); auto t = render(*e, 12.0, sr); std::vector<float> tl(t.end() - 4800, t.end());
            std::printf("  %-10s diff %.4f peak %.3f tail %.6f\n", nm[k], diff / x.size(), s.peak, analyse(tl, sr).peak);
            CHECK(diff / x.size() > 0.002 && s.nan == 0 && s.peak <= 1.0 && analyse(tl, sr).peak < 2e-3, "effect %s", nm[k]);
        }
    }

    std::printf("[7] Clicks on parameter jumps (max sample-to-sample step of a sine)\n");
    {
        auto e = make(sr); e->setParam(P_O1Wave, 0); e->setParam(P_FCut, 20000); e->setParam(P_FEnv, 0); e->setParam(P_Drift, 0); e->setParam(P_E1S, 1); e->noteOn(57, 0.8f); render(*e, 0.5, sr);
        std::vector<float> L(4800), R(4800); double worst = 0;
        for (int i = 0; i < 4800; i += 64) { if (i == 960) e->setParam(P_ALevel, 0.1f); if (i == 1920) e->setParam(P_MasterVol, 0.2f); if (i == 2880) e->setParam(P_FCut, 300); if (i == 3840) e->setParam(P_APan, 1); e->process(L.data() + i, R.data() + i, 64); }
        for (int i = 1; i < 4800; ++i) worst = std::max(worst, (double) std::fabs(L[i] - L[i - 1]));
        std::printf("  worst step %.4f\n", worst); CHECK(worst < 0.06, "zipper/click %.4f", worst);
    }

    std::printf("[7b] Mod wheel: built-in vibrato and matrix routing\n");
    {
        // Высота тона по периодам: размах мгновенной частоты за время звучания.
        auto pitchSpan = [sr] (const std::vector<float>& x) {
            double lo = 1e9, hi = 0; int last = -1; double l1 = 0, l2 = 0, prev = 0; const double lc = 1.0 - std::exp(-6.283185307179586 * 1200.0 / sr);
            for (size_t i = 0; i < x.size(); ++i) {
                l1 += (x[i] - l1) * lc; l2 += (l1 - l2) * lc;
                if (i > x.size() / 4 && prev <= 0 && l2 > 0) { if (last >= 0) { const double f = sr / (double) ((int) i - last); lo = std::min(lo, f); hi = std::max(hi, f); } last = (int) i; }
                prev = l2;
            }
            return hi > 0 ? 1200.0 * std::log2(hi / lo) : 0.0;          // в центах
        };
        auto mk = [&] (float wheel, float vib) { auto e = make(sr); e->setParam(P_Drift, 0); e->setParam(P_O1Wave, 0); e->setParam(P_FCut, 20000); e->setParam(P_FEnv, 0); e->setParam(P_E1S, 1); e->setParam(P_ModVib, vib);
                                                 e->modWheel(wheel); e->noteOn(45, 0.8f); return pitchSpan(render(*e, 2.0, sr)); };
        const double off = mk(0.0f, 0.35f), on = mk(1.0f, 0.35f), full = mk(1.0f, 1.0f), disabled = mk(1.0f, 0.0f);
        std::printf("  default patch, pitch span: wheel down %.1f ct, wheel up %.1f ct, knob at max %.1f ct, knob at zero %.1f ct\n", off, on, full, disabled);
        CHECK(off < 12.0 && disabled < 12.0, "vibrato present with wheel down (%.1f / %.1f ct)", off, disabled);
        CHECK(on > 50.0 && on < 90.0, "wheel vibrato at default depth: %.1f ct (expected about 70)", on);
        CHECK(full > 170.0 && full < 230.0, "wheel vibrato at full depth: %.1f ct (expected about 200)", full);
        auto a = make(sr), b = make(sr);
        for (auto* e : { a.get(), b.get() }) { e->setParam(P_Drift, 0); e->setParam(P_ModVib, 0); e->setParam(P_FCut, 400); e->setParam(P_FEnv, 0); e->setParam(paramIndex("mx_mod_cut"), 0.7f); }
        b->modWheel(1.0f); a->noteOn(48, 0.8f); b->noteOn(48, 0.8f);
        // Яркость = энергия разности соседних сэмплов: открытие фильтра колесом должно заметно её поднять.
        auto bright = [] (const std::vector<float>& x) { double s = 0; for (size_t i = x.size() / 2 + 1; i < x.size(); ++i) { const double d = x[i] - x[i - 1]; s += d * d; } return std::sqrt(s / (x.size() / 2)); };
        const double ba = bright(render(*a, 1.0, sr)), bb = bright(render(*b, 1.0, sr));
        std::printf("  matrix Mod Wheel > Cutoff: brightness %.4f with wheel down, %.4f with wheel up (x%.1f)\n", ba, bb, bb / ba);
        CHECK(bb > ba * 2.0, "matrix route from the mod wheel has no effect");
    }

    std::printf("[8] Sample rates and CPU load\n");
    for (float r : { 44100.0f, 48000.0f, 96000.0f }) {
        auto e = make(r); e->setParam(P_Drift, 0); e->setParam(P_FCut, 20000); e->setParam(P_FEnv, 0); e->setParam(P_O1Wave, 0); e->noteOn(69, 0.8f); auto s = analyse(render(*e, 0.5, r), r);
        std::printf("  %.0f Hz: A4 = %.2f Hz\n", r, s.freq); CHECK(std::fabs(s.freq - 440) < 1.0, "tuning at %.0f", r);
    }
    {
        auto e = make(sr); e->setParam(P_Voices, 16); e->setParam(P_O2Level, 0.7f); e->setParam(P_O3Level, 0.7f); e->setParam(P_NoiseLevel, 0.2f); e->setParam(P_E1S, 1);
        for (int k : { P_DistOn, P_SatOn, P_ChoOn, P_PhaOn, P_DlyOn, P_RevOn }) e->setParam(k, 1);
        for (int s = 0; s < 8; ++s) e->setParam(P_Mx0 + s * NDST + (s * 5) % NDST, 0.3f);
        for (int n = 0; n < 16; ++n) e->noteOn(36 + n * 3, 0.8f);
        const auto t0 = std::chrono::steady_clock::now(); auto s = analyse(render(*e, 5.0, sr), sr);
        const double ms = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - t0).count();
        std::printf("  16 voices x 3 VCO + 8 matrix routes + all FX: %.1f %% of one core, peak %.3f, voices %d\n", ms / 50.0, s.peak, e->uiVoices.load());
        CHECK(s.nan == 0 && ms / 50.0 < 60.0, "cpu %.1f%%", ms / 50.0);
    }

#ifdef HAVE_PRESETS
    std::printf("[9] Factory presets: every preset must load, sound, stay finite and not clip\n");
    {
        std::map<std::string, int> cats; int bad = 0; std::map<std::string, int> names; double loud = 0, quiet = 1e9; int withMatrix = 0;
        FILE* lv = std::fopen("build-test/preset_levels.txt", "w");
        for (int i = 0; i < kNumFactoryPresets; ++i) {
            const auto& fp = kFactoryPresets[i]; cats[fp.category]++; names[fp.name]++;
            auto e = make(sr); std::stringstream ss(fp.data); std::string item; int nset = 0, nmx = 0; bool ok = true;
            while (std::getline(ss, item, ';')) {
                const auto eq = item.find('='); if (eq == std::string::npos) continue;
                const std::string id = item.substr(0, eq); const float val = std::stof(item.substr(eq + 1)); const int idx = paramIndex(id);
                if (idx < 0) { std::printf("  FAIL %s: unknown parameter '%s'\n", fp.name, id.c_str()); ok = false; continue; }
                const auto& d = paramDefs()[(size_t) idx];
                if (val < d.min - 1e-4f || val > d.max + 1e-4f) { std::printf("  FAIL %s: %s=%g out of range [%g, %g]\n", fp.name, id.c_str(), val, d.min, d.max); ok = false; }
                e->setParam(idx, val); ++nset; if (idx >= P_Mx0 && std::fabs(val) > 0.004f) ++nmx;
            }
            e->prepare(sr);
            const bool seq = e->getParam(P_SeqOn) > 0.5f;
            e->noteOn(seq ? 48 : 45, 0.85f); if (!seq) { e->noteOn(57, 0.85f); e->noteOn(64, 0.85f); }
            auto x = render(*e, 4.0, sr); auto s = analyse(x, sr);
            if (nset < 8) { std::printf("  FAIL %s: only %d parameters set\n", fp.name, nset); ok = false; }
            if (s.nan) { std::printf("  FAIL %s: NaN/inf\n", fp.name); ok = false; }
            if (s.peak < 0.08) { std::printf("  FAIL %s: too quiet (peak %.4f)\n", fp.name, s.peak); ok = false; }
            if (nmx > 0) ++withMatrix;
            if (lv) std::fprintf(lv, "%s\t%.5f\t%.4f\n", fp.name, s.peak, e->getParam(P_MasterVol));
            if (s.peak > 0.999) { std::printf("  FAIL %s: hits the limiter ceiling (peak %.3f)\n", fp.name, s.peak); ok = false; }
            loud = std::max(loud, s.peak); quiet = std::min(quiet, s.peak);
            if (!ok) ++bad;
        }
        for (auto& kv : names) if (kv.second > 1) { std::printf("  FAIL duplicate preset name '%s'\n", kv.first.c_str()); ++bad; }
        if (lv) std::fclose(lv);
        std::printf("  %d presets in %d categories; %d use the matrix; peak range %.3f .. %.3f\n", kNumFactoryPresets, (int) cats.size(), withMatrix, quiet, loud);
        for (auto& kv : cats) std::printf("    %-24s %d\n", kv.first.c_str(), kv.second);
        CHECK(bad == 0, "%d bad presets", bad); CHECK(kNumFactoryPresets >= 150, "need 150 presets, have %d", kNumFactoryPresets);
    }
#endif
    std::printf(failures ? "\nRESULT: %d FAILURE(S)\n" : "\nRESULT: ALL TESTS PASSED\n", failures);
    return failures ? 1 : 0;
}
