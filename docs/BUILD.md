# Сборка SYNTHI PRO / Building SYNTHI PRO

## Что нужно / Requirements

| Компонент | Версия | Примечание |
|---|---|---|
| Windows 10/11 x64 | | |
| Visual Studio 2022 (или Build Tools) | рабочая нагрузка «Разработка классических приложений на C++» | MSVC 14.3x, Windows SDK |
| CMake | 3.22 или новее | |
| JUCE | 8.x (подойдёт и 7.0.6+) | исходники с github.com/juce-framework/JUCE; VST3 SDK уже внутри JUCE |
| Inno Setup 6 | необязательно | только для установщика |

## Шаги / Steps

1. Положите исходники JUCE в папку `SynthiPro\JUCE` (так, чтобы существовал файл `SynthiPro\JUCE\CMakeLists.txt`),
   либо держите их где угодно и передайте путь аргументом.
2. Запустите:

   ```bat
   build.bat
   ```

   или, если JUCE лежит в другом месте:

   ```bat
   build.bat C:\SDKs\JUCE
   ```

   Скрипт настраивает проект, собирает и запускает тесты движка, затем собирает Standalone и VST3,
   а при установленном Inno Setup — ещё и установщик.

3. Результат:

   | Файл | Путь |
   |---|---|
   | Автономная программа | `build\SynthiPro_artefacts\Release\Standalone\SYNTHI PRO.exe` |
   | Плагин | `build\SynthiPro_artefacts\Release\VST3\SYNTHI PRO.vst3` |
   | Установщик | `dist\SYNTHI-PRO-1.0.1-Setup.exe` |

Вручную то же самое:

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DJUCE_DIR=C:\SDKs\JUCE
cmake --build build --config Release --target SynthiPro_Standalone SynthiPro_VST3
"%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" installer\SynthiPro.iss
```

## Тесты движка без JUCE / Engine tests without JUCE

```bat
tests\run_engine_test.bat
```

Проверяется строй, алиасинг, режимы фильтра и самовозбуждение, синхронизация, FM, действие матрицы, огибающие,
полифония, глайд, секвенсор, все эффекты, отсутствие щелчков при скачках параметров, три частоты дискретизации,
нагрузка на процессор и каждый из заводских пресетов.

## Пресеты / Presets

Заводские пресеты описаны в `tools\gen_presets.py` и превращаются в `Source\FactoryPresets.h`:

```bat
python tools\gen_presets.py
tests\run_engine_test.bat
python tools\gen_presets.py
```

Первый запуск генерирует пресеты, тест измеряет их пиковый уровень и пишет `build-test\preset_levels.txt`,
второй запуск генератора выравнивает громкость. После правки пресетов повторите цикл два раза.

## Структура / Layout

| Путь | Содержимое |
|---|---|
| `Source\dsp\Params.h` | единая таблица параметров (идентификаторы, диапазоны, названия EN/RU) |
| `Source\dsp\Engine.h` | голоса, генераторы, фильтр, огибающие, LFO, матрица, секвенсор — без JUCE |
| `Source\dsp\Effects.h` | эффекты — без JUCE |
| `Source\PluginProcessor.*` | плагин: параметры хоста, MIDI, состояние, менеджер пресетов |
| `Source\PluginEditor.*` | интерфейс, локализация |
| `Source\FactoryPresets.h` | сгенерированная библиотека пресетов |
| `tests\`, `tools\`, `installer\`, `docs\` | тесты, генератор пресетов, скрипт установщика, документация |

## Замечания / Notes

- **ASIO.** Включается автоматически, если Steinberg ASIO SDK лежит в `SynthiPro\ASIOSDK` (или передан через
  `-DASIO_SDK_DIR=<путь>`); CMake пишет «ASIO enabled». Без SDK остаются WASAPI и DirectSound. Использование ASIO
  подчиняется лицензии Steinberg (см. `ASIOSDK\LICENSE.txt` и «Steinberg ASIO Licensing Agreement.pdf»).
- **Подпись.** `sign.bat <отпечаток SHA-1 сертификата>` подписывает EXE и VST3, пересобирает установщик из
  подписанных файлов и подписывает его (SHA-256, метка времени DigiCert). Сертификат должен лежать в личном
  хранилище Windows. Самоподписанный сертификат даёт подпись, которой Windows не доверяет; для доверенной
  подписи нужен сертификат подписи кода от удостоверяющего центра — скрипт тот же.
- **Лицензия JUCE.** JUCE распространяется по AGPLv3 либо по коммерческой лицензии. Для продажи закрытого продукта
  нужна коммерческая лицензия JUCE; формат VST3 также требует соблюдения условий Steinberg.
- **Статический рантайм.** Проект собирается с `/MT`, распространяемый пакет Visual C++ пользователю не нужен.
