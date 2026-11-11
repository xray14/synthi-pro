// SYNTHI PRO — звуковой движок. Чистый C++17 без зависимостей: один и тот же код работает
// в Standalone, в VST3 и в автономных тестах. В process() нет выделения памяти и блокировок.
#pragma once
#include "Params.h"
#include "Effects.h"
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace sp {

inline float fastExp2(float x) {
    x = x < -40.0f ? -40.0f : (x > 40.0f ? 40.0f : x);
    const float fl = std::floor(x), f = x - fl;
    const float p = 1.0f + f * (0.69314718f + f * (0.24022651f + f * (0.05550411f + f * (0.00961813f + f * 0.00133336f))));
    return std::ldexp(p, (int) fl);
}
inline float fastTan(float x) { const float x2 = x * x; return x * (15.0f - x2) / (15.0f - 6.0f * x2); }
inline float softSat(float x) { if (x > 3.0f) return 1.0f; if (x < -3.0f) return -1.0f; const float x2 = x * x; return x * (27.0f + x2) / (27.0f + 9.0f * x2); }
inline float clampf(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }

struct SineTable {
    enum { N = 2048 };
    float t[N + 1];
    SineTable() { for (int i = 0; i <= N; ++i) t[i] = (float) std::sin(6.283185307179586 * i / N); }
    inline float operator() (double ph) const { const double x = ph * N; const int i = (int) x; const float f = (float) (x - i); return t[i] + (t[i + 1] - t[i]) * f; }
};
inline const SineTable& sineTable() { static const SineTable s; return s; }

struct Rng {
    uint32_t s = 0x1234567u;
    inline float bi() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (int32_t) s * (1.0f / 2147483648.0f); }   // [-1, 1)
    inline float uni() { return 0.5f * bi() + 0.5f; }
};

struct Env {
    int st = 0; float lv = 0;
    inline void gateOn() { st = 1; }
    inline void gateOff() { if (st != 0) st = 4; }
    inline float tick(float aC, float dC, float s, float rC) {
        if (st == 1) { lv += (1.3f - lv) * aC; if (lv >= 1.0f) { lv = 1.0f; st = 2; } }
        else if (st == 2) lv += (s - lv) * dC;
        else if (st == 4) { lv -= lv * rC; if (lv < 1e-4f) { lv = 0; st = 0; } }
        return lv;
    }
};

struct Lfo {
    float ph = 0, cur = 0, nxt = 0, fade = 1;
    inline float tick(int wave, float inc, float fadeInc, Rng& r) {
        ph += inc; if (ph >= 1.0f) { ph -= 1.0f; cur = nxt; nxt = r.bi(); }
        if (fade < 1.0f) { fade += fadeInc; if (fade > 1.0f) fade = 1.0f; }
        float v;
        switch (wave) {
            case 0: v = sineTable() (ph); break;
            case 1: v = ph < 0.25f ? 4.0f * ph : (ph < 0.75f ? 2.0f - 4.0f * ph : 4.0f * ph - 4.0f); break;
            case 2: v = 1.0f - 2.0f * ph; break;
            case 3: v = ph < 0.5f ? 1.0f : -1.0f; break;
            case 4: v = cur; break;
            default: { const float t = ph * ph * (3.0f - 2.0f * ph); v = cur + (nxt - cur) * t; } break;
        }
        return v * fade;
    }
};

// Параметры, общие для всех голосов на время одного подблока (до 16 сэмплов на повышенной частоте).
struct Blk {
    int wave[3]; float off[3]; bool key[3]; float pw[3], lvl[3], fm[3]; bool sync[3]; bool low3;
    float nCol, nLvl, rLvl;
    int fmode; float cutOct, res, drive, fenv, fkey, fvel;
    float alevel, pan, velSens;
    float envA[3], envD[3], envS[3], envR[3];
    int lwave[2]; float linc[2], lfade[2];
    float shInc, shLag; int shSrc;
    int nc; uint8_t cs[NSRC * NDST], cd[NSRC * NDST]; float cg[NSRC * NDST]; bool du[NDST];
    float glideC, bend, tune, drift, mod, at, seqVal, osr;
    float vib;                                               // глубина вибрато от колеса модуляции, октавы на единицу LFO 1
};

