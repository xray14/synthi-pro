# Установка и подключение к DAW / Installation and DAW setup

## Русский

### Установка

1. Запустите `SYNTHI-PRO-1.0.0-Setup.exe`.
2. Выберите компоненты: автономная программа, плагин VST3, документация.
3. Плагин ставится в стандартную папку `C:\Program Files\Common Files\VST3\SYNTHI PRO.vst3`,
   программа — в `C:\Program Files\SYNTHI PRO`.

Без установщика: скопируйте папку `SYNTHI PRO.vst3` целиком в `C:\Program Files\Common Files\VST3\`,
а `SYNTHI PRO.exe` — в любое место.

### Автономная версия

Запустите `SYNTHI PRO.exe`. Кнопка **Options** в заголовке окна открывает настройки звука и MIDI:
устройство вывода, частота дискретизации, размер буфера, активные MIDI-входы. Для игры без задержки выбирайте
буфер 128–256 сэмплов.

### DAW

| DAW | Как подключить |
|---|---|
| Ableton Live | Preferences → Plug-Ins → включите «Use VST3 Plug-In System Folders» → Rescan. Плагин появится в Plug-Ins → VST3; перетащите его на MIDI-дорожку. |
| FL Studio | Options → Manage plugins → Find installed plugins. Затем Add → More plugins → SYNTHI PRO. |
| Cubase / Nuendo | Папка VST3 сканируется автоматически. Создайте Instrument Track и выберите SYNTHI PRO. |
| Reaper | Options → Preferences → Plug-ins → VST → Re-scan. Затем Insert → Virtual instrument on new track. |
| Studio One | Папка VST3 сканируется при запуске. Перетащите SYNTHI PRO из браузера Instruments на дорожку. |

Если плагин не появился, проверьте, что DAW 64-битная и что сканируется папка
`C:\Program Files\Common Files\VST3`.

### Удаление

«Параметры Windows → Приложения → SYNTHI PRO → Удалить». Пользовательские пресеты в
`Документы\SYNTHI PRO` не удаляются.

## English

### Installation

1. Run `SYNTHI-PRO-1.0.0-Setup.exe`.
2. Choose components: standalone application, VST3 plug-in, documentation.
3. The plug-in goes to the standard folder `C:\Program Files\Common Files\VST3\SYNTHI PRO.vst3`,
   the application to `C:\Program Files\SYNTHI PRO`.

Without the installer: copy the whole `SYNTHI PRO.vst3` folder into `C:\Program Files\Common Files\VST3\`
and put `SYNTHI PRO.exe` anywhere.

### Standalone

Run `SYNTHI PRO.exe`. The **Options** button in the window title bar opens audio and MIDI settings: output
device, sample rate, buffer size, active MIDI inputs. For low-latency playing use a 128–256 sample buffer.

### DAWs

| DAW | How to load |
|---|---|
| Ableton Live | Preferences → Plug-Ins → enable "Use VST3 Plug-In System Folders" → Rescan. Find it under Plug-Ins → VST3 and drag it onto a MIDI track. |
| FL Studio | Options → Manage plugins → Find installed plugins. Then Add → More plugins → SYNTHI PRO. |
| Cubase / Nuendo | The VST3 folder is scanned automatically. Create an Instrument Track and choose SYNTHI PRO. |
| Reaper | Options → Preferences → Plug-ins → VST → Re-scan. Then Insert → Virtual instrument on new track. |
| Studio One | The VST3 folder is scanned at startup. Drag SYNTHI PRO from the Instruments browser onto a track. |

If the plug-in does not show up, check that the DAW is 64-bit and that it scans
`C:\Program Files\Common Files\VST3`.

### Uninstall

Windows Settings → Apps → SYNTHI PRO → Uninstall. User presets in `Documents\SYNTHI PRO` are kept.
