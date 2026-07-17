/**
 * @file deck_preview_widget.h
 * @ingroup VisualDeckPreviewWidgets
 */
//! \todo Document this file.

#ifndef DECK_PREVIEW_WIDGET_H
#define DECK_PREVIEW_WIDGET_H

#include "../../cards/additional_info/color_identity_widget.h"
#include "../../cards/deck_preview_card_picture_widget.h"
#include "../visual_deck_storage_widget.h"
#include "deck_preview_deck_tags_display_widget.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QEvent>
#include <QLabel>
#include <QModelIndex>
#include <QVBoxLayout>
#include <QWidget>

class QMenu;
class VisualDeckStorageWidget;
class DeckPreviewDeckTagsDisplayWidget;

class DeckPreviewWidget final : public QWidget
{
    Q_OBJECT
public:
    explicit DeckPreviewWidget(QWidget *_parent, VisualDeckStorageWidget *_visualDeckStorageWidget);
    void setModelIndex(const QModelIndex &proxyIndex);
    void retranslateUi();
    [[nodiscard]] QString getDisplayName() const;
    [[nodiscard]] QString getFilePath() const;
    [[nodiscard]] DeckLoader *getDeckLoader() const;
    [[nodiscard]] QStringList getAllModelTags() const;

    VisualDeckStorageWidget *visualDeckStorageWidget;
    QVBoxLayout *layout;

signals:
    void deckLoadRequested(const QString &filePath);
    void openDeckEditor(const LoadedDeck &deck);

public slots:
    void refreshBannerCardText();
    void refreshBannerCardToolTip();
    void updateBannerCardComboBox(const QString &currentText);
    void setBannerCard(int);
    void imageClickedEvent(QMouseEvent *event, DeckPreviewCardPictureWidget *instance);
    void imageDoubleClickedEvent(QMouseEvent *event, DeckPreviewCardPictureWidget *instance);
    void initializeUi(bool deckLoadSuccess);
    void resyncWidgets();
    void updateFromModel();
    void updateColorIdentityVisibility(bool visible);
    void updateBannerCardComboBoxVisibility(bool visible);
    void updateTagsVisibility(bool visible);
    void resizeEvent(QResizeEvent *event) override;

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif

private:
    QSortFilterProxyModel *proxyModel = nullptr;
    int sourceModelRow = -1;
    DeckPreviewCardPictureWidget *bannerCardDisplayWidget = nullptr;
    ColorIdentityWidget *colorIdentityWidget = nullptr;
    DeckPreviewDeckTagsDisplayWidget *deckTagsDisplayWidget = nullptr;
    QLabel *bannerCardLabel = nullptr;
    QComboBox *bannerCardComboBox = nullptr;

    void updateLastModifiedTime();
    void writeDeckToFile();
    QMenu *createRightClickMenu();
    void addSetBannerCardMenu(QMenu *menu);
    int sourceRow() const;
    [[nodiscard]] VisualDeckStorageModel *getSourceModel() const;

private slots:
    void setTags(const QStringList &tags);

    void actRenameDeck();
    void actRenameFile();
    void actDeleteFile();
};

class NoScrollFilter : public QObject
{
    Q_OBJECT
public:
    explicit NoScrollFilter(QObject *parent = nullptr) : QObject(parent)
    {
    }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (event->type() == QEvent::Wheel) {
            if (auto *combo = qobject_cast<QComboBox *>(obj)) {
                if (!combo->view()->isVisible()) {
                    QWidget *parent = combo->parentWidget();
                    while (parent) {
                        if (auto *scroll = qobject_cast<QAbstractScrollArea *>(parent)) {
                            QApplication::sendEvent(scroll->viewport(), event);
                            return true;
                        }
                        parent = parent->parentWidget();
                    }
                    return true;
                }
            }
        }
        return QObject::eventFilter(obj, event);
    }
};

#endif // DECK_PREVIEW_WIDGET_H