struct Voice {
    bool active = false, gate = false, sustained = false;
    int note = -1; float vel = 0, pitch = 60, target = 60; uint64_t age = 0;
    double ph[3] = { 0, 0, 0 }; float z[3] = { 0, 0, 0 };
    float drift[3] = { 0, 0, 0 }, driftT[3] = { 0, 0, 0 }; int driftCnt = 0;
    Env env[3]; Lfo lfo[2];
    float shPh = 1, shVal = 0, shOut = 0;
    float ls[4] = { 0, 0, 0, 0 }, ic1 = 0, ic2 = 0;
    float pink[3] = { 0, 0, 0 };
    float src[NSRC] = {};
    Rng rng;

    static inline float naive(int w, double ph, float pw) {
        switch (w) {
            case 0: return sineTable() (ph);
            case 1: return ph < 0.5 ? (float) (4.0 * ph - 1.0) : (float) (3.0 - 4.0 * ph);
            case 2: return (float) (2.0 * ph - 1.0);
            case 3: return ph < 0.5 ? 1.0f : -1.0f;
            default: return ph < pw ? 1.0f : -1.0f;
        }
    }

    // Один сэмпл генератора: polyBLEP на разрывах, жёсткая синхронизация с компенсацией скачка, фазовая (линейная) FM.
    inline float osc(int i, float inc, int wave, float pw, float pm, bool syncNow, float tSync, bool& wrapped, float& tWrap) {
        double p = ph[i]; const double old = p; p += inc; wrapped = false; tWrap = 0;
        if (p >= 1.0) { p -= 1.0; wrapped = true; tWrap = (float) (p / inc); }
        float cur, corrPrev = 0;
        auto blep = [&] (float h, float t) { t = clampf(t, 0.0f, 1.0f); const float u = 1.0f - t; cur -= h * 0.5f * u * u; corrPrev += h * 0.5f * t * t; };
        if (syncNow) {
            double pr = p - (double) inc * tSync; if (pr < 0) pr += 1.0;
            const float before = naive(wave, pr, pw);
            p = (double) inc * tSync; if (p >= 1.0) p -= std::floor(p);
            cur = naive(wave, p, pw);
            blep(naive(wave, 0.0, pw) - before, tSync);
        } else if (pm != 0.0f) {
            double pp = p + (double) pm; pp -= std::floor(pp);
            cur = naive(wave, pp, pw);
        } else {
            cur = naive(wave, p, pw);
            if (wave == 2) { if (wrapped) blep(-2.0f, tWrap); }
            else if (wave >= 3) {
                const double w = wave == 3 ? 0.5 : (double) pw;
                if (wrapped) blep(2.0f, tWrap);
                if (wrapped) { if (old < w) blep(-2.0f, (float) ((p + 1.0 - w) / inc)); else if (p >= w) blep(-2.0f, (float) ((p - w) / inc)); }
                else if (old < w && p >= w) blep(-2.0f, (float) ((p - w) / inc));
            }
        }
        const float out = z[i] + corrPrev;
        z[i] = cur; ph[i] = p;
        return out;
    }

    void start(int n, float v, float startPitch, bool retrig, const Blk& b, const float* globalLfoPh) {
        if (!active) {
            for (auto& s : ls) s = 0; ic1 = ic2 = 0; z[0] = z[1] = z[2] = 0;
            for (auto& e : env) { e.lv = 0; e.st = 0; }
            shOut = shVal = 0; shPh = 1;
            std::memset(src, 0, sizeof (src));
            retrig = true;
        }
        note = n; vel = v; target = (float) n; pitch = startPitch; gate = true; sustained = false; active = true;
        if (retrig) {
            for (auto& e : env) e.gateOn();
            for (int i = 0; i < 2; ++i) { lfo[i].fade = b.lfade[i] >= 1.0f ? 1.0f : 0.0f; }
        }
        (void) globalLfoPh;
    }
    void release() { gate = false; sustained = false; for (auto& e : env) e.gateOff(); }

