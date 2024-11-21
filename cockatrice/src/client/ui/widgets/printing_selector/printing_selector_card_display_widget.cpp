#include "printing_selector_card_display_widget.h"

#include "../../../../deck/deck_loader.h"
#include "../../../../game/cards/card_database_manager.h"

#include <QLabel>
#include <QVBoxLayout>

PrintingSelectorCardDisplayWidget::PrintingSelectorCardDisplayWidget(QWidget *parent,
                                                                     TabDeckEditor *deckEditor,
                                                                     DeckListModel *deckModel,
                                                                     QTreeView *deckView,
                                                                     QSlider *cardSizeSlider,
                                                                     CardInfoPtr &rootCard,
                                                                     CardInfoPerSet &setInfoForCard,
                                                                     QString &currentZone)
    : QWidget(parent), deckEditor(deckEditor), deckModel(deckModel), deckView(deckView), cardSizeSlider(cardSizeSlider),
      rootCard(rootCard), setInfoForCard(setInfoForCard), currentZone(currentZone)
{
    layout = new QVBoxLayout(this);
    setLayout(layout);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    cardInfoPicture = new CardInfoPictureWidget(this);
    cardInfoPicture->setMinimumSize(0, 0);
    setCard = CardDatabaseManager::getInstance()->getCardByNameAndProviderId(rootCard->getName(),
                                                                             setInfoForCard.getProperty("uuid"));
    cardInfoPicture->setCard(setCard);
    layout->addWidget(cardInfoPicture, 0, Qt::AlignHCenter | Qt::AlignTop);

    connect(cardSizeSlider, &QSlider::valueChanged, cardInfoPicture, &CardInfoPictureWidget::setScaleFactor);

    mainboardAndSideboardButtonBoxContainer = new QWidget(this);
    mainboardAndSideboardButtonBoxLayout = new QVBoxLayout(this);
    mainboardAndSideboardButtonBoxContainer->setLayout(mainboardAndSideboardButtonBoxLayout);

    buttonBoxMainboard = new QHBoxLayout(this);
    buttonBoxMainboardContainer = new QWidget(this);
    buttonBoxMainboardContainer->setLayout(buttonBoxMainboard);
    buttonBoxMainboardContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    buttonBoxMainboardLabel = new QLabel(this);
    buttonBoxMainboardLabel->setText(tr("Mainboard"));

    incrementButtonMainboard = new QPushButton("+", buttonBoxMainboardContainer);
    connect(incrementButtonMainboard, SIGNAL(clicked()), this, SLOT(addPrintingMainboard()));
    decrementButtonMainboard = new QPushButton("-", buttonBoxMainboardContainer);
    connect(decrementButtonMainboard, SIGNAL(clicked()), this, SLOT(removePrintingMainboard()));

    cardCountMainboard = new QLabel(QString::number(countCardsInZone(DECK_ZONE_MAIN)), buttonBoxMainboardContainer);

    buttonBoxMainboard->addWidget(decrementButtonMainboard);
    buttonBoxMainboard->addWidget(cardCountMainboard, 0, Qt::AlignCenter);
    buttonBoxMainboard->addWidget(incrementButtonMainboard);

    buttonBoxSideboard = new QHBoxLayout(this);
    buttonBoxSideboardContainer = new QWidget(this);
    buttonBoxSideboardContainer->setLayout(buttonBoxSideboard);
    buttonBoxSideboardContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    buttonBoxSideboardLabel = new QLabel(this);
    buttonBoxSideboardLabel->setText(tr("Sideboard"));

    incrementButtonSideboard = new QPushButton("+", buttonBoxSideboardContainer);
    connect(incrementButtonSideboard, SIGNAL(clicked()), this, SLOT(addPrintingSideboard()));
    decrementButtonSideboard = new QPushButton("-", buttonBoxSideboardContainer);
    connect(decrementButtonSideboard, SIGNAL(clicked()), this, SLOT(removePrintingSideboard()));

    cardCountSideboard = new QLabel(QString::fromStdString(std::to_string(countCardsInZone(DECK_ZONE_SIDE))),
                                    buttonBoxSideboardContainer);

    buttonBoxSideboard->addWidget(decrementButtonSideboard);
    buttonBoxSideboard->addWidget(cardCountSideboard, 0, Qt::AlignCenter);
    buttonBoxSideboard->addWidget(incrementButtonSideboard);

    mainboardAndSideboardButtonBoxLayout->addWidget(buttonBoxMainboardLabel, 0, Qt::AlignCenter);
    mainboardAndSideboardButtonBoxLayout->addWidget(buttonBoxMainboardContainer, 0, Qt::AlignCenter);

    mainboardAndSideboardButtonBoxLayout->addWidget(buttonBoxSideboardLabel, 0, Qt::AlignCenter);
    mainboardAndSideboardButtonBoxLayout->addWidget(buttonBoxSideboardContainer, 0, Qt::AlignCenter);

    layout->addWidget(mainboardAndSideboardButtonBoxContainer, 0, Qt::AlignCenter);

    connect(deckModel, &DeckListModel::dataChanged, this, &PrintingSelectorCardDisplayWidget::updateCardCounts);
    connect(deckModel, &QAbstractItemModel::rowsRemoved, this, &PrintingSelectorCardDisplayWidget::updateCardCounts);

    setAndNumberContainer = new QWidget(this);
    setAndNumberLayout = new QVBoxLayout(this);
    setAndNumberContainer->setLayout(setAndNumberLayout);

    setName = new QLabel(setInfoForCard.getPtr()->getLongName() + " (" + setInfoForCard.getPtr()->getShortName() + ")");
    setName->setWordWrap(true);
    setName->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

    setNumber = new QLabel(setInfoForCard.getProperty("num"));

    setAndNumberLayout->addWidget(setName);
    setAndNumberLayout->addWidget(setNumber, 0, Qt::AlignCenter);

    layout->addWidget(setAndNumberContainer, 0, Qt::AlignHCenter | Qt::AlignBottom);
}

void PrintingSelectorCardDisplayWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event); // Ensure the parent class handles the event first

    // Determine the smaller width between cardInfoPicture and buttonBoxMainboardContainer
    int maxWidth = qMax(cardInfoPicture->width(), buttonBoxMainboardContainer->width());

    // Set the maximum width for the setName QLabel
    setName->setMaximumWidth(maxWidth);
}

void PrintingSelectorCardDisplayWidget::updateCardCounts()
{
    cardCountMainboard->setText(QString::number(countCardsInZone(DECK_ZONE_MAIN)));
    cardCountSideboard->setText(QString::number(countCardsInZone(DECK_ZONE_SIDE)));
}

void PrintingSelectorCardDisplayWidget::addPrintingMainboard()
{
    const auto newCardIndex = deckModel->addCard(rootCard->getName(), setInfoForCard, DECK_ZONE_MAIN);
    recursiveExpand(newCardIndex);
    QModelIndex find_card = deckModel->findCard(rootCard->getName(), DECK_ZONE_MAIN);
    if (find_card.isValid() && find_card != newCardIndex) {
        deckModel->removeRow(find_card.row(), find_card.parent());
    };
}

void PrintingSelectorCardDisplayWidget::addPrintingSideboard()
{
    const auto newCardIndex = deckModel->addCard(rootCard->getName(), setInfoForCard, DECK_ZONE_SIDE);
    recursiveExpand(newCardIndex);
}

void PrintingSelectorCardDisplayWidget::removePrintingMainboard()
{
    decrementCardHelper(DECK_ZONE_MAIN);
}

void PrintingSelectorCardDisplayWidget::removePrintingSideboard()
{
    decrementCardHelper(DECK_ZONE_SIDE);
}

void PrintingSelectorCardDisplayWidget::recursiveExpand(const QModelIndex &index)
{
    if (index.parent().isValid()) {
        recursiveExpand(index.parent());
    }
    deckView->expand(index);
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
    } else {
        deckModel->setData(numberIndex, new_count, Qt::EditRole);
    }
    deckEditor->setModified(true);
}

void PrintingSelectorCardDisplayWidget::decrementCardHelper(const QString &zoneName)
{
    QModelIndex idx;
    idx = deckModel->findCard(setCard->getName(), zoneName, setInfoForCard.getProperty("uuid"));
    offsetCountAtIndex(idx, -1);
}

int PrintingSelectorCardDisplayWidget::countCardsInZone(const QString &deckZone)
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
