#include "card_info_property_display_widget.h"

CardInfoPropertyDisplayWidget::CardInfoPropertyDisplayWidget(QWidget *parent,
                                                             const QString &_propertyName,
                                                             const QString &_valueText)
    : QFrame(parent)
{
    layout = new QGridLayout(this);

    propertyLabel = new QLabel(_propertyName, this);

    valueLabel = new QLabel(_valueText, this);

    layout->addWidget(propertyLabel, 0, 0, 1, 1);
    layout->addWidget(valueLabel, 0, 1, 1, 1);

    setLayout(layout);
}
