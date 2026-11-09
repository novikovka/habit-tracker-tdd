#pragma once
#include <QSqlDatabase>
#include "HabitLogic.h"

class HabitStore {
public:
    explicit HabitStore(QSqlDatabase connection) : db(connection) {}
    bool initialize(QString &error);
    bool createHabit(const QString &name, qint64 &id, QString &error);
    bool setCompletion(qint64 habitId, const QDate &day, bool completed, const QDate &today, QString &error);
    bool renameHabit(qint64 id, const QString &name, QString &error);
    bool deleteHabit(qint64 id, QString &error);
    bool loadSnapshot(QVector<HabitRecord> &habits, QString &error);
private:
    bool ensureReady(QString &error) const;
    bool ready = false;
    QSqlDatabase db;
};
