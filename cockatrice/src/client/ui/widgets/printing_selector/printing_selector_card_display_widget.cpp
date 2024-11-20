#include "printing_selector_card_display_widget.h"

#include "../../../../deck/deck_loader.h"
#include "../../../../game/cards/card_database_manager.h"

#include <QLabel>
#include <QVBoxLayout>

PrintingSelectorCardDisplayWidget::PrintingSelectorCardDisplayWidget(TabDeckEditor *deckEditor,
                                                                     DeckListModel *deckModel,
                                                                     QTreeView *deckView,
                                                                     CardInfoPtr &rootCard,
                                                                     CardInfoPerSet &setInfoForCard,
                                                                     QString &currentZone,
                                                                     QWidget *parent)
    : QWidget(parent), deckEditor(deckEditor), deckModel(deckModel), deckView(deckView), rootCard(rootCard),
      setInfoForCard(setInfoForCard), currentZone(currentZone)
{
    layout = new QVBoxLayout();
    setLayout(layout);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    cardInfoPicture = new CardInfoPictureWidget();
    cardInfoPicture->setMinimumSize(0, 0);
    setCard = CardDatabaseManager::getInstance()->getCardByNameAndProviderId(rootCard->getName(),
                                                                             setInfoForCard.getProperty("uuid"));
    cardInfoPicture->setCard(setCard);
    layout->addWidget(cardInfoPicture);

    buttonBoxMainboard = new QHBoxLayout();

    incrementButtonMainboard = new QPushButton("+");
    connect(incrementButtonMainboard, SIGNAL(clicked()), this, SLOT(addPrintingMainboard()));
    decrementButtonMainboard = new QPushButton("-");
    connect(decrementButtonMainboard, SIGNAL(clicked()), this, SLOT(removePrintingMainboard()));

    cardCountMainboard = new QLabel(QString::number(countCardsInZone(DECK_ZONE_MAIN)));

    buttonBoxMainboard->addWidget(decrementButtonMainboard);
    buttonBoxMainboard->addWidget(cardCountMainboard, 0, Qt::AlignmentFlag::AlignCenter);
    buttonBoxMainboard->addWidget(incrementButtonMainboard);

    auto *buttonBoxMainboardContainer = new QWidget();
    buttonBoxMainboardContainer->setLayout(buttonBoxMainboard);
    buttonBoxMainboardContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    layout->addWidget(buttonBoxMainboardContainer, 0, Qt::AlignCenter);

    buttonBoxSideboard = new QHBoxLayout();

    incrementButtonSideboard = new QPushButton("+");
    connect(incrementButtonSideboard, SIGNAL(clicked()), this, SLOT(addPrintingSideboard()));
    decrementButtonSideboard = new QPushButton("-");
    connect(decrementButtonSideboard, SIGNAL(clicked()), this, SLOT(removePrintingSideboard()));

    cardCountSideboard = new QLabel(QString::fromStdString(std::to_string(countCardsInZone(DECK_ZONE_SIDE))));

    buttonBoxSideboard->addWidget(decrementButtonSideboard);
    buttonBoxSideboard->addWidget(cardCountSideboard, 0, Qt::AlignmentFlag::AlignCenter);
    buttonBoxSideboard->addWidget(incrementButtonSideboard);

    auto *buttonBoxSideboardContainer = new QWidget();
    buttonBoxSideboardContainer->setLayout(buttonBoxSideboard);
    buttonBoxSideboardContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    layout->addWidget(buttonBoxSideboardContainer, 0, Qt::AlignCenter);

    connect(deckModel, &DeckListModel::dataChanged, this, &PrintingSelectorCardDisplayWidget::updateCardCounts);

    setName = new QLabel(setInfoForCard.getPtr()->getLongName() + " (" + setInfoForCard.getPtr()->getShortName() + ")");
    layout->addWidget(setName, 0, Qt::AlignmentFlag::AlignCenter);
    setNumber = new QLabel(setInfoForCard.getProperty("num"));
    layout->addWidget(setNumber, 0, Qt::AlignmentFlag::AlignCenter);
}

void PrintingSelectorCardDisplayWidget::updateCardCounts()
{
    cardCountMainboard->setText(QString::number(countCardsInZone(DECK_ZONE_MAIN)));
    cardCountSideboard->setText(QString::number(countCardsInZone(DECK_ZONE_SIDE)));
}

void PrintingSelectorCardDisplayWidget::addPrintingMainboard()
{
    deckModel->addCard(rootCard->getName(), setInfoForCard, DECK_ZONE_MAIN);
}

void PrintingSelectorCardDisplayWidget::addPrintingSideboard()
{
    deckModel->addCard(rootCard->getName(), setInfoForCard, DECK_ZONE_SIDE);
}

void PrintingSelectorCardDisplayWidget::removePrintingMainboard()
{
    decrementCardHelper(DECK_ZONE_MAIN);
}

void PrintingSelectorCardDisplayWidget::removePrintingSideboard()
{
    decrementCardHelper(DECK_ZONE_SIDE);
}

void PrintingSelectorCardDisplayWidget::offsetCountAtIndex(const QModelIndex &idx, int offset)
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
        updateCardCounts();
    } else {
        deckModel->setData(numberIndex, new_count, Qt::EditRole);
    }
    deckEditor->setModified(true);
}

void PrintingSelectorCardDisplayWidget::decrementCardHelper(const QString& zoneName)
{
    QModelIndex idx;
    idx = deckModel->findCard(setCard->getName(), zoneName, setInfoForCard.getProperty("uuid"));
    offsetCountAtIndex(idx, -1);
}

int PrintingSelectorCardDisplayWidget::countCardsInZone(const QString& deckZone)
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

    for (auto i : *listRoot) {
        auto *countCurrentZone = dynamic_cast<InnerDecklistNode *>(i);
        if (!countCurrentZone) {
            continue;
        }

        for (int j = 0; j < countCurrentZone->size(); j++) {
            if (countCurrentZone->getName() != deckZone) {
                continue;
            }

            auto *currentCard = dynamic_cast<DecklistCardNode *>(countCurrentZone->at(j));
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
