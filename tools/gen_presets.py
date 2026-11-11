# -*- coding: utf-8 -*-
"""Генератор заводских пресетов SYNTHI PRO -> Source/FactoryPresets.h

Каждый пресет описан вручную как набор реальных параметров движка и маршрутов матрицы
(всё, что не указано, остаётся по умолчанию). Проверка всех пресетов рендером — tests/engine_test.cpp.
Калибровка громкости: tests пишет build-test/preset_levels.txt, генератор читает его и подбирает master_vol.

    python tools/gen_presets.py
"""
import os, sys

SIN, TRI, SAW, SQR, PUL = range(5)
LP24, LP18, LP12, BP, HP, NOTCH = range(6)
LSIN, LTRI, LSAW, LSQR, LSH, LSMOOTH = range(6)
POLY, MONO, LEGATO = range(3)

def O(n, wave, level, oct=0, semi=0, fine=0, pw=0.5, fm=0, key=1):
    return {f'o{n}_wave': wave, f'o{n}_level': level, f'o{n}_oct': oct, f'o{n}_semi': semi, f'o{n}_fine': fine, f'o{n}_pw': pw, f'o{n}_fm': fm, f'o{n}_key': key}
def F(mode, cut, res, env=0.0, key=0.3, drive=0.0, vel=0.0):
    return {'f_mode': mode, 'f_cut': cut, 'f_res': res, 'f_env': env, 'f_key': key, 'f_drive': drive, 'f_vel': vel}
def E(n, a, d, s, r): return {f'e{n}_a': a, f'e{n}_d': d, f'e{n}_s': s, f'e{n}_r': r}
def L(n, wave, rate, fade=0.0, retrig=0): return {f'l{n}_wave': wave, f'l{n}_rate': rate, f'l{n}_fade': fade, f'l{n}_retrig': retrig}
def M(**r): return {'mx_' + k: v for k, v in r.items()}
def SH(rate, lag=0.0, src=0): return {'sh_rate': rate, 'sh_lag': lag, 'sh_src': src}
def REV(mix, size=0.6, damp=0.4): return {'rev_on': 1, 'rev_mix': mix, 'rev_size': size, 'rev_damp': damp}
def DLY(mix, time=0.35, fb=0.4, tone=0.6, pp=1): return {'dly_on': 1, 'dly_mix': mix, 'dly_time': time, 'dly_fb': fb, 'dly_tone': tone, 'dly_pp': pp}
def CHO(mix=0.5, rate=0.6, depth=0.5): return {'cho_on': 1, 'cho_mix': mix, 'cho_rate': rate, 'cho_depth': depth}
def PHA(mix=0.5, rate=0.4, depth=0.7, fb=0.5): return {'pha_on': 1, 'pha_mix': mix, 'pha_rate': rate, 'pha_depth': depth, 'pha_fb': fb}
def DIST(drive=0.5, tone=0.6, mix=1.0): return {'dist_on': 1, 'dist_drive': drive, 'dist_tone': tone, 'dist_mix': mix}
def SAT(drive=0.4, mix=1.0): return {'sat_on': 1, 'sat_drive': drive, 'sat_mix': mix}
def SEQ(notes, gates=None, rate=3, gate=0.5, bpm=120):
    d = {'seq_on': 1, 'seq_rate': rate, 'seq_gate': gate, 'seq_bpm': bpm, 'seq_len': len(notes)}
    for i, n in enumerate(notes): d[f'seq_n{i + 1}'] = n
    for i, g in enumerate(gates or [1] * len(notes)): d[f'seq_g{i + 1}'] = g
    return d
def G(**k): return k

PRESETS = []
def P(name, cat, *parts):
    d = {}
    for p in parts: d.update(p)
    PRESETS.append((name, cat, d))

# =====================================================================================================
C = 'Classic EMS Sounds'
P('Putney Drone', C, O(1, SIN, 0.7, oct=-2), O(2, SAW, 0.5, oct=-2, fine=9), G(ring_level=0.5), F(LP18, 700, 0.6, env=0.1), E(1, 0.8, 1.0, 1.0, 2.0), L(2, LTRI, 0.08), M(lfo2_cut=0.45, lfo2_pw2=0.3), REV(0.4, 0.8), G(drift=0.4))
P('Silver Sweep', C, O(1, SAW, 0.0), G(noise_level=0.5, noise_color=0.4), F(LP18, 500, 0.97, env=0.0, key=0.6), E(1, 0.4, 1.0, 1.0, 1.5), E(3, 2.5, 2.5, 0.2, 2.0), M(env3_cut=0.75, lfo1_cut=0.12), L(1, LSIN, 5.5), REV(0.45, 0.7))
P('Tin Robot Voice', C, O(1, SIN, 0.0, oct=-3, semi=7, key=0), O(2, SAW, 0.0), G(ring_level=1.0), F(BP, 1400, 0.45, env=0.0, key=0.5), E(1, 0.01, 0.2, 1.0, 0.15), M(lfo1_p1=0.12), L(1, LSIN, 0.7), G(voice_mode=LEGATO, glide=0.04), SAT(0.5))
P('Trapezoid Blip', C, O(1, SIN, 0.9), F(LP12, 6000, 0.1, env=0.0), E(1, 0.001, 0.12, 0.0, 0.1), E(2, 0.001, 0.07, 0.0, 0.07), M(env2_p1=0.55), DLY(0.3, 0.19, 0.45), REV(0.2))
P('Meadow Birds', C, O(1, SIN, 0.8, oct=2), F(LP12, 12000, 0.2, env=0.0, key=0.0), E(1, 0.02, 0.2, 0.5, 0.3), SH(9, 0.25), L(1, LSAW, 11, retrig=0), M(sh_p1=0.42, lfo1_p1=0.3, lfo2_vca=0.8), L(2, LSQR, 1.3), DLY(0.3, 0.27, 0.5), REV(0.35, 0.7))
P('Spring Tank Hit', C, O(1, SAW, 0.0), G(noise_level=0.9), F(BP, 1800, 0.7, env=0.7, key=0.0), E(1, 0.001, 0.09, 0.0, 0.08), E(2, 0.001, 0.06, 0.0, 0.06), REV(0.75, 0.85, 0.25))
P('Third Oscillator Sweep', C, O(1, SAW, 0.8, oct=-1), O(3, TRI, 0.0, oct=-1, key=0), G(o3_low=1), M(vco3_p1=0.62, vco3_cut=0.5), F(LP24, 900, 0.5, env=0.0), E(1, 0.02, 0.5, 1.0, 0.6), REV(0.3))
P('Whistling Filter', C, O(1, SAW, 0.0), F(LP24, 262, 1.0, env=0.0, key=1.0), E(1, 0.05, 0.3, 1.0, 0.4), G(voice_mode=LEGATO, glide=0.12), L(1, LSIN, 5.2, fade=0.6), M(lfo1_cut=0.07), REV(0.4, 0.7), DLY(0.2, 0.4, 0.35))
P('Radiophonic Bubbles', C, O(1, SIN, 0.3, oct=1), F(LP24, 350, 0.98, env=0.0, key=0.2), E(1, 0.01, 0.3, 1.0, 0.5), L(1, LSAW, 7, retrig=1), SH(3.5), M(lfo1_cut=0.55, sh_cut=0.5, sh_p1=0.3), DLY(0.3, 0.33, 0.5), REV(0.3))
P('Wobbulator', C, O(1, SQR, 0.6, oct=-1), O(2, SAW, 0.6, oct=-1, fine=5), O(3, TRI, 0.0, oct=1, key=0), G(o3_low=1), M(vco3_p1=0.35, vco3_p2=-0.35, vco3_pan=0.6), F(LP18, 1600, 0.4, env=0.2), E(1, 0.01, 0.4, 0.9, 0.4), PHA(0.4, 0.2))
P('Matrix Chaos', C, O(1, SAW, 0.6, oct=-1), O(2, SQR, 0.5, semi=7), G(ring_level=0.4), M(vco1_p2=0.5, vco2_p1=0.42, vco2_cut=0.4, lfo2_p1=0.2), L(2, LSMOOTH, 0.4), F(LP18, 1500, 0.55, env=0.2), E(1, 0.01, 0.5, 0.9, 0.6), REV(0.35), G(drift=0.6))
P('Running Sequence', C, O(1, SAW, 0.8, oct=-1), O(2, SQR, 0.4, oct=-1, fine=6), F(LP24, 500, 0.6, env=0.35, key=0.2), E(1, 0.001, 0.12, 0.3, 0.08), E(2, 0.001, 0.1, 0.0, 0.08), L(2, LTRI, 0.07), M(lfo2_cut=0.6), SEQ([0, 3, 5, 3, 7, 5, 10, 12], rate=3, gate=0.45, bpm=165), G(voice_mode=MONO), DLY(0.25, 0.27, 0.4), REV(0.15))

