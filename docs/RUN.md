# Сборка, запуск и проверки

Окружение: Windows 10, Qt 6.9.1 в D:/Qt/6.9.1/mingw_64,
MinGW 13.1.0, CMake 3.30.5, Ninja 1.12.1. PostgreSQL 18.6 установлен
в C:/Program Files/PostgreSQL/18. Системный PATH не меняется.

## Постоянные данные

Рабочая база — `habit_tracker_tdd`, роль — `habit_tracker_tdd_app`.
Для подготовки используйте [инструкцию](DATABASE_SETUP.md). Настройка
основного сервера агентом не выполнена: административный пароль недоступен.

По умолчанию используются localhost:5432 и эти имена базы/роли.
Несекретные параметры можно задать во внешнем
`%LOCALAPPDATA%\HabitTrackerTDD\connection.ini`:

```ini
host=localhost
port=5432
database=habit_tracker_tdd
user=habit_tracker_tdd_app
```

Образец — config/connection.example.ini. Поля password нет.
Приложение проверяет фактическое current_database() и версию схемы до
подготовки таблиц; допускает только habit_tracker_tdd / habit_tracker_tdd_test.
Неизвестная версия вызывает отказ без перезаписи данных.

Для ручной приёмки установите во внешнем INI `database=habit_tracker_tdd_test`.
run.ps1 принимает пароль той же роли скрытым вводом и для этой базы.
После приёмки верните `database=habit_tracker_tdd` для обычного использования.

## Команды из корня проекта

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\run.ps1 -SkipBuild
```

Можно выполнить только run.ps1 без SkipBuild — он сначала соберёт проект.
run.ps1 запросит пароль скрытым вводом и передаст его окружению только
дочернего приложения (HABIT_TRACKER_DB_PASSWORD). Закройте окно для
завершения скрипта. Пароль не сохраняется в файлы и не передаётся аргументом.

В сборке временно используется свободный H: для обхода ошибки Qt moc на
пути с кириллицей. При занятом диске у build.ps1/test.ps1 есть параметр
`-BuildDrive J:`. При смене буквы нужен новый/очищенный build/ascii:
CMake хранит абсолютные пути. Созданное отображение освобождается.
Приложение требует DLL найденных Qt/PostgreSQL; отдельный установщик для
чистой машины не подготовлен.

## Тесты

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\check-all.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\test.ps1 -Suite Logic
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\test.ps1 -Suite Database
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\test.ps1 -Suite Ui
```

Database и Ui создают временный кластер в build/pg-tests с базой
habit_tracker_tdd_test, не используют постоянный сервер и его пароль.
При начальной настройке доступ ограничен loopback, затем до тестов включается
SCRAM. Случайный пароль находится только в памяти, тестовая роль не имеет
SUPERUSER/CREATEDB/CREATEROLE. После тестов сервер останавливается,
данные удаляются с проверкой пути. Ранние неудавшиеся настройки могут
оставить остановленные диагностические каталоги в build/pg-tests.
В песочнице запуск pg_ctl может быть запрещён; используйте обычный локальный
PowerShell либо разрешённый запуск вне песочницы.

Ui использует Qt offscreen и шрифты Windows. Параметры: `-ScaleFactor 1.25`
или `1.5`, `-Capture` для снимков. Это автоматическая проверка Qt-масштаба;
системный масштаб Windows не меняется, ручной осмотр не подменяется.

Для протокола добавьте `-EvidenceName новое-имя`. RED запускается с
`-ExpectFailure` и засчитывается только при FAIL! реально запущенного теста,
а не ошибке сборки/настройки. Не перезаписывайте исторические имена циклов.
Без этого параметра вывод — build/ascii/latest-tests.txt. JSON фиксирует
UTC-время тестового процесса; время разработки отражено в WORK_LOG.md.
