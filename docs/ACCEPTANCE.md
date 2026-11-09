# Приёмка версии TDD

Дата автоматического прогона: 09.11.2026. Контрольная D=08.11.2026 задаётся
тестами, часы ОС не меняются. Проверки написаны независимо по требованиям SDD.

| Набор | Поведенческие тесты | Итог | Протокол |
| --- | --- | --- | --- |
| Логика | 22 | Всё прошло | evidence/final-logic.txt |
| PostgreSQL | 14 | Всё прошло | evidence/final-database.txt |
| Qt Widgets | 24 | Всё прошло | evidence/final-ui.txt |
| Qt Widgets 125% | те же 24 | Всё прошло | evidence/final-ui-125.txt |
| Qt Widgets 150% | те же 24 | Всё прошло | evidence/final-ui-150.txt |

Итого 60 уникальных поведенческих тестов, без init/cleanup. Каждый из трёх
наборов CTest прошёл отдельно. Масштабные прогоны повторяют те же проверки.

## Эквиваленты сценариев приёмки SDD

| Сценарий | Подтверждение | Граница |
| --- | --- | --- |
| Текущая неделя, пустой список, цитата | startupOpensWeekContainingLocalToday; quoteIsBuiltInAndStableDuringNavigation; empty.png | Подставная дата, временная БД |
| Названия: пробелы, кириллица, лимит, одинаковые | Валидация; habitCreationPersistsTrimmedCyrillicName; newConnectionLoadsIndependentDuplicateNamesAndHistory; addDialogKeepsInvalidInputAndSavesValidName | Автоматический ввод |
| Переименование и отмена | renamePersistsWithoutChangingHistory; renameDialogSavesOnEnterAndCancelsOnEscape | Проверены Enter/Escape |
| Навигация, границы дат, будущие клетки | weekNavigationMovesExactlySevenCalendarDays; weekNavigationStopsAtSupportedYearBounds; nextWeekNavigationKeepsMarksOnTheirDates | Календарные QDate, годы 1–9999 |
| Поставить/снять/повторить без дублей | repeatedCompletionCreatesOnlyOneSavedRecord; removingCompletionTwicePreservesOtherDates; spaceTogglesCompletionAndPersistsIt; clickingCellCenterTogglesSavedCompletion; enterTogglesFocusedCompletionCell | Реальный временный PostgreSQL |
| Серии сегодня/вчера/пропуск | Проверки currentStreak и activationRefreshesLocalDayWithoutChangingSelectedWeek | Включены 45 дней, 29 февраля и переход года |
| Чтение 3/5, Прогулка 2/2, общие 5/7 | savedAcceptanceDataProducesExpectedStatistics; statisticsShowsSavedTotalsAndReturnsToSelectedWeek | Согласованный снимок из одного SQL-запроса |
| Отмена удаления и каскад истории | deletionDefaultsToCancelAndRequiresConfirmation; deletionRemovesHabitAndItsHistoryOnly | Вторая привычка остаётся |
| Сохранение после закрытия | dataSurvivesBetweenSeparateProcesses; newConnectionLoadsIndependentDuplicateNamesAndHistory; configuredExecutableConnectsOnBothLaunches | Два запуска настоящего exe с временным профилем; обычное закрытие GUI с постоянной БД ещё требует ручной проверки |
| Пустая статистика, ошибка БД, повтор | emptyStatisticsContainZeroDays; lostConnectionDoesNotReportSuccessfulEmptySnapshot; connectionFailureDisablesDataActionsAndCanBeRetried | Основной сервер не останавливался |
| Полночь, часы назад, длинное имя, клавиатура, масштаб | midnightAndClockRollbackRecalculateStreak; activationRefreshesLocalDayWithoutChangingSelectedWeek; longNameRemainsReadableThroughTooltipInNarrowWindow; shortWindowKeepsWeeklyControlsInsideAvailableHeight; tabNavigationReachesWeeklyActions; прогоны 125/150% | Qt offscreen, системные настройки не менялись |

