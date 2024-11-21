#ifndef PRINTING_SELECTOR_H
#define PRINTING_SELECTOR_H

#include "../../../../deck/deck_list_model.h"
#include "../../../../deck/deck_view.h"
#include "../../../../game/cards/card_database.h"
#include "../general/layout_containers/flow_widget.h"

#include <QComboBox>
#include <QPushButton>
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
    QList<CardInfoPerSet> filterSets(const QList<CardInfoPerSet> &sets) const;
    void getAllSetsForCurrentCard();

public slots:
    void updateDisplay();
    void updateSortOrder();

private:
    QVBoxLayout *layout;
    QHBoxLayout *sortToolBar;
    QComboBox *sortOptionsSelector;
    bool descendingSort;
    QPushButton *toggleSortOrder;
    QLineEdit *searchBar;
    QTimer *searchDebounceTimer;
    FlowWidget *flowWidget;
    TabDeckEditor *deckEditor;
    DeckListModel *deckModel;
    QTreeView *deckView;
    CardInfoPtr selectedCard;
    QString currentZone;
};

#endif // PRINTING_SELECTOR_H
