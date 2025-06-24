#ifndef ABSTRACT_CARD_HOVER_ITEM_H
#define ABSTRACT_CARD_HOVER_ITEM_H
#include "../../client/ui/widgets/cards/card_info_edit_widget.h"
#include "abstract_card_item.h"

#include <QVBoxLayout>
#include <QWidget>

class AbstractCardItem;
class AbstractCardHoverItem : public QWidget
{
    Q_OBJECT
public:
    explicit AbstractCardHoverItem(AbstractCardItem *parent);
    AbstractCardItem *parent;

private:
    QVBoxLayout *layout;
    CardInfoEditWidget *editWidget;

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
};

#endif // ABSTRACT_CARD_HOVER_ITEM_H
