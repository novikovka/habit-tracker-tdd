#include "CompletionDelegate.h"
#include <QApplication>
#include <QPainter>
#include <QPainterPath>

void CompletionDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    QStyleOptionViewItem panel(option);
    initStyleOption(&panel, index);
    panel.features.setFlag(QStyleOptionViewItem::HasCheckIndicator, false);
    panel.text.clear();
    const auto style = option.widget ? option.widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &panel, painter, option.widget);

    const bool checked = index.data(Qt::CheckStateRole).toInt() == Qt::Checked;
    const bool enabled = (index.flags() & Qt::ItemIsEnabled) && (option.state & QStyle::State_Enabled);
    const QRectF mark(option.rect.center().x() - 18.0, option.rect.center().y() - 18.0, 36.0, 36.0);
    painter->save();
    painter->setClipRect(option.rect);
    painter->setRenderHint(QPainter::Antialiasing);
    if (!enabled) painter->setOpacity(0.5);
    painter->setPen(QPen(QColor(checked ? "#b7d4c2" : "#d8c9c1"), 1.5));
    painter->setBrush(QColor(checked ? "#dceee3" : enabled ? "#fffdfb" : "#eeebe7"));
    painter->drawRoundedRect(mark, 10, 10);
    if (checked) {
        QPainterPath tick;
        tick.moveTo(mark.left() + 10, mark.top() + 18);
        tick.lineTo(mark.left() + 16, mark.top() + 24);
        tick.lineTo(mark.left() + 27, mark.top() + 12);
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(QColor("#3e7159"), 2.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->drawPath(tick);
    } else if (enabled) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor("#b79ca5"));
        painter->drawEllipse(mark.center(), 2.1, 2.1);
    } else {
        painter->setPen(QPen(QColor("#9c918a"), 1.5, Qt::SolidLine, Qt::RoundCap));
        painter->drawLine(mark.center() - QPointF(5, 0), mark.center() + QPointF(5, 0));
    }
    painter->restore();
}