    void render(const Blk& b, float* L, float* R, int n) {
        pitch += (target - pitch) * b.glideC;
        if (--driftCnt <= 0) { driftCnt = 150 + (int) (rng.uni() * 500.0f); for (auto& d : driftT) d = rng.bi(); }
        float inc[3];
        for (int i = 0; i < 3; ++i) {
            drift[i] += (driftT[i] - drift[i]) * 0.004f;
            float semis = (b.key[i] ? pitch : 60.0f) + b.off[i] + b.bend + b.tune + drift[i] * b.drift * 0.18f;
            if (i == 2 && b.low3) semis -= 84.0f;
            inc[i] = 440.0f * fastExp2((semis - 69.0f) * (1.0f / 12.0f)) / b.osr;
        }
        const float keyOct = (pitch - 60.0f) * (1.0f / 12.0f);
        const float cutBase = b.cutOct + b.fkey * keyOct + b.fvel * (vel - 0.5f) * 4.0f;
        const float velGain = 1.0f - b.velSens + b.velSens * vel;
        src[S_VEL] = vel; src[S_KEY] = clampf(keyOct * 0.5f, -1.0f, 1.0f); src[S_MOD] = b.mod; src[S_AT] = b.at; src[S_SEQ] = b.seqVal;
        const float pa = (clampf(b.pan, -1.0f, 1.0f) + 1.0f) * 0.78539816f;
        float panL = std::cos(pa) * 1.41421356f, panR = std::sin(pa) * 1.41421356f;
        const float maxG = 0.9f, piOsr = 3.14159265f / b.osr, maxFc = b.osr * 0.3f;
        const float drv = 1.0f + 5.0f * b.drive, drvPost = 1.0f / (1.0f + 1.6f * b.drive);
        float dst[NDST];

        for (int s = 0; s < n; ++s) {
            // --- источники управления ---
            src[S_ENV1] = env[0].tick(b.envA[0], b.envD[0], b.envS[0], b.envR[0]);
            src[S_ENV2] = env[1].tick(b.envA[1], b.envD[1], b.envS[1], b.envR[1]);
            src[S_ENV3] = env[2].tick(b.envA[2], b.envD[2], b.envS[2], b.envR[2]);
            src[S_LFO1] = lfo[0].tick(b.lwave[0], b.linc[0], b.lfade[0], rng);
            src[S_LFO2] = lfo[1].tick(b.lwave[1], b.linc[1], b.lfade[1], rng);
            shPh += b.shInc;
            if (shPh >= 1.0f) { shPh -= 1.0f; if (shPh >= 1.0f) shPh = 0; shVal = b.shSrc == 0 ? rng.bi() : (b.shSrc == 1 ? src[S_LFO1] : src[S_VCO3]); }
            shOut += (shVal - shOut) * b.shLag; src[S_SH] = shOut;

            // --- матрица коммутации (источники аудиодиапазона взяты с предыдущего сэмпла) ---
            for (int d = 0; d < NDST; ++d) dst[d] = 0;
            for (int c = 0; c < b.nc; ++c) dst[b.cd[c]] += src[b.cs[c]] * b.cg[c];

            // --- три генератора ---
            float o[3]; bool wr0 = false, wr; float tw0 = 0, tw;
            const float vibMul = b.vib > 0.0f ? fastExp2(b.vib * src[S_LFO1]) : 1.0f;      // колесо MOD -> вибрато
            for (int i = 0; i < 3; ++i) {
                float f = inc[i] * vibMul; if (b.du[D_P1 + i]) f *= fastExp2(4.0f * dst[D_P1 + i]);
                if (f > 0.45f) f = 0.45f;
                const float pw = b.du[D_PW1 + i] ? clampf(b.pw[i] + 0.45f * dst[D_PW1 + i], 0.03f, 0.97f) : b.pw[i];
                const float pm = b.fm[i] > 0.0f ? b.fm[i] * src[S_VCO1 + (i + 1) % 3] : 0.0f;
                o[i] = osc(i, f, b.wave[i], pw, pm, b.sync[i] && wr0, tw0, wr, tw);
                if (i == 0) { wr0 = wr; tw0 = tw; }
            }
            // --- шум (белый -> розовый) и кольцевой модулятор ---
            const float white = rng.bi();
            pink[0] = 0.99765f * pink[0] + white * 0.0990460f; pink[1] = 0.96300f * pink[1] + white * 0.2965164f; pink[2] = 0.57000f * pink[2] + white * 1.0526913f;
            const float pk = (pink[0] + pink[1] + pink[2] + white * 0.1848f) * 0.25f;
            const float noise = white + (pk - white) * b.nCol;
            const float ring = o[0] * o[1];
            src[S_VCO1] = o[0]; src[S_VCO2] = o[1]; src[S_VCO3] = o[2]; src[S_NOISE] = noise; src[S_RING] = ring;

            // --- микшер ---
            float l0 = b.lvl[0], l1 = b.lvl[1], l2 = b.lvl[2], ln = b.nLvl, lr = b.rLvl;
            if (b.du[D_L1]) l0 = std::max(0.0f, l0 + dst[D_L1]);
            if (b.du[D_L2]) l1 = std::max(0.0f, l1 + dst[D_L2]);
            if (b.du[D_L3]) l2 = std::max(0.0f, l2 + dst[D_L3]);
            if (b.du[D_LN]) ln = std::max(0.0f, ln + dst[D_LN]);
            if (b.du[D_LR]) lr = std::max(0.0f, lr + dst[D_LR]);
            float x = (o[0] * l0 + o[1] * l1 + o[2] * l2 + noise * ln + ring * lr) * 0.5f + dst[D_FIN] * 0.5f;
            x = x * drv + 1e-9f * white;

            // --- фильтр ---
            float fc = fastExp2(cutBase + b.fenv * 5.0f * src[S_ENV2] + 5.0f * dst[D_CUT]);
            fc = fc < 16.0f ? 16.0f : (fc > maxFc ? maxFc : fc);
            float g = fastTan(fc * piOsr); if (g > maxG * 2.2f) g = maxG * 2.2f;
            const float res = b.du[D_RES] ? clampf(b.res + dst[D_RES], 0.0f, 1.0f) : b.res;
            float y;
            if (b.fmode <= 2) {                               // лестничный ZDF-фильтр, 24 / 18 / 12 дБ на октаву
                const float G = g / (1.0f + g), k = res * 4.15f, G2 = G * G;
                const float S = (1.0f - G) * (G2 * G * ls[0] + G2 * ls[1] + G * ls[2] + ls[3]);
                float u = softSat((x * (1.0f + 0.45f * k) - k * S) / (1.0f + k * G2 * G2));
                float v = (u - ls[0]) * G; const float y1 = v + ls[0]; ls[0] = y1 + v;
                v = (y1 - ls[1]) * G; const float y2 = v + ls[1]; ls[1] = y2 + v;
                v = (y2 - ls[2]) * G; const float y3 = v + ls[2]; ls[2] = y3 + v;
                v = (y3 - ls[3]) * G; const float y4 = v + ls[3]; ls[3] = y4 + v;
                y = b.fmode == 0 ? y4 : (b.fmode == 1 ? y3 : y2);
            } else {                                          // SVF: полосовой / ВЧ / режекторный
                const float k = 2.0f - 1.96f * res, a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
                const float xi = softSat(x);
                const float v3 = xi - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
                ic1 = 2.0f * v1 - ic1; ic2 = 2.0f * v2 - ic2;
                y = b.fmode == 3 ? v1 * (0.6f + 0.7f * k) : (b.fmode == 4 ? xi - k * v1 - v2 : xi - k * v1);
            }
            y *= drvPost;

            // --- VCA и панорама ---
            float amp = src[S_ENV1] * velGain * b.alevel;
            if (b.du[D_VCA]) amp *= std::max(0.0f, 1.0f + dst[D_VCA]);
            y *= amp * 0.8f;
            if (b.du[D_PAN]) { const float q = (clampf(b.pan + dst[D_PAN], -1.0f, 1.0f) + 1.0f) * 0.78539816f; panL = std::cos(q) * 1.41421356f; panR = std::sin(q) * 1.41421356f; }
            L[s] += y * panL; R[s] += y * panR;
        }
        if (env[0].st == 0) { active = false; gate = false; }
    }
};

