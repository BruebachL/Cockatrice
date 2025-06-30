#ifndef CARD_INFO_PROPERTY_DISPLAY_WIDGET_H
#define CARD_INFO_PROPERTY_DISPLAY_WIDGET_H

#include "../../../card/card_info.h"

#include <QFrame>
#include <QGridLayout>
#include <QLabel>

class CardInfoPropertyDisplayWidget : public QFrame
{
    Q_OBJECT

private:
    QVBoxLayout *layout;
    QLabel *propertyLabel;
    QLabel *valueLabel;

public:
    CardInfoPropertyDisplayWidget(QWidget *parent, const QString &_propertyName, const QString &_valueText);
};

#endif // CARD_INFO_PROPERTY_DISPLAY_WIDGET_H
