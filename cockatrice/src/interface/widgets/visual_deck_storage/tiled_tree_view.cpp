/**
 * @file tiled_tree_view.cpp
 * @ingroup VisualDeckStorageWidgets
 */

#include "tiled_tree_view.h"

#include "deck_storage_model.h"

#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QWheelEvent>

TiledTreeView::TiledTreeView(QWidget *parent) : QAbstractItemView(parent), cellW_(kDefaultPreferredCellW)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setMouseTracking(true);
    verticalScrollBar()->setSingleStep(40);
}

// ---------------------------------------------------------------------------
// Cell width
// ---------------------------------------------------------------------------

void TiledTreeView::setCellWidth(int w)
{
    if (w == cellW_)
        return;
    cellW_ = qMax(80, w);
    rebuildLayout();
}

// ---------------------------------------------------------------------------
// Layout cache
// ---------------------------------------------------------------------------

void TiledTreeView::rebuildLayout()
{
    if (!model()) {
        layout_.clear();
        contentHeight_ = 0;
        verticalScrollBar()->setRange(0, 0);
        viewport()->update();
        return;
    }

    layout_.clear();

    const int vw = viewport()->width();
    const int numCols = qMax(1, (vw + kSpacing) / (cellW_ + kSpacing));
    const int actualCW = (vw - kSpacing * (numCols - 1)) / numCols;
    const int cellH = actualCW * kCellAspectNumerator / kCellAspectDenominator;

    int y = kSpacing;
    const int rootRows = model()->rowCount(rootIndex());

    for (int r = 0; r < rootRows; ++r) {
        const QModelIndex topIdx = model()->index(r, 0, rootIndex());
        const bool isFolder = topIdx.data(DeckStorageModel::IsFolderRole).toBool();

        if (isFolder) {
            // Full-width header
            layout_.append({QPersistentModelIndex(topIdx), QRect(0, y, vw, kHeaderH), true});
            y += kHeaderH + kSpacing;

            if (topIdx.data(DeckStorageModel::IsCollapsedRole).toBool())
                continue;

            // Tile children
            int col = 0;
            const int rows = model()->rowCount(topIdx);
            for (int c = 0; c < rows; ++c) {
                const QModelIndex childIdx = model()->index(c, 0, topIdx);
                const int x = kSpacing + col * (actualCW + kSpacing);
                layout_.append({QPersistentModelIndex(childIdx), QRect(x, y, actualCW, cellH), false});

                if (++col >= numCols) {
                    col = 0;
                    y += cellH + kSpacing;
                }
            }
            if (col > 0)
                y += cellH + kSpacing;

            y += kSpacing; // inter-folder gap

        } else {
            // Flat-mode deck (direct child of root)
            // const int col = (layout_.size()) % numCols; // approximate; reset per row below

            // Recount properly
            int flatDecks = 0;
            for (const CellInfo &ci : layout_)
                if (!ci.isHeader)
                    ++flatDecks;

            const int col2 = flatDecks % numCols;
            const int x = kSpacing + col2 * (actualCW + kSpacing);

            // Start a new row if we just wrapped
            if (col2 == 0 && flatDecks > 0)
                y += cellH + kSpacing;

            // For the very first flat deck, y is already at kSpacing
            layout_.append({QPersistentModelIndex(topIdx), QRect(x, y, actualCW, cellH), false});
        }
    }

    // Close off the last flat row if needed
    {
        int flatDecks = 0;
        for (const CellInfo &ci : layout_)
            if (!ci.isHeader)
                ++flatDecks;
        if (flatDecks > 0) {
            const int numCols2 = qMax(1, (vw + kSpacing) / (cellW_ + kSpacing));
            const int cellH2 = actualCW * kCellAspectNumerator / kCellAspectDenominator;
            if (flatDecks % numCols2 != 0 || flatDecks == 0)
                y += cellH2 + kSpacing;
        }
    }

    contentHeight_ = y + kSpacing;
    verticalScrollBar()->setRange(0, qMax(0, contentHeight_ - viewport()->height()));
    verticalScrollBar()->setPageStep(viewport()->height());
    viewport()->update();
}

