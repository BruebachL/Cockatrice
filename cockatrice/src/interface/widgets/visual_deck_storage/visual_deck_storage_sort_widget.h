/**
 * @file visual_deck_storage_sort_widget.h
 * @ingroup VisualDeckStorageWidgets
 */
//! \todo Document this file.

#ifndef VISUAL_DECK_STORAGE_SORT_WIDGET_H
#define VISUAL_DECK_STORAGE_SORT_WIDGET_H

#include "visual_deck_storage_proxy_model.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QWidget>

class VisualDeckStorageWidget;
class VisualDeckStorageSortWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VisualDeckStorageSortWidget(VisualDeckStorageWidget *parent);
    void retranslateUi();
    void applySortOrder(VisualDeckStorageProxyModel *proxyModel);

signals:
    void sortOrderChanged();

private:
    QHBoxLayout *layout;
    VisualDeckStorageWidget *parent;
    QComboBox *sortComboBox;
};

#endif // VISUAL_DECK_STORAGE_SORT_WIDGET_H
