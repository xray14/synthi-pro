// SYNTHI PRO — единая таблица параметров. Используется звуковым движком (без JUCE),
// плагином (APVTS), интерфейсом (локализованные названия) и генератором пресетов.
#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace sp {

enum class PT { Float, Exp, Log, Int, Choice, Bool };   // Exp — кубическая шкала (времена), Log — логарифмическая (Гц)

struct ParamDef {
    std::string id, en, ru;
    float min, max, def;
    PT type;
    std::string unit, choices;      // choices: варианты через '|'
};

// id, enum, EN, RU, min, max, default, тип, единица, варианты
#define SP_FIXED_PARAMS(X) \
 X(MasterVol, "master_vol", "Master Volume", "Общая громкость", 0, 1, 0.7f, Float, "", "") \
 X(MasterTune, "master_tune", "Master Tune", "Общая подстройка", -100, 100, 0, Float, "ct", "") \
 X(BendRange, "bend_range", "Bend Range", "Диапазон Pitch Bend", 0, 24, 2, Int, "st", "") \
 X(VoiceMode, "voice_mode", "Voice Mode", "Режим голосов", 0, 2, 0, Choice, "", "Poly|Mono|Legato") \
 X(Voices, "voices", "Voices", "Число голосов", 1, 16, 8, Int, "", "") \
 X(Glide, "glide", "Glide", "Глайд", 0, 5, 0, Exp, "s", "") \
 X(Drift, "drift", "Analog Drift", "Аналоговый дрейф", 0, 1, 0.15f, Float, "", "") \
 X(VelSens, "vel_sens", "Velocity > Amp", "Velocity > громкость", 0, 1, 0.5f, Float, "", "") \
 X(ModVib, "mod_vib", "Mod Wheel > Vibrato (LFO 1)", "Колесо мод. > вибрато (LFO 1)", 0, 1, 0.35f, Float, "", "") \
 X(O1Wave, "o1_wave", "VCO 1 Wave", "VCO 1 форма волны", 0, 4, 2, Choice, "", "Sine|Triangle|Saw|Square|Pulse") \
 X(O1Oct, "o1_oct", "VCO 1 Range", "VCO 1 диапазон", -3, 3, 0, Int, "oct", "") \
 X(O1Semi, "o1_semi", "VCO 1 Semitones", "VCO 1 полутоны", -24, 24, 0, Int, "st", "") \
 X(O1Fine, "o1_fine", "VCO 1 Detune", "VCO 1 расстройка", -100, 100, 0, Float, "ct", "") \
 X(O1PW, "o1_pw", "VCO 1 Pulse Width", "VCO 1 скважность", 0.05f, 0.95f, 0.5f, Float, "", "") \
 X(O1Level, "o1_level", "VCO 1 Level", "VCO 1 уровень", 0, 1, 0.8f, Float, "", "") \
 X(O1Key, "o1_key", "VCO 1 Key Track", "VCO 1 слежение за клавиатурой", 0, 1, 1, Bool, "", "") \
 X(O1FM, "o1_fm", "VCO 1 FM (from VCO 2)", "VCO 1 FM (от VCO 2)", 0, 1, 0, Float, "", "") \
 X(O2Wave, "o2_wave", "VCO 2 Wave", "VCO 2 форма волны", 0, 4, 2, Choice, "", "Sine|Triangle|Saw|Square|Pulse") \
 X(O2Oct, "o2_oct", "VCO 2 Range", "VCO 2 диапазон", -3, 3, 0, Int, "oct", "") \
 X(O2Semi, "o2_semi", "VCO 2 Semitones", "VCO 2 полутоны", -24, 24, 0, Int, "st", "") \
 X(O2Fine, "o2_fine", "VCO 2 Detune", "VCO 2 расстройка", -100, 100, 0, Float, "ct", "") \
 X(O2PW, "o2_pw", "VCO 2 Pulse Width", "VCO 2 скважность", 0.05f, 0.95f, 0.5f, Float, "", "") \
 X(O2Level, "o2_level", "VCO 2 Level", "VCO 2 уровень", 0, 1, 0, Float, "", "") \
 X(O2Key, "o2_key", "VCO 2 Key Track", "VCO 2 слежение за клавиатурой", 0, 1, 1, Bool, "", "") \
 X(O2FM, "o2_fm", "VCO 2 FM (from VCO 3)", "VCO 2 FM (от VCO 3)", 0, 1, 0, Float, "", "") \
 X(O3Wave, "o3_wave", "VCO 3 Wave", "VCO 3 форма волны", 0, 4, 1, Choice, "", "Sine|Triangle|Saw|Square|Pulse") \
 X(O3Oct, "o3_oct", "VCO 3 Range", "VCO 3 диапазон", -3, 3, 0, Int, "oct", "") \
 X(O3Semi, "o3_semi", "VCO 3 Semitones", "VCO 3 полутоны", -24, 24, 0, Int, "st", "") \
 X(O3Fine, "o3_fine", "VCO 3 Detune", "VCO 3 расстройка", -100, 100, 0, Float, "ct", "") \
 X(O3PW, "o3_pw", "VCO 3 Pulse Width", "VCO 3 скважность", 0.05f, 0.95f, 0.5f, Float, "", "") \
 X(O3Level, "o3_level", "VCO 3 Level", "VCO 3 уровень", 0, 1, 0, Float, "", "") \
 X(O3Key, "o3_key", "VCO 3 Key Track", "VCO 3 слежение за клавиатурой", 0, 1, 1, Bool, "", "") \
 X(O3FM, "o3_fm", "VCO 3 FM (from VCO 1)", "VCO 3 FM (от VCO 1)", 0, 1, 0, Float, "", "") \
 X(O2Sync, "o2_sync", "VCO 2 Sync to VCO 1", "VCO 2 синхронизация с VCO 1", 0, 1, 0, Bool, "", "") \
 X(O3Sync, "o3_sync", "VCO 3 Sync to VCO 1", "VCO 3 синхронизация с VCO 1", 0, 1, 0, Bool, "", "") \
 X(O3Low, "o3_low", "VCO 3 Low Range", "VCO 3 инфранизкий диапазон", 0, 1, 0, Bool, "", "") \
 X(NoiseColor, "noise_color", "Noise Colour (white > pink)", "Окраска шума (белый > розовый)", 0, 1, 0, Float, "", "") \
 X(NoiseLevel, "noise_level", "Noise Level", "Уровень шума", 0, 1, 0, Float, "", "") \
 X(RingLevel, "ring_level", "Ring Mod Level (VCO 1 x VCO 2)", "Уровень кольцевого модулятора (VCO 1 x VCO 2)", 0, 1, 0, Float, "", "") \
 X(FMode, "f_mode", "Filter Mode", "Режим фильтра", 0, 5, 0, Choice, "", "LP 24|LP 18|LP 12|Band Pass|High Pass|Notch") \
 X(FCut, "f_cut", "Cutoff", "Частота среза", 20, 20000, 2000, Log, "Hz", "") \
 X(FRes, "f_res", "Resonance", "Резонанс", 0, 1, 0.2f, Float, "", "") \
 X(FDrive, "f_drive", "Filter Drive", "Перегруз фильтра", 0, 1, 0, Float, "", "") \
 X(FEnv, "f_env", "Envelope 2 > Cutoff", "Огибающая 2 > срез", -1, 1, 0.4f, Float, "", "") \
 X(FKey, "f_key", "Key Track > Cutoff", "Клавиатура > срез", 0, 1, 0.3f, Float, "", "") \
 X(FVel, "f_vel", "Velocity > Cutoff", "Velocity > срез", 0, 1, 0, Float, "", "") \
 X(ALevel, "a_level", "VCA Level", "Уровень VCA", 0, 1, 0.8f, Float, "", "") \
 X(APan, "a_pan", "Pan", "Панорама", -1, 1, 0, Float, "", "") \
 X(E1A, "e1_a", "Env 1 Attack", "Огиб. 1 атака", 0.001f, 10, 0.005f, Log, "s", "") \
 X(E1D, "e1_d", "Env 1 Decay", "Огиб. 1 спад", 0.001f, 10, 0.3f, Log, "s", "") \
 X(E1S, "e1_s", "Env 1 Sustain", "Огиб. 1 поддержка", 0, 1, 0.8f, Float, "", "") \
 X(E1R, "e1_r", "Env 1 Release", "Огиб. 1 затухание", 0.001f, 10, 0.3f, Log, "s", "") \
 X(E2A, "e2_a", "Env 2 Attack", "Огиб. 2 атака", 0.001f, 10, 0.005f, Log, "s", "") \
 X(E2D, "e2_d", "Env 2 Decay", "Огиб. 2 спад", 0.001f, 10, 0.4f, Log, "s", "") \
 X(E2S, "e2_s", "Env 2 Sustain", "Огиб. 2 поддержка", 0, 1, 0.3f, Float, "", "") \
 X(E2R, "e2_r", "Env 2 Release", "Огиб. 2 затухание", 0.001f, 10, 0.3f, Log, "s", "") \
 X(E3A, "e3_a", "Env 3 Attack", "Огиб. 3 атака", 0.001f, 10, 0.5f, Log, "s", "") \
 X(E3D, "e3_d", "Env 3 Decay", "Огиб. 3 спад", 0.001f, 10, 1.0f, Log, "s", "") \
 X(E3S, "e3_s", "Env 3 Sustain", "Огиб. 3 поддержка", 0, 1, 0.5f, Float, "", "") \
 X(E3R, "e3_r", "Env 3 Release", "Огиб. 3 затухание", 0.001f, 10, 0.5f, Log, "s", "") \
 X(L1Wave, "l1_wave", "LFO 1 Wave", "LFO 1 форма волны", 0, 5, 0, Choice, "", "Sine|Triangle|Saw|Square|S&H|Smooth Random") \
 X(L1Rate, "l1_rate", "LFO 1 Rate", "LFO 1 частота", 0.01f, 50, 5, Log, "Hz", "") \
 X(L1Fade, "l1_fade", "LFO 1 Fade In", "LFO 1 плавное нарастание", 0, 5, 0, Exp, "s", "") \
 X(L1Retrig, "l1_retrig", "LFO 1 Key Retrigger", "LFO 1 перезапуск от клавиши", 0, 1, 0, Bool, "", "") \
 X(L2Wave, "l2_wave", "LFO 2 Wave", "LFO 2 форма волны", 0, 5, 1, Choice, "", "Sine|Triangle|Saw|Square|S&H|Smooth Random") \
 X(L2Rate, "l2_rate", "LFO 2 Rate", "LFO 2 частота", 0.01f, 50, 0.5f, Log, "Hz", "") \
 X(L2Fade, "l2_fade", "LFO 2 Fade In", "LFO 2 плавное нарастание", 0, 5, 0, Exp, "s", "") \
 X(L2Retrig, "l2_retrig", "LFO 2 Key Retrigger", "LFO 2 перезапуск от клавиши", 0, 1, 0, Bool, "", "") \
 X(ShRate, "sh_rate", "S&H Rate", "S&H частота", 0.1f, 50, 6, Log, "Hz", "") \
 X(ShLag, "sh_lag", "S&H Lag", "S&H сглаживание", 0, 1, 0, Float, "", "") \
 X(ShSrc, "sh_src", "S&H Source", "S&H источник", 0, 2, 0, Choice, "", "Noise|LFO 1|VCO 3") \
 X(SeqOn, "seq_on", "Sequencer On", "Секвенсор включён", 0, 1, 0, Bool, "", "") \
 X(SeqRate, "seq_rate", "Sequencer Step", "Шаг секвенсора", 0, 5, 3, Choice, "", "1/4|1/8|1/8T|1/16|1/16T|1/32") \
 X(SeqBpm, "seq_bpm", "Tempo (standalone)", "Темп (автономно)", 40, 240, 120, Float, "BPM", "") \
 X(SeqGate, "seq_gate", "Sequencer Gate", "Длительность шага", 0.05f, 1, 0.5f, Float, "", "") \
 X(SeqLen, "seq_len", "Sequencer Steps", "Число шагов", 1, 16, 8, Int, "", "") \
 X(DistOn, "dist_on", "Distortion On", "Distortion вкл.", 0, 1, 0, Bool, "", "") \
 X(DistDrive, "dist_drive", "Distortion Drive", "Distortion перегруз", 0, 1, 0.5f, Float, "", "") \
 X(DistTone, "dist_tone", "Distortion Tone", "Distortion тембр", 0, 1, 0.6f, Float, "", "") \
 X(DistMix, "dist_mix", "Distortion Mix", "Distortion баланс", 0, 1, 1, Float, "", "") \
 X(SatOn, "sat_on", "Saturation On", "Saturation вкл.", 0, 1, 0, Bool, "", "") \
 X(SatDrive, "sat_drive", "Saturation Drive", "Saturation насыщение", 0, 1, 0.4f, Float, "", "") \
 X(SatMix, "sat_mix", "Saturation Mix", "Saturation баланс", 0, 1, 1, Float, "", "") \
 X(ChoOn, "cho_on", "Chorus On", "Chorus вкл.", 0, 1, 0, Bool, "", "") \
 X(ChoRate, "cho_rate", "Chorus Rate", "Chorus частота", 0.05f, 8, 0.6f, Log, "Hz", "") \
 X(ChoDepth, "cho_depth", "Chorus Depth", "Chorus глубина", 0, 1, 0.5f, Float, "", "") \
 X(ChoMix, "cho_mix", "Chorus Mix", "Chorus баланс", 0, 1, 0.5f, Float, "", "") \
 X(PhaOn, "pha_on", "Phaser On", "Phaser вкл.", 0, 1, 0, Bool, "", "") \
 X(PhaRate, "pha_rate", "Phaser Rate", "Phaser частота", 0.02f, 8, 0.4f, Log, "Hz", "") \
 X(PhaDepth, "pha_depth", "Phaser Depth", "Phaser глубина", 0, 1, 0.7f, Float, "", "") \
 X(PhaFb, "pha_fb", "Phaser Feedback", "Phaser обратная связь", 0, 0.95f, 0.5f, Float, "", "") \
 X(PhaMix, "pha_mix", "Phaser Mix", "Phaser баланс", 0, 1, 0.5f, Float, "", "") \
 X(DlyOn, "dly_on", "Delay On", "Delay вкл.", 0, 1, 0, Bool, "", "") \
 X(DlyTime, "dly_time", "Delay Time", "Delay время", 0.01f, 2, 0.35f, Log, "s", "") \
 X(DlyFb, "dly_fb", "Delay Feedback", "Delay обратная связь", 0, 0.95f, 0.4f, Float, "", "") \
 X(DlyTone, "dly_tone", "Delay Tone", "Delay тембр", 0, 1, 0.6f, Float, "", "") \
 X(DlyMix, "dly_mix", "Delay Mix", "Delay баланс", 0, 1, 0.3f, Float, "", "") \
 X(DlyPP, "dly_pp", "Delay Ping-Pong", "Delay пинг-понг", 0, 1, 1, Bool, "", "") \
 X(RevOn, "rev_on", "Reverb On", "Reverb вкл.", 0, 1, 0, Bool, "", "") \
 X(RevSize, "rev_size", "Reverb Size", "Reverb размер", 0, 1, 0.6f, Float, "", "") \
 X(RevDamp, "rev_damp", "Reverb Damping", "Reverb демпфирование", 0, 1, 0.4f, Float, "", "") \
 X(RevMix, "rev_mix", "Reverb Mix", "Reverb баланс", 0, 1, 0.3f, Float, "", "") \
 X(Width, "width", "Stereo Width", "Ширина стерео", 0, 2, 1, Float, "", "")

