#ifndef PRINTING_SELECTOR_CARD_DISPLAY_WIDGET_H
#define PRINTING_SELECTOR_CARD_DISPLAY_WIDGET_H

#include "../../../../client/ui/widgets/cards/card_info_picture_widget.h"
#include "../../../../deck/deck_list_model.h"
#include "../../../../deck/deck_view.h"
#include "../../../../game/cards/card_database.h"
#include "../../../tabs/tab_deck_editor.h"

#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>

class PrintingSelectorCardDisplayWidget : public QWidget
{
    Q_OBJECT

public:
    PrintingSelectorCardDisplayWidget(TabDeckEditor *deckEditor,
                                      DeckListModel *deckModel,
                                      QTreeView *deckView,
                                      CardInfoPtr &rootCard,
                                      CardInfoPerSet &setInfoForCard,
                                      QString &currentZone,
                                      QWidget *parent = nullptr);
    int countCards();

private:
    QVBoxLayout *layout;
    QHBoxLayout *buttonBox;
    QPushButton *incrementButton;
    QPushButton *decrementButton;
    TabDeckEditor *deckEditor;
    DeckListModel *deckModel;
    QTreeView *deckView;
    CardInfoPtr rootCard;
    CardInfoPtr setCard;
    CardInfoPerSet setInfoForCard;
    QString currentZone;

    CardInfoPictureWidget *cardInfoPicture;
    QLabel *cardCount;
    QLabel *setName;
    QLabel *setNumber;

    void offsetCountAtIndex(const QModelIndex &idx, int offset);
    void decrementCardHelper(QString zoneName);

private slots:
    void addPrinting();
    void removePrinting();
};

#endif // PRINTING_SELECTOR_CARD_DISPLAY_WIDGET_H
