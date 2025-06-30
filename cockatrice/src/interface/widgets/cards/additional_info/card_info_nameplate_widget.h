#ifndef CARD_INFO_NAMEPLATE_WIDGET_H
#define CARD_INFO_NAMEPLATE_WIDGET_H

#include "../../general/display/dynamic_font_size_label.h"

#include <QHBoxLayout>
#include <QWidget>

class CardInfoNameplateWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CardInfoNameplateWidget(QWidget *parent, CardInfoPtr card);

private:
    CardInfoPtr card;
    QHBoxLayout *layout;
    DynamicFontSizeLabel *label;
};

#endif // CARD_INFO_NAMEPLATE_WIDGET_H
