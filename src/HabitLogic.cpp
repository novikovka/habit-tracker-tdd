#include "HabitLogic.h"

NameValidation validateHabitName(const QString &input)
{
    const auto name = input.trimmed();
    if (input.contains(u'\n') || input.contains(u'\r'))
        return {false, {}, QStringLiteral("Название должно быть однострочным.")};
    if (name.isEmpty())
        return {false, {}, QStringLiteral("Введите название привычки.")};
    if (name.size() > 100)
        return {false, {}, QStringLiteral("Название должно содержать не более 100 символов.")};
    return {true, name, {}};
}

WeekRange weekContaining(const QDate &today)
{
    const auto monday = today.addDays(1 - today.dayOfWeek());
    return {monday, monday.addDays(6)};
}

WeekRange shiftedWeek(const WeekRange &week, int direction)
{
    const WeekRange shifted{week.monday.addDays(qint64(direction) * 7), week.sunday.addDays(qint64(direction) * 7)};
    if (!shifted.monday.isValid() || !shifted.sunday.isValid() ||
        shifted.monday.year() < 1 || shifted.sunday.year() > 9999)
        return week;
    return shifted;
}

int currentStreak(const QSet<QDate> &completedDays, const QDate &today)
{
    int count = 0;
    const auto start = completedDays.contains(today) ? today : today.addDays(-1);
    for (auto day = start; day.isValid() && completedDays.contains(day); day = day.addDays(-1))
        ++count;
    return count;
}

bool canChangeCompletion(const QDate &day, const QDate &today)
{
    return day.isValid() && today.isValid() && day.year() >= 1 && day.year() <= 9999 && day <= today;
}

QVector<DailyCount> completionChart(const QVector<HabitRecord> &habits, const QDate &today, int days)
{
    QVector<DailyCount> result;
    for (int offset = days - 1; offset >= 0; --offset) {
        const auto day = today.addDays(-offset);
        int count = 0;
        for (const auto &habit : habits)
            count += habit.completedDays.contains(day) ? 1 : 0;
        result.append({day, count});
    }
    return result;
}

HabitSummary summarizeHabit(const HabitRecord &habit, const QDate &today)
{
    HabitSummary result{currentStreak(habit.completedDays, today), 0, 0};
    for (const auto &day : habit.completedDays) {
        if (day >= today.addDays(-6) && day <= today)
            ++result.last7Days;
        if (day >= today.addDays(-29) && day <= today)
            ++result.last30Days;
    }
    return result;
}

QStringList builtInQuotes()
{
    return {
        QStringLiteral("Маленький шаг сегодня — большая перемена со временем."),
        QStringLiteral("Начни с малого. Продолжай в своём ритме."),
        QStringLiteral("Каждый день — новая возможность позаботиться о себе."),
        QStringLiteral("Постоянство растёт из простых ежедневных действий."),
        QStringLiteral("Замечай свой прогресс, даже если он кажется небольшим.")
    };
}
