#include "printing_selector.h"

#include "../../../../dialogs/dlg_select_set_for_cards.h"
#include "../../../../settings/cache_settings.h"
#include "printing_selector_card_display_widget.h"
#include "printing_selector_card_search_widget.h"
#include "printing_selector_card_selection_widget.h"
#include "printing_selector_card_sorting_widget.h"

#include <QScrollBar>

/**
 * @brief Constructs a PrintingSelector widget to display and manage card printings.
 *
 * This constructor initializes the PrintingSelector widget, setting up various child widgets
 * such as sorting tools, search bar, card size options, and navigation controls. It also connects
 * signals and slots to update the display when the deck model changes, and loads available printings
 * for the selected card.
 *
 * @param parent The parent widget for the PrintingSelector.
 * @param deckEditor The TabDeckEditor instance used for managing the deck.
 * @param deckModel The DeckListModel instance that provides data for the deck's contents.
 * @param deckView The QTreeView instance used to display the deck and its contents.
 */
PrintingSelector::PrintingSelector(QWidget *parent, AbstractTabDeckEditor *_deckEditor)
    : QWidget(parent), deckEditor(_deckEditor), deckModel(deckEditor->deckDockWidget->deckModel),
      deckView(deckEditor->deckDockWidget->deckView)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout = new QVBoxLayout(this);
    setLayout(layout);

    widgetLoadingBufferTimer = new QTimer(this);

    flowWidget = new FlowWidget(this, Qt::Horizontal, Qt::ScrollBarAlwaysOff, Qt::ScrollBarAsNeeded);

    sortToolBar = new PrintingSelectorCardSortingWidget(this);
    sortToolBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    displayOptionsWidget = new SettingsButtonWidget(this);
    displayOptionsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

    // Create the checkbox for navigation buttons visibility
    navigationCheckBox = new QCheckBox(this);
    navigationCheckBox->setChecked(SettingsCache::instance().getPrintingSelectorNavigationButtonsVisible());
    connect(navigationCheckBox, &QCheckBox::QT_STATE_CHANGED, this,
            &PrintingSelector::toggleVisibilityNavigationButtons);
    connect(navigationCheckBox, &QCheckBox::QT_STATE_CHANGED, &SettingsCache::instance(),
            &SettingsCache::setPrintingSelectorNavigationButtonsVisible);

    cardSizeWidget =
        new CardSizeWidget(displayOptionsWidget, flowWidget, SettingsCache::instance().getPrintingSelectorCardSize());

    displayOptionsWidget->addSettingsWidget(sortToolBar);
    displayOptionsWidget->addSettingsWidget(navigationCheckBox);
    displayOptionsWidget->addSettingsWidget(cardSizeWidget);

    sortAndOptionsContainer = new QWidget(this);
    sortAndOptionsLayout = new QHBoxLayout(sortAndOptionsContainer);
    sortAndOptionsLayout->setSpacing(3);
    sortAndOptionsLayout->setContentsMargins(0, 0, 0, 0);
    sortAndOptionsContainer->setLayout(sortAndOptionsLayout);

    searchBar = new PrintingSelectorCardSearchWidget(this);

    sortAndOptionsLayout->addWidget(searchBar);
    sortAndOptionsLayout->addWidget(displayOptionsWidget);
    searchAndSetLayout->addWidget(searchBar);

    selectSetForCardsButton = new QPushButton(this);
    connect(selectSetForCardsButton, &QPushButton::clicked, this, &PrintingSelector::selectSetForCards);
    searchAndSetLayout->addWidget(selectSetForCardsButton);

    layout->addLayout(searchAndSetLayout);

    layout->addWidget(sortAndOptionsContainer);

    layout->addWidget(flowWidget);

    cardSelectionBar = new PrintingSelectorCardSelectionWidget(this);
    cardSelectionBar->setVisible(SettingsCache::instance().getPrintingSelectorNavigationButtonsVisible());
    layout->addWidget(cardSelectionBar);

    // Connect deck model data change signal to update display
    connect(deckModel, &DeckListModel::rowsInserted, this, &PrintingSelector::printingsInDeckChanged);
    connect(deckModel, &DeckListModel::rowsRemoved, this, &PrintingSelector::printingsInDeckChanged);

    retranslateUi();
}

void PrintingSelector::retranslateUi()
{
    navigationCheckBox->setText(tr("Display Navigation Buttons"));
}

void PrintingSelector::printingsInDeckChanged()
{
    // Delay the update to avoid race conditions
    QTimer::singleShot(100, this, &PrintingSelector::updateDisplay);
}

/**
 * @brief Updates the display by clearing the layout and loading new sets for the current card.
 */
void PrintingSelector::updateDisplay()
{
    widgetLoadingBufferTimer->stop();
    widgetLoadingBufferTimer->deleteLater();
    widgetLoadingBufferTimer = new QTimer(this);
    flowWidget->clearLayout();
    if (selectedCard != nullptr) {
        setWindowTitle(selectedCard->getName());
    }
    getAllSetsForCurrentCard();
}