class Engine {
public:
    enum { MAXV = 16, SUB = 16 };
    std::atomic<float> peakL { 0 }, peakR { 0 };
    std::atomic<int> uiSeqStep { -1 }, uiVoices { 0 };

    Engine() {
        const auto& d = paramDefs();
        for (int i = 0; i < P_COUNT; ++i) { tgt[i].store(d[(size_t) i].def); cur[i] = sm[i] = d[(size_t) i].def; cont[i] = isContinuous(d[(size_t) i].type); }
        for (int i = 0; i < MAXV; ++i) v[i].rng.s = 0x9E3779B9u * (uint32_t) (i + 1) + 12345u;
        std::memset(keys, 0, sizeof (keys));
    }

    void prepare(double sampleRate) {
        sr = (float) sampleRate; os = sampleRate < 88000.0 ? 2 : 1; osr = sr * (float) os;
        fx.prepare(sampleRate, os);
        // децимирующий КИХ-фильтр (окно Кайзера), срез чуть ниже базовой частоты Найквиста
        const double fcN = 0.24, beta = 8.0; double sum = 0;
        auto bessel = [] (double x) { double s = 1, t = 1; for (int k = 1; k < 30; ++k) { t *= (x / (2.0 * k)) * (x / (2.0 * k)); s += t; } return s; };
        for (int i = 0; i < DT; ++i) {
            const double m = i - (DT - 1) / 2.0, sinc = m == 0 ? 2 * fcN : std::sin(6.283185307179586 * fcN * m) / (3.141592653589793 * m);
            const double r = 2.0 * i / (DT - 1) - 1.0, w = bessel(beta * std::sqrt(1 - r * r)) / bessel(beta);
            dh[i] = (float) (sinc * w); sum += dh[i];
        }
        for (auto& h : dh) h = (float) (h / sum);
        std::memset(db, 0, sizeof (db)); dw = 0;
        for (int i = 0; i < P_COUNT; ++i) cur[i] = sm[i] = tgt[i].load();
        for (auto& x : v) { x.active = false; x.gate = false; }
        seqRun = false; seqNote = -1; nStack = 0; lastPitch = 60;
    }

