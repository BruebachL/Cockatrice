#include "visual_deck_storage_search_widget.h"

#include "../../../client/settings/cache_settings.h"
#include "../../../filters/syntax_help.h"
#include "../../pixel_map_generator.h"
#include "visual_deck_storage_proxy_model.h"
#include "visual_deck_storage_widget.h"

#include <QAction>
#include <QFileInfo>

VisualDeckStorageSearchWidget::VisualDeckStorageSearchWidget(VisualDeckStorageWidget *parent) : parent(parent)
{
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    searchBar = new QLineEdit(this);
    searchBar->setPlaceholderText(tr("Search by filename (or search expression)"));
    searchBar->setClearButtonEnabled(true);
    searchBar->addAction(loadColorAdjustedPixmap("theme:icons/search"), QLineEdit::LeadingPosition);

    auto help = searchBar->addAction(QPixmap("theme:icons/info"), QLineEdit::TrailingPosition);
    connect(help, &QAction::triggered, this, [this] { createDeckSearchSyntaxHelpWindow(searchBar); });

    layout->addWidget(searchBar);

    searchDebounceTimer = new QTimer(this);
    searchDebounceTimer->setSingleShot(true);
    connect(searchBar, &QLineEdit::textChanged, this, [this]() {
        searchDebounceTimer->start(300);
    });

    connect(searchDebounceTimer, &QTimer::timeout, parent, &VisualDeckStorageWidget::updateSearchFilter);
}

QString VisualDeckStorageSearchWidget::getSearchText()
{
    return searchBar->text();
}

void VisualDeckStorageSearchWidget::applySearchFilter(VisualDeckStorageProxyModel *proxyModel)
{
    proxyModel->setSearchText(searchBar->text());
}
