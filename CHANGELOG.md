# Changelog / История изменений

## 1.1.0 — 2026-11-14

### Added / Добавлено

- **MIDI Learn.** Right-click a knob, list or switch — it blinks light red — then move a knob, fader or button on
  the MIDI controller. Right-clicking an assigned control offers "Assign another controller" and "Remove assignment";
  Settings has "Remove all MIDI assignments". Links survive preset changes and restarts
  (`Documents\SYNTHI PRO\midi_map.txt`).
  **MIDI Learn.** Правый щелчок по ручке, списку или переключателю — он мигает светло-красным — затем подвигайте ручку,
  фейдер или кнопку на MIDI-контроллере. Правый щелчок по привязанному регулятору: «Назначить другой контроллер» и
  «Снять назначение»; в «Настройках» — «Снять все MIDI-назначения». Привязки сохраняются при смене пресета и между
  запусками.
- "Mod>Vib" knob: depth of the mod-wheel vibrato. / Ручка «Мод>Вибр»: глубина вибрато от колеса модуляции.

### Fixed / Исправлено

- The on-screen BEND and MOD wheels did not move when the hardware wheels were used. They now follow the MIDI keyboard.
  Экранные колёса BEND и MOD не двигались при работе колёсами на MIDI-клавиатуре. Теперь они повторяют их движения.
- The mod wheel did nothing in presets without a "Mod Wheel" route in the matrix (143 of 156). It now always
  adds vibrato from LFO 1; matrix routes from the wheel still work and add to it.
  Колесо модуляции ничего не делало в пресетах без маршрута «Mod Wheel» в матрице (143 из 156). Теперь оно всегда
  даёт вибрато от LFO 1; маршруты матрицы от колеса по-прежнему работают и складываются с ним.

### Known limitations / Известные ограничения

- Matrix pins and sequencer steps cannot be assigned through MIDI Learn. / Точки матрицы и шаги секвенсора через
  MIDI Learn не назначаются.
- Relative (endless) encoders, NRPN and 14-bit controllers are not supported; no soft pick-up: the parameter jumps
  to the knob position on first touch. / Относительные энкодеры, NRPN и 14-битные контроллеры не поддерживаются;
  плавного подхвата нет: при первом касании параметр прыгает в положение ручки.
- Tested with one controller only (Novation Launchkey 49 MK1). / Проверено только с одним контроллером
  (Novation Launchkey 49 MK1).

## 1.0.1 — 2026-11-11

### Changed / Изменено

- The About window links to this repository for the source code instead of "available on request".
  В окне «О программе» вместо «по запросу» указана ссылка на этот репозиторий с исходным кодом.
- `LICENSE.txt` now holds the full text of the GNU AGPL v3; copyright and third-party notices moved to `NOTICE.txt`.
  `LICENSE.txt` содержит полный текст GNU AGPL v3; уведомления об авторстве и сторонних компонентах — в `NOTICE.txt`.

## 1.0.0 — 2026-11-11

First public release. / Первый публичный выпуск.

- Sound engine: 3 VCOs with hard sync and FM, noise, ring modulator, multimode self-oscillating filter, 3 envelopes,
  2 LFOs, Sample & Hold, 16 x 16 patch matrix, 16-step sequencer, poly / mono / legato, glide, 7 effects.
  Звуковой движок: 3 генератора с синхронизацией и FM, шум, кольцевой модулятор, многорежимный фильтр
  с самовозбуждением, 3 огибающие, 2 LFO, Sample & Hold, матрица 16 x 16, секвенсор на 16 шагов, поли / моно / легато,
  глайд, 7 эффектов.
- 156 factory presets in 13 categories, user presets, favorites, random patch.
  156 заводских пресетов в 13 категориях, пользовательские пресеты, избранное, случайный патч.
- Standalone application (WASAPI, DirectSound, ASIO) and VST3 plug-in, installer.
  Автономная программа (WASAPI, DirectSound, ASIO), плагин VST3, установщик.
- EASY and PRO modes, English and Russian interface, resizable window, on-screen keyboard with wheels.
  Режимы EASY и PRO, интерфейс на русском и английском, масштабируемое окно, экранная клавиатура с колёсами.
- Logo with a glint animation, About window, application icons.
  Логотип с анимацией блика, окно «О программе», иконки программы.

### Known limitations / Известные ограничения

- Not yet tested in DAWs. / Работа в DAW пока не проверена.
- The binaries are signed with a self-signed certificate, so Windows SmartScreen shows a warning.
  Файлы подписаны самоподписанным сертификатом, поэтому Windows SmartScreen показывает предупреждение.
