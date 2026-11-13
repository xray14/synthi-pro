# SYNTHI PRO

Matrix modular software synthesizer for Windows 10/11 (x64): standalone application and VST3 plug-in.
Матричный модульный программный синтезатор для Windows 10/11 (x64): автономная программа и плагин VST3.

**[Download the latest version / Скачать последнюю версию](https://github.com/xray14/synthi-pro/releases/latest)** ·
[What's new / Что нового](CHANGELOG.md)

![SYNTHI PRO - PRO mode](docs/screenshots/pro.png)

## Screenshots / Скриншоты

| | |
|---|---|
| ![Patch matrix](docs/screenshots/matrix.png) Patch matrix / Матрица коммутации | ![MIDI Learn](docs/screenshots/midi-learn.png) MIDI Learn |
| ![EASY mode](docs/screenshots/easy.png) EASY mode / Режим EASY | ![Preset browser](docs/screenshots/presets.png) Preset browser / Браузер пресетов |
| ![English interface](docs/screenshots/pro-en.png) English interface / Английский интерфейс | ![About](docs/screenshots/about.png) About / О программе |

## Features / Возможности

- 3 VCOs (Sine, Triangle, Saw, Square, Pulse) with hard sync, phase FM and audio-rate cross-modulation
- White/pink noise, ring modulator, mixer
- Multimode filter: ladder LP 24 / 18 / 12 (self-oscillating), band-pass, high-pass, notch
- 3 ADSR envelopes, 2 LFOs, Sample & Hold
- 16 x 16 patch matrix with bipolar depth per pin
- 16-step sequencer, mono / legato / poly (up to 16 voices), glide
- Effects: distortion, saturation, phaser, chorus, delay, reverb, stereo width
- 156 factory presets in 13 categories, user presets, favorites, random patch
- MIDI Learn: right-click any control and move a knob or fader on your controller
- EASY and PRO modes, English and Russian interface, resizable window
- WASAPI, DirectSound and ASIO in the standalone version

## Installation / Установка

Download `SYNTHI-PRO-x.y.z-Setup.exe` from [Releases](https://github.com/xray14/synthi-pro/releases/latest) and run it,
or take the portable zip. The files are signed with a self-signed certificate, so Windows SmartScreen shows a warning:
choose "More info" -> "Run anyway".

Скачайте `SYNTHI-PRO-x.y.z-Setup.exe` из раздела [Releases](https://github.com/xray14/synthi-pro/releases/latest)
и запустите, либо возьмите архив без установки. Файлы подписаны самоподписанным сертификатом, поэтому Windows SmartScreen
покажет предупреждение: «Подробнее» -> «Выполнить в любом случае».

## Documentation / Документация

| | |
|---|---|
| User guide (EN) | [docs/USER_GUIDE_EN.md](docs/USER_GUIDE_EN.md) |
| Руководство пользователя (RU) | [docs/USER_GUIDE_RU.md](docs/USER_GUIDE_RU.md) |
| Installation and DAW setup / Установка и подключение к DAW | [docs/INSTALL.md](docs/INSTALL.md) |
| Building from source / Сборка из исходников | [docs/BUILD.md](docs/BUILD.md) |
| Changelog / История изменений | [CHANGELOG.md](CHANGELOG.md) |

## Building / Сборка

Requirements: Visual Studio 2022 with C++ tools, CMake 3.22+, [JUCE 8.0.4](https://github.com/juce-framework/JUCE/releases/tag/8.0.4).
The Steinberg ASIO SDK is optional. Neither SDK is included in this repository.

```bat
git clone --depth 1 --branch 8.0.4 https://github.com/juce-framework/JUCE.git JUCE
build.bat
```

The DSP engine has no dependencies and can be tested on its own:

```bat
tests\run_engine_test.bat
```

## Layout / Структура

| Path | Contents |
|---|---|
| `Source/dsp` | DSP engine, effects and the parameter table (plain C++17, no JUCE) |
| `Source` | plug-in processor, editor, factory presets |
| `tests` | stand-alone engine tests |
| `tools` | preset generator, icon generator |
| `installer` | Inno Setup script |
| `assets` | icons |
| `docs` | user guides, build and installation notes, screenshots |

## Feedback / Обратная связь

Bug reports and ideas are welcome in [Issues](https://github.com/xray14/synthi-pro/issues). Please mention your DAW,
Windows version and MIDI controller. / Сообщения об ошибках и идеи приветствуются в разделе
[Issues](https://github.com/xray14/synthi-pro/issues). Укажите, пожалуйста, DAW, версию Windows и MIDI-контроллер.

## License / Лицензия

Copyright (C) 2026 Khafizov Dinar.

SYNTHI PRO is free software, licensed under the GNU Affero General Public License version 3 — see
[LICENSE.txt](LICENSE.txt) (full text) and [NOTICE.txt](NOTICE.txt) (copyright and third-party notices).
It is distributed WITHOUT ANY WARRANTY.

SYNTHI PRO — свободное программное обеспечение под лицензией GNU AGPL версии 3. Программа поставляется
без каких-либо гарантий.

Built with [JUCE](https://juce.com). VST is a registered trademark of Steinberg Media Technologies GmbH.
ASIO is a trademark and software of Steinberg Media Technologies GmbH. SYNTHI PRO is an independent project,
not affiliated with or endorsed by EMS or Arturia.
