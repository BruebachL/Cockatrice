#include "card_info_edit_widget.h"

#include "../../../card/game_specific_terms.h"
#include "../../../game/board/card_item.h"

#include <QGridLayout>
#include <QLabel>
#include <QTextEdit>

CardInfoEditWidget::CardInfoEditWidget(QWidget *parent, CardInfoPtr _info) : QFrame(parent), info(_info)
{
    layout = new QGridLayout(this);
    if (info) {
        qInfo() << "Card Info Edit Widget created with card: " << info->getName();
    } else {
        qInfo() << "Card Info Edit Widget created with nullptr card";
    }
    cardPropertiesDisplayWidget = new CardInfoPropertyEditWidget(this, info);

    layout->addWidget(cardPropertiesDisplayWidget, 0, 0);

    retranslateUi();
}

void CardInfoEditWidget::setCard(CardInfoPtr card)
{
    info = card;
    cardPropertiesDisplayWidget->setCard(card);
}

void CardInfoEditWidget::setInvalidCardName(const QString &cardName)
{
    Q_UNUSED(cardName);
}

void CardInfoEditWidget::retranslateUi()
{
    /*
     * There's no way we can really translate the text currently being rendered.
     * The best we can do is invalidate the current text.
     */
    setInvalidCardName("");
}
