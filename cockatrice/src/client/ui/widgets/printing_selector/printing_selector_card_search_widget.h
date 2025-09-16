#ifndef PRINTING_SELECTOR_CARD_SEARCH_WIDGET_H
#define PRINTING_SELECTOR_CARD_SEARCH_WIDGET_H

#include "printing_selector.h"
#include "printing_selector_frame_effect_selector.h"

#include <QLineEdit>
#include <QTimer>
#include <QWidget>

class PrintingSelectorCardSearchWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PrintingSelectorCardSearchWidget(PrintingSelector *parent);
    QString getSearchText();
    QStringList checkedFrameEffects();

private:
    QHBoxLayout *layout;
    PrintingSelector *parent;
    QLineEdit *searchBar;
    QTimer *searchDebounceTimer;
    SettingsButtonWidget *frameEffectsFilter;
    PrintingSelectorFrameEffectSelector *frameEffectSelector;
};

#endif // PRINTING_SELECTOR_CARD_SEARCH_WIDGET_H
