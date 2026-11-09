#pragma once
#include <QMainWindow>
#include <QSqlDatabase>
#include <QDate>
#include <functional>
#include "HabitStore.h"

class QLabel;
class QTableWidget;
class QPushButton;
class QStackedWidget;
class CompletionChart;

class AppWindow : public QMainWindow {
public:
    explicit AppWindow(QSqlDatabase database, std::function<QDate()> clock = QDate::currentDate);
protected:
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
private:
    void reload();
    void renderWeek();
    void renderStatistics();
    void editHabit(qint64 id = 0, const QString &initialName = {});
    void reconnect();
    void displayError(const QString &message);
    QSqlDatabase db;
    HabitStore store;
    std::function<QDate()> clock;
    WeekRange selectedWeek;
    QVector<HabitRecord> habits;
    QDate todayShown;
    QLabel *range;
    QLabel *errorLabel;
    QLabel *emptyHint;
    QWidget *recoveryPanel;
    QTableWidget *grid;
    QPushButton *previous;
    QPushButton *next;
    QStackedWidget *pages;
    QLabel *total7;
    QLabel *total30;
    QTableWidget *summary;
    CompletionChart *chart7;
    CompletionChart *chart30;
    bool uiReady = false;
};
