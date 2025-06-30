#ifndef ABSTRACT_CARD_HOVER_ITEM_H
#define ABSTRACT_CARD_HOVER_ITEM_H
#include "../../interface/widgets/cards/card_info_edit_widget.h"
#include "../../interface/widgets/cards/card_info_picture_widget.h"
#include "abstract_card_item.h"

#include <QHBoxLayout>
#include <QWidget>

class AbstractCardItem;
class AbstractCardHoverItem : public QWidget
{
    Q_OBJECT
public:
    explicit AbstractCardHoverItem(AbstractCardItem *parent);
    void updateCard();
    AbstractCardItem *parent;

private:
    QHBoxLayout *layout;
    CardInfoPictureWidget *pictureWidget;
    CardInfoEditWidget *editWidget;

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
};

#endif // ABSTRACT_CARD_HOVER_ITEM_H