# =====================================================================================================
C = 'Analog Bass'
BASS = [G(voice_mode=MONO, drift=0.1), E(1, 0.002, 0.3, 0.7, 0.12), E(2, 0.002, 0.25, 0.1, 0.15)]
P('Round Sub', C, *BASS, O(1, SIN, 0.9, oct=-2), O(2, TRI, 0.5, oct=-1), F(LP24, 240, 0.1, env=0.25, key=0.4), SAT(0.3))
P('Ladder Punch', C, *BASS, O(1, SAW, 0.9, oct=-2), O(2, SQR, 0.5, oct=-1), F(LP24, 180, 0.35, env=0.6, key=0.4, drive=0.3), E(2, 0.001, 0.18, 0.0, 0.12))
P('Rubber Band', C, *BASS, O(1, SQR, 0.8, oct=-2), F(LP24, 140, 0.55, env=0.7, key=0.5), E(2, 0.03, 0.3, 0.0, 0.2), G(glide=0.08, voice_mode=LEGATO))
P('Acid Squelch', C, *BASS, O(1, SAW, 0.9, oct=-2), F(LP18, 220, 0.82, env=0.65, key=0.5, drive=0.25, vel=0.6), E(1, 0.001, 0.25, 0.4, 0.08), E(2, 0.001, 0.16, 0.0, 0.1), G(glide=0.05, voice_mode=LEGATO), DIST(0.25, 0.7, 0.4))
P('Pulse Width Bass', C, *BASS, O(1, PUL, 0.9, oct=-2, pw=0.3), O(2, PUL, 0.6, oct=-2, pw=0.6, fine=8), L(1, LTRI, 0.6), M(lfo1_pw1=0.45, lfo1_pw2=-0.4), F(LP24, 500, 0.25, env=0.4))
P('Detuned Growl', C, *BASS, O(1, SAW, 0.8, oct=-2, fine=-12), O(2, SAW, 0.8, oct=-2, fine=12), O(3, SQR, 0.5, oct=-3), F(LP24, 320, 0.3, env=0.45, drive=0.5), SAT(0.5))
P('Sync Bass', C, *BASS, O(1, SAW, 0.0, oct=-2), O(2, SAW, 0.9, oct=-2, semi=7), G(o2_sync=1), M(env2_p2=0.6), E(2, 0.001, 0.3, 0.0, 0.2), F(LP24, 1400, 0.2, env=0.3))
P('FM Thump', C, *BASS, O(1, SIN, 0.9, oct=-2, fm=0.45), O(2, SIN, 0.0, oct=-1), F(LP12, 2500, 0.1, env=0.3), E(1, 0.001, 0.35, 0.3, 0.15), M(env2_p1=0.3), E(2, 0.001, 0.05, 0.0, 0.05))
P('Dark Reese', C, *BASS, O(1, SAW, 0.8, oct=-2, fine=-18), O(2, SAW, 0.8, oct=-2, fine=18), F(LP24, 380, 0.2, env=0.1), E(1, 0.01, 0.4, 1.0, 0.25), L(2, LTRI, 0.15), M(lfo2_cut=0.3), PHA(0.35, 0.12, 0.6, 0.3), G(voice_mode=LEGATO, glide=0.06))
P('Ring Bass', C, *BASS, O(1, SQR, 0.6, oct=-2), O(2, SIN, 0.0, oct=-1, semi=7), G(ring_level=0.7), F(LP24, 450, 0.3, env=0.5), E(2, 0.001, 0.22, 0.05, 0.15))
P('Velocity Pluck Bass', C, *BASS, O(1, SAW, 0.8, oct=-2), O(2, TRI, 0.6, oct=-2), F(LP24, 160, 0.4, env=0.5, vel=1.0), E(1, 0.001, 0.28, 0.0, 0.2), E(2, 0.001, 0.14, 0.0, 0.14), G(vel_sens=0.8))
P('Noise Kick Bass', C, *BASS, O(1, SIN, 1.0, oct=-2), G(noise_level=0.25, noise_color=0.8), F(LP24, 200, 0.2, env=0.6), E(2, 0.001, 0.08, 0.0, 0.08), M(env2_p1=0.42, env2_ln=0.5), E(1, 0.001, 0.4, 0.5, 0.2))

# =====================================================================================================
C = 'Leads'
LEAD = [G(voice_mode=LEGATO, glide=0.05), E(1, 0.005, 0.3, 0.85, 0.2), L(1, LSIN, 5.6, fade=0.5), M(lfo1_p1=0.07, lfo1_p2=0.07, mod_cut=0.4)]
P('Solo Saw', C, *LEAD, O(1, SAW, 0.8), O(2, SAW, 0.6, fine=8), F(LP24, 2200, 0.3, env=0.3), DLY(0.2, 0.32, 0.35), REV(0.15))
P('Square Singer', C, *LEAD, O(1, SQR, 0.8), O(2, PUL, 0.4, oct=1, pw=0.3), F(LP18, 1800, 0.45, env=0.25), DLY(0.22, 0.38, 0.4))
P('Sync Screamer', C, *LEAD, O(1, SAW, 0.0), O(2, SAW, 0.9, semi=12), G(o2_sync=1), E(3, 0.3, 1.2, 0.3, 0.4), M(env3_p2=0.55, lfo1_p1=0.07, mod_p2=0.5), F(LP12, 6000, 0.2, env=0.1), DIST(0.3, 0.7, 0.5), DLY(0.2, 0.3, 0.35))
P('Fifth Stack', C, *LEAD, O(1, SAW, 0.7), O(2, SAW, 0.6, semi=7), O(3, SQR, 0.4, oct=-1), F(LP24, 1700, 0.35, env=0.4), CHO(0.3), DLY(0.18))
P('Glass Flute', C, *LEAD, O(1, TRI, 0.9), O(2, SIN, 0.3, oct=1), G(noise_level=0.06), F(LP12, 3000, 0.15, env=0.15), E(1, 0.06, 0.3, 0.85, 0.25), REV(0.3, 0.7))
P('Resonant Whiner', C, *LEAD, O(1, PUL, 0.8, pw=0.25), F(LP24, 900, 0.8, env=0.45, key=0.8), E(2, 0.05, 0.6, 0.4, 0.3), DLY(0.25, 0.42, 0.45), G(glide=0.1))
P('FM Bell Lead', C, *LEAD, O(1, SIN, 0.9, fm=0.55), O(2, SIN, 0.0, semi=19), F(LP12, 9000, 0.1, env=0.0), E(1, 0.002, 0.8, 0.4, 0.5), REV(0.3), DLY(0.2, 0.25, 0.3))
P('Ring Lead', C, *LEAD, O(1, SAW, 0.5), O(2, SIN, 0.0, semi=7), G(ring_level=0.7), F(LP24, 2600, 0.3, env=0.3), DLY(0.25))
P('Octave Jumper', C, *LEAD, O(1, SQR, 0.8), O(2, SQR, 0.5, oct=1), L(2, LSQR, 6, retrig=1), M(lfo2_p2=0.5, lfo1_p1=0.07), F(LP24, 2400, 0.3, env=0.3), DLY(0.2, 0.2, 0.4))
P('Nasal Reed', C, *LEAD, O(1, PUL, 0.9, pw=0.12), F(BP, 1300, 0.5, env=0.2, key=0.7), E(1, 0.02, 0.2, 0.9, 0.15), REV(0.2))
P('Thick Portamento', C, *LEAD, O(1, SAW, 0.8, fine=-7), O(2, SAW, 0.8, fine=7), O(3, SAW, 0.5, oct=-1), F(LP24, 1500, 0.4, env=0.5, drive=0.3), E(2, 0.01, 0.5, 0.3, 0.3), G(glide=0.2), CHO(0.3), DLY(0.2, 0.36, 0.35))
P('Aftertouch Screamer', C, *LEAD, O(1, SAW, 0.9), O(2, SQR, 0.5, fine=5), F(LP24, 1200, 0.5, env=0.3), M(at_cut=0.6, at_p1=0.05, lfo1_p1=0.07, mod_cut=0.5), SAT(0.4), DLY(0.2))

