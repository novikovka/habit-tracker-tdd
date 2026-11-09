#include <QtTest>
#include "HabitLogic.h"

class LogicTests : public QObject {
    Q_OBJECT
private slots:
    void whitespaceNameIsRejected()
    {
        const auto result = validateHabitName(QStringLiteral("   "));
        QVERIFY(!result.accepted);
        QVERIFY(!result.error.isEmpty());
    }
    void nameIsTrimmed()
    {
        const auto result = validateHabitName(QStringLiteral("  Чтение  "));
        QVERIFY(result.accepted);
        QCOMPARE(result.name, QStringLiteral("Чтение"));
    }
    void initialWeekContainsTodayFromMondayToSunday()
    {
        const auto week = weekContaining(QDate(2026, 11, 8));
        QCOMPARE(week.monday, QDate(2026, 11, 2));
        QCOMPARE(week.sunday, QDate(2026, 11, 8));
    }
    void nameLongerThan100CharactersIsRejected()
    {
        const auto result = validateHabitName(QString(101, QChar(u'я')));
        QVERIFY(!result.accepted);
        QVERIFY(!result.error.isEmpty());
    }
    void multilineNameIsRejected()
    {
        QVERIFY(!validateHabitName(QStringLiteral("Чтение\nвечером")).accepted);
    }
    void exactly100CharactersAreAcceptedAfterTrimming()
    {
        const QString name(100, QChar(u'я'));
        const auto result = validateHabitName(QStringLiteral("  ") + name + QStringLiteral("  "));
        QVERIFY(result.accepted);
        QCOMPARE(result.name, name);
    }
    void weekCanCrossYearBoundary()
    {
        const auto week = weekContaining(QDate(2027, 1, 1));
        QCOMPARE(week.monday, QDate(2026, 12, 28));
        QCOMPARE(week.sunday, QDate(2027, 1, 3));
    }
    void leapDayBelongsToCalendarWeek()
    {
        const auto week = weekContaining(QDate(2024, 2, 29));
        QCOMPARE(week.monday, QDate(2024, 2, 26));
        QCOMPARE(week.sunday, QDate(2024, 3, 3));
    }
    void streakCountsConsecutiveDaysEndingToday()
    {
        const QDate today(2026, 11, 8);
        QCOMPARE(currentStreak({today.addDays(-2), today.addDays(-1), today}, today), 3);
    }
    void streakUsesYesterdayWhenTodayIsNotMarked()
    {
        const QDate today(2026, 11, 8);
        QCOMPARE(currentStreak({today.addDays(-2), today.addDays(-1)}, today), 2);
    }
    void gapBreaksStreakAndFutureDaysDoNotCount()
    {
        const QDate today(2026, 11, 8);
        QCOMPARE(currentStreak({today.addDays(-2), today, today.addDays(1)}, today), 1);
        QCOMPARE(currentStreak({today.addDays(-2)}, today), 0);
        QCOMPARE(currentStreak({}, today), 0);
    }
    void streakCrossesLeapDayAndIsNotLimitedTo30Days()
    {
        const QDate today(2024, 3, 2);
        QSet<QDate> marks;
        for (int offset = 0; offset < 45; ++offset)
            marks.insert(today.addDays(-offset));
        QCOMPARE(currentStreak(marks, today), 45);
    }
    void futureCompletionIsForbidden()
    {
        const QDate today(2026, 11, 8);
        QVERIFY(!canChangeCompletion(today.addDays(1), today));
    }
    void sevenDayChartIncludesZerosAndBothBounds()
    {
        const QDate today(2026, 11, 8);
        const QVector<HabitRecord> habits = {
            {1, QStringLiteral("Чтение"), {today, today.addDays(-1), today.addDays(-6), today.addDays(-7)}},
            {2, QStringLiteral("Прогулка"), {today, today.addDays(-2), today.addDays(1)}}
        };
        const auto chart = completionChart(habits, today, 7);
        QCOMPARE(chart.size(), 7);
        QCOMPARE(chart.first().day, today.addDays(-6));
        QCOMPARE(chart.last().day, today);
        const QVector<int> expected{1, 0, 0, 0, 1, 1, 2};
        for (int i = 0; i < expected.size(); ++i)
            QCOMPARE(chart[i].count, expected[i]);
    }
    void habitSummaryUsesInclusiveSevenAndThirtyDayPeriods()
    {
        const QDate today(2026, 11, 8);
        const HabitRecord reading{1, QStringLiteral("Чтение"),
            {today, today.addDays(-1), today.addDays(-6), today.addDays(-7),
             today.addDays(-29), today.addDays(-30), today.addDays(1)}};
        const auto summary = summarizeHabit(reading, today);
        QCOMPARE(summary.last7Days, 3);
        QCOMPARE(summary.last30Days, 5);
        QCOMPARE(summary.streak, 2);
    }
    void weekNavigationMovesExactlySevenCalendarDays()
    {
        const auto initial = weekContaining(QDate(2027, 1, 1));
        const auto next = shiftedWeek(initial, 1);
        QCOMPARE(next.monday, QDate(2027, 1, 4));
        QCOMPARE(next.sunday, QDate(2027, 1, 10));
        const auto previous = shiftedWeek(next, -1);
        QCOMPARE(previous.monday, initial.monday);
        QCOMPARE(previous.sunday, initial.sunday);
    }
    void weekNavigationStopsAtSupportedYearBounds()
    {
        const auto first = weekContaining(QDate(1, 1, 1));
        QCOMPARE(shiftedWeek(first, -1).monday, first.monday);
        const auto lastFullWeek = weekContaining(QDate(9999, 12, 20));
        QCOMPARE(shiftedWeek(lastFullWeek, 1).monday, lastFullWeek.monday);
    }
    void thirtyDayChartMatchesAcceptanceTotals()
    {
        const QDate today(2026, 11, 8);
        const QVector<HabitRecord> habits{
            {1, QStringLiteral("Чтение"), {today, today.addDays(-1), today.addDays(-6),
                today.addDays(-7), today.addDays(-29), today.addDays(-30)}},
            {2, QStringLiteral("Прогулка"), {today, today.addDays(-2)}}
        };
        for (const auto period : {7, 30}) {
            const auto chart = completionChart(habits, today, period);
            QCOMPARE(chart.size(), period);
            QCOMPARE(chart.first().day, today.addDays(1 - period));
            QCOMPARE(chart.last().count, 2);
            int total = 0;
            for (const auto &bar : chart)
                total += bar.count;
            QCOMPARE(total, period == 7 ? 5 : 7);
        }
    }
    void emptyStatisticsContainZeroDays()
    {
        const QDate today(2026, 11, 8);
        for (const auto period : {7, 30}) {
            const auto chart = completionChart({}, today, period);
            QCOMPARE(chart.size(), period);
            for (const auto &bar : chart)
                QCOMPARE(bar.count, 0);
        }
        const auto summary = summarizeHabit({1, QStringLiteral("Новая"), {}}, today);
        QCOMPARE(summary.streak, 0);
        QCOMPARE(summary.last7Days, 0);
        QCOMPARE(summary.last30Days, 0);
    }
    void midnightAndClockRollbackRecalculateStreak()
    {
        const QDate markedDay(2026, 12, 31);
        const QSet<QDate> marks{markedDay.addDays(-1), markedDay};
        QCOMPARE(currentStreak(marks, markedDay), 2);
        QCOMPARE(currentStreak(marks, markedDay.addDays(1)), 2);
        QCOMPARE(currentStreak(marks, markedDay.addDays(2)), 0);
        QCOMPARE(currentStreak(marks, markedDay.addDays(-1)), 1);
    }
    void todayAndPastAreEditableButInvalidDatesAreNot()
    {
        const QDate today(2026, 11, 8);
        QVERIFY(canChangeCompletion(today, today));
        QVERIFY(canChangeCompletion(QDate(2000, 1, 1), today));
        QVERIFY(!canChangeCompletion(QDate(), today));
        QVERIFY(!canChangeCompletion(QDate(-1, 1, 1), today));
        QVERIFY(!canChangeCompletion(QDate(10000, 1, 1), QDate(10000, 1, 2)));
    }
    void atLeastFiveNonemptyQuotesAreBuiltIn()
    {
        const auto quotes = builtInQuotes();
        QVERIFY(quotes.size() >= 5);
        for (const auto &quote : quotes)
            QVERIFY(!quote.trimmed().isEmpty());
    }
};

QTEST_APPLESS_MAIN(LogicTests)
#include "LogicTests.moc"
