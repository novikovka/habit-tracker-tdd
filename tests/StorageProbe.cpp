#include <QCoreApplication>
#include "HabitStore.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    if (app.arguments().size() != 2 || qEnvironmentVariableIsEmpty("HABIT_TRACKER_TDD_DB_PASSWORD")) return 1;
    auto db = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"));
    db.setHostName(qEnvironmentVariable("HABIT_TRACKER_TDD_DB_HOST"));
    db.setPort(qEnvironmentVariableIntValue("HABIT_TRACKER_TDD_DB_PORT"));
    db.setDatabaseName(QStringLiteral("habit_tracker_tdd_test"));
    db.setUserName(QStringLiteral("habit_tracker_tdd_app"));
    if (!db.open(db.userName(), qEnvironmentVariable("HABIT_TRACKER_TDD_DB_PASSWORD"))) return 2;
    HabitStore store(db);
    QString error;
    if (!store.initialize(error)) return 3;
    const auto name = QStringLiteral("Между процессами");
    const QDate day(2026, 11, 8);
    if (app.arguments()[1] == QStringLiteral("write")) {
        qint64 id;
        if (!store.createHabit(name, id, error) || !store.setCompletion(id, day, true, day, error)) return 4;
        return 0;
    }
    QVector<HabitRecord> habits;
    if (!store.loadSnapshot(habits, error) || habits.size() != 1) return 5;
    return habits[0].name == name && habits[0].completedDays == QSet<QDate>{day} ? 0 : 6;
}