/**
 * @brief Sets the current card for the selector and updates the display.
 *
 * @param newCard The new card to set.
 * @param _currentZone The current zone the card is in.
 */
void PrintingSelector::setCard(const CardInfoPtr &newCard, const QString &_currentZone)
{
    if (newCard.isNull()) {
        return;
    }

    // we don't need to redraw the widget if the card is the same
    if (!selectedCard.isNull() && selectedCard->getName() == newCard->getName()) {
        return;
    }

    selectedCard = newCard;
    currentZone = _currentZone;
    if (isVisible()) {
        updateDisplay();
    }
    flowWidget->setMinimumSizeToMaxSizeHint();
    flowWidget->scrollArea->verticalScrollBar()->setValue(0);
    flowWidget->repaint();
}

/**
 * @brief Selects the previous card in the list.
 */
void PrintingSelector::selectPreviousCard()
{
    selectCard(-1);
}

/**
 * @brief Selects the next card in the list.
 */
void PrintingSelector::selectNextCard()
{
    selectCard(1);
}

/**
 * @brief Selects a card based on the change direction.
 *
 * @param changeBy The direction to change, -1 for previous, 1 for next.
 */
void PrintingSelector::selectCard(const int changeBy)
{
    if (changeBy == 0) {
        return;
    }

    // Get the current index of the selected item
    auto deckViewCurrentIndex = deckView->currentIndex();

    auto nextIndex = deckViewCurrentIndex.siblingAtRow(deckViewCurrentIndex.row() + changeBy);
    if (!nextIndex.isValid()) {
        nextIndex = deckViewCurrentIndex;

        // Increment to the next valid index, skipping header rows
        AbstractDecklistNode *node;
        do {
            if (changeBy > 0) {
                nextIndex = deckView->indexBelow(nextIndex);
            } else {
                nextIndex = deckView->indexAbove(nextIndex);
            }
            node = static_cast<AbstractDecklistNode *>(nextIndex.internalPointer());
        } while (node && node->isDeckHeader());
    }

    if (nextIndex.isValid()) {
        deckView->setCurrentIndex(nextIndex);
        deckView->setFocus(Qt::FocusReason::MouseFocusReason);
    }
}

/**
 * @brief Loads and displays all sets for the current selected card.
 */
void PrintingSelector::getAllSetsForCurrentCard()
{
    if (selectedCard.isNull()) {
        return;
    }

    CardInfoPerSetMap cardInfoPerSets = selectedCard->getSets();
    const QList<CardInfoPerSet> sortedSets = sortToolBar->sortSets(cardInfoPerSets);
    const QList<CardInfoPerSet> filteredSets =
        sortToolBar->filterSets(sortedSets, searchBar->getSearchText().trimmed().toLower());
    QList<CardInfoPerSet> setsToUse;

    if (SettingsCache::instance().getBumpSetsWithCardsInDeckToTop()) {
        setsToUse = sortToolBar->prependPrintingsInDeck(filteredSets, selectedCard, deckModel);
    } else {
        setsToUse = filteredSets;
    }
    setsToUse = sortToolBar->prependPinnedPrintings(setsToUse, selectedCard->getName());

    // Defer widget creation
    currentIndex = 0;

    connect(widgetLoadingBufferTimer, &QTimer::timeout, this, [=, this]() mutable {
        for (int i = 0; i < BATCH_SIZE && currentIndex < setsToUse.size(); ++i, ++currentIndex) {
            auto *cardDisplayWidget = new PrintingSelectorCardDisplayWidget(this, deckEditor, deckModel, deckView,
                                                                            cardSizeWidget->getSlider(), selectedCard,
                                                                            setsToUse[currentIndex], currentZone);
            flowWidget->addWidget(cardDisplayWidget);
            cardDisplayWidget->clampSetNameToPicture();
            connect(cardDisplayWidget, &PrintingSelectorCardDisplayWidget::cardPreferenceChanged, this,
                    &PrintingSelector::updateDisplay);
        }

        // Stop timer when done
        if (currentIndex >= setsToUse.size()) {
            widgetLoadingBufferTimer->stop();
        }
    });
    currentIndex = 0;
    widgetLoadingBufferTimer->start(0); // Process as soon as possible
}

/**
 * @brief Toggles the visibility of the navigation buttons.
 *
 * @param _state The visibility state to set.
 */
void PrintingSelector::toggleVisibilityNavigationButtons(bool _state)
{
    cardSelectionBar->setVisible(_state);
}

void PrintingSelector::selectSetForCards()
{
    DlgSelectSetForCards *setSelectionDialog = new DlgSelectSetForCards(nullptr, deckModel);
    setSelectionDialog->show();
}
