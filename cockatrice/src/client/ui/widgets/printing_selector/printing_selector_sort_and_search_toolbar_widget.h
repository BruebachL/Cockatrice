#ifndef PRINTING_SELECTOR_SORT_AND_SEARCH_TOOLBAR_WIDGET_H
#define PRINTING_SELECTOR_SORT_AND_SEARCH_TOOLBAR_WIDGET_H

#include "printing_selector_card_search_widget.h"
#include "printing_selector_card_sorting_widget.h"

#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

class PrintingSelectorSortAndSearchToolbarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PrintingSelectorSortAndSearchToolbarWidget(QWidget *parent, PrintingSelector *printingSelector);
    void collapse();
    void expand();
    void onWidgetChanged(int);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    PrintingSelectorCardSortingWidget *getSortingWidget() const;
    PrintingSelectorCardSearchWidget *getSearchWidget() const;

private:
    QVBoxLayout *layout;
    PrintingSelector *printingSelector;
    PrintingSelectorCardSortingWidget *sortToolBar;
    PrintingSelectorCardSearchWidget *searchBar;
    QWidget *expandedWidget;
    QPushButton *collapseButton;
    QWidget *collapsedWidget;
    QPushButton *expandButton;
    QStackedWidget *stackedWidget;
};

#endif // PRINTING_SELECTOR_SORT_AND_SEARCH_TOOLBAR_WIDGET_H
