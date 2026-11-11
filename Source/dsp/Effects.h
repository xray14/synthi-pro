// SYNTHI PRO — секция эффектов. Чистый C++17, без выделения памяти в process().
#pragma once
#include "Params.h"
#include <cmath>
#include <vector>
#include <algorithm>

namespace sp {

struct DelayLine {
    std::vector<float> b; int w = 0, mask = 0;
    void init(int minLen) { int n = 1; while (n < minLen + 4) n <<= 1; b.assign((size_t) n, 0.0f); mask = n - 1; w = 0; }
    void clear() { std::fill(b.begin(), b.end(), 0.0f); }
    inline void push(float x) { b[(size_t) w] = x; w = (w + 1) & mask; }
    inline float tap(int d) const { return b[(size_t) ((w - 1 - d) & mask)]; }
    inline float read(float d) const {          // линейная интерполяция, d — задержка в сэмплах
        const int i = (int) d; const float f = d - (float) i;
        const float a = b[(size_t) ((w - 1 - i) & mask)], c = b[(size_t) ((w - 2 - i) & mask)];
        return a + (c - a) * f;
    }
};

class Effects {
public:
    void prepare(double sampleRate, int oversample) {
        sr = (float) sampleRate; osr = sr * (float) oversample;
        for (int c = 0; c < 2; ++c) { cho[c].init((int) (0.04f * sr)); dly[c].init((int) (2.1f * sr)); }
        static const int combLen[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 }, apLen[4] = { 556, 441, 341, 225 };
        const float k = sr / 44100.0f;
        for (int c = 0; c < 2; ++c) {
            for (int i = 0; i < 8; ++i) { comb[c][i].init((int) ((float) (combLen[i] + c * 23) * k)); combN[c][i] = (int) ((float) (combLen[i] + c * 23) * k); combLp[c][i] = 0; }
            for (int i = 0; i < 4; ++i) { rap[c][i].init((int) ((float) (apLen[i] + c * 23) * k)); rapN[c][i] = (int) ((float) (apLen[i] + c * 23) * k); }
        }
        reset();
    }
    void reset() {
        for (int c = 0; c < 2; ++c) {
            cho[c].clear(); dly[c].clear();
            for (auto& x : comb[c]) x.clear();
            for (auto& x : rap[c]) x.clear();
            for (auto& x : combLp[c]) x = 0;
            for (auto& x : phz[c]) x = 0;
            distLp[c] = distDcX[c] = distDcY[c] = phFb[c] = dlyLp[c] = dlyHp[c] = 0;
        }
        gDist = gSat = gCho = gPha = gDly = gRev = 0; dlyCur = 0.35f * sr; choPh = phaPh = 0;
    }

    // Нелинейные эффекты считаются на повышенной частоте дискретизации (до децимации) — меньше алиасинга.
    void processDrive(float* L, float* R, int n, const float* p) {
        const float sm = 1.0f - std::exp(-(float) n / (0.015f * osr));
        gDist += (p[P_DistOn] - gDist) * sm; gSat += (p[P_SatOn] - gSat) * sm;
        if (gDist > 1e-4f) {
            const float dr = p[P_DistDrive], pre = 1.0f + dr * dr * 60.0f, mix = gDist * p[P_DistMix], post = 0.75f / (1.0f + dr * 0.6f);
            const float lpc = 1.0f - std::exp(-6.2831853f * (500.0f * std::pow(2.0f, p[P_DistTone] * 5.3f)) / osr);
            float* ch[2] = { L, R };
            for (int c = 0; c < 2; ++c) {
                float lp = distLp[c], dx = distDcX[c], dy = distDcY[c];
                for (int i = 0; i < n; ++i) {
                    const float x = ch[c][i], t = x * pre + 0.15f * x * x * pre;       // лёгкая асимметрия -> чётные гармоники
                    float y = t / std::sqrt(1.0f + t * t);
                    const float hp = y - dx + 0.9995f * dy; dx = y; dy = hp;            // убрать постоянную составляющую
                    lp += (hp - lp) * lpc;
                    ch[c][i] = x + (lp * post - x) * mix;
                }
                distLp[c] = lp; distDcX[c] = dx; distDcY[c] = dy;
            }
        }
        if (gSat > 1e-4f) {
            const float d = 1.0f + p[P_SatDrive] * 5.0f, mix = gSat * p[P_SatMix], post = 1.0f / std::pow(d, 0.6f);
            for (int i = 0; i < n; ++i) {
                L[i] += (std::tanh(L[i] * d) * post - L[i]) * mix;
                R[i] += (std::tanh(R[i] * d) * post - R[i]) * mix;
            }
        }
    }

