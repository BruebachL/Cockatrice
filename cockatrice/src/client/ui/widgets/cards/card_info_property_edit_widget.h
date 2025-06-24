#ifndef CARD_INFO_PROPERTY_EDIT_WIDGET_H
#define CARD_INFO_PROPERTY_EDIT_WIDGET_H

#include "../../../../game/cards/card_info.h"

#include <QFrame>
#include <QGridLayout>
class QLabel;
class QTextEdit;

class CardInfoPropertyEditWidget : public QFrame
{
    Q_OBJECT

private:
    CardInfoPtr info;
    QGridLayout *layout;
    QLabel *nameLabel;

public:
    explicit CardInfoPropertyEditWidget(QWidget *parent, CardInfoPtr info);
    void retranslateUi();
    void setInvalidCardName(const QString &cardName);

    signals:
        void linkActivated(const QString &link);
public slots:
    void setCard(CardInfoPtr card);
    void generateWidgetsForProperties();
};

#endif //CARD_INFO_PROPERTY_EDIT_WIDGET_H
