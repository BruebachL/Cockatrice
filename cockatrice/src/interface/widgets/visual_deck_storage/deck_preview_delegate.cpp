#include "deck_preview_delegate.h"
/**
 * @file deck_preview_delegate.cpp
 * @ingroup VisualDeckStorageWidgets
 */

#include "../../../client/settings/cache_settings.h"
#include "../../../interface/card_picture_loader/card_picture_loader.h"
#include "deck_preview_delegate.h"
#include "deck_storage_model.h"
#include "visual_deck_storage_quick_settings_widget.h"

#include <QApplication>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>

DeckPreviewDelegate::DeckPreviewDelegate(const VisualDeckStorageQuickSettingsWidget *settings, QObject *parent)
    : QStyledItemDelegate(parent), settings_(settings)
{
}

// ---------------------------------------------------------------------------
// Entry points
// ---------------------------------------------------------------------------

void DeckPreviewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    if (index.data(DeckStorageModel::IsFolderRole).toBool())
        paintFolderHeader(painter, option, index);
    else
        paintDeckCell(painter, option, index);

    painter->restore();
}

QSize DeckPreviewDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return option.rect.size();
}

// ---------------------------------------------------------------------------
// Folder header  (matches BannerWidget paint)
// ---------------------------------------------------------------------------

void DeckPreviewDelegate::paintFolderHeader(QPainter *painter,
                                            const QStyleOptionViewItem &option,
                                            const QModelIndex &index) const
{
    const QRect &r = option.rect;
    const bool isCollapsed = index.data(DeckStorageModel::IsCollapsedRole).toBool();
    const QString text = index.data(Qt::DisplayRole).toString();

    // --- Background gradient (identical to BannerWidget) ---
    QLinearGradient gradient(r.topLeft(), r.bottomLeft());
    gradient.setColorAt(0, QColor(200, 200, 200, 204));
    gradient.setColorAt(1, QColor(100, 100, 100, 136));
    painter->fillRect(r, gradient);

    // --- Chevron (left) ---
    const int chevSize = 24;
    const QPixmap chev = chevronPixmap(chevSize, !isCollapsed);
    const QPoint chevPos(r.left() + 6, r.top() + (r.height() - chevSize) / 2);
    painter->drawPixmap(chevPos, chev);

    // --- Text (centred) ---
    QFont font = painter->font();
    font.setPointSize(14);
    font.setBold(true);
    painter->setFont(font);
    painter->setPen(Qt::white);
    painter->drawText(r, Qt::AlignCenter, text);
}

// ---------------------------------------------------------------------------
// Chevron pixmap — mirrors DropdownIconPixmapGenerator
// ---------------------------------------------------------------------------

QPixmap DeckPreviewDelegate::chevronPixmap(int size, bool expanded)
{
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    // Simple triangle pointing down (expanded) or right (collapsed)
    QPolygon poly;
    if (expanded) {
        poly << QPoint(4, 8) << QPoint(size / 2, size - 6) << QPoint(size - 4, 8);
    } else {
        poly << QPoint(8, 4) << QPoint(size - 6, size / 2) << QPoint(8, size - 4);
    }
    p.drawPolyline(poly);
    return pix;
}

// ---------------------------------------------------------------------------
// Deck cell
// ---------------------------------------------------------------------------