    // Линейные эффекты на базовой частоте: Phaser -> Chorus -> Delay -> Reverb -> Width.
    void process(float* L, float* R, int n, const float* p) {
        const float sm = 1.0f - std::exp(-(float) n / (0.015f * sr));
        gPha += (p[P_PhaOn] - gPha) * sm; gCho += (p[P_ChoOn] - gCho) * sm; gDly += (p[P_DlyOn] - gDly) * sm; gRev += (p[P_RevOn] - gRev) * sm;
        float* ch[2] = { L, R };

        if (gPha > 1e-4f) {
            const float inc = p[P_PhaRate] / sr, depth = p[P_PhaDepth], fb = p[P_PhaFb], m = 0.5f * p[P_PhaMix] * gPha;
            for (int i = 0; i < n; ++i) {
                phaPh += inc; if (phaPh >= 1.0f) phaPh -= 1.0f;
                for (int c = 0; c < 2; ++c) {
                    float ph = phaPh + (c ? 0.25f : 0.0f); if (ph >= 1.0f) ph -= 1.0f;
                    const float tri = 1.0f - std::fabs(2.0f * ph - 1.0f);                        // 0..1
                    const float f = 250.0f * std::pow(2.0f, (0.5f + (tri - 0.5f) * depth) * 4.5f);
                    const float t = std::tan(3.14159265f * std::min(f, sr * 0.45f) / sr), a = (t - 1.0f) / (t + 1.0f);
                    const float x = ch[c][i]; float y = x + phFb[c] * fb;
                    for (int s = 0; s < 6; ++s) { const float o = a * y + phz[c][s]; phz[c][s] = y - a * o; y = o; }
                    phFb[c] = y;
                    ch[c][i] = x * (1.0f - m) + y * m;
                }
            }
        }

        if (gCho > 1e-4f) {
            const float inc = p[P_ChoRate] / sr, depth = p[P_ChoDepth] * 0.006f * sr, base = 0.012f * sr, mix = p[P_ChoMix] * gCho;
            for (int i = 0; i < n; ++i) {
                choPh += inc; if (choPh >= 1.0f) choPh -= 1.0f;
                for (int c = 0; c < 2; ++c) {
                    const float l1 = std::sin(6.2831853f * (choPh + (c ? 0.25f : 0.0f))), l2 = std::sin(6.2831853f * (choPh * 1.37f + (c ? 0.6f : 0.1f)));
                    const float x = ch[c][i];
                    cho[c].push(x);
                    const float wet = 0.6f * cho[c].read(base + depth * (0.5f + 0.5f * l1)) + 0.4f * cho[c].read(base * 1.4f + depth * (0.5f + 0.5f * l2));
                    ch[c][i] = x * (1.0f - 0.3f * mix) + wet * mix;
                }
            }
        }

        if (gDly > 1e-4f) {
            const float target = p[P_DlyTime] * sr, fb = p[P_DlyFb], mix = p[P_DlyMix] * gDly; const bool pp = p[P_DlyPP] > 0.5f;
            const float lpc = 1.0f - std::exp(-6.2831853f * (800.0f * std::pow(2.0f, p[P_DlyTone] * 4.5f)) / sr), hpc = 1.0f - std::exp(-6.2831853f * 90.0f / sr);
            for (int i = 0; i < n; ++i) {
                dlyCur += (target - dlyCur) * 0.0004f;                                   // «ленточное» изменение времени без щелчков
                float wl = dly[0].read(dlyCur), wr = dly[1].read(dlyCur);
                dlyLp[0] += (wl - dlyLp[0]) * lpc; dlyLp[1] += (wr - dlyLp[1]) * lpc;
                dlyHp[0] += (dlyLp[0] - dlyHp[0]) * hpc; dlyHp[1] += (dlyLp[1] - dlyHp[1]) * hpc;
                const float fl = dlyLp[0] - dlyHp[0], fr = dlyLp[1] - dlyHp[1];
                if (pp) { dly[0].push(0.5f * (L[i] + R[i]) * gDly + fr * fb); dly[1].push(fl * fb); }
                else { dly[0].push(L[i] * gDly + fl * fb); dly[1].push(R[i] * gDly + fr * fb); }
                L[i] += wl * mix; R[i] += wr * mix;
            }
        } else if (gDly <= 1e-4f && p[P_DlyOn] < 0.5f) dlyCur = p[P_DlyTime] * sr;

        if (gRev > 1e-4f) {
            const float fbk = 0.70f + p[P_RevSize] * 0.28f, damp = 0.05f + p[P_RevDamp] * 0.55f, mix = p[P_RevMix] * gRev;
            for (int i = 0; i < n; ++i) {
                const float in = (L[i] + R[i]) * 0.015f * gRev;
                for (int c = 0; c < 2; ++c) {
                    float acc = 0;
                    for (int k = 0; k < 8; ++k) {
                        const float y = comb[c][k].tap(combN[c][k] - 1);
                        combLp[c][k] = y * (1.0f - damp) + combLp[c][k] * damp;
                        comb[c][k].push(in + combLp[c][k] * fbk + 1e-20f);
                        acc += y;
                    }
                    for (int k = 0; k < 4; ++k) {
                        const float bo = rap[c][k].tap(rapN[c][k] - 1);
                        rap[c][k].push(acc + bo * 0.5f);
                        acc = bo - acc;
                    }
                    ch[c][i] = ch[c][i] * (1.0f - 0.5f * mix) + acc * 3.0f * mix;
                }
            }
        }

        const float w = p[P_Width];
        if (std::fabs(w - 1.0f) > 1e-3f)
            for (int i = 0; i < n; ++i) { const float m = 0.5f * (L[i] + R[i]), s = 0.5f * (L[i] - R[i]) * w; L[i] = m + s; R[i] = m - s; }
    }

private:
    float sr = 48000, osr = 96000;
    DelayLine cho[2], dly[2], comb[2][8], rap[2][4];
    int combN[2][8] = {}, rapN[2][4] = {};
    float combLp[2][8] = {}, phz[2][6] = {}, phFb[2] = {}, distLp[2] = {}, distDcX[2] = {}, distDcY[2] = {}, dlyLp[2] = {}, dlyHp[2] = {};
    float gDist = 0, gSat = 0, gCho = 0, gPha = 0, gDly = 0, gRev = 0, dlyCur = 0, choPh = 0, phaPh = 0;
};

} // namespace sp
