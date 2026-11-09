#include "HabitStore.h"
#include <QSqlQuery>
#include <QSqlError>

bool HabitStore::initialize(QString &error)
{
    ready = false;
    error.clear();
    QSqlQuery identity(db);
    if (!identity.exec(QStringLiteral("SELECT current_database()")) || !identity.next()) {
        error = QStringLiteral("Нет соединения с базой данных.");
        return false;
    }
    const auto name = identity.value(0).toString();
    if (name != QStringLiteral("habit_tracker_tdd") && name != QStringLiteral("habit_tracker_tdd_test")) {
        error = QStringLiteral("Открыта другая база. Требуется отдельная база TDD.");
        return false;
    }
    if (!db.transaction()) {
        error = QStringLiteral("Не удалось начать настройку хранилища.");
        return false;
    }
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS schema_version (singleton BOOLEAN PRIMARY KEY DEFAULT true CHECK(singleton), version INTEGER NOT NULL)")) ||
        !query.exec(QStringLiteral("INSERT INTO schema_version(singleton, version) VALUES(true, 1) ON CONFLICT DO NOTHING"))) {
        db.rollback();
        error = QStringLiteral("Не удалось подготовить хранилище.");
        return false;
    }
    if (!query.exec(QStringLiteral("SELECT version FROM schema_version")) || !query.next() || query.value(0).toInt() != 1) {
        db.rollback();
        error = QStringLiteral("Неизвестная версия схемы базы данных.");
        return false;
    }
    if (!query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS habits (id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY, name TEXT NOT NULL CHECK(name !~ '^[[:space:]]*$' AND char_length(name) <= 100), created_at TIMESTAMPTZ NOT NULL DEFAULT clock_timestamp())")) ||
        !query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS habit_completions (habit_id BIGINT NOT NULL REFERENCES habits(id) ON DELETE CASCADE, completed_on DATE NOT NULL, PRIMARY KEY(habit_id, completed_on))")) || !db.commit()) {
        db.rollback();
        error = QStringLiteral("Не удалось сохранить настройки хранилища.");
        return false;
    }
    ready = true;
    return true;
}

bool HabitStore::ensureReady(QString &error) const
{
    if (ready && db.isOpen())
        return true;
    error = QStringLiteral("Хранилище недоступно. Повторите подключение.");
    return false;
}

bool HabitStore::createHabit(const QString &name, qint64 &id, QString &error)
{
    id = 0;
    error.clear();
    if (!ensureReady(error))
        return false;
    const auto validated = validateHabitName(name);
    if (!validated.accepted) {
        error = validated.error;
        return false;
    }
    QSqlQuery query(db);
    query.prepare(QStringLiteral("INSERT INTO habits(name) VALUES(:name) RETURNING id"));
    query.bindValue(QStringLiteral(":name"), validated.name);
    if (!query.exec() || !query.next()) {
        error = QStringLiteral("Не удалось сохранить привычку. Повторите попытку.");
        return false;
    }
    id = query.value(0).toLongLong();
    return true;
}

bool HabitStore::setCompletion(qint64 habitId, const QDate &day, bool completed, const QDate &today, QString &error)
{
    error.clear();
    if (!ensureReady(error))
        return false;
    if (!canChangeCompletion(day, today)) {
        error = QStringLiteral("Эту дату нельзя изменить.");
        return false;
    }
    QSqlQuery query(db);
    query.prepare(completed
        ? QStringLiteral("INSERT INTO habit_completions(habit_id, completed_on) VALUES(:id, :day) ON CONFLICT DO NOTHING")
        : QStringLiteral("DELETE FROM habit_completions WHERE habit_id=:id AND completed_on=:day"));
    query.bindValue(QStringLiteral(":id"), habitId);
    query.bindValue(QStringLiteral(":day"), day);
    if (!query.exec()) {
        error = QStringLiteral("Не удалось сохранить отметку. Повторите попытку.");
        return false;
    }
    return true;
}

bool HabitStore::renameHabit(qint64 id, const QString &name, QString &error)
{
    error.clear();
    if (!ensureReady(error))
        return false;
    const auto validated = validateHabitName(name);
    if (!validated.accepted) {
        error = validated.error;
        return false;
    }
    QSqlQuery query(db);
    query.prepare(QStringLiteral("UPDATE habits SET name=:name WHERE id=:id"));
    query.bindValue(QStringLiteral(":name"), validated.name);
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec() || query.numRowsAffected() != 1) {
        error = QStringLiteral("Не удалось переименовать привычку. Повторите попытку.");
        return false;
    }
    return true;
}

bool HabitStore::deleteHabit(qint64 id, QString &error)
{
    error.clear();
    if (!ensureReady(error))
        return false;
    QSqlQuery query(db);
    query.prepare(QStringLiteral("DELETE FROM habits WHERE id=:id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec() || query.numRowsAffected() != 1) {
        error = QStringLiteral("Не удалось удалить привычку. Повторите попытку.");
        return false;
    }
    return true;
}

bool HabitStore::loadSnapshot(QVector<HabitRecord> &habits, QString &error)
{
    error.clear();
    if (!ensureReady(error))
        return false;
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("SELECT h.id, h.name, c.completed_on FROM habits h LEFT JOIN habit_completions c ON c.habit_id=h.id ORDER BY h.created_at, h.id, c.completed_on"))) {
        error = QStringLiteral("Не удалось загрузить данные. Повторите попытку.");
        return false;
    }
    QVector<HabitRecord> result;
    while (query.next()) {
        const auto id = query.value(0).toLongLong();
        if (result.isEmpty() || result.last().id != id)
            result.append({id, query.value(1).toString(), {}});
        if (!query.value(2).isNull())
            result.last().completedDays.insert(query.value(2).toDate());
    }
    if (query.lastError().isValid()) {
        error = QStringLiteral("Не удалось полностью загрузить данные.");
        return false;
    }
    habits = std::move(result);
    return true;
}
