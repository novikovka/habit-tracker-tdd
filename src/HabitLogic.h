#pragma once
#include <QString>
#include <QDate>
#include <QSet>
#include <QVector>
#include <QStringList>

struct NameValidation {
    bool accepted;
    QString name;
    QString error;
};

NameValidation validateHabitName(const QString &input);

struct WeekRange {
    QDate monday;
    QDate sunday;
};

WeekRange weekContaining(const QDate &today);
WeekRange shiftedWeek(const WeekRange &week, int direction);
int currentStreak(const QSet<QDate> &completedDays, const QDate &today);
bool canChangeCompletion(const QDate &day, const QDate &today);

struct HabitRecord {
    qint64 id;
    QString name;
    QSet<QDate> completedDays;
};

struct DailyCount {
    QDate day;
    int count;
};

QVector<DailyCount> completionChart(const QVector<HabitRecord> &habits, const QDate &today, int days);

struct HabitSummary {
    int streak;
    int last7Days;
    int last30Days;
};

HabitSummary summarizeHabit(const HabitRecord &habit, const QDate &today);
QStringList builtInQuotes();
