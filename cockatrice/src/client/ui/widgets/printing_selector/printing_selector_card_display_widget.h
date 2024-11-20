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
    int countCardsInZone(const QString&);

private:
    QVBoxLayout *layout;
    QHBoxLayout *buttonBoxMainboard;
    QPushButton *incrementButtonMainboard;
    QPushButton *decrementButtonMainboard;
    QHBoxLayout *buttonBoxSideboard;
    QPushButton *incrementButtonSideboard;
    QPushButton *decrementButtonSideboard;
    TabDeckEditor *deckEditor;
    DeckListModel *deckModel;
    QTreeView *deckView;
    CardInfoPtr rootCard;
    CardInfoPtr setCard;
    CardInfoPerSet setInfoForCard;
    QString currentZone;

    CardInfoPictureWidget *cardInfoPicture;
    QLabel *cardCountMainboard;
    QLabel *cardCountSideboard;
    QLabel *setName;
    QLabel *setNumber;

    void offsetCountAtIndex(const QModelIndex &idx, int offset);
    void decrementCardHelper(const QString& zoneName);

private slots:
    void addPrintingMainboard();
    void addPrintingSideboard();
    void removePrintingMainboard();
    void removePrintingSideboard();
    void updateCardCounts();
};

#endif // PRINTING_SELECTOR_CARD_DISPLAY_WIDGET_H