# =====================================================================================================
C = 'Pads'
PAD = [G(voice_mode=POLY, voices=8, drift=0.3), E(1, 0.9, 1.5, 0.85, 1.8), E(2, 1.2, 2.0, 0.6, 1.8)]
P('Warm Analog Pad', C, *PAD, O(1, SAW, 0.6, fine=-8), O(2, SAW, 0.6, fine=8), F(LP24, 900, 0.2, env=0.3), CHO(0.5), REV(0.35, 0.75))
P('PWM Strings', C, *PAD, O(1, PUL, 0.6, pw=0.4), O(2, PUL, 0.6, pw=0.6, fine=6), L(1, LTRI, 0.35), L(2, LTRI, 0.27), M(lfo1_pw1=0.5, lfo2_pw2=0.5), F(LP24, 1800, 0.15, env=0.2), E(1, 0.5, 1.0, 0.9, 1.2), CHO(0.5, 0.5), REV(0.3))
P('Slow Sweep Pad', C, *PAD, O(1, SAW, 0.6), O(2, SQR, 0.4, oct=-1, fine=5), F(LP24, 400, 0.45, env=0.5), E(2, 3.0, 3.0, 0.5, 2.5), PHA(0.4, 0.1), REV(0.4, 0.8))
P('Glass Pad', C, *PAD, O(1, TRI, 0.7, oct=1), O(2, SIN, 0.5, oct=1, semi=7), O(3, SIN, 0.4), F(LP12, 4000, 0.2, env=0.1), CHO(0.4), REV(0.5, 0.85), DLY(0.2, 0.5, 0.4))
P('Hollow Choir', C, *PAD, O(1, SQR, 0.6), O(2, SQR, 0.5, fine=9), F(BP, 900, 0.5, env=0.15, key=0.5), L(2, LSIN, 0.2), M(lfo2_cut=0.3), CHO(0.6, 0.4), REV(0.45, 0.8))
P('Fifth Cathedral', C, *PAD, O(1, SAW, 0.5), O(2, SAW, 0.5, semi=7), O(3, TRI, 0.5, oct=-1), F(LP24, 1200, 0.25, env=0.25), REV(0.55, 0.92, 0.3), CHO(0.3))
P('Shimmer Notch', C, *PAD, O(1, SAW, 0.6, fine=-5), O(2, SAW, 0.6, fine=5, oct=1), F(NOTCH, 1500, 0.4, env=0.0), L(2, LTRI, 0.11), M(lfo2_cut=0.6), CHO(0.5), REV(0.4, 0.8))
P('Dark Velvet', C, *PAD, O(1, TRI, 0.8, oct=-1), O(2, SAW, 0.4, oct=-1, fine=7), F(LP24, 500, 0.3, env=0.2), E(1, 1.5, 2.0, 0.9, 2.5), REV(0.45, 0.85, 0.6))
P('Breathing Pad', C, *PAD, O(1, SAW, 0.6), O(2, PUL, 0.5, pw=0.35, fine=6), G(noise_level=0.08, noise_color=1.0), F(LP24, 1100, 0.3, env=0.2), L(1, LSIN, 0.18), M(lfo1_vca=0.5, lfo1_cut=0.3), CHO(0.4), REV(0.4))
P('High Strings', C, *PAD, O(1, SAW, 0.6, oct=1, fine=-6), O(2, SAW, 0.6, oct=1, fine=6), O(3, SAW, 0.4), F(LP12, 3500, 0.1, env=0.15), E(1, 0.35, 1.0, 0.9, 0.9), L(1, LSIN, 5.5, fade=0.8), M(lfo1_p1=0.05, lfo1_p2=0.05), CHO(0.6, 0.7), REV(0.3))
P('Random Pan Pad', C, *PAD, O(1, SAW, 0.6, fine=-4), O(2, TRI, 0.6, fine=4), F(LP24, 1300, 0.25, env=0.25), L(2, LSMOOTH, 0.5), M(lfo2_pan=0.8, lfo2_cut=0.25), REV(0.4, 0.8), DLY(0.2, 0.45, 0.4))
P('Ring Halo', C, *PAD, O(1, SIN, 0.6), O(2, SIN, 0.3, semi=7, fine=4), G(ring_level=0.5), O(3, TRI, 0.4, oct=-1), F(LP12, 2500, 0.2, env=0.2), CHO(0.4), REV(0.55, 0.9))

# =====================================================================================================
C = 'Drones'
DR = [G(voice_mode=POLY, drift=0.6), E(1, 2.0, 2.0, 1.0, 4.0)]
P('Deep Earth', C, *DR, O(1, SAW, 0.7, oct=-3), O(2, SAW, 0.7, oct=-3, fine=11), O(3, SIN, 0.6, oct=-2), F(LP24, 220, 0.4, env=0.0), L(2, LSMOOTH, 0.07), M(lfo2_cut=0.5), REV(0.5, 0.9, 0.6))
P('Tanpura Buzz', C, *DR, O(1, SAW, 0.7, oct=-2), O(2, SAW, 0.6, oct=-2, semi=7), O(3, PUL, 0.5, oct=-1, pw=0.15), F(BP, 900, 0.55, env=0.0, key=0.2), L(1, LTRI, 0.13), M(lfo1_cut=0.5, lfo1_pw3=0.4), REV(0.4, 0.8))
P('Slow Beating', C, *DR, O(1, SIN, 0.8, oct=-1), O(2, SIN, 0.8, oct=-1, fine=4), O(3, SIN, 0.5, oct=-1, fine=-3), F(LP12, 5000, 0.0, env=0.0), REV(0.4, 0.85))
P('Machine Room', C, *DR, O(1, SQR, 0.5, oct=-3), O(2, SAW, 0.5, oct=-2, fine=15), G(noise_level=0.2, noise_color=1.0, ring_level=0.4), F(LP24, 600, 0.5, env=0.0), L(2, LSH, 0.8), M(lfo2_cut=0.35), SAT(0.5), REV(0.35))
P('Aurora', C, *DR, O(1, TRI, 0.6, oct=1), O(2, SIN, 0.6, oct=1, semi=7, fine=5), O(3, SIN, 0.5, oct=2), F(LP12, 3000, 0.3, env=0.0), L(1, LSMOOTH, 0.3), L(2, LSIN, 0.09), M(lfo1_l3=0.5, lfo2_cut=0.4, lfo1_pan=0.6), CHO(0.5, 0.3), REV(0.6, 0.92))
P('Feedback Monk', C, *DR, O(1, SAW, 0.6, oct=-2), O(2, SQR, 0.4, oct=-2, fine=7), F(BP, 500, 0.75, env=0.0, key=0.0), L(2, LTRI, 0.05), M(lfo2_cut=0.55, vco1_cut=0.15), REV(0.5, 0.9))
P('Harmonic Series', C, *DR, O(1, SIN, 0.7, oct=-1), O(2, SIN, 0.5, semi=7), O(3, SIN, 0.4, oct=1, semi=4), F(LP12, 8000, 0.0, env=0.0), L(1, LSIN, 0.11), L(2, LSIN, 0.17), M(lfo1_l2=0.5, lfo2_l3=0.5), REV(0.45, 0.85))
P('Rust Belt', C, *DR, O(1, SAW, 0.7, oct=-2), O(2, SAW, 0.6, oct=-2, semi=1), G(ring_level=0.5), F(LP18, 700, 0.5, env=0.0, drive=0.6), DIST(0.35, 0.4, 0.6), REV(0.4, 0.8))
P('Tidal Filter', C, *DR, O(1, SAW, 0.6, oct=-2), O(2, PUL, 0.6, oct=-1, pw=0.3, fine=8), F(LP24, 300, 0.7, env=0.0), L(2, LSIN, 0.04), M(lfo2_cut=0.75, lfo2_pw2=0.4), PHA(0.4, 0.07), REV(0.45, 0.85))
P('Sync Organ Drone', C, *DR, O(1, SAW, 0.4, oct=-2), O(2, SAW, 0.7, oct=-1, semi=5), G(o2_sync=1), L(2, LTRI, 0.06), M(lfo2_p2=0.45), F(LP12, 2500, 0.2, env=0.0), CHO(0.4), REV(0.45, 0.85))
P('Cold Wind Drone', C, *DR, O(1, SAW, 0.0), G(noise_level=0.8, noise_color=0.6), F(BP, 700, 0.85, env=0.0, key=0.7), L(1, LSMOOTH, 0.35), L(2, LSIN, 0.07), M(lfo1_cut=0.4, lfo2_cut=0.4, lfo1_vca=0.4), REV(0.5, 0.9))
P('Fourth World', C, *DR, O(1, SAW, 0.6, oct=-1), O(2, SAW, 0.6, oct=-1, semi=5, fine=6), O(3, TRI, 0.6, oct=-2), F(LP24, 800, 0.35, env=0.0), L(2, LSMOOTH, 0.15), M(lfo2_cut=0.4, lfo2_pan=0.5), DLY(0.3, 0.6, 0.55), REV(0.45, 0.88))

