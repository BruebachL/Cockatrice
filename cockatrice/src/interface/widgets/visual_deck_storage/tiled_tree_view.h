/**
 * @file tiled_tree_view.h
 * @ingroup VisualDeckStorageWidgets
 * @brief A QAbstractItemView that renders a tree model as a tiled grid.
 *
 * Top-level items (folders) are drawn as full-width banner headers.
 * Their children (decks) are tiled left-to-right, wrapping into rows.
 *
 * The view maintains a flat layout cache (vector of CellInfo structs),
 * rebuilt on resize and model changes. Only visible cells are painted,
 * giving near-O(visible) render cost regardless of collection size.
 */

#ifndef TILED_TREE_VIEW_H
#define TILED_TREE_VIEW_H

#include <QAbstractItemView>
#include <QVector>

class TiledTreeView : public QAbstractItemView
{
    Q_OBJECT

public:
    static constexpr int kDefaultPreferredCellW = 200;
    static constexpr int kCellAspectNumerator = 140; // card aspect ~1.40
    static constexpr int kCellAspectDenominator = 100;
    static constexpr int kHeaderH = 50;
    static constexpr int kSpacing = 6;

    explicit TiledTreeView(QWidget *parent = nullptr);

    // --- QAbstractItemView pure virtuals ---
    QRect visualRect(const QModelIndex &index) const override;
    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) override;
    QModelIndex indexAt(const QPoint &point) const override;

    // --- Cell width driven by card-size setting ---
    void setCellWidth(int w);
    int cellWidth() const
    {
        return cellW_;
    }

    /** Recalculate and repaint — call after model or settings change. */
    void setModel(QAbstractItemModel *model) override;

    void rebuildLayout();

signals:
    void folderToggled(const QModelIndex &index);
    void deckActivated(const QModelIndex &index); // double-click / Enter
    void deckContextMenuRequested(const QModelIndex &index, const QPoint &globalPos);
    void deckHovered(const QModelIndex &index); // for reload-on-hover

protected:
    // --- QAbstractItemView overrides ---
    QModelIndex moveCursor(CursorAction action, Qt::KeyboardModifiers mods) override;
    int horizontalOffset() const override
    {
        return 0;
    }
    int verticalOffset() const override;
    bool isIndexHidden(const QModelIndex &) const override
    {
        return false;
    }
    void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags flags) override;
    QRegion visualRegionForSelection(const QItemSelection &selection) const override;

    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

    // --- Model change notifications ---
    void rowsInserted(const QModelIndex &parent, int start, int end) override;
    // rowsRemoved is not a virtual on QAbstractItemView; we connect to the
    // model signal directly in setModel() instead.
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) override;
    void reset() override;

private:
    struct CellInfo
    {
        QPersistentModelIndex index;
        QRect rect; ///< content coordinates
        bool isHeader;
    };

    QPoint contentPos(const QPoint &viewportPos) const;
    const CellInfo *cellAt(const QPoint &contentPos) const;

    QVector<CellInfo> layout_;
    int contentHeight_ = 0;
    int cellW_;
};

#endif // TILED_TREE_VIEW_H