## Визуальная проверка и оставшиеся действия

Просмотрены снимки Qt Widgets: главное окно, пустое состояние, полная
статистика, невалидное имя, удаление и ошибка подключения. Файлы —
docs/screenshots. По снимку найдено дублирование старых кнопок после
обновления: цикл 36 воспроизвёл дефект, исправление проверено тестом и снимком.

Макеты SDD не открывались по ограничению пользователя. Палитра, состав
экранов и поведение сверены по разрешённой спецификации; пиксельное совпадение
не утверждается. Исходники и тесты SDD не использовались.

Ещё нужны пользовательские проверки:

- Постоянные базы и вход роли на localhost:5432 с локально введённым паролем.
- Запуск run.ps1, создание данных и повторный запуск GUI с этой БД.
- Ручной осмотр Windows 10, системного масштаба и полного Tab-порядка.
- Визуальное сравнение с первой версией при её отдельном просмотре пользователем.

Эти пункты не объявляются пройденными. Приложение и постоянное хранение
реализованы; учётные данные основного сервера агенту недоступны.

## Продолжение в Git

Циклы 38 и 39 исправили ограничение высоты окна (теперь проверено 800×500)
и недостижимость части действий через Tab. Для каждого сохранены отдельные
RED- и GREEN-коммиты и исходные протоколы. Стрелки выбирают клетку,
Space/Enter меняют отметку; Tab переходит между элементами управления.

Новый тест настоящего exe использует временный LOCALAPPDATA и несекретный
INI с параметрами отдельной БД. Для каждого из двух запусков подтверждено
реальное подключение через pg_stat_activity; данные проверяются после
повторного запуска. Данные в этом сценарии подготавливает тест через
хранилище, а не ввод мышью в дочернем окне. Offscreen-процессы завершаются
принудительно; ручное штатное закрытие видимого GUI этим не заменяется.
Автоматическая Tab-проверка охватывает основные действия с одной привычкой;
полный пользовательский осмотр клавиатурного взаимодействия остаётся полезен.

## Визуальная доработка после согласования

Новых пользовательских функций не добавлено. Проверено соответствие
пяти согласованным пунктам:

| Изменение | Проверка |
| --- | --- |
| Единая заметная отметка по центру | completionStatesDifferInsideOneCenteredIndicator; существующие мышь/Space/Enter; снимки week |
| Пустая ошибка не видна и не занимает место | addDialogDoesNotShowAnEmptyErrorPlate; прежний тест ошибки; add-normal и invalid-name |
| Светлые поверхности и небольшие розовые акценты | Фактический просмотр week, statistics, диалогов при 100/125/150% |
| Итоги и заголовок не уезжают | statisticsKeepsHeadingAndTotalsVisibleWhileChartsScroll; компактные снимки сверху/снизу |
| Графики адаптируются и видны целиком | chartsAdaptToWideAndNarrowStatisticsWithoutHorizontalClipping; compactStatisticsShowsAWholeChartCard |

В широком окне графики стоят рядом, в узком — один над другим; высота
карточки соответствует viewport. Таблица итогов находится вне прокрутки
графиков и имеет свою прокрутку с заголовками. Подписи дат горизонтальны,
шаг подписей зависит от ширины; все 7/30 столбцов и исходные значения сохранены.
Расчёты диапазонов и серий, схема/SQL и настройки PostgreSQL не изменены:
это дополнительно проверено сравнением защищённых файлов с b5648f4.

Реальный Qt-интерфейс и habit_tracker.exe запускались в тестовом
offscreen-режиме с отдельной БД; изображения не являются макетами.
Просмотрены week, statistics, add-normal, invalid-name, компактная
статистика сверху/снизу и соответствующие снимки из dpi125/dpi150.
Пользовательский осмотр обновлённого нативного окна остаётся отдельным этапом.
