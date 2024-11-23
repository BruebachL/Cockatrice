#include "all_zones_card_amount_widget.h"

AllZonesCardAmountWidget::AllZonesCardAmountWidget(QWidget *parent,
                                                   TabDeckEditor *deckEditor,
                                                   DeckListModel *deckModel,
                                                   QTreeView *deckView,
                                                   CardInfoPtr rootCard,
                                                   CardInfoPerSet setInfoForCard)
    : QWidget(parent), deckEditor(deckEditor), deckModel(deckModel), deckView(deckView), rootCard(rootCard),
      setInfoForCard(setInfoForCard)
{
    layout = new QVBoxLayout(this);
    setLayout(layout);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    zoneLabelMainboard = new QLabel(tr("Mainboard"), this);
    buttonBoxMainboard =
        new CardAmountWidget(this, deckEditor, deckModel, deckView, rootCard, setInfoForCard, DECK_ZONE_MAIN);
    zoneLabelSideboard = new QLabel(tr("Sideboard"), this);
    buttonBoxSideboard =
        new CardAmountWidget(this, deckEditor, deckModel, deckView, rootCard, setInfoForCard, DECK_ZONE_SIDE);

    layout->addWidget(zoneLabelMainboard, 0, Qt::AlignCenter);
    layout->addWidget(buttonBoxMainboard, 0, Qt::AlignCenter);
    layout->addWidget(zoneLabelSideboard, 0, Qt::AlignCenter);
    layout->addWidget(buttonBoxSideboard, 0, Qt::AlignCenter);
}