enum { NSRC = 16, NDST = 16, NSTEP = 16 };

enum PID {
#define X(e, id, en, ru, mn, mx, df, ty, un, ch) P_##e,
    SP_FIXED_PARAMS(X)
#undef X
    P_SeqN0, P_SeqG0 = P_SeqN0 + NSTEP, P_Mx0 = P_SeqG0 + NSTEP, P_COUNT = P_Mx0 + NSRC * NDST
};

enum Src { S_VCO1, S_VCO2, S_VCO3, S_NOISE, S_RING, S_LFO1, S_LFO2, S_SH, S_ENV1, S_ENV2, S_ENV3, S_VEL, S_KEY, S_MOD, S_AT, S_SEQ };
enum Dst { D_P1, D_P2, D_P3, D_PW1, D_PW2, D_PW3, D_L1, D_L2, D_L3, D_LN, D_LR, D_CUT, D_RES, D_FIN, D_VCA, D_PAN };

inline const char* const* srcIds() { static const char* a[NSRC] = { "vco1", "vco2", "vco3", "noise", "ring", "lfo1", "lfo2", "sh", "env1", "env2", "env3", "vel", "key", "mod", "at", "seq" }; return a; }
inline const char* const* dstIds() { static const char* a[NDST] = { "p1", "p2", "p3", "pw1", "pw2", "pw3", "l1", "l2", "l3", "ln", "lr", "cut", "res", "fin", "vca", "pan" }; return a; }
inline const char* const* srcNames(bool ru) {
    static const char* en[NSRC] = { "VCO 1", "VCO 2", "VCO 3", "Noise", "Ring Mod", "LFO 1", "LFO 2", "S&H", "Env 1", "Env 2", "Env 3", "Velocity", "Key Track", "Mod Wheel", "Aftertouch", "Sequencer" };
    static const char* rr[NSRC] = { "VCO 1", "VCO 2", "VCO 3", "Шум", "Кольц. мод.", "LFO 1", "LFO 2", "S&H", "Огиб. 1", "Огиб. 2", "Огиб. 3", "Velocity", "Клавиатура", "Колесо мод.", "Aftertouch", "Секвенсор" };
    return ru ? rr : en;
}
inline const char* const* dstNames(bool ru) {
    static const char* en[NDST] = { "VCO 1 Pitch", "VCO 2 Pitch", "VCO 3 Pitch", "VCO 1 PW", "VCO 2 PW", "VCO 3 PW", "VCO 1 Level", "VCO 2 Level", "VCO 3 Level", "Noise Level", "Ring Level", "Cutoff", "Resonance", "Filter In", "VCA (AM)", "Pan" };
    static const char* rr[NDST] = { "VCO 1 высота", "VCO 2 высота", "VCO 3 высота", "VCO 1 скважн.", "VCO 2 скважн.", "VCO 3 скважн.", "VCO 1 уровень", "VCO 2 уровень", "VCO 3 уровень", "Шум уровень", "Кольц. уровень", "Срез", "Резонанс", "Вход фильтра", "VCA (AM)", "Панорама" };
    return ru ? rr : en;
}

