#include "printing_selector_card_search_widget.h"

PrintingSelectorCardSearchWidget::PrintingSelectorCardSearchWidget(QWidget *_parent,
                                                                   PrintingSelector *_printingSelector)
    : QWidget(_parent), printingSelector(_printingSelector)
{
    layout = new QHBoxLayout(this);
    setLayout(layout);

    searchBar = new QLineEdit(this);
    searchBar->setPlaceholderText(tr("Search by set name or set code"));
    layout->addWidget(searchBar);

    // Add a debounce timer for the search bar
    searchDebounceTimer = new QTimer(this);
    searchDebounceTimer->setSingleShot(true);
    connect(searchBar, &QLineEdit::textChanged, this, [this]() {
        searchDebounceTimer->start(300); // 300ms debounce
    });

    connect(searchDebounceTimer, &QTimer::timeout, printingSelector, &PrintingSelector::updateDisplay);
}

QString PrintingSelectorCardSearchWidget::getSearchText()
{
    return searchBar->text();
}