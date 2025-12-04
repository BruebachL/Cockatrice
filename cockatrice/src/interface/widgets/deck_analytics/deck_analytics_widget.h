/**
 * @file deck_analytics_widget.h
 * @ingroup DeckEditorAnalyticsWidgets
 * @brief TODO: Document this.
 */

#ifndef DECK_ANALYTICS_WIDGET_H
#define DECK_ANALYTICS_WIDGET_H

#include "deck_analytics_widget_base.h"

#include <QListWidget>
#include <QWidget>

class DeckListModel;
class DeckListStatisticsAnalyzer;

class DeckAnalyticsWidget : public QWidget
{
    Q_OBJECT

public slots:
    void updateDisplays();

public:
    DeckAnalyticsWidget(QWidget *parent, DeckListModel *model);

private slots:
    void onAddPanel();
    void onRemoveSelected();
    void saveLayout();
    void loadLayout();
    void addDefaultPanels();
    bool loadLayoutInternal();

private:
    QListWidget *listWidget = nullptr; // holds panels as QListWidgetItems with widgets
    DeckListStatisticsAnalyzer *statsAnalyzer = nullptr;

    void addPanelInstance(AnalyticsWidgetBase *panel, const QJsonObject &cfg = {});
};

#endif // DECK_ANALYTICS_WIDGET_H