    // --- параметры (любой поток) ---
    void setParam(int id, float value) { if (id >= 0 && id < P_COUNT) tgt[id].store(value, std::memory_order_relaxed); }
    float getParam(int id) const { return tgt[id].load(std::memory_order_relaxed); }
    void setHostTempo(double bpm) { hostBpm = (float) bpm; }

    // --- события (аудиопоток) ---
    void noteOn(int note, float vel) {
        note = note < 0 ? 0 : (note > 127 ? 127 : note);
        if (!keys[note]) { keys[note] = true; ++nKeys; }
        keyVel = vel; seqBase = note;
        if (pv(P_SeqOn) > 0.5f) return;                        // при включённом секвенсоре клавиши только задают тональность
        voiceOn(note, vel);
    }
    void noteOff(int note) {
        note = note < 0 ? 0 : (note > 127 ? 127 : note);
        if (keys[note]) { keys[note] = false; --nKeys; }
        if (pv(P_SeqOn) > 0.5f) { if (nKeys > 0) { for (int i = 127; i >= 0; --i) if (keys[i]) { seqBase = i; break; } } return; }
        voiceOff(note);
    }
    void pitchBend(float x) { bend = clampf(x, -1.0f, 1.0f); }
    void modWheel(float x) { mod = clampf(x, 0.0f, 1.0f); }
    void aftertouch(float x) { at = clampf(x, 0.0f, 1.0f); }
    void sustain(bool on) { sus = on; if (!on) for (auto& x : v) if (x.active && x.sustained) x.release(); }
    void allNotesOff() {
        for (auto& x : v) if (x.active) x.release();
        std::memset(keys, 0, sizeof (keys)); nKeys = 0; nStack = 0; seqRun = false; seqNote = -1;
    }
    void panic() { allNotesOff(); for (auto& x : v) { x.active = false; x.env[0].lv = 0; x.env[0].st = 0; } fx.reset(); }

