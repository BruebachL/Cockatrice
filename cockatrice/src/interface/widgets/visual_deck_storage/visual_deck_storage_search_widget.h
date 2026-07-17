/**
 * @file visual_deck_storage_search_widget.h
 * @ingroup VisualDeckStorageWidgets
 */
//! \todo Document this file.

#ifndef VISUAL_DECK_STORAGE_SEARCH_WIDGET_H
#define VISUAL_DECK_STORAGE_SEARCH_WIDGET_H

#include <QHBoxLayout>
#include <QLineEdit>
#include <QWidget>

class VisualDeckStorageWidget;
class VisualDeckStorageProxyModel;
class VisualDeckStorageSearchWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VisualDeckStorageSearchWidget(VisualDeckStorageWidget *parent);
    QString getSearchText();
    void applySearchFilter(VisualDeckStorageProxyModel *proxyModel);

private:
    QHBoxLayout *layout;
    VisualDeckStorageWidget *parent;
    QLineEdit *searchBar;
    QTimer *searchDebounceTimer;
};

#endif // VISUAL_DECK_STORAGE_SEARCH_WIDGET_H
