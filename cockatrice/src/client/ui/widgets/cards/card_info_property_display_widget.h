#ifndef CARD_INFO_PROPERTY_DISPLAY_WIDGET_H
#define CARD_INFO_PROPERTY_DISPLAY_WIDGET_H

#include "../../../../game/cards/card_info.h"

#include <QLabel>
#include <QFrame>
#include <QGridLayout>

class CardInfoPropertyDisplayWidget : public QFrame
{
    Q_OBJECT

private:
    QGridLayout *layout;
    QLabel *propertyLabel;
    QLabel *valueLabel;

public:
    CardInfoPropertyDisplayWidget(QWidget *parent, const QString &_propertyName, const QString &_valueText);
};

#endif //CARD_INFO_PROPERTY_DISPLAY_WIDGET_H
