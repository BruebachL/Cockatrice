/**
 * @file deck_preview_color_identity_filter_widget.h
 * @ingroup VisualDeckPreviewWidgets
 */
//! \todo Document this file.

#ifndef DECK_PREVIEW_COLOR_IDENTITY_FILTER_WIDGET_H
#define DECK_PREVIEW_COLOR_IDENTITY_FILTER_WIDGET_H

#include <QHBoxLayout>
#include <QMap>
#include <QPushButton>
#include <QWidget>

class VisualDeckStorageWidget;
class VisualDeckStorageProxyModel;

class DeckPreviewColorIdentityFilterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DeckPreviewColorIdentityFilterWidget(VisualDeckStorageWidget *parent);
    void retranslateUi();
    void applyColorFilter(VisualDeckStorageProxyModel *proxyModel);

signals:
    void filterModeChanged(bool exactMatchMode);
    void activeColorsChanged();

private slots:
    void handleColorToggled(QChar color, bool active);
    void updateFilterMode(bool checked);

private:
    QHBoxLayout *layout;
    QPushButton *toggleButton;
    QMap<QChar, bool> activeColors;
    bool exactMatchMode = false;
};

#endif // DECK_PREVIEW_COLOR_IDENTITY_FILTER_WIDGET_H