void DeckPreviewDelegate::paintDeckCell(QPainter *painter,
                                        const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const
{
    const QRect &r = option.rect;
    const bool isLoaded = index.data(DeckStorageModel::IsLoadedRole).toBool();

    // --- Reserve bottom strip for colour pips and tags ---
    const bool showColorId = settings_ && settings_->getShowColorIdentity();
    const bool showTags = settings_ && settings_->getShowTagsOnDeckPreviews();
    const int bottomStrip =
        ((showColorId ? kPipSize + kPipSpacing * 2 : 0) + (showTags ? kChipH + kPipSpacing * 2 : 0));
    const QRect imageRect = r.adjusted(0, 0, 0, -bottomStrip);

    // --- Card image ---
    const ExactCard card = index.data(DeckStorageModel::BannerCardRole).value<ExactCard>();
    QPixmap pix;
    if (card) {
        CardPictureLoader::getPixmap(pix, card, imageRect.size());
    }
    if (pix.isNull()) {
        CardPictureLoader::getCardBackLoadingFailedPixmap(pix, imageRect.size());
    }

    if (!pix.isNull()) {
        // Maintain aspect ratio and round corners like CardInfoPictureWidget
        QSize scaled = pix.size().scaled(imageRect.size(), Qt::KeepAspectRatio);
        QRect dest(imageRect.left() + (imageRect.width() - scaled.width()) / 2,
                   imageRect.top() + (imageRect.height() - scaled.height()) / 2, scaled.width(), scaled.height());

        const qreal radius = SettingsCache::instance().getRoundCardCorners() ? 0.05 * dest.width() : 0.0;
        QPainterPath clip;
        clip.addRoundedRect(dest, radius, radius);
        painter->save();
        painter->setClipPath(clip);
        painter->drawPixmap(dest, pix.scaled(scaled, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        painter->restore();

        // Selection highlight (replicates CardInfoPictureWithTextOverlayWidget::highlighted)
        if (option.state & QStyle::State_Selected) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.addRoundedRect(dest.adjusted(-4, -4, 4, 4), 8, 8);
            painter->setPen(QPen(QColor(0, 150, 255, 80), 6));
            painter->drawPath(path);
            painter->setPen(QPen(QColor(0, 150, 255, 200), 2));
            painter->drawRoundedRect(dest, 8, 8);
            painter->restore();
        }

        // --- Overlay text (deck name) ---
        const QString displayName = index.data(DeckStorageModel::DeckNameRole).toString();
        if (!displayName.isEmpty()) {
            drawOutlinedText(*painter, dest, displayName, Qt::AlignCenter, Qt::white, Qt::black, 14);
        }

    } else if (!isLoaded) {
        // Loading placeholder
        painter->save();
        painter->fillRect(imageRect, QColor(60, 60, 60));
        painter->setPen(Qt::lightGray);
        painter->drawText(imageRect, Qt::AlignCenter, tr("Loading…"));
        painter->restore();
    }

    // --- Tooltip set via view (no-op in delegate) ---

    // --- Bottom strip ---
    int stripY = imageRect.bottom() + kPipSpacing;

    if (showColorId) {
        const QString ci = index.data(DeckStorageModel::ColorIdentityRole).toString();
        const bool drawAll = settings_ && settings_->getDrawUnusedColorIdentities();
        const int opacity = settings_ ? settings_->getUnusedColorIdentitiesOpacity() : 30;
        QRect pipRect(r.left(), stripY, r.width(), kPipSize + kPipSpacing);
        drawColorIdentityPips(*painter, pipRect, ci, drawAll, opacity);
        stripY += kPipSize + kPipSpacing * 2;
    }

    if (showTags) {
        const QStringList tags = index.data(DeckStorageModel::TagsRole).toStringList();
        QRect chipRect(r.left(), stripY, r.width(), kChipH + kPipSpacing);
        drawTagChips(*painter, chipRect, tags);
    }
}

// ---------------------------------------------------------------------------
// Outlined text helper
// ---------------------------------------------------------------------------

void DeckPreviewDelegate::drawOutlinedText(QPainter &painter,
                                           const QRect &rect,
                                           const QString &text,
                                           Qt::Alignment alignment,
                                           const QColor &textColor,
                                           const QColor &outlineColor,
                                           int fontSize)
{
    QFont font = painter.font();
    font.setPointSize(fontSize);
    painter.setFont(font);

    QTextOption opt;
    opt.setAlignment(alignment);
    opt.setWrapMode(QTextOption::WordWrap);

    painter.setPen(outlineColor);
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
            if (dx || dy)
                painter.drawText(rect.translated(dx, dy), text, opt);

    painter.setPen(textColor);
    painter.drawText(rect, text, opt);
}

// ---------------------------------------------------------------------------
// Colour identity pips
// ---------------------------------------------------------------------------

static const QMap<QChar, QColor> &colorMap()
{
    static QMap<QChar, QColor> m = {
        {'W', QColor(248, 231, 185)}, {'U', QColor(14, 104, 171)}, {'B', QColor(21, 11, 0)},
        {'R', QColor(211, 32, 42)},   {'G', QColor(0, 115, 62)},
    };
    return m;
}

void DeckPreviewDelegate::drawColorIdentityPips(QPainter &painter,
                                                const QRect &cellRect,
                                                const QString &colorIdentity,
                                                bool drawUnused,
                                                int unusedOpacityPct)
{
    const QString wubrg = QStringLiteral("WUBRG");
    const int total = drawUnused ? 5 : colorIdentity.size();
    if (total == 0)
        return;

    const int totalW = total * kPipSize + (total - 1) * kPipSpacing;
    int x = cellRect.left() + (cellRect.width() - totalW) / 2;
    const int y = cellRect.top() + (cellRect.height() - kPipSize) / 2;

    for (const QChar &c : wubrg) {
        bool present = colorIdentity.contains(c);
        if (!present && !drawUnused)
            continue;

        QColor col = colorMap().value(c, Qt::gray);
        if (!present)
            col.setAlpha(col.alpha() * unusedOpacityPct / 100);

        painter.save();
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(col);
        painter.setPen(Qt::black);
        painter.drawEllipse(x, y, kPipSize, kPipSize);

        // Single-letter label
        QFont f = painter.font();
        f.setPointSize(7);
        f.setBold(true);
        painter.setFont(f);
        painter.setPen((c == 'W' || c == 'R' || c == 'G') ? Qt::black : Qt::white);
        painter.drawText(QRect(x, y, kPipSize, kPipSize), Qt::AlignCenter, c);

        painter.restore();
        x += kPipSize + kPipSpacing;
    }
}

// ---------------------------------------------------------------------------
// Tag chips
// ---------------------------------------------------------------------------

void DeckPreviewDelegate::drawTagChips(QPainter &painter, const QRect &cellRect, const QStringList &tags)
{
    if (tags.isEmpty())
        return;

    QFont font = painter.font();
    font.setPointSize(7);
    painter.setFont(font);
    const QFontMetrics fm(font);

    const int chipPad = 4;
    int x = cellRect.left() + chipPad;
    const int y = cellRect.top() + (cellRect.height() - kChipH) / 2;

    for (const QString &tag : tags) {
        const int tw = fm.horizontalAdvance(tag) + chipPad * 2;
        if (x + tw > cellRect.right())
            break; // clip silently

        QRect chip(x, y, tw, kChipH);
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QColor(80, 80, 120, 200));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(chip, kChipR, kChipR);
        painter.setPen(Qt::white);
        painter.drawText(chip, Qt::AlignCenter, tag);
        painter.restore();

        x += tw + chipPad;
    }
}