# =====================================================================================================
C = 'Experimental'
P('Cross Mod Animal', C, O(1, SAW, 0.6), O(2, SQR, 0.5, semi=5), M(vco1_p2=0.6, vco2_p1=0.55, sh_p1=0.3, lfo2_cut=0.4), SH(4, 0.3), L(2, LSMOOTH, 0.8), F(LP18, 1800, 0.5, env=0.2), E(1, 0.01, 0.5, 0.9, 0.5), REV(0.3))
P('Filter FM Vowels', C, O(1, SAW, 0.8, oct=-1), O(2, SIN, 0.0, oct=1), M(vco2_cut=0.55, lfo2_cut=0.4), L(2, LTRI, 0.25), F(BP, 900, 0.7, env=0.2, key=0.4), E(1, 0.02, 0.4, 0.9, 0.4), REV(0.25))
P('Audio Rate AM', C, O(1, SAW, 0.8), O(2, SIN, 0.0, semi=6), M(vco2_vca=0.9, lfo2_p2=0.3), L(2, LSIN, 0.3), F(LP24, 3000, 0.2, env=0.2), E(1, 0.01, 0.4, 0.9, 0.4), DLY(0.25))
P('Random Staircase', C, O(1, SQR, 0.8), SH(7, 0.0), M(sh_p1=0.5, sh_cut=0.5, sh_pan=0.7), F(LP24, 1500, 0.55, env=0.0), E(1, 0.01, 0.3, 1.0, 0.3), DLY(0.3, 0.21, 0.5), REV(0.25))
P('Self Patch Feedback', C, O(1, SAW, 0.7, oct=-1), O(2, SAW, 0.5), M(vco1_pw1=0.6, vco2_fin=0.6, ring_cut=0.4, env2_p2=0.4), G(ring_level=0.3), F(LP24, 1200, 0.75, env=0.3), E(1, 0.01, 0.6, 0.8, 0.5), E(2, 0.4, 1.5, 0.2, 0.5), REV(0.3))
P('Noise Pitch Dirt', C, O(1, SAW, 0.8), M(noise_p1=0.3, noise_cut=0.3, lfo1_p1=0.15), L(1, LSMOOTH, 3), F(LP18, 1400, 0.5, env=0.3), E(1, 0.01, 0.4, 0.9, 0.3), SAT(0.5))
P('Granular Stutter', C, O(1, SAW, 0.8), O(2, SQR, 0.5, oct=1), L(1, LSQR, 13, retrig=1), L(2, LSH, 5), M(lfo1_vca=1.0, lfo2_p2=0.4, lfo2_cut=0.5), F(LP24, 2500, 0.4, env=0.0), E(1, 0.005, 0.3, 1.0, 0.2), DLY(0.3, 0.14, 0.55), REV(0.2))
P('Sync Sweep Formant', C, O(1, SAW, 0.0, oct=-1), O(2, PUL, 0.9, oct=0, pw=0.3), G(o2_sync=1), L(2, LSIN, 0.3), M(lfo2_p2=0.55, lfo2_pw2=0.3, key_p2=0.3), F(LP12, 5000, 0.2, env=0.0), E(1, 0.01, 0.3, 1.0, 0.3), PHA(0.3))
P('Broken Radio', C, O(1, SIN, 0.5, oct=1, fm=0.8), O(2, SAW, 0.0, semi=11), G(noise_level=0.3, ring_level=0.4), SH(11, 0.1), M(sh_p2=0.5, sh_cut=0.4, noise_vca=0.5), F(BP, 1800, 0.6, env=0.0), E(1, 0.01, 0.3, 1.0, 0.3), DIST(0.3, 0.5, 0.5))
P('Envelope Bender', C, O(1, SAW, 0.7), O(2, SAW, 0.6, fine=6), E(3, 1.5, 2.0, 0.0, 1.0), M(env3_p1=0.4, env3_p2=-0.4, env3_cut=0.5, env3_pan=0.6), F(LP24, 800, 0.5, env=0.0), E(1, 0.01, 0.4, 1.0, 1.0), REV(0.35))
P('Three Way FM', C, O(1, SIN, 0.8, fm=0.5), O(2, SIN, 0.0, semi=7, fm=0.5), O(3, SIN, 0.0, semi=14, fm=0.4), F(LP12, 9000, 0.1, env=0.0), E(1, 0.01, 0.8, 0.7, 0.6), L(2, LSIN, 0.2), M(lfo2_p3=0.2), REV(0.3), CHO(0.3))
P('Velocity Morph', C, O(1, SAW, 0.6), O(2, SQR, 0.0, oct=1), M(vel_l2=0.9, vel_cut=0.6, vel_res=0.5, key_pan=0.7), F(LP24, 700, 0.3, env=0.3, vel=0.5), E(1, 0.005, 0.5, 0.7, 0.4), G(vel_sens=0.7), DLY(0.2))

# =====================================================================================================
C = 'Ambient'
AMB = [G(voice_mode=POLY, drift=0.4), E(1, 1.2, 2.0, 0.8, 3.0)]
P('Floating Islands', C, *AMB, O(1, TRI, 0.7), O(2, SIN, 0.6, oct=1, fine=5), F(LP12, 2200, 0.2, env=0.1), L(2, LSMOOTH, 0.2), M(lfo2_pan=0.7, lfo2_cut=0.3), CHO(0.4), DLY(0.35, 0.55, 0.55), REV(0.55, 0.92))
P('Morning Mist', C, *AMB, O(1, SIN, 0.8), O(2, TRI, 0.4, oct=1, semi=7), G(noise_level=0.1, noise_color=1.0), F(LP12, 3000, 0.1, env=0.1), REV(0.6, 0.9, 0.5), DLY(0.25, 0.7, 0.5))
P('Slow Bells', C, *AMB, O(1, SIN, 0.8, oct=1, fm=0.4), O(2, SIN, 0.0, oct=1, semi=19), F(LP12, 8000, 0.0, env=0.0), E(1, 0.005, 3.0, 0.0, 3.0), DLY(0.35, 0.6, 0.6), REV(0.55, 0.9))
P('Underwater', C, *AMB, O(1, SAW, 0.6, oct=-1), O(2, TRI, 0.6, fine=7), F(LP24, 450, 0.55, env=0.0), L(1, LSMOOTH, 0.6), L(2, LSIN, 0.1), M(lfo1_cut=0.4, lfo2_cut=0.4, lfo1_p1=0.05), PHA(0.5, 0.15), REV(0.5, 0.88, 0.6))
P('Sky Organ', C, *AMB, O(1, SIN, 0.7), O(2, SIN, 0.5, oct=1), O(3, SIN, 0.4, oct=1, semi=7), F(LP12, 6000, 0.0, env=0.0), E(1, 0.4, 1.0, 1.0, 2.0), CHO(0.5, 0.25), REV(0.6, 0.93))
P('Dusk Swell', C, *AMB, O(1, SAW, 0.6, fine=-6), O(2, SAW, 0.6, fine=6), F(LP24, 500, 0.3, env=0.5), E(2, 3.0, 3.0, 0.6, 3.0), E(1, 2.0, 2.0, 0.9, 3.5), CHO(0.4), REV(0.55, 0.9), DLY(0.2, 0.5, 0.5))
P('Frozen Lake', C, *AMB, O(1, TRI, 0.7, oct=1), O(2, PUL, 0.3, oct=2, pw=0.2), F(BP, 2500, 0.5, env=0.0, key=0.5), L(2, LSMOOTH, 0.25), M(lfo2_cut=0.4, lfo2_pan=0.6), DLY(0.4, 0.44, 0.6), REV(0.55, 0.92, 0.2))
P('Sleeping Giants', C, *AMB, O(1, SAW, 0.6, oct=-2), O(2, TRI, 0.7, oct=-1), O(3, SIN, 0.5), F(LP24, 380, 0.3, env=0.2), E(2, 3.5, 3.0, 0.5, 3.0), REV(0.6, 0.95, 0.5))
P('Wind Chimes', C, *AMB, O(1, SIN, 0.8, oct=2), F(LP12, 9000, 0.1, env=0.0), E(1, 0.002, 0.6, 0.0, 0.6), SH(5, 0.0), L(1, LSQR, 3.3), M(sh_p1=0.45, lfo1_vca=0.9, sh_pan=0.8), E(3, 0.01, 0.5, 1.0, 0.5), DLY(0.4, 0.37, 0.6), REV(0.5, 0.9))
P('Tape Memory', C, *AMB, O(1, SAW, 0.5), O(2, TRI, 0.7, fine=10), F(LP18, 1200, 0.2, env=0.15), L(1, LSMOOTH, 1.2), M(lfo1_p1=0.06, lfo1_p2=0.06), SAT(0.5), DLY(0.35, 0.5, 0.55, tone=0.3), REV(0.4, 0.85), G(drift=0.9))
P('Vapor Trail', C, *AMB, O(1, PUL, 0.5, pw=0.3), O(2, SAW, 0.5, oct=1, fine=8), F(HP, 500, 0.3, env=0.0), L(1, LTRI, 0.2), M(lfo1_pw1=0.5, lfo1_cut=0.4), CHO(0.6), REV(0.6, 0.92), DLY(0.25, 0.6, 0.5))
P('Night Garden', C, *AMB, O(1, TRI, 0.8), O(2, SIN, 0.5, semi=12, fine=4), G(ring_level=0.2), F(LP12, 2800, 0.25, env=0.1), L(2, LSMOOTH, 0.3), M(lfo2_lr=0.4, lfo2_pan=0.6), REV(0.55, 0.9), DLY(0.3, 0.66, 0.5))