    // --- рендер: L и R перезаписываются ---
    void process(float* L, float* R, int n) {
        int done = 0;
        const int chunk = SUB / os;
        while (done < n) {
            const int nb = std::min(chunk, n - done), no = nb * os;
            updateBlock(no);
            runSequencer(nb);
            float bl[SUB], br[SUB];
            for (int i = 0; i < no; ++i) bl[i] = br[i] = 0;
            int active = 0;
            for (int i = 0; i < MAXV; ++i) if (v[i].active) { v[i].render(blk, bl, br, no); ++active; }
            uiVoices.store(active, std::memory_order_relaxed);
            fx.processDrive(bl, br, no, sm);
            float* ol = L + done; float* orr = R + done;
            if (os == 2) {
                for (int i = 0; i < nb; ++i) {
                    db[0][dw] = bl[2 * i]; db[1][dw] = br[2 * i]; dw = (dw + 1) & DM;
                    db[0][dw] = bl[2 * i + 1]; db[1][dw] = br[2 * i + 1]; dw = (dw + 1) & DM;
                    float a = 0, c = 0;
                    for (int t = 0; t < DT; ++t) { const int k = (dw - 1 - t) & DM; a += db[0][k] * dh[t]; c += db[1][k] * dh[t]; }
                    ol[i] = a; orr[i] = c;
                }
            } else for (int i = 0; i < nb; ++i) { ol[i] = bl[i]; orr[i] = br[i]; }
            fx.process(ol, orr, nb, sm);
            const float g0 = masterG, g1 = sm[P_MasterVol] * sm[P_MasterVol] * 2.4f; masterG = g1;
            float pl = 0, pr = 0;
            for (int i = 0; i < nb; ++i) {
                const float g = g0 + (g1 - g0) * (float) (i + 1) / (float) nb;
                float a = limit(ol[i] * g), c = limit(orr[i] * g);
                ol[i] = a; orr[i] = c;
                a = std::fabs(a); c = std::fabs(c); if (a > pl) pl = a; if (c > pr) pr = c;
            }
            if (pl > peakL.load(std::memory_order_relaxed)) peakL.store(pl, std::memory_order_relaxed);
            if (pr > peakR.load(std::memory_order_relaxed)) peakR.store(pr, std::memory_order_relaxed);
            done += nb;
        }
    }

private:
    enum { DT = 95, DM = 127 };
    static inline float limit(float x) {                      // прозрачно до 0,8; выше — мягкое ограничение
        const float a = std::fabs(x); if (a <= 0.8f) return x;
        const float y = 0.8f + 0.2f * std::tanh((a - 0.8f) * 5.0f); return x < 0 ? -y : y;
    }

