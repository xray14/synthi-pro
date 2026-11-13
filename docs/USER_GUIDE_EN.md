# SYNTHI PRO — User Guide

SYNTHI PRO is a virtual modular synthesizer built around a patch matrix: three oscillators, noise, a ring modulator,
a multimode filter, three envelopes, two LFOs, Sample & Hold, a 16-step sequencer, a 16×16 matrix and seven effects.
It runs as a standalone application and as a VST3 plug-in.

## 1. The window

| Area | Contents |
|---|---|
| Top bar | title, preset arrows and name (click opens the browser), Save, Load, Random, Init, EASY/PRO, EN/RU, Settings, voice counter |
| Centre (PRO) | synthesis modules on the left, matrix and sequencer on the right |
| Centre (EASY) | preset browser on the left, twelve essential knobs on the right |
| Right panel | effects, stereo width, master volume, level meters |
| Bottom | Pitch Bend and Modulation wheels, octave and range buttons, Panic, the keyboard |

Knobs: drag up/down; double-click resets; hovering shows the full parameter name, turning shows the value.
Resize the window by its corner or pick a scale in Settings.

## 2. Modules

**VCO 1–3.** Waveform (Sine, Triangle, Saw, Square, Pulse), range in octaves, semitones, detune in cents, pulse
width, FM and mixer level. KEY enables keyboard tracking (off = fixed frequency). SYNC on VCO 2 and VCO 3 is hard
sync to VCO 1. LOW moves VCO 3 into the sub-audio range so it can serve as a third LFO.

The FM knob is linear (phase) modulation that stays in tune: VCO 1 ← VCO 2, VCO 2 ← VCO 3, VCO 3 ← VCO 1.
Exponential audio-rate FM is patched in the matrix (for example VCO 2 → VCO 1 Pitch).

**Noise.** Colour from white to pink, and level.
**Ring Mod.** VCO 1 × VCO 2 — works even when the oscillators' own levels are down.
**Filter.** LP 24, LP 18, LP 12 (ladder design, self-oscillates near maximum resonance), Band Pass, High Pass,
Notch. Controls: cutoff, resonance, drive, Envelope 2 amount (bipolar), key tracking, velocity amount.
**VCA.** Level, pan and velocity sensitivity.
**Envelopes.** Three ADSRs: the first drives loudness, the second drives the cutoff (Env 2 knob), the third is free
and is patched through the matrix.
**LFO 1–2.** Sine, Triangle, Saw, Square, S&H, Smooth Random; 0.01–50 Hz; fade-in; KEY restarts the phase per note.
**Sample & Hold.** Source (noise, LFO 1, VCO 3), rate, lag.
**Voices.** Poly / Mono / Legato, 1–16 voices, bend range, master tune. Glide is the portamento time; in Legato it
only applies to overlapping notes. Drift adds slight analog pitch instability.

## 3. Patch matrix

Rows are sources, columns are destinations.

- Click an empty point to insert a pin at +50 %.
- Drag up/down to set the depth from −100 to +100 % (amber = positive, blue = negative, size = depth).
- Click an existing pin, or right-click, to remove it. The mouse wheel fine-tunes.
- "Clear matrix" removes everything. The number of active routes is shown next to it.

Sources: VCO 1–3, noise, ring modulator, LFO 1–2, S&H, envelopes 1–3, velocity, key tracking, mod wheel,
aftertouch, sequencer.
Destinations: pitch of VCO 1–3, pulse width of VCO 1–3, levels of VCO 1–3, noise and ring modulator, cutoff,
resonance, filter input (audio bypassing the mixer), VCA (amplitude modulation), pan.

The depth scale is quadratic, so small values give subtle modulation. For pitch: 7 % ≈ gentle vibrato,
50 % = ±1 octave, 100 % = ±4 octaves. For cutoff, 100 % = ±5 octaves.

Any oscillator can be a modulation source: VCO → another VCO's pitch (FM), VCO → VCA (AM), VCO → cutoff (filter FM).

## 4. Sequencer

Switch it ON and hold a key: the key sets the root and the steps play as semitone offsets. Bars are step pitches
(−24…+24), the squares below are gates. Step length 1/4…1/32, gate time, 1–16 steps. With Gate at maximum the
notes are tied (in Legato mode this produces slides). The plug-in follows the DAW tempo; the standalone version
uses the Tempo knob. The step value is available in the matrix as the "Sequencer" source.

## 5. Effects

Chain: Distortion → Saturation → Phaser → Chorus → Delay → Reverb → Stereo Width. Each block is switched with its
name button. Distortion and Saturation run at twice the sample rate.

## 6. Presets

- 156 factory presets in 13 categories.
- Browser: search by name, category filter, star = favorite, the Favorites button shows only those.
- Save creates a user preset (name and category). Files live in `Documents\SYNTHI PRO\Presets\*.synthipreset`;
  Settings has an item that opens the folder.
- Random builds a patch of one of four types (bass, lead, pad, texture) within limits: consonant oscillator
  intervals, sensible envelope times, proven matrix routes.
- Init resets every parameter.

## 7. EASY and PRO

EASY shows the preset browser and twelve knobs: cutoff, resonance, filter envelope, drive, attack, decay, sustain,
release, glide, LFO rate, noise, drift. Effects and the keyboard are available in both modes.
PRO shows every module, the matrix and the sequencer.

## 8. Keyboard and MIDI

- Mouse: the lower you click on a key, the higher the velocity.
- Computer keys: the A S D F… row plays white keys, W E T Y U… black keys. The "−" / "+" buttons shift the octave.
- The range button cycles 88 / 61 / 49 keys.
- MIDI: notes with velocity, pitch bend, mod wheel (CC 1), sustain pedal (CC 64), aftertouch.
- The on-screen BEND and MOD wheels follow the wheels of your MIDI keyboard.
- The mod wheel always adds vibrato from LFO 1; the "Mod>Vib" knob in Voices / Global sets its depth.
- Every parameter can be automated in a DAW.

### MIDI Learn — assigning hardware knobs and faders

1. Right-click a knob, list or switch in the window — it starts blinking light red.
2. Turn a knob, move a fader or press a button on your MIDI controller. The link is made: a message with the
   controller number appears at the top and a red dot stays on the control.
3. Right-clicking the blinking control again cancels.
4. Right-clicking an assigned control opens a menu: "Assign another controller" or "Remove assignment".
5. Settings -> "Remove all MIDI assignments" clears everything.

One controller drives one parameter and one parameter has one controller: a new link replaces the old one.
Links are stored in `Documents\SYNTHI PRO\midi_map.txt` and apply to all presets; changing a preset keeps them.
CC 0, 1, 32, 64 and 120-127 are reserved and cannot be assigned. Matrix pins and sequencer steps cannot be
assigned through MIDI Learn.
## 9. State

The standalone version remembers the whole patch, language, mode and scale on exit and restores them on launch.
In a DAW the state is stored in the project.