# =====================================================================================================
C = 'Sci-Fi'
P('Tractor Beam', C, O(1, SAW, 0.7), O(2, SQR, 0.5, fine=14), L(1, LSAW, 2.5), M(lfo1_p1=0.45, lfo1_p2=0.45, lfo1_cut=0.4), F(LP24, 1500, 0.6, env=0.0), E(1, 0.05, 0.3, 1.0, 0.4), PHA(0.5, 0.3), REV(0.3))
P('Laser Zap', C, O(1, SAW, 0.9, oct=1), E(2, 0.001, 0.25, 0.0, 0.25), M(env2_p1=0.9, env2_cut=0.6), F(LP24, 600, 0.6, env=0.0), E(1, 0.001, 0.3, 0.0, 0.25), DLY(0.3, 0.16, 0.5), REV(0.2))
P('Computer Thinking', C, O(1, SQR, 0.8, oct=1), SH(12, 0.0), M(sh_p1=0.55), F(LP12, 6000, 0.2, env=0.0), E(1, 0.002, 0.2, 1.0, 0.1), DLY(0.25, 0.12, 0.4), REV(0.2))
P('Flying Saucer', C, O(1, SIN, 0.8, oct=1), O(2, SIN, 0.5, oct=1, semi=5), L(1, LSIN, 7), L(2, LSIN, 0.3), M(lfo1_p1=0.3, lfo1_p2=-0.3, lfo2_p1=0.25, lfo2_pan=0.9), G(ring_level=0.4), F(LP12, 8000, 0.1, env=0.0), E(1, 0.4, 0.5, 1.0, 1.0), REV(0.4, 0.8))
P('Warp Engine', C, O(1, SAW, 0.7, oct=-2), O(2, SAW, 0.7, oct=-2, fine=20), E(3, 3.0, 2.0, 1.0, 1.5), M(env3_p1=0.5, env3_p2=0.52, env3_cut=0.6), G(noise_level=0.2), F(LP24, 300, 0.5, env=0.0), E(1, 0.3, 1.0, 1.0, 1.5), PHA(0.4, 0.5), REV(0.3))
P('Alien Speech', C, O(1, SAW, 0.8, oct=-1), SH(6, 0.4), L(2, LSMOOTH, 2.5), M(sh_cut=0.6, lfo2_p1=0.3, lfo2_res=0.3), F(BP, 900, 0.75, env=0.0, key=0.3), E(1, 0.02, 0.3, 1.0, 0.2), REV(0.25))
P('Ray Gun', C, O(1, SQR, 0.8, oct=1), O(2, SAW, 0.0, oct=-2, key=0), M(vco2_p1=0.55, env2_p2=0.7), E(2, 0.001, 0.4, 0.0, 0.3), F(LP24, 4000, 0.3, env=0.0), E(1, 0.001, 0.45, 0.0, 0.3), DLY(0.25, 0.2, 0.4))
P('Teleporter', C, O(1, SIN, 0.7, oct=1), O(2, TRI, 0.6, oct=2), G(noise_level=0.3), E(3, 1.5, 1.0, 0.0, 1.0), L(1, LSQR, 16), M(env3_p1=0.6, env3_p2=0.7, lfo1_vca=0.8, env3_ln=0.5), F(HP, 800, 0.4, env=0.0), E(1, 0.05, 1.0, 1.0, 1.0), REV(0.45, 0.85), DLY(0.3, 0.1, 0.6))
P('Robot Choir', C, O(1, PUL, 0.7, pw=0.2), O(2, SIN, 0.0, oct=-2, semi=5, key=0), G(ring_level=0.8), F(BP, 1100, 0.6, env=0.0, key=0.5), E(1, 0.08, 0.4, 1.0, 0.5), G(voice_mode=POLY), CHO(0.4), REV(0.35))
P('Scanner Sweep', C, O(1, SAW, 0.0), G(noise_level=0.6, noise_color=0.2), F(BP, 400, 0.93, env=0.0, key=0.5), L(1, LSAW, 0.8), M(lfo1_cut=0.8), E(1, 0.1, 0.5, 1.0, 0.6), DLY(0.3, 0.25, 0.5), REV(0.3))
P('Space Radio', C, O(1, SIN, 0.6, oct=1, fm=0.6), O(2, SIN, 0.0, semi=3), L(2, LSMOOTH, 0.7), SH(2, 0.6), M(lfo2_p2=0.4, sh_p1=0.25, lfo2_vca=0.4), G(noise_level=0.12), F(BP, 2200, 0.4, env=0.0), E(1, 0.2, 0.5, 1.0, 0.8), DLY(0.35, 0.5, 0.6), REV(0.4))
P('Force Field', C, O(1, SAW, 0.6, oct=-1), O(2, SAW, 0.6, oct=-1, semi=7, fine=9), G(ring_level=0.5), L(1, LSQR, 28), M(lfo1_cut=0.3, lfo1_vca=0.3), F(LP18, 2000, 0.6, env=0.0), E(1, 0.02, 0.5, 1.0, 0.4), PHA(0.5, 1.5, 0.8, 0.7), REV(0.3))

