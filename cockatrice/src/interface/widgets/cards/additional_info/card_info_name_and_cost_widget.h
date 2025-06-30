#ifndef CARD_INFO_NAME_AND_COST_WIDGET_H
#define CARD_INFO_NAME_AND_COST_WIDGET_H
#include "../../../../database/card_database.h"
#include "card_info_nameplate_widget.h"
#include "mana_cost_widget.h"

#include <QHBoxLayout>
#include <QWidget>

class CardInfoNameAndCostWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CardInfoNameAndCostWidget(QWidget *parent, CardInfoPtr card);

public slots:
    void resizeEvent(QResizeEvent *event) override;

private:
    CardInfoPtr card;
    QHBoxLayout *layout;
    CardInfoNameplateWidget *nameplateWidget;
    ManaCostWidget *manaCostWidget;
};

#endif // CARD_INFO_NAME_AND_COST_WIDGET_H
