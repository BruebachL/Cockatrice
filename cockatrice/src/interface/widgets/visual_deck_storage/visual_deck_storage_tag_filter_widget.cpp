#include "visual_deck_storage_tag_filter_widget.h"

#include "../general/layout_containers/flow_widget.h"
#include "deck_preview/deck_preview_tag_display_widget.h"
#include "visual_deck_storage_model.h"
#include "visual_deck_storage_proxy_model.h"
#include "visual_deck_storage_widget.h"

#include <QHBoxLayout>

VisualDeckStorageTagFilterWidget::VisualDeckStorageTagFilterWidget(VisualDeckStorageWidget *_parent)
    : QWidget(_parent), parent(_parent)
{

    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 0, 5, 0);

    setFixedHeight(100);

    auto *flowWidget = new FlowWidget(this, Qt::Horizontal, Qt::ScrollBarAlwaysOff, Qt::ScrollBarAsNeeded);

    layout->addWidget(flowWidget);
}

void VisualDeckStorageTagFilterWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshTags();
}

void VisualDeckStorageTagFilterWidget::updateFilterFromModel()
{
    QStringList selectedTags;
    QStringList excludedTags;

    for (DeckPreviewTagDisplayWidget *tagWidget : findChildren<DeckPreviewTagDisplayWidget *>()) {
        switch (tagWidget->getState()) {
            case TagState::Selected:
                selectedTags.append(tagWidget->getTagName());
                break;
            case TagState::Excluded:
                excludedTags.append(tagWidget->getTagName());
                break;
            default:
                break;
        }
    }

    QSet<QString> allTags = gatherAllTags();
    removeTagsNotInList(allTags);
    addTagsIfNotPresent(allTags);
    sortTags();
}

void VisualDeckStorageTagFilterWidget::applyTagFilter(VisualDeckStorageProxyModel *proxyModel)
{
    QStringList selectedTags;
    QStringList excludedTags;

    for (DeckPreviewTagDisplayWidget *tagWidget : findChildren<DeckPreviewTagDisplayWidget *>()) {
        switch (tagWidget->getState()) {
            case TagState::Selected:
                selectedTags.append(tagWidget->getTagName());
                break;
            case TagState::Excluded:
                excludedTags.append(tagWidget->getTagName());
                break;
            default:
                break;
        }
    }

    proxyModel->setTagFilter(selectedTags, excludedTags);
}

void VisualDeckStorageTagFilterWidget::refreshTags()
{
    QSet<QString> allTags = gatherAllTags();
    removeTagsNotInList(allTags);
    addTagsIfNotPresent(allTags);
    sortTags();
}

void VisualDeckStorageTagFilterWidget::removeTagsNotInList(const QSet<QString> &tags)
{
    auto *flowWidget = findChild<FlowWidget *>();

    for (DeckPreviewTagDisplayWidget *tagWidget : findChildren<DeckPreviewTagDisplayWidget *>()) {
        const QString &tagName = tagWidget->getTagName();

        if (!tags.contains(tagName) && tagWidget->getState() == TagState::NotSelected) {
            flowWidget->removeWidget(tagWidget);
            tagWidget->deleteLater();
        }
    }
}

void VisualDeckStorageTagFilterWidget::addTagsIfNotPresent(const QSet<QString> &tags)
{
    for (const QString &tag : tags) {
        addTagIfNotPresent(tag);
    }
}

void VisualDeckStorageTagFilterWidget::addTagIfNotPresent(const QString &tag)
{
    bool tagExists = false;
    for (DeckPreviewTagDisplayWidget *tagWidget : findChildren<DeckPreviewTagDisplayWidget *>()) {
        if (tagWidget->getTagName() == tag) {
            tagExists = true;
            break;
        }
    }

    if (!tagExists) {
        auto *newTagWidget = new DeckPreviewTagDisplayWidget(this, tag);
        connect(newTagWidget, &DeckPreviewTagDisplayWidget::tagClicked, parent,
                &VisualDeckStorageWidget::updateTagFilter);
        connect(newTagWidget, &DeckPreviewTagDisplayWidget::tagClicked, this,
                &VisualDeckStorageTagFilterWidget::refreshTags);
        auto *flowWidget = findChild<FlowWidget *>();
        flowWidget->addWidget(newTagWidget);
    }
}

void VisualDeckStorageTagFilterWidget::sortTags()
{
    auto *flowWidget = findChild<FlowWidget *>();
    if (!flowWidget) {
        return;
    }

    QList<DeckPreviewTagDisplayWidget *> tagWidgets = findChildren<DeckPreviewTagDisplayWidget *>();

    std::sort(tagWidgets.begin(), tagWidgets.end(), [](DeckPreviewTagDisplayWidget *a, DeckPreviewTagDisplayWidget *b) {
        return a->getTagName().toLower() < b->getTagName().toLower();
    });

    for (DeckPreviewTagDisplayWidget *tagWidget : tagWidgets) {
        flowWidget->removeWidget(tagWidget);
    }
    for (DeckPreviewTagDisplayWidget *tagWidget : tagWidgets) {
        flowWidget->addWidget(tagWidget);
    }
}

QSet<QString> VisualDeckStorageTagFilterWidget::gatherAllTags() const
{
    QSet<QString> allTags;
    auto *model = parent->deckStorageModel;

    for (int i = 0; i < model->rowCount(); ++i) {
        const auto &entry = model->entryAt(i);
        if (entry.loaded) {
            for (const QString &tag : entry.tags) {
                allTags.insert(tag);
            }
        }
    }
    return allTags;
}

QStringList VisualDeckStorageTagFilterWidget::getAllKnownTags() const
{
    QStringList allTags;

    for (DeckPreviewTagDisplayWidget *tagWidget : findChildren<DeckPreviewTagDisplayWidget *>()) {
        allTags.append(tagWidget->getTagName());
    }

    allTags.removeDuplicates();

    return allTags;
}
