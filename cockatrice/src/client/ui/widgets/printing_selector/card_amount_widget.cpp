#include "card_amount_widget.h"

CardAmountWidget::CardAmountWidget(QWidget *parent,
                                   TabDeckEditor *deckEditor,
                                   DeckListModel *deckModel,
                                   QTreeView *deckView,
                                   CardInfoPtr &rootCard,
                                   CardInfoPerSet &setInfoForCard,
                                   QString zoneName)
    : QWidget(parent), deckEditor(deckEditor), deckModel(deckModel), deckView(deckView), rootCard(rootCard),
      setInfoForCard(setInfoForCard), zoneName(zoneName)
{
    layout = new QHBoxLayout(this);
    this->setLayout(layout);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    incrementButton = new QPushButton("+", this);
    decrementButton = new QPushButton("-", this);

    if (zoneName == DECK_ZONE_MAIN) {
        connect(incrementButton, SIGNAL(clicked()), this, SLOT(addPrintingMainboard()));
        connect(decrementButton, SIGNAL(clicked()), this, SLOT(removePrintingMainboard()));
    } else if (zoneName == DECK_ZONE_SIDE) {
        connect(incrementButton, SIGNAL(clicked()), this, SLOT(addPrintingSideboard()));
        connect(decrementButton, SIGNAL(clicked()), this, SLOT(removePrintingSideboard()));
    }

    cardCountInZone = new QLabel(QString::number(countCardsInZone(zoneName)), this);

    layout->addWidget(decrementButton);
    layout->addWidget(cardCountInZone, 0, Qt::AlignCenter);
    layout->addWidget(incrementButton);

    connect(deckModel, &DeckListModel::dataChanged, this, &CardAmountWidget::updateCardCount);
    connect(deckModel, &QAbstractItemModel::rowsRemoved, this, &CardAmountWidget::updateCardCount);
}

void CardAmountWidget::updateCardCount()
{
    cardCountInZone->setText(QString::number(countCardsInZone(zoneName)));
}

void CardAmountWidget::addPrintingMainboard()
{
    const auto newCardIndex = deckModel->addCard(rootCard->getName(), setInfoForCard, DECK_ZONE_MAIN);
    recursiveExpand(newCardIndex);
    QModelIndex find_card = deckModel->findCard(rootCard->getName(), DECK_ZONE_MAIN);
    if (find_card.isValid() && find_card != newCardIndex) {
        deckModel->removeRow(find_card.row(), find_card.parent());
    };
}

void CardAmountWidget::addPrintingSideboard()
{
    const auto newCardIndex = deckModel->addCard(rootCard->getName(), setInfoForCard, DECK_ZONE_SIDE);
    recursiveExpand(newCardIndex);
}

void CardAmountWidget::removePrintingMainboard()
{
    decrementCardHelper(DECK_ZONE_MAIN);
}

void CardAmountWidget::removePrintingSideboard()
{
    decrementCardHelper(DECK_ZONE_SIDE);
}

void CardAmountWidget::recursiveExpand(const QModelIndex &index)
{
    if (index.parent().isValid()) {
        recursiveExpand(index.parent());
    }
    deckView->expand(index);
}

void CardAmountWidget::offsetCountAtIndex(const QModelIndex &idx, int offset)
{
    if (!idx.isValid() || offset == 0) {
        return;
    }

    const QModelIndex numberIndex = idx.sibling(idx.row(), 0);
    const int count = deckModel->data(numberIndex, Qt::EditRole).toInt();
    const int new_count = count + offset;
    deckView->setCurrentIndex(numberIndex);
    if (new_count <= 0) {
        deckModel->removeRow(idx.row(), idx.parent());
    } else {
        deckModel->setData(numberIndex, new_count, Qt::EditRole);
    }
    deckEditor->setModified(true);
}

void CardAmountWidget::decrementCardHelper(const QString &zone)
{
    QModelIndex idx;
    idx = deckModel->findCard(rootCard->getName(), zone, setInfoForCard.getProperty("uuid"));
    offsetCountAtIndex(idx, -1);
}

int CardAmountWidget::countCardsInZone(const QString &deckZone)
{
    int count = 0;

    if (!deckModel) {
        return -1;
    }

    DeckList *decklist = deckModel->getDeckList();
    if (!decklist) {
        return -1;
    }

    InnerDecklistNode *listRoot = decklist->getRoot();
    if (!listRoot) {
        return -1;
    }

    for (auto *i : *listRoot) {
        auto *countCurrentZone = dynamic_cast<InnerDecklistNode *>(i);
        if (!countCurrentZone) {
            continue;
        }

        if (countCurrentZone->getName() != deckZone) {
            continue;
        }

        for (auto *cardNode : *countCurrentZone) {
            auto *currentCard = dynamic_cast<DecklistCardNode *>(cardNode);
            if (!currentCard) {
                continue;
            }

            for (int k = 0; k < currentCard->getNumber(); ++k) {
                if (currentCard->getCardProviderId() == setInfoForCard.getProperty("uuid")) {
                    count++;
                }
            }
        }
    }
    return count;
}