# =====================================================================================================
C = 'Industrial'
P('Steel Mill', C, O(1, SQR, 0.7, oct=-2), O(2, SAW, 0.6, oct=-2, semi=1), G(ring_level=0.7, noise_level=0.2), F(LP18, 1200, 0.4, env=0.3, drive=0.8), E(1, 0.005, 0.4, 0.8, 0.3), DIST(0.6, 0.5), REV(0.2))
P('Grinder', C, O(1, SAW, 0.8, oct=-2), O(2, SAW, 0.8, oct=-2, fine=30), M(vco2_p1=0.3, vco1_cut=0.4), F(LP24, 900, 0.5, env=0.2, drive=1.0), E(1, 0.005, 0.4, 0.9, 0.2), DIST(0.75, 0.55), G(voice_mode=MONO))
P('Metal Pipe', C, O(1, SQR, 0.0), O(2, SIN, 0.0, semi=7), G(ring_level=0.9), M(vco2_p1=0.35), F(BP, 2500, 0.8, env=0.3, key=0.5), E(1, 0.001, 0.5, 0.0, 0.4), E(2, 0.001, 0.2, 0.0, 0.2), REV(0.35, 0.6, 0.2))
P('Hydraulic Press', C, O(1, SAW, 0.8, oct=-3), G(noise_level=0.5, noise_color=0.7), E(2, 0.4, 0.5, 0.0, 0.3), M(env2_p1=-0.5, env2_cut=0.6), F(LP24, 400, 0.5, env=0.0, drive=0.6), E(1, 0.02, 0.8, 0.0, 0.4), DIST(0.5, 0.4), REV(0.25))
P('Alarm Siren', C, O(1, SQR, 0.8, oct=1), L(1, LTRI, 1.6, retrig=1), M(lfo1_p1=0.5), F(LP24, 3500, 0.4, env=0.0), E(1, 0.01, 0.3, 1.0, 0.1), DIST(0.3, 0.7, 0.6), DLY(0.15, 0.3, 0.3))
P('Conveyor Belt', C, O(1, SAW, 0.7, oct=-2), G(noise_level=0.4), L(1, LSQR, 8, retrig=1), M(lfo1_vca=0.9, lfo1_cut=0.4), F(BP, 1000, 0.6, env=0.0), E(1, 0.005, 0.3, 1.0, 0.2), SAT(0.7), DLY(0.2, 0.19, 0.4))
P('Distorted Stab', C, O(1, SAW, 0.8, oct=-1), O(2, SQR, 0.7, oct=-1, semi=7), F(LP24, 1400, 0.45, env=0.6, drive=0.7), E(1, 0.001, 0.25, 0.0, 0.2), E(2, 0.001, 0.2, 0.0, 0.2), DIST(0.65, 0.6), REV(0.2, 0.5))
P('Electric Arc', C, O(1, SAW, 0.5, oct=1), G(noise_level=0.6), SH(30, 0.0), M(sh_vca=0.9, sh_p1=0.4, noise_cut=0.5), F(HP, 2000, 0.5, env=0.0), E(1, 0.001, 0.4, 1.0, 0.1), DIST(0.5, 0.8))
P('Factory Floor', C, O(1, SQR, 0.6, oct=-2), O(2, SQR, 0.5, oct=-2, semi=6), G(noise_level=0.3, noise_color=0.5), L(2, LSH, 4), M(lfo2_cut=0.5, lfo2_l2=0.5), F(LP24, 700, 0.6, env=0.0, drive=0.5), E(1, 0.01, 0.5, 1.0, 0.3), SAT(0.6), REV(0.3, 0.7))
P('Chainsaw Lead', C, O(1, SAW, 0.9), O(2, SAW, 0.9, semi=12), G(o2_sync=1, voice_mode=LEGATO, glide=0.04), L(1, LSAW, 22), M(lfo1_p2=0.35, mod_p2=0.5), F(LP12, 5000, 0.3, env=0.0, drive=0.6), E(1, 0.005, 0.3, 1.0, 0.15), DIST(0.6, 0.65))
P('Rusty Gate', C, O(1, SAW, 0.6, oct=-1), O(2, SAW, 0.0, oct=-2, semi=3), E(3, 0.8, 1.2, 0.3, 0.8), M(vco2_p1=0.45, env3_p2=0.5, env3_cut=0.4), F(BP, 1500, 0.8, env=0.0), E(1, 0.1, 0.5, 1.0, 0.5), DIST(0.4, 0.5, 0.7), REV(0.3))
P('Pneumatic Burst', C, O(1, SAW, 0.0), G(noise_level=1.0, noise_color=0.1), F(HP, 1200, 0.4, env=-0.5), E(1, 0.001, 0.2, 0.0, 0.15), E(2, 0.001, 0.15, 0.0, 0.15), SAT(0.6), REV(0.25, 0.4))

# =====================================================================================================
C = 'Percussion'
PERC = [G(voice_mode=POLY, drift=0.0, vel_sens=0.8)]
P('Analog Kick', C, *PERC, O(1, SIN, 1.0, oct=-2), E(2, 0.001, 0.06, 0.0, 0.06), M(env2_p1=0.62), F(LP24, 800, 0.0, env=0.3), E(1, 0.001, 0.35, 0.0, 0.3), SAT(0.4))
P('Snappy Snare', C, *PERC, O(1, TRI, 0.6, oct=-1), G(noise_level=0.8), E(2, 0.001, 0.05, 0.0, 0.05), M(env2_p1=0.4), F(BP, 1800, 0.3, env=0.3, key=0.0), E(1, 0.001, 0.16, 0.0, 0.14))
P('Closed Hat', C, *PERC, O(1, SQR, 0.3, oct=3), O(2, SQR, 0.3, oct=3, semi=5), G(noise_level=0.9, ring_level=0.4), F(HP, 6500, 0.3, env=0.0, key=0.0), E(1, 0.001, 0.05, 0.0, 0.05))
P('Open Hat', C, *PERC, O(1, SQR, 0.3, oct=3), O(2, SQR, 0.3, oct=3, semi=6), G(noise_level=0.9, ring_level=0.5), F(HP, 6000, 0.35, env=0.0, key=0.0), E(1, 0.001, 0.4, 0.0, 0.35))
P('Electro Tom', C, *PERC, O(1, SIN, 1.0, oct=-1), E(2, 0.001, 0.14, 0.0, 0.14), M(env2_p1=0.42), F(LP12, 3000, 0.1, env=0.0), E(1, 0.001, 0.4, 0.0, 0.35), REV(0.15, 0.4))
P('Clave Tick', C, *PERC, O(1, SIN, 0.9, oct=2), F(BP, 2500, 0.6, env=0.0, key=0.6), E(1, 0.001, 0.045, 0.0, 0.04), REV(0.2, 0.4))
P('Cowbell', C, *PERC, O(1, SQR, 0.7, oct=1), O(2, SQR, 0.7, oct=1, semi=7, fine=40), F(BP, 1700, 0.5, env=0.0, key=0.3), E(1, 0.001, 0.25, 0.0, 0.2))
P('Zap Drum', C, *PERC, O(1, SAW, 0.9), E(2, 0.001, 0.09, 0.0, 0.09), M(env2_p1=0.85), F(LP24, 500, 0.5, env=0.7), E(1, 0.001, 0.2, 0.0, 0.18))
P('Metal Hit', C, *PERC, O(1, SQR, 0.0, oct=1), O(2, SQR, 0.0, oct=1, semi=8, fine=27), G(ring_level=1.0), M(vco2_p1=0.3), F(HP, 900, 0.4, env=0.0), E(1, 0.001, 0.5, 0.0, 0.45), REV(0.3, 0.6, 0.2))
P('Noise Clap', C, *PERC, O(1, SAW, 0.0), G(noise_level=1.0), L(1, LSAW, 38, retrig=1), M(lfo1_vca=0.8), F(BP, 1300, 0.45, env=0.0, key=0.0), E(1, 0.001, 0.14, 0.0, 0.12), REV(0.3, 0.45))
P('Wood Block', C, *PERC, O(1, TRI, 0.9, oct=1), O(2, SIN, 0.4, oct=2, semi=5), E(2, 0.001, 0.02, 0.0, 0.02), M(env2_p1=0.3), F(LP12, 5000, 0.2, env=0.0), E(1, 0.001, 0.07, 0.0, 0.06))
P('Resonant Ping', C, *PERC, O(1, SAW, 0.0), G(noise_level=0.0), F(LP24, 262, 0.99, env=0.0, key=1.0), E(1, 0.001, 0.9, 0.0, 0.9), E(3, 0.001, 0.03, 0.0, 0.03), M(env3_ln=1.0), G(a_level=1.0, f_drive=0.3), SAT(0.9), DLY(0.3, 0.3, 0.5), REV(0.3))

