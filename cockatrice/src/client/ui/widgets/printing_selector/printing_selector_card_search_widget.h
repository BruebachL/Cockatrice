#ifndef PRINTING_SELECTOR_CARD_SEARCH_WIDGET_H
#define PRINTING_SELECTOR_CARD_SEARCH_WIDGET_H

#include "printing_selector.h"

#include <QLineEdit>
#include <QTimer>
#include <QWidget>

class PrintingSelectorCardSearchWidget : public QWidget
{
    Q_OBJECT

public:
    PrintingSelectorCardSearchWidget(QWidget *_parent, PrintingSelector *_printingSelector);
    QString getSearchText();

private:
    QHBoxLayout *layout;
    PrintingSelector *printingSelector;
    QLineEdit *searchBar;
    QTimer *searchDebounceTimer;
};

#endif // PRINTING_SELECTOR_CARD_SEARCH_WIDGET_H
