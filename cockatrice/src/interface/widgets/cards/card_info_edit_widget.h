#ifndef CARD_INFO_EDIT_WIDGET_H
#define CARD_INFO_EDIT_WIDGET_H

#include "../../../card/card_info.h"
#include "card_info_property_edit_widget.h"

#include <QFrame>
#include <QGridLayout>
class QLabel;
class QTextEdit;

class CardInfoEditWidget : public QFrame
{
    Q_OBJECT

private:
    CardInfoPtr info;
    QGridLayout *layout;
    CardInfoPropertyEditWidget *cardPropertiesDisplayWidget;

public:
    explicit CardInfoEditWidget(QWidget *parent, CardInfoPtr info);
    void retranslateUi();
    void setInvalidCardName(const QString &cardName);

signals:
    void linkActivated(const QString &link);
public slots:
    void setCard(CardInfoPtr card);
};

#endif // CARD_INFO_EDIT_WIDGET_H