# =====================================================================================================
C = 'Sound Effects'
P('Thunderstorm', C, O(1, SAW, 0.0), G(noise_level=1.0, noise_color=1.0), F(LP24, 200, 0.3, env=0.0, key=0.0), L(1, LSMOOTH, 1.8), L(2, LSMOOTH, 0.4), M(lfo1_cut=0.6, lfo2_vca=0.7, lfo1_vca=0.4), E(1, 0.3, 1.0, 1.0, 2.5), REV(0.5, 0.92, 0.6))
P('Ocean Surf', C, O(1, SAW, 0.0), G(noise_level=0.9, noise_color=0.5), F(LP12, 900, 0.1, env=0.0, key=0.0), L(1, LSIN, 0.13), M(lfo1_cut=0.6, lfo1_vca=0.7, lfo2_pan=0.5), L(2, LSMOOTH, 0.3), E(1, 1.0, 1.0, 1.0, 2.5), REV(0.3, 0.7))
P('Howling Wind', C, O(1, SAW, 0.0), G(noise_level=0.9), F(BP, 600, 0.9, env=0.0, key=0.4), L(1, LSMOOTH, 0.5), L(2, LSMOOTH, 0.17), M(lfo1_cut=0.45, lfo2_cut=0.45, lfo1_vca=0.3), E(1, 0.8, 1.0, 1.0, 2.0), REV(0.35, 0.8))
P('Helicopter', C, O(1, SAW, 0.3, oct=-3), G(noise_level=0.9, noise_color=0.6), F(LP24, 500, 0.3, env=0.0, key=0.0), L(1, LSAW, 11), M(lfo1_vca=0.95, lfo1_cut=0.4, lfo2_pan=0.7), L(2, LSIN, 0.1), E(1, 0.5, 1.0, 1.0, 1.0))
P('Rainfall', C, O(1, SAW, 0.0), G(noise_level=0.9), F(HP, 3500, 0.2, env=0.0, key=0.0), SH(45, 0.0), M(sh_vca=0.9, sh_cut=0.3, sh_pan=0.9), E(1, 0.5, 1.0, 1.0, 1.5), REV(0.35, 0.6))
P('Engine Start', C, O(1, SAW, 0.8, oct=-3), O(2, SQR, 0.6, oct=-3, fine=25), G(noise_level=0.2), E(3, 3.0, 2.0, 0.8, 1.5), M(env3_p1=0.5, env3_p2=0.5, env3_cut=0.5), F(LP24, 300, 0.3, env=0.0, drive=0.5), E(1, 0.1, 1.0, 1.0, 0.8), SAT(0.5))
P('Dripping Cave', C, O(1, SIN, 0.9, oct=2), E(2, 0.001, 0.04, 0.0, 0.04), L(1, LSH, 2.2), M(env2_p1=0.4, lfo1_p1=0.3, lfo1_pan=0.8), F(LP12, 7000, 0.3, env=0.0), E(1, 0.001, 0.09, 0.0, 0.09), G(voice_mode=MONO), SEQ([0, 7, 3, 12, 5], [1, 0, 1, 1, 0], rate=1, gate=0.2, bpm=90), DLY(0.4, 0.41, 0.55), REV(0.55, 0.9))
P('Siren Rise', C, O(1, SAW, 0.8), O(2, SQR, 0.4, fine=9), L(1, LSIN, 0.45, retrig=1), M(lfo1_p1=0.5, lfo1_p2=0.5), F(LP24, 2500, 0.3, env=0.0), E(1, 0.05, 0.5, 1.0, 0.3), DLY(0.15), REV(0.2))
P('Geiger Counter', C, O(1, SAW, 0.0), G(noise_level=1.0), F(HP, 3000, 0.5, env=0.0, key=0.0), SH(48, 0.0), L(1, LSH, 25), M(sh_vca=1.0, lfo1_vca=1.0), E(1, 0.01, 0.5, 1.0, 0.1))
P('Explosion', C, O(1, SAW, 0.4, oct=-3), G(noise_level=1.0, noise_color=0.8), E(2, 0.001, 1.5, 0.0, 1.5), F(LP24, 150, 0.2, env=0.9, key=0.0, drive=0.8), E(1, 0.001, 2.2, 0.0, 2.0), M(env2_p1=0.3), DIST(0.5, 0.4, 0.7), REV(0.4, 0.85, 0.5))
P('Heartbeat', C, O(1, SIN, 1.0, oct=-3), E(2, 0.001, 0.05, 0.0, 0.05), M(env2_p1=0.4), F(LP24, 300, 0.1, env=0.0), E(1, 0.002, 0.16, 0.0, 0.14), G(voice_mode=MONO), SEQ([0, 0, 0, 0, 0, 0, 0, 0], [1, 0, 1, 0, 0, 0, 0, 0], rate=1, gate=0.4, bpm=132), REV(0.15, 0.4))
P('Insect Swarm', C, O(1, SAW, 0.5, oct=2), O(2, SAW, 0.5, oct=2, fine=35), O(3, SQR, 0.4, oct=2, fine=-28), L(1, LSMOOTH, 9), L(2, LSMOOTH, 6), M(lfo1_p1=0.2, lfo2_p2=0.2, lfo1_vca=0.5, lfo2_pan=0.8), F(BP, 4500, 0.5, env=0.0), E(1, 0.3, 0.5, 1.0, 0.6), G(drift=1.0))

# =====================================================================================================
C = 'Sequences'
SQ = [G(voice_mode=MONO, drift=0.1)]
P('Berlin School', C, *SQ, O(1, SAW, 0.8, oct=-1), O(2, SAW, 0.5, oct=-1, fine=7), F(LP24, 600, 0.5, env=0.45), E(1, 0.001, 0.15, 0.3, 0.1), E(2, 0.001, 0.12, 0.0, 0.1), L(2, LSIN, 0.05), M(lfo2_cut=0.55), SEQ([0, 0, 12, 0, 7, 0, 10, 12], rate=3, gate=0.5, bpm=128), DLY(0.3, 0.352, 0.5), REV(0.2))
P('Octave Bounce', C, *SQ, O(1, SQR, 0.8, oct=-1), F(LP24, 900, 0.4, env=0.4), E(1, 0.001, 0.12, 0.2, 0.08), E(2, 0.001, 0.1, 0.0, 0.08), SEQ([0, 12, 0, 12, 0, 12, 0, 12], rate=3, gate=0.4, bpm=124), DLY(0.2, 0.24, 0.4))
P('Minor Arp Up', C, *SQ, O(1, SAW, 0.7), O(2, PUL, 0.5, pw=0.3, fine=5), F(LP18, 1500, 0.35, env=0.4), E(1, 0.001, 0.2, 0.2, 0.15), E(2, 0.001, 0.15, 0.0, 0.12), SEQ([0, 3, 7, 12, 15, 12, 7, 3], rate=3, gate=0.6, bpm=118), CHO(0.3), DLY(0.3, 0.38, 0.45), REV(0.2))
P('Acid Line', C, *SQ, O(1, SAW, 0.9, oct=-2), F(LP18, 300, 0.85, env=0.6, drive=0.3), E(1, 0.001, 0.2, 0.3, 0.06), E(2, 0.001, 0.14, 0.0, 0.1), G(voice_mode=LEGATO, glide=0.04), M(seq_cut=0.5), SEQ([0, 0, 12, 0, 3, 0, 15, 10, 0, 12, 0, 7, 0, 3, 5, 12], [1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1], rate=3, gate=1.0, bpm=130), DIST(0.25, 0.7, 0.4), DLY(0.2, 0.345, 0.4))
P('Trance Gate', C, *SQ, O(1, SAW, 0.7, fine=-8), O(2, SAW, 0.7, fine=8), F(LP24, 2000, 0.3, env=0.3), E(1, 0.001, 0.1, 0.6, 0.06), SEQ([0, 0, 0, 0, 0, 0, 0, 0], [1, 0, 1, 1, 0, 1, 1, 0], rate=3, gate=0.6, bpm=138), CHO(0.4), DLY(0.3, 0.326, 0.5), REV(0.25))
P('Fifths Machine', C, *SQ, O(1, SQR, 0.7, oct=-1), O(2, SAW, 0.6, oct=-1, semi=7), F(LP24, 800, 0.5, env=0.5), E(1, 0.001, 0.14, 0.2, 0.1), E(2, 0.001, 0.12, 0.0, 0.1), SEQ([0, 0, 7, 0, 5, 0, 7, 12], [1, 0, 1, 1, 1, 0, 1, 1], rate=3, gate=0.5, bpm=120), PHA(0.3, 0.15), DLY(0.25))
P('Triplet Pulse', C, *SQ, O(1, PUL, 0.8, oct=-1, pw=0.25), F(LP24, 700, 0.45, env=0.5), E(1, 0.001, 0.15, 0.1, 0.1), E(2, 0.001, 0.13, 0.0, 0.1), L(2, LTRI, 0.1), M(lfo2_pw1=0.4, lfo2_cut=0.4), SEQ([0, 7, 12, 0, 10, 7], rate=4, gate=0.5, bpm=110), DLY(0.3, 0.273, 0.5), REV(0.2))
P('Random Feeling', C, *SQ, O(1, TRI, 0.9), O(2, SIN, 0.4, oct=1), F(LP12, 3500, 0.2, env=0.2), E(1, 0.002, 0.25, 0.0, 0.25), SH(3.2, 0.0), M(sh_cut=0.4, sh_pan=0.8, seq_pan=0.4), SEQ([0, 5, 7, 12, 14, 7, 10, 5, 3, 12, 0, 7], [1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1], rate=3, gate=0.4, bpm=104), DLY(0.4, 0.43, 0.55), REV(0.4, 0.8))
P('Dark Pulse Bass', C, *SQ, O(1, SAW, 0.8, oct=-2), O(2, SQR, 0.6, oct=-2), F(LP24, 250, 0.5, env=0.5, drive=0.4), E(1, 0.001, 0.14, 0.4, 0.08), E(2, 0.001, 0.1, 0.0, 0.08), SEQ([0, 0, 0, 3, 0, 0, 5, 3], rate=3, gate=0.55, bpm=126), SAT(0.4))
P('Sync Riff', C, *SQ, O(1, SAW, 0.0, oct=-1), O(2, SAW, 0.9, oct=-1, semi=7), G(o2_sync=1), M(seq_p2=0.5, env2_p2=0.4), E(2, 0.001, 0.15, 0.0, 0.1), F(LP12, 4000, 0.2, env=0.2), E(1, 0.001, 0.15, 0.3, 0.1), SEQ([0, 12, 7, 0, 10, 0, 12, 5], rate=3, gate=0.5, bpm=132), DLY(0.25, 0.34, 0.45))
P('Slow Mover', C, *SQ, O(1, SAW, 0.6, oct=-1, fine=-6), O(2, SAW, 0.6, oct=-1, fine=6), F(LP24, 700, 0.35, env=0.4), E(1, 0.02, 0.5, 0.5, 0.4), E(2, 0.05, 0.5, 0.2, 0.4), G(voice_mode=LEGATO, glide=0.08), SEQ([0, 3, 7, 10, 12, 10, 7, 3], rate=1, gate=1.0, bpm=96), CHO(0.4), DLY(0.3, 0.468, 0.5), REV(0.35, 0.8))
P('Bleep Pattern', C, *SQ, O(1, SIN, 0.9, oct=1), F(LP12, 8000, 0.1, env=0.0), E(1, 0.001, 0.08, 0.0, 0.08), M(seq_pan=0.8), SEQ([0, 12, 7, 19, 12, 24, 7, 15, 0, 12, 3, 19, 12, 22, 10, 15], rate=3, gate=0.3, bpm=122), DLY(0.35, 0.369, 0.55), REV(0.3))

