#include "printing_selector.h"

#include "../../../../utility/card_set_comparator.h"
#include "printing_selector_card_display_widget.h"

#include <QComboBox>
#include <QDebug>
#include <QHBoxLayout>

const QString PrintingSelector::SORT_OPTIONS_ALPHABETICAL = tr("Alphabetical");
const QString PrintingSelector::SORT_OPTIONS_PREFERENCE = tr("Preference");
const QString PrintingSelector::SORT_OPTIONS_RELEASE_DATE = tr("Release Date");
const QString PrintingSelector::SORT_OPTIONS_CONTAINED_IN_DECK = tr("Contained in Deck");
const QString PrintingSelector::SORT_OPTIONS_POTENTIAL_CARDS = tr("Potential Cards in Deck");

const QStringList PrintingSelector::SORT_OPTIONS = {SORT_OPTIONS_ALPHABETICAL, SORT_OPTIONS_PREFERENCE,
                                                    SORT_OPTIONS_RELEASE_DATE, SORT_OPTIONS_CONTAINED_IN_DECK,
                                                    SORT_OPTIONS_POTENTIAL_CARDS};
PrintingSelector::PrintingSelector(QWidget *parent,
                                   TabDeckEditor *deckEditor,
                                   DeckListModel *deckModel,
                                   QTreeView *deckView)
    : QWidget(parent), deckEditor(deckEditor), deckModel(deckModel), deckView(deckView)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout = new QVBoxLayout();
    setLayout(layout);

    sortToolBar = new QHBoxLayout(this);

    sortOptionsSelector = new QComboBox(this);
    sortOptionsSelector->addItems(SORT_OPTIONS);
    connect(sortOptionsSelector, &QComboBox::currentTextChanged, this, &PrintingSelector::updateDisplay);
    sortToolBar->addWidget(sortOptionsSelector);

    toggleSortOrder = new QPushButton(this);
    toggleSortOrder->setText(tr("Ascending"));
    descendingSort = false;
    connect(toggleSortOrder, &QPushButton::clicked, this, &PrintingSelector::updateSortOrder);
    sortToolBar->addWidget(toggleSortOrder);

    layout->addLayout(sortToolBar);

    // Add the search bar
    searchBar = new QLineEdit(this);
    searchBar->setPlaceholderText(tr("Search..."));
    layout->addWidget(searchBar);

    // Add a debounce timer for the search bar
    searchDebounceTimer = new QTimer(this);
    searchDebounceTimer->setSingleShot(true);
    connect(searchBar, &QLineEdit::textChanged, this, [this]() {
        searchDebounceTimer->start(300); // 300ms debounce
    });
    connect(searchDebounceTimer, &QTimer::timeout, this, &PrintingSelector::updateDisplay);

    flowWidget = new FlowWidget(this, Qt::ScrollBarAlwaysOff, Qt::ScrollBarAsNeeded);
    layout->addWidget(flowWidget);
}

void PrintingSelector::updateSortOrder()
{
    if (descendingSort) {
        toggleSortOrder->setText(tr("Ascending"));
    } else {
        toggleSortOrder->setText(tr("Descending"));
    }
    descendingSort = !descendingSort;
    updateDisplay();
}

void PrintingSelector::updateDisplay()
{
    flowWidget->clearLayout();
    if (selectedCard != nullptr) {
        setWindowTitle(selectedCard->getName());
    }
    getAllSetsForCurrentCard();
}

void PrintingSelector::setCard(const CardInfoPtr &newCard, const QString &_currentZone)
{
    selectedCard = newCard;
    currentZone = _currentZone;
    if (isVisible()) {
        updateDisplay();
    }
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

    QList<CardSetPtr> sortedSets;

    for (const auto &set : cardInfoPerSets) {
        sortedSets << set.getPtr();
    }

    if (sortedSets.empty()) {
        sortedSets << CardSet::newInstance("", "", "", QDate());
    }

    if (sortOptionsSelector->currentText() == SORT_OPTIONS_PREFERENCE) {
        std::sort(sortedSets.begin(), sortedSets.end(), SetPriorityComparator());
    } else if (sortOptionsSelector->currentText() == SORT_OPTIONS_RELEASE_DATE) {
        std::sort(sortedSets.begin(), sortedSets.end(), SetReleaseDateComparator());
    }

    QList<CardInfoPerSet> sortedCardInfoPerSets;
    // Reconstruct sorted list of CardInfoPerSet
    for (const auto &set : sortedSets) {
        for (auto it = cardInfoPerSets.begin(); it != cardInfoPerSets.end(); ++it) {
            if (it.value().getPtr() == set) {
                sortedCardInfoPerSets << it.value();
            }
        }
    }

    if (descendingSort) {
        std::reverse(sortedCardInfoPerSets.begin(), sortedCardInfoPerSets.end());
    }

    return sortedCardInfoPerSets;
}

QList<CardInfoPerSet> PrintingSelector::filterSets(const QList<CardInfoPerSet> &sets) const
{
    const QString searchText = searchBar->text().trimmed().toLower();

    if (searchText.isEmpty()) {
        return sets;
    }

    QList<CardInfoPerSet> filteredSets;

    for (const auto &set : sets) {
        const QString longName = set.getPtr()->getLongName().toLower();
        const QString shortName = set.getPtr()->getShortName().toLower();

        if (longName.contains(searchText) || shortName.contains(searchText)) {
            filteredSets << set;
        }
    }

    return filteredSets;
}

void PrintingSelector::getAllSetsForCurrentCard()
{
    const QList<CardInfoPerSet> sortedSets = sortSets();
    const QList<CardInfoPerSet> filteredSets = filterSets(sortedSets);

    for (auto cardInfoPerSet : filteredSets) {
        auto *cardDisplayWidget = new PrintingSelectorCardDisplayWidget(this, deckEditor, deckModel, deckView,
                                                                        selectedCard, cardInfoPerSet, currentZone);
        flowWidget->addWidget(cardDisplayWidget);
    }
}
