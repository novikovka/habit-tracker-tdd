#include "CompletionChart.h"
#include <QPainter>
#include <QFontMetricsF>
#include <QtMath>

CompletionChart::CompletionChart(const QString &chartTitle, QWidget *parent)
    : QWidget(parent), title(chartTitle)
{
    setMinimumSize(300, 140);
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
    painter.setPen(QColor(QStringLiteral("#e8ded8")));
    painter.setBrush(QColor(QStringLiteral("#fffdfb")));
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 16, 16);
    painter.setPen(QColor(QStringLiteral("#463d42")));
    auto heading = font();
    heading.setPixelSize(17);
    heading.setWeight(QFont::DemiBold);
    painter.setFont(heading);
    const auto headingText = title.section(QChar(0x00b7), 0, 0).trimmed();
    painter.drawText(QRect(20, 12, width() - 40, 25), Qt::AlignLeft | Qt::AlignVCenter, headingText);
    auto axis = font();
    axis.setPixelSize(12);
    painter.setFont(axis);
    painter.setPen(QColor(QStringLiteral("#776870")));
    painter.drawText(QRect(20, 39, width() - 40, 17), Qt::AlignLeft | Qt::AlignVCenter,
                     title.section(QChar(0x00b7), 1).trimmed());
    if (bars.isEmpty()) return;
    int maximum = 1;
    for (const auto &bar : bars) maximum = qMax(maximum, bar.count);
    const int step = qMax(1, (maximum + 4) / 5);
    maximum = ((maximum + step - 1) / step) * step;
    const QRectF area(50, 78, width() - 74, height() - 118);
    for (int value = 0; value <= maximum; value += step) {
        const auto y = area.bottom() - area.height() * value / maximum;
        painter.setPen(QColor(QStringLiteral("#eee6e0")));
        painter.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
        if (value == 0 || value == maximum || area.height() * step / maximum >= 22) {
            painter.setPen(QColor(QStringLiteral("#75676e")));
            painter.drawText(QRectF(5, y - 9, 35, 18), Qt::AlignRight | Qt::AlignVCenter, QString::number(value));
        }
    }
    const qreal slot = area.width() / bars.size();
    const QFontMetricsF metrics(axis);
    for (int i = 0; i < bars.size(); ++i) {
        const auto x = area.left() + slot * i;
        const auto height = area.height() * bars[i].count / maximum;
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(i + 1 == bars.size() ? QStringLiteral("#a75173") : QStringLiteral("#e8b9cb")));
        painter.drawRoundedRect(QRectF(x + slot * .18, area.bottom() - qMax(2.0, height), slot * .64, qMax(2.0, height)), 3, 3);
        const auto valueText = QString::number(bars[i].count);
        if (area.height() >= 72 && (bars.size() <= 7 ||
            (bars[i].count > 0 && slot >= metrics.horizontalAdvance(valueText) + 3))) {
            painter.setPen(QColor(QStringLiteral("#66505d")));
            painter.drawText(QRectF(x, area.bottom() - height - 21, slot, 18), Qt::AlignCenter, valueText);
        }
    }
    const qreal labelWidth = metrics.horizontalAdvance(QStringLiteral("00.00")) + 6;
    const int every = qMax(1, qCeil((labelWidth + 12) / slot));
    const qreal lastCenter = area.left() + slot * (bars.size() - .5);
    const qreal lastLeft = qBound(14.0, lastCenter - labelWidth / 2, width() - labelWidth - 14);
    qreal previousRight = -1000;
    for (int i = 0; i < bars.size(); ++i) {
        const bool last = i + 1 == bars.size();
        if (!last && i % every != 0) continue;
        const qreal center = area.left() + slot * (i + .5);
        const qreal left = qBound(14.0, center - labelWidth / 2, width() - labelWidth - 14);
        const QRectF label(left, area.bottom() + 10, labelWidth, 18);
        if (!last && i != 0 && label.right() + 8 > lastLeft) continue;
        if (label.left() < previousRight + 8) continue;
        painter.setPen(QColor(QStringLiteral("#75676e")));
        painter.drawText(label, Qt::AlignCenter, bars[i].day.toString(QStringLiteral("dd.MM")));
        previousRight = label.right();
    }
}
