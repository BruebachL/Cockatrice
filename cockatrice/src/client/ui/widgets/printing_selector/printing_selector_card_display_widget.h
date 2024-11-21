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
    PrintingSelectorCardDisplayWidget(QWidget *parent,
                                      TabDeckEditor *deckEditor,
                                      DeckListModel *deckModel,
                                      QTreeView *deckView,
                                      QSlider *cardSizeSlider,
                                      CardInfoPtr &rootCard,
                                      CardInfoPerSet &setInfoForCard,
                                      QString &currentZone);
    void resizeEvent(QResizeEvent *event);
    int countCardsInZone(const QString &);

private:
    QVBoxLayout *layout;
    QWidget *buttonBoxMainboardContainer;
    QHBoxLayout *buttonBoxMainboard;
    QLabel *buttonBoxMainboardLabel;
    QPushButton *incrementButtonMainboard;
    QPushButton *decrementButtonMainboard;
    QWidget *buttonBoxSideboardContainer;
    QHBoxLayout *buttonBoxSideboard;
    QLabel *buttonBoxSideboardLabel;
    QPushButton *incrementButtonSideboard;
    QPushButton *decrementButtonSideboard;
    TabDeckEditor *deckEditor;
    DeckListModel *deckModel;
    QTreeView *deckView;
    QSlider *cardSizeSlider;
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
    void decrementCardHelper(const QString &zoneName);
    void recursiveExpand(const QModelIndex &index);

private slots:
    void addPrintingMainboard();
    void addPrintingSideboard();
    void removePrintingMainboard();
    void removePrintingSideboard();
    void updateCardCounts();
};

#endif // PRINTING_SELECTOR_CARD_DISPLAY_WIDGET_H