    void updateBlock(int no) {
        const float c = 1.0f - std::exp(-(float) no / (0.006f * osr));
        bool seqWas = cur[P_SeqOn] > 0.5f; const int modeWas = (int) cur[P_VoiceMode];
        for (int i = 0; i < P_COUNT; ++i) {
            const float t = tgt[i].load(std::memory_order_relaxed); cur[i] = t;
            if (cont[i]) sm[i] += (t - sm[i]) * c; else sm[i] = t;
        }
        if ((cur[P_SeqOn] > 0.5f) != seqWas || (int) cur[P_VoiceMode] != modeWas) { for (auto& x : v) if (x.active) x.release(); nStack = 0; seqRun = false; seqNote = -1; if (cur[P_SeqOn] < 0.5f) { std::memset(keys, 0, sizeof (keys)); nKeys = 0; } }

        Blk& b = blk; b.osr = osr;
        for (int i = 0; i < 3; ++i) {
            const int q = P_O1Wave + i * 8;
            b.wave[i] = (int) sm[q]; b.off[i] = sm[q + 1] * 12.0f + sm[q + 2] + sm[q + 3] * 0.01f;
            b.pw[i] = sm[q + 4]; b.lvl[i] = sm[q + 5]; b.key[i] = sm[q + 6] > 0.5f; b.fm[i] = sm[q + 7] * sm[q + 7] * 2.0f;
        }
        b.sync[0] = false; b.sync[1] = sm[P_O2Sync] > 0.5f; b.sync[2] = sm[P_O3Sync] > 0.5f; b.low3 = sm[P_O3Low] > 0.5f;
        b.nCol = sm[P_NoiseColor]; b.nLvl = sm[P_NoiseLevel]; b.rLvl = sm[P_RingLevel];
        b.fmode = (int) sm[P_FMode]; b.cutOct = std::log2(std::max(16.0f, sm[P_FCut])); b.res = sm[P_FRes]; b.drive = sm[P_FDrive];
        b.fenv = sm[P_FEnv]; b.fkey = sm[P_FKey]; b.fvel = sm[P_FVel];
        b.alevel = sm[P_ALevel]; b.pan = sm[P_APan]; b.velSens = sm[P_VelSens];
        for (int i = 0; i < 3; ++i) {
            const int q = P_E1A + i * 4;
            b.envA[i] = 1.0f - std::exp(-1.466f / (sm[q] * osr)); b.envD[i] = 1.0f - std::exp(-4.0f / (sm[q + 1] * osr));
            b.envS[i] = sm[q + 2]; b.envR[i] = 1.0f - std::exp(-4.0f / (sm[q + 3] * osr));
        }
        for (int i = 0; i < 2; ++i) {
            const int q = P_L1Wave + i * 4;
            b.lwave[i] = (int) sm[q]; b.linc[i] = sm[q + 1] / osr; b.lfade[i] = sm[q + 2] < 0.002f ? 1.0f : 1.0f / (sm[q + 2] * osr);
            retrig[i] = sm[q + 3] > 0.5f;
            glPh[i] += b.linc[i] * (float) no; glPh[i] -= std::floor(glPh[i]);
        }
        b.shInc = sm[P_ShRate] / osr; b.shSrc = (int) sm[P_ShSrc];
        b.shLag = sm[P_ShLag] < 0.001f ? 1.0f : 1.0f - std::exp(-1.0f / (sm[P_ShLag] * sm[P_ShLag] * 0.5f * osr));
        int nc = 0; for (auto& u : b.du) u = false;
        for (int i = 0; i < NSRC * NDST; ++i) {
            const float d = sm[P_Mx0 + i];
            if (std::fabs(d) > 0.004f) { b.cs[nc] = (uint8_t) (i / NDST); b.cd[nc] = (uint8_t) (i % NDST); b.cg[nc] = d * std::fabs(d); b.du[i % NDST] = true; ++nc; }
        }
        b.nc = nc;
        const float gl = sm[P_Glide];
        b.glideC = gl < 0.002f ? 1.0f : 1.0f - std::exp(-(float) no / (gl * 0.25f * osr));
        bendSm += (bend - bendSm) * c; modSm += (mod - modSm) * c; atSm += (at - atSm) * c;
        b.bend = bendSm * sm[P_BendRange]; b.tune = sm[P_MasterTune] * 0.01f; b.drift = sm[P_Drift];
        b.mod = modSm; b.at = atSm; b.seqVal = seqVal;
        b.vib = modSm * sm[P_ModVib] * (1.0f / 12.0f);        // при полном колесе и ручке на максимуме — ±1 полутон
    }

    float pv(int id) const { return tgt[id].load(std::memory_order_relaxed); }
    int maxVoices() const { const int m = (int) pv(P_VoiceMode); return m == 0 ? std::max(1, std::min((int) MAXV, (int) pv(P_Voices))) : 1; }

    void voiceOn(int note, float vel) {
        const int mode = (int) pv(P_VoiceMode); const bool glide = pv(P_Glide) > 0.002f;
        if (mode != 0) {                                                   // Mono / Legato
            for (int i = 0; i < nStack; ++i) if (stack[i] == note) { for (int k = i; k < nStack - 1; ++k) stack[k] = stack[k + 1]; --nStack; break; }
            if (nStack < 32) stack[nStack++] = note;
            Voice& x = v[0];
            const bool overlap = x.active && x.gate;
            const bool retrigger = mode == 1 || !overlap;
            const float startPitch = (glide && x.active && (mode == 1 || overlap)) ? x.pitch : (float) note;
            x.age = ++counter;
            x.start(note, vel, startPitch, retrigger, blk, glPh);
            if (retrigger) retrigLfos(x);
            lastPitch = (float) note;
            return;
        }
        const int mv = maxVoices(); int pick = -1;
        for (int i = 0; i < mv; ++i) if (v[i].active && v[i].note == note) { pick = i; break; }
        if (pick < 0) for (int i = 0; i < mv; ++i) if (!v[i].active) { pick = i; break; }
        if (pick < 0) { uint64_t best = ~0ull; for (int i = 0; i < mv; ++i) if (!v[i].gate && v[i].age < best) { best = v[i].age; pick = i; } }
        if (pick < 0) { uint64_t best = ~0ull; for (int i = 0; i < mv; ++i) if (v[i].age < best) { best = v[i].age; pick = i; } }
        Voice& x = v[pick];
        x.age = ++counter;
        x.start(note, vel, glide ? lastPitch : (float) note, true, blk, glPh);
        retrigLfos(x);
        lastPitch = (float) note;
        for (int i = mv; i < MAXV; ++i) if (v[i].active && v[i].gate) v[i].release();   // число голосов уменьшили на лету
    }
    void retrigLfos(Voice& x) { for (int i = 0; i < 2; ++i) x.lfo[i].ph = retrig[i] ? 0.0f : glPh[i]; }

