#include "abstract_card_hover_item.h"

#include "../../database/card_database_manager.h"
#include "../../interface/widgets/cards/card_info_edit_widget.h"
#include "../../interface/widgets/cards/card_info_picture_widget.h"

#include <QEvent>

AbstractCardHoverItem::AbstractCardHoverItem(AbstractCardItem *parent) : QWidget(nullptr), parent(parent)
{
    setWindowFlags(Qt::ToolTip);
    setAttribute(Qt::WA_Hover);
    setMouseTracking(true); // Ensures enter/leave events are delivered

    layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    pictureWidget = new CardInfoPictureWidget(this);
    pictureWidget->setCard(CardDatabaseManager::query()->getCard(parent->getCardRef()));
    pictureWidget->setMinimumWidth(200);
    editWidget = new CardInfoEditWidget(this, parent->getCard().getCardPtr());
    layout->addWidget(pictureWidget);
    layout->addWidget(editWidget);
}

void AbstractCardHoverItem::updateCard()
{
    editWidget->setCard(parent->getCard().getCardPtr());
    pictureWidget->setCard(parent->getCard());
}

void AbstractCardHoverItem::enterEvent(QEnterEvent *event)
{
    if (parent) {
        qInfo() << "AbstractCardHoverItem is cancelling any outstanding hides since the mouse entered it";
        parent->cancelHideTimer(); // prevent hiding while hovering
    }

    QWidget::enterEvent(event);
}

void AbstractCardHoverItem::leaveEvent(QEvent *event)
{
    if (parent) {
        qInfo() << "AbstractCardHoverItem is requesting to be hidden since the mouse left it";
        parent->startHideTimerIfNotHovered(); // check both widgets before hiding
    }

    QWidget::leaveEvent(event);
}
