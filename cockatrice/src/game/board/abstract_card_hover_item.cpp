#include "abstract_card_hover_item.h"
#include <QEvent>

AbstractCardHoverItem::AbstractCardHoverItem(AbstractCardItem *parent)
    : QWidget(this->window()), parent(parent)
{
    setWindowFlags(Qt::ToolTip);
    setAttribute(Qt::WA_Hover);
    setMouseTracking(true); // Ensures enter/leave events are delivered

    layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    editWidget = new CardInfoEditWidget(this, parent->getInfo());
    layout->addWidget(editWidget);
}

void AbstractCardHoverItem::enterEvent(QEnterEvent *event)
{
    if (parent)
        parent->cancelHideTimer();  // prevent hiding while hovering

    QWidget::enterEvent(event);
}

void AbstractCardHoverItem::leaveEvent(QEvent *event)
{
    if (parent)
        parent->startHideTimerIfNotHovered();  // check both widgets before hiding

    QWidget::leaveEvent(event);
}
