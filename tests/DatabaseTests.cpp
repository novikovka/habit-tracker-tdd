#include <QtTest>
#include <QSqlQuery>
#include <QProcess>
#include "HabitStore.h"

class DatabaseTests : public QObject {
    Q_OBJECT
    QSqlDatabase db;
private slots:
    void initTestCase()
    {
        QVERIFY2(!qEnvironmentVariableIsEmpty("HABIT_TRACKER_TDD_DB_PASSWORD"),
                 "Run via scripts/test.ps1 -Suite Database; isolated database credentials are required.");
        db = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"));
        db.setHostName(qEnvironmentVariable("HABIT_TRACKER_TDD_DB_HOST"));
        db.setPort(qEnvironmentVariableIntValue("HABIT_TRACKER_TDD_DB_PORT"));
        db.setDatabaseName(QStringLiteral("habit_tracker_tdd_test"));
        db.setUserName(QStringLiteral("habit_tracker_tdd_app"));
        QVERIFY2(db.open(db.userName(), qEnvironmentVariable("HABIT_TRACKER_TDD_DB_PASSWORD")), "Cannot open isolated test database.");
        QSqlQuery identity(db);
        QVERIFY(identity.exec(QStringLiteral("SELECT current_database(), current_user")));
        QVERIFY(identity.next());
        QCOMPARE(identity.value(0).toString(), QStringLiteral("habit_tracker_tdd_test"));
        QCOMPARE(identity.value(1).toString(), QStringLiteral("habit_tracker_tdd_app"));
        QVERIFY(identity.exec(QStringLiteral("SELECT current_setting('server_version'), rolsuper, rolcreatedb, rolcreaterole FROM pg_roles WHERE rolname=current_user")));
        QVERIFY(identity.next());
        QVERIFY(identity.value(0).toString().startsWith(QStringLiteral("18.6")));
        QVERIFY(!identity.value(1).toBool());
        QVERIFY(!identity.value(2).toBool());
        QVERIFY(!identity.value(3).toBool());
    }
    void initializationCreatesVersionedSchema()
    {
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        QSqlQuery query(db);
        QVERIFY(query.exec(QStringLiteral("SELECT version FROM schema_version")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 1);
    }
    void unknownSchemaVersionIsRejectedWithoutOverwrite()
    {
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        QSqlQuery query(db);
        QVERIFY(query.exec(QStringLiteral("UPDATE schema_version SET version = 2")));
        QVERIFY(!store.initialize(error));
        QVERIFY(!error.isEmpty());
        QVERIFY(query.exec(QStringLiteral("SELECT version FROM schema_version")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 2);
    }
    void unrelatedDatabaseIsRejectedBeforeSchemaChanges()
    {
        {
            auto other = QSqlDatabase::cloneDatabase(db, QStringLiteral("unrelated"));
            other.setDatabaseName(QStringLiteral("postgres"));
            QVERIFY(other.open(other.userName(), qEnvironmentVariable("HABIT_TRACKER_TDD_DB_PASSWORD")));
            HabitStore store(other);
            QString error;
            QVERIFY(!store.initialize(error));
            QVERIFY(error.contains(QStringLiteral("TDD")));
            QSqlQuery query(other);
            QVERIFY(query.exec(QStringLiteral("SELECT to_regclass('public.schema_version') IS NULL")));
            QVERIFY(query.next());
            QVERIFY(query.value(0).toBool());
            other.close();
        }
        QSqlDatabase::removeDatabase(QStringLiteral("unrelated"));
    }
    void habitCreationPersistsTrimmedCyrillicName()
    {
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        qint64 id = 0;
        QVERIFY(store.createHabit(QStringLiteral("  Чтение  "), id, error));
        QVERIFY(id > 0);
        QSqlQuery query(db);
        query.prepare(QStringLiteral("SELECT name FROM habits WHERE id = :id"));
        query.bindValue(QStringLiteral(":id"), id);
        QVERIFY(query.exec());
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toString(), QStringLiteral("Чтение"));
    }
    void repeatedCompletionCreatesOnlyOneSavedRecord()
    {
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        qint64 id;
        QVERIFY(store.createHabit(QStringLiteral("Чтение"), id, error));
        const QDate day(2026, 11, 8);
        QVERIFY(store.setCompletion(id, day, true, day, error));
        QVERIFY(store.setCompletion(id, day, true, day, error));
        QSqlQuery query(db);
        query.prepare(QStringLiteral("SELECT count(*) FROM habit_completions WHERE habit_id=:id AND completed_on=:day"));
        query.bindValue(QStringLiteral(":id"), id);
        query.bindValue(QStringLiteral(":day"), day);
        QVERIFY(query.exec());
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 1);
    }
    void removingCompletionTwicePreservesOtherDates()
    {
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        qint64 id;
        QVERIFY(store.createHabit(QStringLiteral("Чтение"), id, error));
        const QDate today(2026, 11, 8);
        QVERIFY(store.setCompletion(id, today, true, today, error));
        QVERIFY(store.setCompletion(id, today.addDays(-1), true, today, error));
        QVERIFY(store.setCompletion(id, today, false, today, error));
        QVERIFY(store.setCompletion(id, today, false, today, error));
        QSqlQuery query(db);
        QVERIFY(query.exec(QStringLiteral("SELECT completed_on FROM habit_completions")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toDate(), today.addDays(-1));
        QVERIFY(!query.next());
    }
    void renamePersistsWithoutChangingHistory()
    {
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        qint64 id;
        QVERIFY(store.createHabit(QStringLiteral("Чтение"), id, error));
        const QDate today(2026, 11, 8);
        QVERIFY(store.setCompletion(id, today, true, today, error));
        QVERIFY(store.renameHabit(id, QStringLiteral("  Книга  "), error));
        QSqlQuery query(db);
        QVERIFY(query.exec(QStringLiteral("SELECT name, completed_on FROM habits JOIN habit_completions ON habits.id=habit_id")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toString(), QStringLiteral("Книга"));
        QCOMPARE(query.value(1).toDate(), today);
    }
    void deletionRemovesHabitAndItsHistoryOnly()
    {
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        qint64 first, second;
        QVERIFY(store.createHabit(QStringLiteral("Чтение"), first, error));
        QVERIFY(store.createHabit(QStringLiteral("Прогулка"), second, error));
        const QDate today(2026, 11, 8);
        QVERIFY(store.setCompletion(first, today, true, today, error));
        QVERIFY(store.setCompletion(second, today, true, today, error));
        QVERIFY(store.deleteHabit(first, error));
        QSqlQuery query(db);
        QVERIFY(query.exec(QStringLiteral("SELECT habits.id, habit_id FROM habits JOIN habit_completions ON habits.id=habit_id")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toLongLong(), second);
        QVERIFY(!query.next());
        QVERIFY(query.exec(QStringLiteral("SELECT count(*) FROM habit_completions")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 1);
    }
    void newConnectionLoadsIndependentDuplicateNamesAndHistory()
    {
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        qint64 first, second;
        QVERIFY(store.createHabit(QStringLiteral("Чтение"), first, error));
        QVERIFY(store.createHabit(QStringLiteral("Чтение"), second, error));
        const QDate today(2026, 11, 8);
        QVERIFY(store.setCompletion(first, today, true, today, error));
        {
            auto reopened = QSqlDatabase::cloneDatabase(db, QStringLiteral("reopened"));
            QVERIFY(reopened.open(reopened.userName(), qEnvironmentVariable("HABIT_TRACKER_TDD_DB_PASSWORD")));
            HabitStore loaded(reopened);
            QVERIFY(loaded.initialize(error));
            QVector<HabitRecord> habits;
            QVERIFY(loaded.loadSnapshot(habits, error));
            QCOMPARE(habits.size(), 2);
            QCOMPARE(habits[0].id, first);
            QCOMPARE(habits[1].id, second);
            QCOMPARE(habits[0].name, habits[1].name);
            QCOMPARE(habits[0].completedDays, QSet<QDate>{today});
            QVERIFY(habits[1].completedDays.isEmpty());
            reopened.close();
        }
        QSqlDatabase::removeDatabase(QStringLiteral("reopened"));
    }
    void failedInitializationBlocksFurtherWrites()
    {
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        qint64 id;
        QVERIFY(store.createHabit(QStringLiteral("Чтение"), id, error));
        QSqlQuery query(db);
        QVERIFY(query.exec(QStringLiteral("UPDATE schema_version SET version=2")));
        QVERIFY(!store.initialize(error));
        QVERIFY(!store.renameHabit(id, QStringLiteral("Изменено"), error));
        QVERIFY(query.exec(QStringLiteral("SELECT name FROM habits")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toString(), QStringLiteral("Чтение"));
    }
    void invalidInputDoesNotPersistAndSqlLikeNamesAreLiteral()
    {
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        qint64 id;
        QVERIFY(!store.createHabit(QStringLiteral("  "), id, error));
        QVERIFY(!store.createHabit(QString(101, u'x'), id, error));
        const QString name = QStringLiteral("Книга'); DROP TABLE habits; --");
        QVERIFY(store.createHabit(name, id, error));
        QVERIFY(!store.renameHabit(id, QStringLiteral("  "), error));
        const QDate today(2026, 11, 8);
        QVERIFY(!store.setCompletion(id, today.addDays(1), true, today, error));
        QVector<HabitRecord> habits;
        QVERIFY(store.loadSnapshot(habits, error));
        QCOMPARE(habits.size(), 1);
        QCOMPARE(habits[0].name, name);
        QVERIFY(habits[0].completedDays.isEmpty());
    }
    void savedAcceptanceDataProducesExpectedStatistics()
    {
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        qint64 reading, walking;
        QVERIFY(store.createHabit(QStringLiteral("Чтение"), reading, error));
        QVERIFY(store.createHabit(QStringLiteral("Прогулка"), walking, error));
        const QDate today(2026, 11, 8);
        for (const int offset : {0, 1, 6, 7, 29, 30})
            QVERIFY(store.setCompletion(reading, today.addDays(-offset), true, today, error));
        for (const int offset : {0, 2})
            QVERIFY(store.setCompletion(walking, today.addDays(-offset), true, today, error));
        QVector<HabitRecord> habits;
        QVERIFY(store.loadSnapshot(habits, error));
        QCOMPARE(habits.size(), 2);
        QCOMPARE(summarizeHabit(habits[0], today).last7Days, 3);
        QCOMPARE(summarizeHabit(habits[0], today).last30Days, 5);
        QCOMPARE(summarizeHabit(habits[1], today).last7Days, 2);
        QCOMPARE(summarizeHabit(habits[1], today).last30Days, 2);
        for (const int period : {7, 30}) {
            const auto bars = completionChart(habits, today, period);
            int total = 0;
            for (const auto &bar : bars) total += bar.count;
            QCOMPARE(total, period == 7 ? 5 : 7);
        }
        QVERIFY(store.deleteHabit(reading, error));
        QVERIFY(store.loadSnapshot(habits, error));
        QCOMPARE(habits.size(), 1);
        QCOMPARE(summarizeHabit(habits[0], today).last30Days, 2);
    }
    void lostConnectionDoesNotReportSuccessfulEmptySnapshot()
    {
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        qint64 id;
        QVERIFY(store.createHabit(QStringLiteral("Чтение"), id, error));
        QVector<HabitRecord> habits;
        QVERIFY(store.loadSnapshot(habits, error));
        db.close();
        QVERIFY(!store.loadSnapshot(habits, error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!error.contains(qEnvironmentVariable("HABIT_TRACKER_TDD_DB_PASSWORD")));
        QCOMPARE(habits.size(), 1);
        QVERIFY(db.open(db.userName(), qEnvironmentVariable("HABIT_TRACKER_TDD_DB_PASSWORD")));
    }
    void dataSurvivesBetweenSeparateProcesses()
    {
        for (const auto &mode : {QStringLiteral("write"), QStringLiteral("read")}) {
            QProcess child;
            child.start(QCoreApplication::applicationDirPath() + QStringLiteral("/storage_probe.exe"), {mode});
            QVERIFY(child.waitForFinished(10000));
            QCOMPARE(child.exitStatus(), QProcess::NormalExit);
            QCOMPARE(child.exitCode(), 0);
        }
    }
    void cleanup()
    {
        QSqlQuery query(db);
        QVERIFY(query.exec(QStringLiteral("DROP TABLE IF EXISTS habit_completions, habits, schema_version CASCADE")));
    }
    void cleanupTestCase()
    {
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    }
};

QTEST_GUILESS_MAIN(DatabaseTests)
#include "DatabaseTests.moc"
