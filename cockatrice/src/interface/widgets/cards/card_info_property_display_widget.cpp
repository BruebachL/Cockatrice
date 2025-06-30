#include "card_info_property_display_widget.h"

CardInfoPropertyDisplayWidget::CardInfoPropertyDisplayWidget(QWidget *parent,
                                                             const QString &_propertyName,
                                                             const QString &_valueText)
    : QFrame(parent)
{
    layout = new QVBoxLayout(this);

    propertyLabel = new QLabel(_propertyName, this);
    propertyLabel->setAlignment(Qt::AlignLeft);

    valueLabel = new QLabel(_valueText, this);
    valueLabel->setAlignment(Qt::AlignRight);

    layout->addWidget(propertyLabel);
    layout->addWidget(valueLabel);

    setLayout(layout);
}
