#include "printing_selector.h"

#include "../../../../deck/deck_list_model.h"
#include "../../../../game/cards/card_database.h"
#include "../../../../utility/card_set_comparator.h"
#include "printing_selector_card_display_widget.h"

#include <QComboBox>
#include <QHBoxLayout>

PrintingSelector::PrintingSelector(TabDeckEditor *deckEditor,
                                   DeckListModel *deckModel,
                                   QTreeView *deckView,
                                   QWidget *parent)
    : QWidget(parent), deckEditor(deckEditor), deckModel(deckModel), deckView(deckView)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout = new QVBoxLayout();
    setLayout(layout);

    sortOptionsSelector = new QComboBox(this);
    QStringList sortOptions;
    sortOptions << "Alphabetical"
                << "Preference"
                << "Release Date (Asc)"
                << "Release Date (Desc)"
                << "Contained in Deck"
                << "Potential Cards in Deck";
    sortOptionsSelector->addItems(sortOptions);
    connect(sortOptionsSelector, SIGNAL(currentTextChanged(QString)), this, SLOT(updateDisplay()));
    layout->addWidget(sortOptionsSelector);

    flowWidget = new FlowWidget(this, Qt::ScrollBarAlwaysOff, Qt::ScrollBarAsNeeded);
    layout->addWidget(flowWidget);
}

void PrintingSelector::updateDisplay()
{
    flowWidget->clearLayout();
    getAllSetsForCurrentCard();
}

void PrintingSelector::setCard(const CardInfoPtr &newCard, const QString &_currentZone)
{
    selectedCard = newCard;
    currentZone = _currentZone;
    updateDisplay();
}

CardInfoPerSet PrintingSelector::getSetForUUID(const QString &uuid)
{
    CardInfoPerSetMap cardInfoPerSets = selectedCard->getSets();

    for (const auto &cardInfoPerSet : cardInfoPerSets) {
        if (cardInfoPerSet.getProperty("uuid") == uuid) {
            return cardInfoPerSet;
        }
    }

    return CardInfoPerSet();
}

QList<CardInfoPerSet> PrintingSelector::sortSets()
{
    if (selectedCard.isNull()) {
        return {};
    }
    CardInfoPerSetMap cardInfoPerSets = selectedCard->getSets();
    if (sortOptionsSelector->currentText() == "Alphabetical") {
        // Convert CardInfoPerSetMap to QList<CardInfoPerSet> and return
        QList<CardInfoPerSet> result;
        for (const auto &entry : cardInfoPerSets) {
            result << entry;
        }
        return result;
    }

    QList<CardSetPtr> sortedSets;

    for (const auto &set : cardInfoPerSets) {
        sortedSets << set.getPtr();
    }
    if (sortedSets.empty()) {
        sortedSets << CardSet::newInstance("", "", "", QDate());
    }
    if (sortOptionsSelector->currentText() == "Preference") {
        std::sort(sortedSets.begin(), sortedSets.end(), SetPriorityComparator());
    } else if (sortOptionsSelector->currentText() == "Release Date (Asc)") {
        std::sort(sortedSets.begin(), sortedSets.end(), SetReleaseDateComparator());
    } else {
        std::sort(sortedSets.begin(), sortedSets.end(), SetReleaseDateComparator());
        std::reverse(sortedSets.begin(), sortedSets.end());
    }

    QList<CardInfoPerSet> sortedCardInfoPerSets;

    // Debug: Output sorted set names
    qDebug() << "Sorted sets:";
    for (const auto &set : sortedSets) {
        qDebug() << set->getLongName();
    }

    // Reconstruct sorted list of CardInfoPerSet
    for (const auto &set : sortedSets) {
        for (auto it = cardInfoPerSets.begin(); it != cardInfoPerSets.end(); ++it) {
            if (it.value().getPtr() == set) {
                qDebug() << "Adding: " << it.value().getPtr()->getLongName();
                sortedCardInfoPerSets << it.value();
            }
        }
    }

    // Debug: Output final sorted list
    qDebug() << "Sorted CardInfoPerSet:";
    for (const auto &cardInfo : sortedCardInfoPerSets) {
        qDebug() << cardInfo.getPtr()->getLongName();
    }

    return sortedCardInfoPerSets;
}

void PrintingSelector::getAllSetsForCurrentCard()
{
    auto sortedMap = sortSets();

    for (auto cardInfoPerSet : sortedMap) {
        auto *cardDisplayWidget = new PrintingSelectorCardDisplayWidget(deckEditor, deckModel, deckView, selectedCard,
                                                                        cardInfoPerSet, currentZone);
        flowWidget->addWidget(cardDisplayWidget);
    }
}