inline const std::vector<ParamDef>& paramDefs() {
    static const std::vector<ParamDef> defs = [] {
        std::vector<ParamDef> v;
        v.reserve(P_COUNT);
#define X(e, id, en, ru, mn, mx, df, ty, un, ch) v.push_back({ id, en, ru, (float) (mn), (float) (mx), (float) (df), PT::ty, un, ch });
        SP_FIXED_PARAMS(X)
#undef X
        for (int i = 0; i < NSTEP; ++i) { auto n = std::to_string(i + 1); v.push_back({ "seq_n" + n, "Step " + n + " Note", "Шаг " + n + " нота", -24, 24, 0, PT::Int, "st", "" }); }
        for (int i = 0; i < NSTEP; ++i) { auto n = std::to_string(i + 1); v.push_back({ "seq_g" + n, "Step " + n + " Gate", "Шаг " + n + " гейт", 0, 1, 1, PT::Bool, "", "" }); }
        for (int s = 0; s < NSRC; ++s)
            for (int d = 0; d < NDST; ++d)
                v.push_back({ std::string("mx_") + srcIds()[s] + "_" + dstIds()[d],
                              std::string(srcNames(false)[s]) + " > " + dstNames(false)[d],
                              std::string(srcNames(true)[s]) + " > " + dstNames(true)[d], -1, 1, 0, PT::Float, "", "" });
        return v;
    }();
    return defs;
}

inline int paramIndex(const std::string& id) {
    static const std::unordered_map<std::string, int> m = [] {
        std::unordered_map<std::string, int> r; const auto& d = paramDefs();
        for (int i = 0; i < (int) d.size(); ++i) r[d[i].id] = i;
        return r;
    }();
    auto it = m.find(id);
    return it == m.end() ? -1 : it->second;
}

inline bool isContinuous(PT t) { return t == PT::Float || t == PT::Exp || t == PT::Log; }

} // namespace sp
