#include "printing_selector_sort_and_search_toolbar_widget.h"
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>

PrintingSelectorSortAndSearchToolbarWidget::PrintingSelectorSortAndSearchToolbarWidget(QWidget *_parent, PrintingSelector *_printingSelector)
    : QWidget(_parent), printingSelector(_printingSelector)
{
    layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setLayout(layout);

    expandedWidget = new QWidget(this);
    QVBoxLayout *expandedLayout = new QVBoxLayout(expandedWidget);
    expandedLayout->setContentsMargins(0, 0, 0, 0);
    expandedLayout->setSpacing(0);

    collapseButton = new QPushButton("▼", this);
    collapseButton->setFixedSize(20, 20);
    collapseButton->setToolTip("Collapse");
    collapseButton->setStyleSheet("border: none;");
    connect(collapseButton, &QPushButton::clicked, this, &PrintingSelectorSortAndSearchToolbarWidget::collapse);
    expandedLayout->addWidget(collapseButton, 0, Qt::AlignLeft);

    sortToolBar = new PrintingSelectorCardSortingWidget(this, printingSelector);
    expandedLayout->addWidget(sortToolBar);

    searchBar = new PrintingSelectorCardSearchWidget(this, printingSelector);
    expandedLayout->addWidget(searchBar);

    expandedWidget->setLayout(expandedLayout);

    collapsedWidget = new QWidget(this);
    QHBoxLayout *collapsedLayout = new QHBoxLayout(collapsedWidget);
    collapsedLayout->setContentsMargins(5, 0, 5, 0);
    collapsedLayout->setSpacing(0);

    expandButton = new QPushButton("▲", this);
    expandButton->setFixedSize(20, 20);
    expandButton->setToolTip("Expand");
    expandButton->setStyleSheet("border: none;");
    connect(expandButton, &QPushButton::clicked, this, &PrintingSelectorSortAndSearchToolbarWidget::expand);
    collapsedLayout->addWidget(expandButton);

    QLabel *collapsedLabel = new QLabel(tr("Sort and Search Toolbar"), this);
    collapsedLayout->addWidget(collapsedLabel);

    collapsedWidget->setLayout(collapsedLayout);

    stackedWidget = new QStackedWidget(this);
    stackedWidget->addWidget(expandedWidget);
    stackedWidget->addWidget(collapsedWidget);

    layout->addWidget(stackedWidget);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    stackedWidget->setCurrentWidget(expandedWidget);

    connect(stackedWidget, &QStackedWidget::currentChanged, this, &PrintingSelectorSortAndSearchToolbarWidget::onWidgetChanged);
}

void PrintingSelectorSortAndSearchToolbarWidget::collapse()
{
    stackedWidget->setCurrentWidget(collapsedWidget);
    updateGeometry();
}

void PrintingSelectorSortAndSearchToolbarWidget::expand()
{
    stackedWidget->setCurrentWidget(expandedWidget);
    updateGeometry();
}

// Handle Geometry Update
void PrintingSelectorSortAndSearchToolbarWidget::onWidgetChanged(int)
{
    updateGeometry();
    if (parentWidget() && parentWidget()->layout()) {
        parentWidget()->layout()->invalidate();
    }
}

// Size Hints
QSize PrintingSelectorSortAndSearchToolbarWidget::sizeHint() const
{
    return stackedWidget->currentWidget()->sizeHint();
}

QSize PrintingSelectorSortAndSearchToolbarWidget::minimumSizeHint() const
{
    return stackedWidget->currentWidget()->minimumSizeHint();
}



PrintingSelectorCardSortingWidget* PrintingSelectorSortAndSearchToolbarWidget::getSortingWidget() const
{
    return sortToolBar;
}

PrintingSelectorCardSearchWidget* PrintingSelectorSortAndSearchToolbarWidget::getSearchWidget() const
{
    return searchBar;
}