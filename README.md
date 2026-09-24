# Dead by Daylight — Server Fix / Region Selector

Утилита для переключения AWS GameLift региона, к которому подключается Dead by
Daylight: блокирует остальные регионы через hosts-файл и правила Windows
Firewall, оставляя доступным только выбранный. Портирована с C#/WinForms на
C++ (Dear ImGui + DirectX 11 + CMake).

## Возможности

- Список регионов (15 AWS GameLift зон) с автоматическим замером пинга
  (ICMP) и автовыбором региона с наименьшей задержкой (обновление каждые 15 с).
- «Create Rules»: добавляет в hosts-файл блокировку всех регионов, кроме
  выбранного, и создаёт правила брандмауэра `DbdBlockRule<PLATFORM>_<REGION>_IN/_OUT`,
  блокирующие IP-префиксы остальных регионов (список берётся с
  https://ip-ranges.amazonaws.com/ip-ranges.json), затем сбрасывает DNS.
- «Remove Rules»: убирает записи из hosts-файла и удаляет правила
  брандмауэра.
- Поддержка платформ: Steam (`DeadByDaylight-Win64-Shipping.exe`),
  Epic Games Store (`DeadByDaylight-EGS-Shipping.exe`), MS Store
  (`DeadByDaylight-WinGDK-Shipping.exe`).
- Тёмная тема, повторяющая оригинальное WinForms-приложение.

## Требования

- Windows 10/11 x64.
- Visual Studio 2022+ с workload **"Разработка классических приложений на C++"**
  (дает MSVC, Windows SDK, CMake, Ninja).
- Git (для загрузки Dear ImGui через CMake FetchContent).

## Сборка

```bat
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Готовый исполняемый файл: `build\dbd_server_fix.exe`. Список регионов
встраивается в бинарник при компиляции, отдельный файл `regions.json` не
нужен.

Приложение требует прав администратора (запись в hosts-файл и изменение
правил брандмауэра) — манифест `app.manifest` включает `requireAdministrator`.

## Использование

1. Запустите `dbd_server_fix.exe` и подтвердите UAC.
2. Дождитесь результатов пингов (столбец Ping) или выберите регион вручную.
3. Нажмите **Browse** и укажите путь к исполняемому файлу Dead by Daylight.
4. Нажмите **Create Rules** (или **Remove Rules** для отката).

## Структура

```
CMakeLists.txt            сборка (FetchContent: Dear ImGui v1.92.9)
src/main.cpp              точка входа, окно Win32 + цикл DirectX 11 / ImGui
src/ui.h, src/ui.cpp      интерфейс (эквивалент MainForm.cs / Themes.cs)
src/core/
  Region.h, RegionCatalog   список регионов (встраивается в бинарник)
  HostsFileService          правка hosts-файла (+ ipconfig /flushdns)
  FirewallService           правила брандмауэра через COM INetFwPolicy2
  PingService               замер пинга (IcmpSendEcho)
  HttpUtil                  HTTP GET (WinINet), загрузка ip-ranges.json
  Json                      минимальный JSON-парсер
  Util                      вспомогательные функции (path, process, ...)
app.manifest              requireAdministrator + DPI-aware
```