    void voiceOff(int note) {
        const int mode = (int) pv(P_VoiceMode);
        if (mode != 0) {
            bool was = false;
            for (int i = 0; i < nStack; ++i) if (stack[i] == note) { for (int k = i; k < nStack - 1; ++k) stack[k] = stack[k + 1]; --nStack; was = true; break; }
            if (!was) return;
            Voice& x = v[0];
            if (nStack > 0) { x.target = (float) stack[nStack - 1]; x.note = stack[nStack - 1]; if (pv(P_Glide) <= 0.002f) x.pitch = x.target; lastPitch = x.target; }
            else if (x.active) { if (sus) x.sustained = true; else x.release(); }
            return;
        }
        for (auto& x : v) if (x.active && x.gate && x.note == note && !x.sustained) { if (sus) x.sustained = true; else x.release(); }
    }

    void runSequencer(int nb) {
        const bool on = cur[P_SeqOn] > 0.5f;
        if (!on || nKeys <= 0) {
            if (seqRun) { if (seqNote >= 0) voiceOff(seqNote); seqNote = -1; seqRun = false; }
            if (!on) uiSeqStep.store(-1, std::memory_order_relaxed);
            return;
        }
        static const float div[6] = { 1, 2, 3, 4, 6, 8 };
        const float bpm = hostBpm > 1.0f ? hostBpm : cur[P_SeqBpm];
        const int len = std::max(1, std::min((int) NSTEP, (int) cur[P_SeqLen]));
        const float gate = cur[P_SeqGate];
        if (!seqRun) { seqRun = true; seqStep = 0; seqPh = 0; trigStep(gate); }
        else {
            seqPh += bpm / 60.0f * div[std::max(0, std::min(5, (int) cur[P_SeqRate]))] * (float) nb / sr;
            if (seqNote >= 0 && gate < 0.99f && seqPh >= gate) { voiceOff(seqNote); seqNote = -1; }
            if (seqPh >= 1.0f) { seqPh -= 1.0f; if (seqPh >= 1.0f) seqPh = 0; seqStep = (seqStep + 1) % len; trigStep(gate); }
        }
        if (seqStep >= len) seqStep = 0;
        uiSeqStep.store(seqStep, std::memory_order_relaxed);
    }
    void trigStep(float gate) {
        const int prev = seqNote; seqNote = -1;
        const bool tie = gate >= 0.99f;
        if (prev >= 0 && !tie) voiceOff(prev);
        if (cur[P_SeqG0 + seqStep] > 0.5f) {
            const int off = (int) cur[P_SeqN0 + seqStep];
            int n = seqBase + off; n = n < 0 ? 0 : (n > 127 ? 127 : n);
            seqVal = (float) off / 24.0f;
            voiceOn(n, keyVel); seqNote = n;
            if (prev >= 0 && tie && prev != n) voiceOff(prev);
        } else if (prev >= 0 && tie) voiceOff(prev);
    }

    std::atomic<float> tgt[P_COUNT];
    float cur[P_COUNT], sm[P_COUNT]; bool cont[P_COUNT];
    float sr = 48000, osr = 96000; int os = 2;
    Voice v[MAXV]; Blk blk {}; Effects fx;
    uint64_t counter = 0; float lastPitch = 60;
    int stack[32]; int nStack = 0;
    bool keys[128]; int nKeys = 0; float keyVel = 0.8f;
    bool sus = false; float bend = 0, mod = 0, at = 0, bendSm = 0, modSm = 0, atSm = 0;
    float glPh[2] = { 0, 0 }; bool retrig[2] = { false, false };
    bool seqRun = false; int seqStep = 0, seqNote = -1, seqBase = 60; float seqPh = 0, seqVal = 0, hostBpm = 0;
    float dh[DT]; float db[2][DM + 1]; int dw = 0;
    float masterG = 0;
};

} // namespace sp
