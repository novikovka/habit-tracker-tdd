# Приёмка версии TDD

Дата автоматического прогона: 09.11.2026. Контрольная D=08.11.2026 задаётся
тестами, часы ОС не меняются. Проверки написаны независимо по требованиям SDD.

| Набор | Поведенческие тесты | Итог | Протокол |
| --- | --- | --- | --- |
| Логика | 22 | Всё прошло | evidence/final-logic.txt |
| PostgreSQL | 14 | Всё прошло | evidence/final-database.txt |
| Qt Widgets | 16 | Всё прошло | evidence/final-ui.txt |
| Qt Widgets 125% | те же 16 | Всё прошло | evidence/final-ui-125.txt |
| Qt Widgets 150% | те же 16 | Всё прошло | evidence/final-ui-150.txt |

Итого 52 уникальных поведенческих теста, без init/cleanup. Каждый из трёх
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
| Сохранение после закрытия | dataSurvivesBetweenSeparateProcesses; newConnectionLoadsIndependentDuplicateNamesAndHistory | Два процесса хранилища; ручной перезапуск GUI с постоянной БД ещё нужен |
| Пустая статистика, ошибка БД, повтор | emptyStatisticsContainZeroDays; lostConnectionDoesNotReportSuccessfulEmptySnapshot; connectionFailureDisablesDataActionsAndCanBeRetried | Основной сервер не останавливался |
| Полночь, часы назад, длинное имя, клавиатура, масштаб | midnightAndClockRollbackRecalculateStreak; activationRefreshesLocalDayWithoutChangingSelectedWeek; longNameRemainsReadableThroughTooltipInNarrowWindow; прогоны 125/150% | Qt offscreen, системные настройки не менялись |

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