// ---------------------------------------------------------------------------
// QAbstractItemView required virtuals
// ---------------------------------------------------------------------------

int TiledTreeView::verticalOffset() const
{
    return verticalScrollBar()->value();
}

QRect TiledTreeView::visualRect(const QModelIndex &index) const
{
    for (const CellInfo &c : layout_)
        if (c.index == index)
            return c.rect.translated(0, -verticalOffset());
    return {};
}

QModelIndex TiledTreeView::indexAt(const QPoint &viewportPos) const
{
    const QPoint p = contentPos(viewportPos);
    for (const CellInfo &c : layout_)
        if (c.rect.contains(p))
            return c.index;
    return {};
}

void TiledTreeView::scrollTo(const QModelIndex &index, ScrollHint hint)
{
    const QRect r = visualRect(index);
    if (r.isNull())
        return;

    const int vh = viewport()->height();
    int newVal = verticalScrollBar()->value();

    switch (hint) {
        case PositionAtTop:
            newVal = r.top() + verticalOffset();
            break;
        case PositionAtBottom:
            newVal = r.bottom() + verticalOffset() - vh;
            break;
        case PositionAtCenter:
            newVal = r.center().y() + verticalOffset() - vh / 2;
            break;
        default: // EnsureVisible
            if (r.top() < 0)
                newVal += r.top();
            else if (r.bottom() > vh)
                newVal += r.bottom() - vh;
            break;
    }

    verticalScrollBar()->setValue(newVal);
}

QModelIndex TiledTreeView::moveCursor(CursorAction action, Qt::KeyboardModifiers /*mods*/)
{
    const QModelIndex cur = currentIndex();
    if (!cur.isValid() || layout_.isEmpty())
        return {};

    int curPos = -1;
    for (int i = 0; i < layout_.size(); ++i)
        if (layout_[i].index == cur) {
            curPos = i;
            break;
        }
    if (curPos < 0)
        return {};

    auto nextDeck = [&](int delta) -> QModelIndex {
        int i = curPos + delta;
        while (i >= 0 && i < layout_.size()) {
            if (!layout_[i].isHeader)
                return layout_[i].index;
            i += delta;
        }
        return {};
    };

    switch (action) {
        case MoveNext:
        case MoveRight:
            return nextDeck(+1);
        case MovePrevious:
        case MoveLeft:
            return nextDeck(-1);
        default:
            return {};
    }
}

void TiledTreeView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags flags)
{
    const QRect content = rect.translated(0, verticalOffset());
    for (const CellInfo &c : layout_) {
        if (!c.isHeader && c.rect.intersects(content))
            selectionModel()->select(c.index, flags);
    }
}

