#include "AppWindow.h"
#include "HabitLogic.h"
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QSignalBlocker>
#include <QTimer>
#include <QPushButton>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QScrollArea>
#include <QEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QKeyEvent>
#include <QPainter>
#include <QRandomGenerator>
#include "Theme.h"
#include "CompletionChart.h"

static QString streakText(int count)
{
    const auto last = count % 10;
    const auto hundreds = count % 100;
    const auto suffix = hundreds >= 11 && hundreds <= 14 ? QStringLiteral("дней")
        : last == 1 ? QStringLiteral("день")
        : last >= 2 && last <= 4 ? QStringLiteral("дня") : QStringLiteral("дней");
    return QString::number(count) + QStringLiteral(" ") + suffix;
}

class WeekHeader : public QHeaderView {
public:
    explicit WeekHeader(QWidget *parent) : QHeaderView(Qt::Horizontal, parent) {}
protected:
    void paintSection(QPainter *painter, const QRect &rect, int section) const override
    {
        if (section != property("todaySection").toInt()) {
            QHeaderView::paintSection(painter, rect, section);
            return;
        }
        painter->save();
        painter->fillRect(rect, QColor(QStringLiteral("#bb708c")));
        painter->setPen(Qt::white);
        auto textFont = font();
        textFont.setBold(true);
        painter->setFont(textFont);
        painter->drawText(rect.adjusted(2, 0, -2, 0), Qt::AlignCenter, model()->headerData(section, Qt::Horizontal).toString());
        painter->restore();
    }
};

