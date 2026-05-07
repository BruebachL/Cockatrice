/**
 * @file deck_preview_delegate.h
 * @ingroup VisualDeckStorageWidgets
 * @brief Delegate that paints deck cells and folder-banner header rows
 *        for TiledTreeView.
 *
 * Folder rows replicate the visual of BannerWidget (gradient background,
 * bold centred text, chevron icon).
 * Deck cells replicate DeckPreviewCardPictureWidget: card image with a
 * name overlay, plus optional colour-identity pips and tag chips below.
 */

#ifndef DECK_PREVIEW_DELEGATE_H
#define DECK_PREVIEW_DELEGATE_H

#include <QCache>
#include <QStyledItemDelegate>

class VisualDeckStorageQuickSettingsWidget;

class DeckPreviewDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit DeckPreviewDelegate(const VisualDeckStorageQuickSettingsWidget *settings, QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    void paintFolderHeader(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    void paintDeckCell(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    static void drawOutlinedText(QPainter &painter,
                                 const QRect &rect,
                                 const QString &text,
                                 Qt::Alignment alignment,
                                 const QColor &textColor,
                                 const QColor &outlineColor,
                                 int fontSize);

    static void drawColorIdentityPips(QPainter &painter,
                                      const QRect &cellRect,
                                      const QString &colorIdentity,
                                      bool drawUnused,
                                      int unusedOpacityPct);

    static void drawTagChips(QPainter &painter, const QRect &cellRect, const QStringList &tags);

    static QPixmap loadCardPixmap(const QModelIndex &index, const QSize &targetSize);

    // Drop-down chevron — matches BannerWidget's DropdownIconPixmapGenerator
    static QPixmap chevronPixmap(int size, bool expanded);

    const VisualDeckStorageQuickSettingsWidget *settings_;

    static constexpr int kPipSize = 14;
    static constexpr int kPipSpacing = 2;
    static constexpr int kChipH = 16;
    static constexpr int kChipR = 4;
};

#endif // DECK_PREVIEW_DELEGATE_H