QRegion TiledTreeView::visualRegionForSelection(const QItemSelection &selection) const
{
    QRegion region;
    for (const QItemSelectionRange &range : selection)
        for (const QModelIndex &idx : range.indexes())
            region += visualRect(idx);
    return region;
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void TiledTreeView::paintEvent(QPaintEvent *event)
{
    if (!model())
        return;

    QPainter painter(viewport());
    const int scrollY = verticalOffset();
    const QRect dirtyContent = event->rect().translated(0, scrollY);

    // viewOptions() was removed in Qt6 — build the base option manually.
    QStyleOptionViewItem baseOpt;
    baseOpt.initFrom(this);
    baseOpt.font = font();
    baseOpt.fontMetrics = fontMetrics();

    for (const CellInfo &c : layout_) {
        if (!c.rect.intersects(dirtyContent))
            continue;

        QStyleOptionViewItem opt = baseOpt;
        opt.rect = c.rect.translated(0, -scrollY);
        opt.state = QStyle::State_Enabled;

        if (selectionModel() && selectionModel()->isSelected(c.index))
            opt.state |= QStyle::State_Selected;

        // itemDelegate(index) is deprecated in Qt6; use itemDelegateForIndex().
        itemDelegateForIndex(c.index)->paint(&painter, opt, c.index);
    }
}

// ---------------------------------------------------------------------------
// Resize / model change
// ---------------------------------------------------------------------------

void TiledTreeView::resizeEvent(QResizeEvent *event)
{
    QAbstractItemView::resizeEvent(event);
    rebuildLayout();
}

void TiledTreeView::setModel(QAbstractItemModel *model)
{
    // Disconnect old model if any
    if (this->model())
        disconnect(this->model(), nullptr, this, nullptr);

    QAbstractItemView::setModel(model);

    if (model) {
        // rowsRemoved is not a virtual slot on QAbstractItemView, so wire it here.
        connect(model, &QAbstractItemModel::rowsRemoved, this,
                [this](const QModelIndex &, int, int) { rebuildLayout(); });
        // modelReset is already handled by reset() override, but be safe.
        connect(model, &QAbstractItemModel::modelReset, this, &TiledTreeView::rebuildLayout);
    }
}

void TiledTreeView::rowsInserted(const QModelIndex &parent, int start, int end)
{
    QAbstractItemView::rowsInserted(parent, start, end);
    rebuildLayout();
}

void TiledTreeView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    QAbstractItemView::dataChanged(topLeft, bottomRight, roles);

    // If collapse state changed we need a full rebuild; otherwise just repaint
    if (roles.contains(DeckStorageModel::IsCollapsedRole))
        rebuildLayout();
    else
        viewport()->update();
}

void TiledTreeView::reset()
{
    QAbstractItemView::reset();
    rebuildLayout();
}

// ---------------------------------------------------------------------------
// Mouse events
// ---------------------------------------------------------------------------

QPoint TiledTreeView::contentPos(const QPoint &viewportPos) const
{
    return viewportPos + QPoint(0, verticalOffset());
}

const TiledTreeView::CellInfo *TiledTreeView::cellAt(const QPoint &cp) const
{
    for (const CellInfo &c : layout_)
        if (c.rect.contains(cp))
            return &c;
    return nullptr;
}

void TiledTreeView::mousePressEvent(QMouseEvent *event)
{
    const QPoint cp = contentPos(event->pos());
    const CellInfo *cell = cellAt(cp);

    if (!cell) {
        QAbstractItemView::mousePressEvent(event);
        return;
    }

    if (cell->isHeader) {
        emit folderToggled(cell->index);
        return; // don't pass to base — base would try to select a folder
    }

    if (event->button() == Qt::RightButton) {
        selectionModel()->select(cell->index, QItemSelectionModel::ClearAndSelect);
        emit deckContextMenuRequested(cell->index, event->globalPosition().toPoint());
        return;
    }

    selectionModel()->select(cell->index, QItemSelectionModel::ClearAndSelect);
    setCurrentIndex(cell->index);
    viewport()->update();

    QAbstractItemView::mousePressEvent(event);
}

void TiledTreeView::mouseDoubleClickEvent(QMouseEvent *event)
{
    const CellInfo *cell = cellAt(contentPos(event->pos()));
    if (cell && !cell->isHeader)
        emit deckActivated(cell->index);
}

void TiledTreeView::mouseMoveEvent(QMouseEvent *event)
{
    QAbstractItemView::mouseMoveEvent(event);
    const CellInfo *cell = cellAt(contentPos(event->pos()));
    if (cell && !cell->isHeader)
        emit deckHovered(cell->index);
}

void TiledTreeView::wheelEvent(QWheelEvent *event)
{
    verticalScrollBar()->setValue(verticalScrollBar()->value() - event->angleDelta().y() / 2);
    event->accept();
}