AppWindow::AppWindow(QSqlDatabase database, std::function<QDate()> dateClock)
    : db(database), store(db), clock(std::move(dateClock)), selectedWeek(weekContaining(clock())), todayShown(clock())
{
    setWindowTitle(QStringLiteral("Трекер привычек"));
    resize(1360, 820);
    setMinimumSize(800, 480);
    setFont(QFont(QStringLiteral("Segoe UI"), 10));
    setStyleSheet(applicationStyle());
    auto central = new QWidget(this);
    auto layout = new QVBoxLayout(central);
    layout->setContentsMargins(28, 22, 28, 22);
    layout->setSpacing(16);
    auto appTitle = new QLabel(QStringLiteral("Трекер привычек"), this);
    appTitle->setObjectName(QStringLiteral("appTitle"));
    auto subtitle = new QLabel(QStringLiteral("Маленькие шаги. Ваш собственный ритм."), this);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    const auto quotes = builtInQuotes();
    auto quote = new QLabel(quotes[QRandomGenerator::global()->bounded(quotes.size())], this);
    quote->setObjectName(QStringLiteral("quote"));
    quote->setWordWrap(true);
    layout->addWidget(appTitle);
    layout->addWidget(subtitle);
    layout->addWidget(quote);
    range = new QLabel(this);
    range->setObjectName(QStringLiteral("weekRange"));
    errorLabel = new QLabel(this);
    errorLabel->setObjectName(QStringLiteral("errorLabel"));
    errorLabel->setWordWrap(true);
    grid = new QTableWidget(this);
    grid->setObjectName(QStringLiteral("weekGrid"));
    grid->installEventFilter(this);
    grid->setColumnCount(10);
    grid->setHorizontalHeader(new WeekHeader(grid));
    grid->horizontalHeader()->setMinimumHeight(66);
    grid->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    grid->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    grid->horizontalHeader()->setMinimumSectionSize(70);
    grid->setColumnWidth(1, 100);
    for (int column = 2; column <= 8; ++column) grid->setColumnWidth(column, 80);
    grid->setColumnWidth(9, 252);
    grid->setTextElideMode(Qt::ElideRight);
    grid->setAlternatingRowColors(true);
    grid->setSelectionMode(QAbstractItemView::SingleSelection);
    grid->setEditTriggers(QAbstractItemView::NoEditTriggers);
    grid->verticalHeader()->hide();
    connect(grid, &QTableWidget::cellClicked, this, [this](int row, int column) {
        if (!grid->isEnabled() || column < 2 || column > 8 || row >= habits.size())
            return;
        auto cell = grid->item(row, column);
        if (!(cell->flags() & Qt::ItemIsUserCheckable)) return;
        const bool savedState = habits[row].completedDays.contains(cell->data(Qt::UserRole + 1).toDate());
        // Clicking the checkbox itself already triggers itemChanged; clicking elsewhere toggles here.
        if ((cell->checkState() == Qt::Checked) == savedState)
            cell->setCheckState(savedState ? Qt::Unchecked : Qt::Checked);
    });
    connect(grid, &QTableWidget::itemChanged, this, [this](QTableWidgetItem *cell) {
        if (cell->column() < 2 || cell->column() > 8)
            return;
        const auto id = cell->data(Qt::UserRole).toLongLong();
        const auto day = cell->data(Qt::UserRole + 1).toDate();
        const auto completed = cell->checkState() == Qt::Checked;
        grid->setEnabled(false);
        QTimer::singleShot(0, this, [this, id, day, completed] {
            QString error;
            const bool saved = store.setCompletion(id, day, completed, clock(), error);
            const int row = grid->currentRow(), column = grid->currentColumn();
            reload();
            grid->setCurrentCell(row, column);
            if (!saved) displayError(error);
        });
    });
    layout->addWidget(errorLabel);
    recoveryPanel = new QWidget(this);
    auto recovery = new QHBoxLayout(recoveryPanel);
    recovery->setContentsMargins(0, 0, 0, 0);
    auto retry = new QPushButton(QStringLiteral("Повторить подключение"), this);
    retry->setObjectName(QStringLiteral("retryConnection"));
    auto close = new QPushButton(QStringLiteral("Закрыть"), this);
    close->setObjectName(QStringLiteral("closeApplication"));
    recovery->addWidget(retry);
    recovery->addWidget(close);
    recovery->addStretch();
    layout->addWidget(recoveryPanel);
    connect(retry, &QPushButton::clicked, this, [this] { db.close(); reconnect(); });
    connect(close, &QPushButton::clicked, this, &QWidget::close);
    pages = new QStackedWidget(this);
    pages->setObjectName(QStringLiteral("pages"));
    auto weekPage = new QWidget(this);
    auto weekLayout = new QVBoxLayout(weekPage);
    weekLayout->setContentsMargins(0, 0, 0, 0);
    weekLayout->setSpacing(18);
    auto openStats = new QPushButton(QStringLiteral("Статистика"), this);
    openStats->setObjectName(QStringLiteral("openStats"));
    auto toolbar = new QHBoxLayout;
    auto add = new QPushButton(QStringLiteral("+ Добавить привычку"), this);
    add->setObjectName(QStringLiteral("addHabit"));
    toolbar->addWidget(add);
    toolbar->addStretch();
    toolbar->addWidget(openStats);
    weekLayout->addLayout(toolbar);
    connect(add, &QPushButton::clicked, this, [this] { editHabit(); });
    auto navigation = new QHBoxLayout;
    previous = new QPushButton(QStringLiteral("‹ Предыдущая неделя"), this);
    previous->setObjectName(QStringLiteral("previousWeek"));
    next = new QPushButton(QStringLiteral("Следующая неделя ›"), this);
    next->setObjectName(QStringLiteral("nextWeek"));
    for (const auto &entry : {qMakePair(previous, -1), qMakePair(next, 1)}) {
        connect(entry.first, &QPushButton::clicked, this, [this, direction = entry.second] {
            selectedWeek = shiftedWeek(selectedWeek, direction);
            reload();
        });
    }
    navigation->addWidget(previous);
    navigation->addWidget(range, 1, Qt::AlignCenter);
    navigation->addWidget(next);
    weekLayout->addLayout(navigation);
    emptyHint = new QLabel(QStringLiteral("Здесь начнётся ваша история. Добавьте первую привычку."), this);
    emptyHint->setObjectName(QStringLiteral("emptyHint"));
    emptyHint->setWordWrap(true);
    weekLayout->addWidget(emptyHint);
    weekLayout->addWidget(grid);
    pages->addWidget(weekPage);
    auto statsPage = new QWidget(this);
    auto statsLayout = new QVBoxLayout(statsPage);
    statsLayout->setContentsMargins(0, 0, 0, 0);
    auto back = new QPushButton(QStringLiteral("‹ Назад к привычкам"), this);
    back->setObjectName(QStringLiteral("backToWeek"));
    statsLayout->addWidget(back, 0, Qt::AlignLeft);
    auto scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("statsScroll"));
    scroll->setWidgetResizable(true);
    auto statsContent = new QWidget(this);
    auto contents = new QVBoxLayout(statsContent);
    auto title = new QLabel(QStringLiteral("Ваш прогресс"), this);
    title->setObjectName(QStringLiteral("statsTitle"));
    contents->addWidget(title);
    auto totals = new QHBoxLayout;
    total7 = new QLabel(QStringLiteral("0"), this);
    total7->setObjectName(QStringLiteral("total7"));
    total30 = new QLabel(QStringLiteral("0"), this);
    total30->setObjectName(QStringLiteral("total30"));
    totals->addWidget(new QLabel(QStringLiteral("Выполнений за 7 дней:"), this));
    totals->addWidget(total7);
    totals->addSpacing(24);
    totals->addWidget(new QLabel(QStringLiteral("За 30 дней:"), this));
    totals->addWidget(total30);
    totals->addStretch();
    contents->addLayout(totals);
    chart7 = new CompletionChart(QStringLiteral("Последние 7 дней · выполнений за день"), this);
    chart30 = new CompletionChart(QStringLiteral("Последние 30 дней · выполнений за день"), this);
    contents->addWidget(chart7);
    contents->addWidget(chart30);
    summary = new QTableWidget(this);
    summary->setColumnCount(4);
    summary->setHorizontalHeaderLabels({QStringLiteral("Привычка"), QStringLiteral("Серия"), QStringLiteral("7 дней"), QStringLiteral("30 дней")});
    summary->setEditTriggers(QAbstractItemView::NoEditTriggers);
    summary->verticalHeader()->hide();
    summary->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    summary->setMinimumHeight(180);
    summary->verticalHeader()->setDefaultSectionSize(50);
    contents->addWidget(summary);
    scroll->setWidget(statsContent);
    statsLayout->addWidget(scroll);
    pages->addWidget(statsPage);
    layout->addWidget(pages);
    connect(openStats, &QPushButton::clicked, this, [this] { pages->setCurrentIndex(1); reload(); });
    connect(back, &QPushButton::clicked, this, [this] { pages->setCurrentIndex(0); reload(); });
    setCentralWidget(central);
    reconnect();
    renderWeek();
    uiReady = true;
    auto timer = new QTimer(this);
    timer->setInterval(1000);
    connect(timer, &QTimer::timeout, this, [this] {
        if (clock() != todayShown) reload();
    });
    timer->start();
}