# =====================================================================================================
C = 'Atmospheric Textures'
TX = [G(voice_mode=POLY, drift=0.7), E(1, 1.5, 2.0, 1.0, 3.5)]
P('Dust Particles', C, *TX, O(1, SAW, 0.0), G(noise_level=0.7), F(BP, 3000, 0.85, env=0.0, key=0.5), SH(14, 0.05), M(sh_cut=0.6, sh_vca=0.6, sh_pan=0.9), DLY(0.4, 0.31, 0.6), REV(0.5, 0.9))
P('Glacier', C, *TX, O(1, SIN, 0.6, oct=1), O(2, TRI, 0.5, oct=1, fine=13), O(3, SIN, 0.4, oct=2, fine=-9), F(HP, 400, 0.2, env=0.0), L(1, LSMOOTH, 0.25), L(2, LSMOOTH, 0.4), M(lfo1_l2=0.5, lfo2_l3=0.5, lfo1_pan=0.6), CHO(0.5, 0.2), REV(0.65, 0.95, 0.2))
P('Radio Static Field', C, *TX, O(1, SIN, 0.3, oct=2, fm=0.9), O(2, SAW, 0.0, semi=1), G(noise_level=0.4, noise_color=0.3), SH(20, 0.2), L(2, LSMOOTH, 0.3), M(sh_p2=0.4, lfo2_cut=0.5, noise_vca=0.4), F(BP, 2500, 0.5, env=0.0), DLY(0.3, 0.53, 0.5), REV(0.45, 0.85))
P('Rusty Strings', C, *TX, O(1, SAW, 0.6, fine=-14), O(2, SAW, 0.6, fine=14), G(ring_level=0.25), F(LP18, 1300, 0.45, env=0.0), L(2, LSMOOTH, 0.5), M(lfo2_cut=0.4, lfo2_p1=0.04, vco2_cut=0.12), SAT(0.5), REV(0.5, 0.88))
P('Deep Space', C, *TX, O(1, SIN, 0.7, oct=-2), O(2, TRI, 0.4, oct=2, fine=7), F(LP12, 3000, 0.2, env=0.0), L(1, LSMOOTH, 0.12), M(lfo1_l2=0.5, lfo1_pan=0.7, lfo2_p2=0.03), L(2, LSIN, 0.3), DLY(0.35, 0.8, 0.6), REV(0.65, 0.96, 0.4))
P('Whispering Voices', C, *TX, O(1, SAW, 0.3), G(noise_level=0.6, noise_color=0.8), F(BP, 1200, 0.8, env=0.0, key=0.4), L(1, LSMOOTH, 2.2), L(2, LSMOOTH, 0.5), M(lfo1_cut=0.45, lfo2_vca=0.6, lfo2_pan=0.6), PHA(0.4, 0.2), REV(0.5, 0.88))
P('Metallic Fog', C, *TX, O(1, SQR, 0.0), O(2, SIN, 0.0, semi=6, fine=20), G(ring_level=0.8), F(BP, 1800, 0.5, env=0.0), L(2, LSMOOTH, 0.2), M(lfo2_p2=0.2, lfo2_cut=0.4, lfo2_pan=0.6), CHO(0.4), REV(0.6, 0.93, 0.3))
P('Shifting Sands', C, *TX, O(1, SAW, 0.0), G(noise_level=0.8, noise_color=0.9), F(LP24, 1500, 0.5, env=0.0, key=0.3), L(1, LSMOOTH, 0.7), L(2, LSIN, 0.08), M(lfo1_cut=0.5, lfo2_cut=0.4, lfo1_pan=0.8, lfo1_vca=0.3), PHA(0.4, 0.09), REV(0.4, 0.8))
P('Electric Meadow', C, *TX, O(1, SIN, 0.6, oct=2), O(2, SIN, 0.5, oct=2, semi=7), SH(8, 0.3), L(1, LSMOOTH, 5), M(sh_p1=0.3, lfo1_p2=0.25, sh_pan=0.8, lfo1_vca=0.5), F(LP12, 9000, 0.1, env=0.0), DLY(0.4, 0.29, 0.6), REV(0.5, 0.9))
P('Cathedral Hum', C, *TX, O(1, SAW, 0.6, oct=-2), O(2, SQR, 0.4, oct=-1, fine=5), O(3, SIN, 0.5, oct=-1, semi=7), F(LP24, 450, 0.4, env=0.0), L(2, LSMOOTH, 0.1), M(lfo2_cut=0.45), REV(0.7, 0.97, 0.5))
P('Broken Glass Cloud', C, *TX, O(1, SIN, 0.5, oct=2, fm=0.7), O(2, SIN, 0.0, oct=2, semi=11), SH(10, 0.0), L(1, LSQR, 7), M(sh_p2=0.5, lfo1_vca=0.8, sh_pan=0.9), F(HP, 1500, 0.3, env=0.0), DLY(0.45, 0.23, 0.65), REV(0.55, 0.92, 0.2))
P('Evolving Organism', C, *TX, O(1, SAW, 0.6, oct=-1), O(2, PUL, 0.5, pw=0.3, fine=9), O(3, TRI, 0.0, oct=-2, key=0), G(o3_low=1, ring_level=0.3), L(1, LSMOOTH, 0.3), L(2, LSMOOTH, 0.13), M(vco3_cut=0.5, lfo1_pw2=0.5, lfo2_lr=0.4, lfo1_pan=0.6, lfo2_res=0.3), F(LP18, 900, 0.5, env=0.0), PHA(0.4, 0.11), REV(0.5, 0.9))

# =====================================================================================================
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def load_levels():
    p = os.path.join(ROOT, 'build-test', 'preset_levels.txt'); lv = {}
    if os.path.exists(p):
        for line in open(p, encoding='utf-8'):
            a = line.rstrip('\n').split('\t')
            if len(a) >= 3: lv[a[0]] = (float(a[1]), float(a[2]))
    return lv

def main():
    names = [p[0] for p in PRESETS]
    assert len(set(names)) == len(names), 'duplicate preset names'
    levels = load_levels(); TARGET = 0.45; trimmed = 0
    out = ['// Сгенерировано tools/gen_presets.py — не редактировать вручную.', '#pragma once', '',
           'struct FactoryPreset { const char* name; const char* category; const char* data; };', '',
           'static const FactoryPreset kFactoryPresets[] = {']
    for name, cat, d in PRESETS:
        d = dict(d)
        if name in levels:                      # калибровка громкости по измеренному пику (уровень ~ master_vol^2)
            peak, mv = levels[name]
            if peak > 1e-4:
                d['master_vol'] = round(max(0.12, min(1.0, mv * (TARGET / (2.0 if peak > 0.9 else peak)) ** 0.5)), 3); trimmed += 1   # пик > 0,9 — сработал ограничитель, реальный уровень выше
        data = ';'.join(f'{k}={v:g}' for k, v in sorted(d.items()))
        out.append(f'    {{ "{name}", "{cat}", "{data}" }},')
    out += ['};', f'static const int kNumFactoryPresets = {len(PRESETS)};', '']
    path = os.path.join(ROOT, 'Source', 'FactoryPresets.h')
    open(path, 'w', encoding='utf-8', newline='\n').write('\n'.join(out))
    cats = {}
    for _, c, _ in PRESETS: cats[c] = cats.get(c, 0) + 1
    print(f'{len(PRESETS)} presets, {len(cats)} categories, {trimmed} level-trimmed -> {path}')

if __name__ == '__main__':
    main()
