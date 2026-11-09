#include <QtTest>
#include <QLabel>
#include <QSqlQuery>
#include <QTableWidget>
#include <QFontDatabase>
#include <QPushButton>
#include <QLineEdit>
#include <QDialog>
#include <QTimer>
#include <QMessageBox>
#include <QStackedWidget>
#include <QScrollArea>
#include <QDir>
#include <QScrollBar>
#include <QScopeGuard>
#include "AppWindow.h"
#include "HabitStore.h"

static void capture(QWidget &widget, const QString &name)
{
    const auto folder = qEnvironmentVariable("HABIT_TRACKER_CAPTURE_DIR");
    if (folder.isEmpty()) return;
    QDir().mkpath(folder);
    QApplication::processEvents();
    widget.grab().save(QDir(folder).filePath(name + QStringLiteral(".png")));
}

class WindowTests : public QObject {
    Q_OBJECT
    QSqlDatabase db;
private slots:
    void initTestCase()
    {
        QApplication::setStyle(QStringLiteral("Fusion"));
        QFontDatabase::addApplicationFont(QStringLiteral("C:/Windows/Fonts/segoeui.ttf"));
        QVERIFY(!qEnvironmentVariableIsEmpty("HABIT_TRACKER_TDD_DB_PASSWORD"));
        db = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"));
        db.setHostName(qEnvironmentVariable("HABIT_TRACKER_TDD_DB_HOST"));
        db.setPort(qEnvironmentVariableIntValue("HABIT_TRACKER_TDD_DB_PORT"));
        db.setDatabaseName(QStringLiteral("habit_tracker_tdd_test"));
        db.setUserName(QStringLiteral("habit_tracker_tdd_app"));
        QVERIFY(db.open(db.userName(), qEnvironmentVariable("HABIT_TRACKER_TDD_DB_PASSWORD")));
    }
    void startupOpensWeekContainingLocalToday()
    {
        AppWindow window(db, [] { return QDate(2026, 11, 8); });
        window.show();
        capture(window, QStringLiteral("empty"));
        auto range = window.findChild<QLabel *>(QStringLiteral("weekRange"));
        QVERIFY(range);
        QVERIFY(range->text().contains(QStringLiteral("02.11.2026")));
        QVERIFY(range->text().contains(QStringLiteral("08.11.2026")));
    }
    void savedCompletionsAppearOnTheirCalendarDates()
    {
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        qint64 id;
        QVERIFY(store.createHabit(QStringLiteral("Чтение"), id, error));
        const QDate today(2026, 11, 8);
        QVERIFY(store.setCompletion(id, today, true, today, error));
        AppWindow window(db, [today] { return today; });
        auto grid = window.findChild<QTableWidget *>(QStringLiteral("weekGrid"));
        QVERIFY(grid);
        QCOMPARE(grid->rowCount(), 1);
        QCOMPARE(grid->item(0, 0)->text(), QStringLiteral("Чтение"));
        QCOMPARE(grid->item(0, 8)->checkState(), Qt::Checked);
        QCOMPARE(grid->item(0, 7)->checkState(), Qt::Unchecked);
        QCOMPARE(grid->item(0, 1)->text(), QStringLiteral("1 день"));
    }
    void spaceTogglesCompletionAndPersistsIt()
    {
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        qint64 id;
        QVERIFY(store.createHabit(QStringLiteral("Чтение"), id, error));
        const QDate today(2026, 11, 8);
        AppWindow window(db, [today] { return today; });
        window.show();
        auto grid = window.findChild<QTableWidget *>(QStringLiteral("weekGrid"));
        QVERIFY(grid);
        grid->setCurrentCell(0, 8);
        grid->setFocus();
        QTest::keyClick(grid, Qt::Key_Space);
        QTRY_COMPARE(grid->item(0, 8)->checkState(), Qt::Checked);
        QTRY_VERIFY(grid->isEnabled());
        QVector<HabitRecord> habits;
        QVERIFY(store.loadSnapshot(habits, error));
        QVERIFY(habits[0].completedDays.contains(today));
        QCOMPARE(grid->item(0, 1)->text(), QStringLiteral("1 день"));
        QTest::keyClick(grid, Qt::Key_Space);
        QTRY_VERIFY(grid->isEnabled());
        QVERIFY(store.loadSnapshot(habits, error));
        QVERIFY(!habits[0].completedDays.contains(today));
    }
    void nextWeekNavigationKeepsMarksOnTheirDates()
    {
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        qint64 id;
        QVERIFY(store.createHabit(QStringLiteral("Чтение"), id, error));
        const QDate today(2026, 11, 8);
        QVERIFY(store.setCompletion(id, today, true, today, error));
        AppWindow window(db, [today] { return today; });
        auto next = window.findChild<QPushButton *>(QStringLiteral("nextWeek"));
        auto previous = window.findChild<QPushButton *>(QStringLiteral("previousWeek"));
        QVERIFY(next);
        QVERIFY(previous);
        next->click();
        auto range = window.findChild<QLabel *>(QStringLiteral("weekRange"));
        QVERIFY(range->text().contains(QStringLiteral("09.11.2026")));
        auto grid = window.findChild<QTableWidget *>(QStringLiteral("weekGrid"));
        QCOMPARE(grid->item(0, 8)->checkState(), Qt::Unchecked);
        QVERIFY(!(grid->item(0, 8)->flags() & Qt::ItemIsEnabled));
        previous->click();
        QCOMPARE(grid->item(0, 8)->checkState(), Qt::Checked);
    }
    void statisticsShowsSavedTotalsAndReturnsToSelectedWeek()
    {
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        qint64 reading, walking;
        QVERIFY(store.createHabit(QStringLiteral("Чтение"), reading, error));
        QVERIFY(store.createHabit(QStringLiteral("Прогулка"), walking, error));
        const QDate today(2026, 11, 8);
        for (const auto offset : {0, 1, 6, 7, 29, 30})
            QVERIFY(store.setCompletion(reading, today.addDays(-offset), true, today, error));
        for (const auto offset : {0, 2})
            QVERIFY(store.setCompletion(walking, today.addDays(-offset), true, today, error));
        AppWindow window(db, [today] { return today; });
        window.show();
        capture(window, QStringLiteral("week"));
        window.findChild<QPushButton *>(QStringLiteral("previousWeek"))->click();
        const auto selected = window.findChild<QLabel *>(QStringLiteral("weekRange"))->text();
        auto open = window.findChild<QPushButton *>(QStringLiteral("openStats"));
        QVERIFY(open);
        open->click();
        auto total7 = window.findChild<QLabel *>(QStringLiteral("total7"));
        auto total30 = window.findChild<QLabel *>(QStringLiteral("total30"));
        QVERIFY(total7);
        QVERIFY(total30);
        QCOMPARE(total7->text(), QStringLiteral("5"));
        QCOMPARE(total30->text(), QStringLiteral("7"));
        capture(window, QStringLiteral("statistics"));
        capture(*window.findChild<QScrollArea *>(QStringLiteral("statsScroll"))->widget(), QStringLiteral("statistics-content"));
        auto back = window.findChild<QPushButton *>(QStringLiteral("backToWeek"));
        QVERIFY(back);
        back->click();
        QCOMPARE(window.findChild<QLabel *>(QStringLiteral("weekRange"))->text(), selected);
    }
    void activationRefreshesLocalDayWithoutChangingSelectedWeek()
    {
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        qint64 id;
        QVERIFY(store.createHabit(QStringLiteral("Чтение"), id, error));
        QDate today(2026, 11, 8);
        QVERIFY(store.setCompletion(id, today.addDays(-1), true, today, error));
        QVERIFY(store.setCompletion(id, today, true, today, error));
        AppWindow window(db, [&today] { return today; });
        window.findChild<QPushButton *>(QStringLiteral("nextWeek"))->click();
        auto grid = window.findChild<QTableWidget *>(QStringLiteral("weekGrid"));
        QVERIFY(!(grid->item(0, 2)->flags() & Qt::ItemIsEnabled));
        const auto selected = window.findChild<QLabel *>(QStringLiteral("weekRange"))->text();
        today = today.addDays(1);
        QEvent activate(QEvent::WindowActivate);
        QApplication::sendEvent(&window, &activate);
        QVERIFY(grid->item(0, 2)->flags() & Qt::ItemIsEnabled);
        QCOMPARE(grid->item(0, 1)->text(), QStringLiteral("2 дня"));
        QCOMPARE(window.findChild<QLabel *>(QStringLiteral("weekRange"))->text(), selected);
        today = today.addDays(1);
        QApplication::sendEvent(&window, &activate);
        QCOMPARE(grid->item(0, 1)->text(), QStringLiteral("0 дней"));
    }
    void addDialogKeepsInvalidInputAndSavesValidName()
    {
        AppWindow window(db, [] { return QDate(2026, 11, 8); });
        window.show();
        auto add = window.findChild<QPushButton *>(QStringLiteral("addHabit"));
        QVERIFY(add);
        bool interactionSucceeded = false;
        QTimer::singleShot(0, &window, [&] {
            auto dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!dialog) return;
            auto input = dialog->findChild<QLineEdit *>(QStringLiteral("habitName"));
            auto save = dialog->findChild<QPushButton *>(QStringLiteral("saveHabit"));
            auto error = dialog->findChild<QLabel *>(QStringLiteral("nameError"));
            if (!input || !save || !error) { dialog->reject(); return; }
            input->setText(QStringLiteral("   "));
            save->click();
            const bool invalidStayed = dialog->isVisible() && !error->text().isEmpty();
            capture(*dialog, QStringLiteral("invalid-name"));
            input->setText(QStringLiteral("  Чтение  "));
            save->click();
            interactionSucceeded = invalidStayed && dialog->result() == QDialog::Accepted;
            if (!interactionSucceeded) dialog->reject();
        });
        add->click();
        QVERIFY(interactionSucceeded);
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        QVector<HabitRecord> habits;
        QVERIFY(store.loadSnapshot(habits, error));
        QCOMPARE(habits.size(), 1);
        QCOMPARE(habits[0].name, QStringLiteral("Чтение"));
    }
    void renameDialogSavesOnEnterAndCancelsOnEscape()
    {
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        qint64 id;
        QVERIFY(store.createHabit(QStringLiteral("Чтение"), id, error));
        AppWindow window(db, [] { return QDate(2026, 11, 8); });
        window.show();
        auto rename = window.findChild<QPushButton *>(QStringLiteral("rename-%1").arg(id));
        QVERIFY(rename);
        QTimer::singleShot(0, &window, [] {
            auto dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!dialog) return;
            auto input = dialog->findChild<QLineEdit *>(QStringLiteral("habitName"));
            if (!input) { dialog->reject(); return; }
            input->setText(QStringLiteral("Книга"));
            QTest::keyClick(input, Qt::Key_Return);
            if (dialog->isVisible()) dialog->reject();
        });
        rename->click();
        QVector<HabitRecord> habits;
        QVERIFY(store.loadSnapshot(habits, error));
        QCOMPARE(habits[0].name, QStringLiteral("Книга"));
        rename = window.findChild<QPushButton *>(QStringLiteral("rename-%1").arg(id));
        QVERIFY(rename);
        QTimer::singleShot(0, &window, [] {
            auto dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!dialog) return;
            auto input = dialog->findChild<QLineEdit *>(QStringLiteral("habitName"));
            if (input) input->setText(QStringLiteral("Отменённое имя"));
            QTest::keyClick(dialog, Qt::Key_Escape);
        });
        rename->click();
        QVERIFY(store.loadSnapshot(habits, error));
        QCOMPARE(habits[0].name, QStringLiteral("Книга"));
    }
    void deletionDefaultsToCancelAndRequiresConfirmation()
    {
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        qint64 id;
        QVERIFY(store.createHabit(QStringLiteral("Чтение"), id, error));
        AppWindow window(db, [] { return QDate(2026, 11, 8); });
        window.show();
        auto remove = window.findChild<QPushButton *>(QStringLiteral("delete-%1").arg(id));
        QVERIFY(remove);
        bool safeDefault = false;
        QTimer::singleShot(0, &window, [&] {
            auto dialog = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            if (!dialog) return;
            safeDefault = dialog->defaultButton() == dialog->button(QMessageBox::Cancel)
                && dialog->text().contains(QStringLiteral("Чтение"))
                && dialog->text().contains(QStringLiteral("истори"));
            capture(*dialog, QStringLiteral("delete-confirmation"));
            QTest::keyClick(dialog, Qt::Key_Return);
        });
        remove->click();
        QVERIFY(safeDefault);
        QVector<HabitRecord> habits;
        QVERIFY(store.loadSnapshot(habits, error));
        QCOMPARE(habits.size(), 1);
        remove = window.findChild<QPushButton *>(QStringLiteral("delete-%1").arg(id));
        QVERIFY(remove);
        QTimer::singleShot(0, &window, [] {
            auto dialog = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            if (dialog) dialog->button(QMessageBox::Yes)->click();
        });
        remove->click();
        QVERIFY(store.loadSnapshot(habits, error));
        QVERIFY(habits.isEmpty());
    }
    void connectionFailureDisablesDataActionsAndCanBeRetried()
    {
        {
            auto disconnected = QSqlDatabase::cloneDatabase(db, QStringLiteral("disconnected"));
            disconnected.setPort(1);
            AppWindow window(disconnected, [] { return QDate(2026, 11, 8); });
            auto retry = window.findChild<QPushButton *>(QStringLiteral("retryConnection"));
            QVERIFY(retry);
            auto pages = window.findChild<QStackedWidget *>(QStringLiteral("pages"));
            QVERIFY(pages);
            QVERIFY(!pages->isEnabled());
            auto error = window.findChild<QLabel *>(QStringLiteral("errorLabel"));
            QVERIFY(!error->text().isEmpty());
            QVERIFY(!error->text().contains(qEnvironmentVariable("HABIT_TRACKER_TDD_DB_PASSWORD")));
            window.show();
            capture(window, QStringLiteral("connection-error"));
            disconnected.setPort(db.port());
            retry->click();
            QVERIFY(pages->isEnabled());
            QVERIFY(error->text().isEmpty());
            disconnected.close();
        }
        QSqlDatabase::removeDatabase(QStringLiteral("disconnected"));
    }
    void clickingCellCenterTogglesSavedCompletion()
    {
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        qint64 id;
        QVERIFY(store.createHabit(QStringLiteral("Чтение"), id, error));
        const QDate today(2026, 11, 8);
        AppWindow window(db, [today] { return today; });
        window.resize(1360, 820);
        window.show();
        auto grid = window.findChild<QTableWidget *>(QStringLiteral("weekGrid"));
        grid->scrollToItem(grid->item(0, 8));
        QTest::mouseClick(grid->viewport(), Qt::LeftButton, Qt::NoModifier, grid->visualItemRect(grid->item(0, 8)).center());
        QTRY_VERIFY_WITH_TIMEOUT(grid->item(0, 8)->checkState() == Qt::Checked, 1000);
        QTRY_VERIFY(grid->isEnabled());
        QVector<HabitRecord> habits;
        QVERIFY(store.loadSnapshot(habits, error));
        QVERIFY(habits[0].completedDays.contains(today));
    }
    void enterTogglesFocusedCompletionCell()
    {
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        qint64 id;
        QVERIFY(store.createHabit(QStringLiteral("Чтение"), id, error));
        AppWindow window(db, [] { return QDate(2026, 11, 8); });
        window.show();
        auto grid = window.findChild<QTableWidget *>(QStringLiteral("weekGrid"));
        grid->setCurrentCell(0, 8);
        grid->setFocus();
        QTest::keyClick(grid, Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(grid->item(0, 8)->checkState() == Qt::Checked, 1000);
        QTRY_VERIFY(grid->isEnabled());
    }
    void refreshDoesNotLeaveDuplicateVisibleRowActions()
    {
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        qint64 id;
        QVERIFY(store.createHabit(QStringLiteral("Чтение"), id, error));
        AppWindow window(db, [] { return QDate(2026, 11, 8); });
        window.show();
        QApplication::processEvents();
        int visible = 0;
        for (const auto button : window.findChildren<QPushButton *>(QStringLiteral("rename-%1").arg(id)))
            if (button->isVisible()) ++visible;
        QCOMPARE(visible, 1);
    }
    void longNameRemainsReadableThroughTooltipInNarrowWindow()
    {
        HabitStore store(db);
        QString error;
        QVERIFY(store.initialize(error));
        const QString name(100, QChar(u'я'));
        qint64 id;
        QVERIFY(store.createHabit(name, id, error));
        AppWindow window(db, [] { return QDate(2026, 11, 8); });
        window.resize(800, 580);
        window.show();
        QApplication::processEvents();
        auto grid = window.findChild<QTableWidget *>(QStringLiteral("weekGrid"));
        QCOMPARE(grid->item(0, 0)->toolTip(), name);
        QVERIFY(grid->horizontalScrollBar()->maximum() > 0);
        auto add = window.findChild<QPushButton *>(QStringLiteral("addHabit"));
        QVERIFY(window.rect().contains(add->mapTo(&window, QPoint(0, 0))));
        capture(window, QStringLiteral("narrow-window"));
    }
    void quoteIsBuiltInAndStableDuringNavigation()
    {
        AppWindow window(db, [] { return QDate(2026, 11, 8); });
        auto quote = window.findChild<QLabel *>(QStringLiteral("quote"));
        QVERIFY(quote);
        const auto initial = quote->text();
        QVERIFY(builtInQuotes().contains(initial));
        window.findChild<QPushButton *>(QStringLiteral("previousWeek"))->click();
        window.findChild<QPushButton *>(QStringLiteral("openStats"))->click();
        QCOMPARE(quote->text(), initial);
    }
    void ordinaryLaunchPasswordCanOpenConfiguredTestDatabase()
    {
        const auto testPassword = qgetenv("HABIT_TRACKER_TDD_DB_PASSWORD");
        const auto mainPassword = qgetenv("HABIT_TRACKER_DB_PASSWORD");
        const auto restore = qScopeGuard([&] {
            qputenv("HABIT_TRACKER_TDD_DB_PASSWORD", testPassword);
            qputenv("HABIT_TRACKER_DB_PASSWORD", mainPassword);
        });
        qputenv("HABIT_TRACKER_DB_PASSWORD", testPassword);
        qunsetenv("HABIT_TRACKER_TDD_DB_PASSWORD");
        bool opened;
        {
            auto configured = QSqlDatabase::cloneDatabase(db, QStringLiteral("configured"));
            AppWindow window(configured, [] { return QDate(2026, 11, 8); });
            opened = window.findChild<QStackedWidget *>(QStringLiteral("pages"))->isEnabled();
            configured.close();
        }
        QSqlDatabase::removeDatabase(QStringLiteral("configured"));
        QVERIFY(opened);
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

QTEST_MAIN(WindowTests)
#include "WindowTests.moc"
