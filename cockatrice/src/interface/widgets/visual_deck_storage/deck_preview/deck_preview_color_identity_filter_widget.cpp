#include "deck_preview_color_identity_filter_widget.h"

#include "../../cards/additional_info/mana_symbol_widget.h"
#include "../visual_deck_storage_proxy_model.h"
#include "../visual_deck_storage_widget.h"

#include <QMouseEvent>

DeckPreviewColorIdentityFilterWidget::DeckPreviewColorIdentityFilterWidget(VisualDeckStorageWidget *parent)
    : QWidget(parent), layout(new QHBoxLayout(this))
{
    setLayout(layout);
    layout->setSpacing(5);
    layout->setContentsMargins(0, 0, 0, 0);

    QString fullColorIdentity = "WUBRG";
    for (const QChar &color : fullColorIdentity) {
        auto *manaSymbol = new ManaSymbolWidget(this, color, false, true);
        manaSymbol->setFixedWidth(25);

        layout->addWidget(manaSymbol);

        activeColors[color] = false;

        connect(manaSymbol, &ManaSymbolWidget::colorToggled, this,
                &DeckPreviewColorIdentityFilterWidget::handleColorToggled);
    }

    toggleButton = new QPushButton(this);
    toggleButton->setCheckable(true);
    layout->addWidget(toggleButton);

    connect(toggleButton, &QPushButton::toggled, this, &DeckPreviewColorIdentityFilterWidget::updateFilterMode);
    connect(this, &DeckPreviewColorIdentityFilterWidget::activeColorsChanged, parent,
            &VisualDeckStorageWidget::updateColorFilter);
    connect(this, &DeckPreviewColorIdentityFilterWidget::filterModeChanged, parent,
            &VisualDeckStorageWidget::updateColorFilter);

    retranslateUi();
}

void DeckPreviewColorIdentityFilterWidget::retranslateUi()
{
    toggleButton->setText(exactMatchMode ? tr("Mode: Exact Match") : tr("Mode: Includes"));
    toggleButton->setToolTip(tr("Color identity filter mode (AND/OR/NOT conjunctions of filters)"));
}

void DeckPreviewColorIdentityFilterWidget::handleColorToggled(QChar color, bool active)
{
    activeColors[color] = active;
    emit activeColorsChanged();
}

void DeckPreviewColorIdentityFilterWidget::updateFilterMode(bool checked)
{
    exactMatchMode = checked;
    retranslateUi();
    emit filterModeChanged(exactMatchMode);
}

void DeckPreviewColorIdentityFilterWidget::applyColorFilter(VisualDeckStorageProxyModel *proxyModel)
{
    proxyModel->setColorFilter(activeColors, exactMatchMode);
}
