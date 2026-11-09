#include "CompletionChart.h"
#include <QPainter>

CompletionChart::CompletionChart(const QString &chartTitle, QWidget *parent)
    : QWidget(parent), title(chartTitle)
{
    setMinimumSize(640, 265);
    setAccessibleName(chartTitle);
}

void CompletionChart::setBars(QVector<DailyCount> values)
{
    bars = std::move(values);
    update();
}

void CompletionChart::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QColor(QStringLiteral("#eaded7")));
    painter.setBrush(QColor(QStringLiteral("#fffdfa")));
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 16, 16);
    painter.setPen(QColor(QStringLiteral("#574443")));
    auto heading = font();
    heading.setBold(true);
    painter.setFont(heading);
    painter.drawText(QRect(20, 13, width() - 40, 27), title);
    heading.setBold(false);
    painter.setFont(heading);
    if (bars.isEmpty()) return;
    int maximum = 1;
    for (const auto &bar : bars) maximum = qMax(maximum, bar.count);
    const int step = qMax(1, (maximum + 4) / 5);
    maximum = ((maximum + step - 1) / step) * step;
    const QRectF area(48, 55, width() - 70, height() - 118);
    for (int value = 0; value <= maximum; value += step) {
        const auto y = area.bottom() - area.height() * value / maximum;
        painter.setPen(QColor(QStringLiteral("#eee5df")));
        painter.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
        painter.setPen(QColor(QStringLiteral("#8a7771")));
        painter.drawText(QRectF(5, y - 10, 33, 20), Qt::AlignRight | Qt::AlignVCenter, QString::number(value));
    }
    const qreal slot = area.width() / bars.size();
    for (int i = 0; i < bars.size(); ++i) {
        const auto x = area.left() + slot * i;
        const auto height = area.height() * bars[i].count / maximum;
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(i + 1 == bars.size() ? QStringLiteral("#bd617e") : QStringLiteral("#e7aec1")));
        painter.drawRoundedRect(QRectF(x + slot * .18, area.bottom() - qMax(2.0, height), slot * .64, qMax(2.0, height)), 3, 3);
        if (bars.size() <= 7 || i % 3 == 0 || i + 1 == bars.size()) {
            painter.save();
            painter.setPen(QColor(QStringLiteral("#8a7771")));
            painter.translate(x + slot * .5, area.bottom() + 12);
            painter.rotate(45);
            painter.drawText(QRectF(0, -8, 64, 18), bars[i].day.toString(QStringLiteral("dd.MM")));
            painter.restore();
        }
    }
}
