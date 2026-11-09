#pragma once
#include <QWidget>
#include "HabitLogic.h"

class CompletionChart : public QWidget {
public:
    explicit CompletionChart(const QString &title, QWidget *parent = nullptr);
    void setBars(QVector<DailyCount> bars);
protected:
    void paintEvent(QPaintEvent *) override;
private:
    QString title;
    QVector<DailyCount> bars;
};
