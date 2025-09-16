#include "printing_selector_frame_effect_selector.h"

#include "../../../../game/cards/card_database_manager.h"

#include <QCheckBox>
#include <QScrollArea>
#include <QVBoxLayout>

PrintingSelectorFrameEffectSelector::PrintingSelectorFrameEffectSelector(QWidget *_parent) : QWidget(_parent)
{
    setMinimumSize(QSize(200, 200));
    // Main layout for this widget
    mainLayout = new QVBoxLayout(this);

    // Scroll area
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    mainLayout->addWidget(scrollArea);

    // Container widget inside scroll area
    container = new QWidget(scrollArea);
    vbox = new QVBoxLayout(container);
    vbox->setAlignment(Qt::AlignTop); // so they stack from top

    if (CardDatabaseManager::getInstance()->getLoadStatus() != Ok) {
        connect(CardDatabaseManager::getInstance(), &CardDatabase::cardDatabaseLoadingFinished, this,
                &PrintingSelectorFrameEffectSelector::initializeFrameEffects);
    } else {
        initializeFrameEffects();
    }

    container->setLayout(vbox);
    scrollArea->setWidget(container);
}

void PrintingSelectorFrameEffectSelector::initializeFrameEffects()
{
    // Populate with checkboxes for each frame effect
    const QStringList frameEffects = CardDatabaseManager::getInstance()->getAllFrameEffects();
    for (const QString &effect : frameEffects) {
        QCheckBox *cb = new QCheckBox(effect, container);
        vbox->addWidget(cb);

        // Connect toggled signal
        connect(cb, &QCheckBox::toggled, this,
                [this, effect](bool checked) { emit frameEffectToggled(effect, checked); });
    }
}

QStringList PrintingSelectorFrameEffectSelector::checkedFrameEffects() const
{
    QStringList selected;
    for (auto cb : findChildren<QCheckBox *>()) {
        if (cb->isChecked()) {
            selected << cb->text();
        }
    }
    return selected;
}

void PrintingSelectorFrameEffectSelector::updateVisibleFrameEffects(const CardInfoPtr &card)
{
    // Collect all frame effects from this card's printings
    QSet<QString> possibleEffects;
    if (!card.isNull()) {
        const SetToPrintingsMap setMap = card->getSets();
        for (const QList<PrintingInfo> &printings : setMap) {
            for (const PrintingInfo &pi : printings) {
                const QString effectsStr = pi.getProperty("frameEffects");
                if (!effectsStr.isEmpty()) {
                    const QStringList effects = effectsStr.split(',', Qt::SkipEmptyParts);
                    for (const QString &fx : effects) {
                        possibleEffects.insert(fx.trimmed());
                    }
                }
            }
        }
    }

    // Show/hide checkboxes based on possibleEffects
    for (QCheckBox *cb : container->findChildren<QCheckBox *>()) {
        bool contains = possibleEffects.contains(cb->text());
        if (!contains) {
            cb->setChecked(false);
        }
        cb->setVisible(contains);
    }
}
