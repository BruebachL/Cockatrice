#ifndef DECK_EDITOR_PRINTING_SELECTOR_DOCK_WIDGET_H
#define DECK_EDITOR_PRINTING_SELECTOR_DOCK_WIDGET_H

#include "../../../tabs/abstract_tab_deck_editor.h"
#include "../printing_selector/printing_selector.h"

#include <QDockWidget>

class TabDeckEditor;
class DeckEditorPrintingSelectorDockWidget : public QDockWidget
{
    Q_OBJECT
public:
    explicit DeckEditorPrintingSelectorDockWidget(AbstractTabDeckEditor *parent);
    void createPrintingSelectorDock();
    void retranslateUi();
    PrintingSelector *printingSelector;

private:
    AbstractTabDeckEditor *deckEditor;
};

#endif // DECK_EDITOR_PRINTING_SELECTOR_DOCK_WIDGET_H
