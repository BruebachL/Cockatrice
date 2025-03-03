#include "card_info_name_and_cost_widget.h"

#include <QResizeEvent>

CardInfoNameAndCostWidget::CardInfoNameAndCostWidget(QWidget *parent, CardInfoPtr _card) : QWidget(parent), card(_card)
{
    layout = new QHBoxLayout(this);
    setLayout(layout);

    nameplateWidget = new CardInfoNameplateWidget(this, card);
    manaCostWidget = new ManaCostWidget(this, card);

    layout->addWidget(nameplateWidget, 4);
    layout->addWidget(manaCostWidget, 1);
}

void CardInfoNameAndCostWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    int totalWidth = event->size().width();
    int manaCostWidth = totalWidth / 5;
    int nameplateWidth = totalWidth - manaCostWidth;

    nameplateWidget->setFixedWidth(nameplateWidth);
    manaCostWidget->setFixedWidth(manaCostWidth);
}