void AppWindow::reconnect()
{
    QString error;
    const auto password = db.databaseName() == QStringLiteral("habit_tracker_tdd_test") && !qEnvironmentVariableIsEmpty("HABIT_TRACKER_TDD_DB_PASSWORD")
        ? qEnvironmentVariable("HABIT_TRACKER_TDD_DB_PASSWORD") : qEnvironmentVariable("HABIT_TRACKER_DB_PASSWORD");
    if ((!db.isOpen() && !db.open(db.userName(), password)) || !store.initialize(error)) {
        displayError(error.isEmpty() ? QStringLiteral("Не удалось подключиться к PostgreSQL. Проверьте настройки и повторите попытку.") : error);
        pages->setEnabled(false);
        return;
    }
    reload();
}

void AppWindow::displayError(const QString &message)
{
    errorLabel->setText(message);
    errorLabel->setVisible(!message.isEmpty());
    recoveryPanel->setVisible(!message.isEmpty());
}

bool AppWindow::event(QEvent *event)
{
    const bool handled = QMainWindow::event(event);
    if (uiReady && event->type() == QEvent::WindowActivate)
        reload();
    return handled;
}

bool AppWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == grid && event->type() == QEvent::KeyPress) {
        const auto key = static_cast<QKeyEvent *>(event)->key();
        auto cell = grid->currentItem();
        if ((key == Qt::Key_Return || key == Qt::Key_Enter) && grid->isEnabled() && cell &&
            (cell->flags() & Qt::ItemIsUserCheckable)) {
            cell->setCheckState(cell->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void AppWindow::reload()
{
    todayShown = clock();
    QString error;
    if (!store.loadSnapshot(habits, error)) {
        displayError(error);
        grid->setEnabled(false);
        pages->setEnabled(false);
        return;
    }
    displayError({});
    grid->setEnabled(true);
    pages->setEnabled(true);
    renderWeek();
    renderStatistics();
}

void AppWindow::renderWeek()
{
    const QSignalBlocker blocker(grid);
    const auto today = todayShown;
    emptyHint->setVisible(habits.isEmpty() && pages->isEnabled());
    previous->setEnabled(shiftedWeek(selectedWeek, -1).monday != selectedWeek.monday);
    next->setEnabled(shiftedWeek(selectedWeek, 1).monday != selectedWeek.monday);
    range->setText(selectedWeek.monday.toString(QStringLiteral("dd.MM.yyyy")) + QStringLiteral(" — ") + selectedWeek.sunday.toString(QStringLiteral("dd.MM.yyyy")));
    QStringList headers{QStringLiteral("Привычка"), QStringLiteral("Серия")};
    const QStringList weekdays{QStringLiteral("Пн"), QStringLiteral("Вт"), QStringLiteral("Ср"), QStringLiteral("Чт"), QStringLiteral("Пт"), QStringLiteral("Сб"), QStringLiteral("Вс")};
    for (int i = 0; i < 7; ++i)
        headers << weekdays[i] + QStringLiteral("\n") + selectedWeek.monday.addDays(i).toString(QStringLiteral("dd.MM"));
    headers << QStringLiteral("Действия");
    grid->setHorizontalHeaderLabels(headers);
    const auto todayOffset = selectedWeek.monday.daysTo(today);
    grid->horizontalHeader()->setProperty("todaySection", todayOffset >= 0 && todayOffset < 7 ? int(todayOffset) + 2 : -1);
    if (todayOffset >= 0 && todayOffset < 7)
        grid->horizontalHeaderItem(int(todayOffset) + 2)->setText(headers[int(todayOffset) + 2] + QStringLiteral("\nсегодня"));
    grid->horizontalHeader()->viewport()->update();
    // QTableWidget deletes replaced index widgets later; hide them before replacement.
    for (int row = 0; row < grid->rowCount(); ++row)
        if (auto oldActions = grid->cellWidget(row, 9)) oldActions->hide();
    grid->setRowCount(habits.size());
    for (int row = 0; row < habits.size(); ++row) {
        grid->setRowHeight(row, 72);
        const auto &habit = habits[row];
        auto name = new QTableWidgetItem(habit.name);
        name->setToolTip(habit.name);
        grid->setItem(row, 0, name);
        auto streak = new QTableWidgetItem(streakText(currentStreak(habit.completedDays, today)));
        streak->setForeground(QColor(QStringLiteral("#b45b7a")));
        streak->setBackground(QColor(QStringLiteral("#f8e5eb")));
        auto streakFont = font();
        streakFont.setBold(true);
        streak->setFont(streakFont);
        streak->setTextAlignment(Qt::AlignCenter);
        grid->setItem(row, 1, streak);
        auto actions = new QWidget(grid);
        auto actionLayout = new QHBoxLayout(actions);
        actionLayout->setContentsMargins(8, 6, 8, 6);
        actionLayout->setSpacing(6);
        auto rename = new QPushButton(QStringLiteral("Переименовать"), actions);
        rename->setObjectName(QStringLiteral("rename-%1").arg(habit.id));
        rename->setProperty("compact", true);
        actionLayout->addWidget(rename);
        connect(rename, &QPushButton::clicked, this, [this, id = habit.id, name = habit.name] { editHabit(id, name); });
        auto remove = new QPushButton(QStringLiteral("Удалить"), actions);
        remove->setObjectName(QStringLiteral("delete-%1").arg(habit.id));
        remove->setProperty("compact", true);
        remove->setProperty("danger", true);
        actionLayout->addWidget(remove);
        connect(remove, &QPushButton::clicked, this, [this, id = habit.id, name = habit.name] {
            QMessageBox dialog(QMessageBox::Question, QStringLiteral("Удалить привычку?"),
                QStringLiteral("Удалить «%1»? Вся история выполнения будет удалена.").arg(name),
                QMessageBox::Yes | QMessageBox::Cancel, this);
            dialog.button(QMessageBox::Yes)->setText(QStringLiteral("Удалить"));
            dialog.button(QMessageBox::Cancel)->setText(QStringLiteral("Отмена"));
            dialog.setDefaultButton(QMessageBox::Cancel);
            dialog.setEscapeButton(QMessageBox::Cancel);
            if (dialog.exec() == QMessageBox::Yes) {
                QString error;
                const bool deleted = store.deleteHabit(id, error);
                reload();
                if (!deleted) displayError(error);
            }
        });
        grid->setCellWidget(row, 9, actions);
        for (int i = 0; i < 7; ++i) {
            const auto day = selectedWeek.monday.addDays(i);
            auto cell = new QTableWidgetItem;
            cell->setFlags(canChangeCompletion(day, today)
                ? Qt::ItemFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable)
                : Qt::NoItemFlags);
            cell->setCheckState(habit.completedDays.contains(day) ? Qt::Checked : Qt::Unchecked);
            cell->setData(Qt::UserRole, habit.id);
            cell->setData(Qt::UserRole + 1, day);
            cell->setToolTip(day.toString(QStringLiteral("dd.MM.yyyy")));
            if (habit.completedDays.contains(day)) cell->setBackground(QColor(QStringLiteral("#dcebd9")));
            else if (day == today) cell->setBackground(QColor(QStringLiteral("#f6ccd9")));
            grid->setItem(row, i + 2, cell);
        }
    }
}

void AppWindow::editHabit(qint64 id, const QString &initialName)
{
    QDialog dialog(this);
    dialog.setWindowTitle(id ? QStringLiteral("Переименовать привычку") : QStringLiteral("Новая привычка"));
    dialog.setMinimumWidth(420);
    auto layout = new QVBoxLayout(&dialog);
    auto caption = new QLabel(QStringLiteral("Название привычки"), &dialog);
    auto input = new QLineEdit(initialName, &dialog);
    input->setObjectName(QStringLiteral("habitName"));
    caption->setBuddy(input);
    auto error = new QLabel(&dialog);
    error->setObjectName(QStringLiteral("nameError"));
    error->setWordWrap(true);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Save)->setText(QStringLiteral("Сохранить"));
    buttons->button(QDialogButtonBox::Save)->setObjectName(QStringLiteral("saveHabit"));
    buttons->button(QDialogButtonBox::Save)->setDefault(true);
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("Отмена"));
    layout->addWidget(caption);
    layout->addWidget(input);
    layout->addWidget(error);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        QString message;
        qint64 createdId;
        const bool saved = id ? store.renameHabit(id, input->text(), message)
                              : store.createHabit(input->text(), createdId, message);
        if (saved) dialog.accept();
        else error->setText(message);
    });
    input->setFocus();
    input->selectAll();
    if (dialog.exec() == QDialog::Accepted)
        reload();
}

void AppWindow::renderStatistics()
{
    const auto bars7 = completionChart(habits, todayShown, 7);
    const auto bars30 = completionChart(habits, todayShown, 30);
    chart7->setBars(bars7);
    chart30->setBars(bars30);
    int sum7 = 0, sum30 = 0;
    for (const auto &bar : bars7) sum7 += bar.count;
    for (const auto &bar : bars30) sum30 += bar.count;
    total7->setText(QString::number(sum7));
    total30->setText(QString::number(sum30));
    summary->setRowCount(habits.size());
    for (int row = 0; row < habits.size(); ++row) {
        const auto values = summarizeHabit(habits[row], todayShown);
        auto name = new QTableWidgetItem(habits[row].name);
        name->setToolTip(habits[row].name);
        summary->setItem(row, 0, name);
        summary->setItem(row, 1, new QTableWidgetItem(QString::number(values.streak)));
        summary->setItem(row, 2, new QTableWidgetItem(QString::number(values.last7Days)));
        summary->setItem(row, 3, new QTableWidgetItem(QString::number(values.last30Days)));
    }
}
