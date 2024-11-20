#ifndef PRINTING_SELECTOR_H
#define PRINTING_SELECTOR_H

#include "../../../../deck/deck_list_model.h"
#include "../../../../deck/deck_view.h"
#include "../../../../game/cards/card_database.h"
#include "../general/layout_containers/flow_widget.h"

#include <QComboBox>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>

class TabDeckEditor;
class PrintingSelector : public QWidget
{
    Q_OBJECT

public:
    PrintingSelector(TabDeckEditor *deckEditor,
                     DeckListModel *deckModel,
                     QTreeView *deckView,
                     QWidget *parent = nullptr);
    void setCard(const CardInfoPtr &newCard, const QString &_currentZone);
    CardInfoPerSet getSetForUUID(const QString &uuid);
    QList<CardInfoPerSet> sortSets();
    void getAllSetsForCurrentCard();

public slots:
    void updateDisplay();

private:
    QVBoxLayout *layout;
    QComboBox *sortOptionsSelector;
    FlowWidget *flowWidget;
    TabDeckEditor *deckEditor;
    DeckListModel *deckModel;
    QTreeView *deckView;
    CardInfoPtr selectedCard;
    QString currentZone;
};

#endif // PRINTING_SELECTOR_H
