#include "card_info_nameplate_widget.h"

CardInfoNameplateWidget::CardInfoNameplateWidget(QWidget *parent, CardInfoPtr _card) : QWidget(parent), card(_card)
{
    layout = new QHBoxLayout(this);
    setLayout(layout);

    label = new DynamicFontSizeLabel(this);
    label->setText(card->getName());

    layout->addWidget(label);

    setFixedHeight(50);

    // Allow expansion in width
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}