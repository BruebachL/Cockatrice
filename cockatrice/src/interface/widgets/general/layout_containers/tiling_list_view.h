#ifndef COCKATRICE_TILING_LIST_VIEW_H
#define COCKATRICE_TILING_LIST_VIEW_H

#include <QListView>

/**
 * @class TilingListView
 * @brief A QListView that lays out items in a responsive multi-column grid.
 *
 * Two layout modes are available:
 *
 *  - **Auto** (requestedColumns == 0): column count is computed to keep each
 *    card as close to @c preferredCardW pixels wide as possible.
 *  - **Fixed** (requestedColumns > 0): the requested count is honoured, but
 *    silently capped so that no card ever falls below @c minCardW pixels.
 *    The original preference is restored automatically when the viewport
 *    widens again — the cap never permanently overwrites the request.
 *
 * cellWidth() exposes the computed card width so GameListItemDelegate can
 * return an exact sizeHint() match without re-deriving it.
 *
 * layoutRecalculated() fires after every resize or setRequestedColumns() call
 * so an external toolbar can mirror the live column count and the cap.
 */
class TilingListView : public QListView
{
    Q_OBJECT

    static constexpr int preferredCardW = 560;
    static constexpr int minCardW = 500;
    static constexpr int cardH = 48;
    static constexpr int cardSpacing = 6;

    int requestedCols = 0; ///< 0 = auto
    int effectiveCols = 1; ///< Column count used by the last layout pass
    int currentCellW = preferredCardW;

public:
    explicit TilingListView(QWidget *parent = nullptr);

    /** Most recent computed card width in pixels. */
    [[nodiscard]] int cellWidth() const
    {
        return currentCellW;
    }

    /** Column count used for the most recent layout pass. */
    [[nodiscard]] int effectiveColumns() const
    {
        return effectiveCols;
    }

    /**
     * Upper bound on column count: the largest value that keeps every card at
     * or above minCardW given the current viewport width.  Changes on resize.
     */
    [[nodiscard]] int maxColumns() const;

    /**
     * Request a fixed column count.  Pass 0 to restore automatic layout.
     * Values above maxColumns() are silently clamped; the layout is
     * recalculated immediately.  The un-clamped value is retained so that
     * widening the window restores the original preference.
     */
    void setRequestedColumns(int n);

    [[nodiscard]] int requestedColumns() const
    {
        return requestedCols;
    }

signals:
    /**
     * Emitted after every layout recalculation.
     * @param effectiveCols  Column count actually used (may be < requestedColumns).
     * @param maxCols        Current cap based on viewport width and minCardW.
     */
    void layoutRecalculated(int effectiveCols, int maxCols);

protected:
    void resizeEvent(QResizeEvent *e) override;

private:
    void recalcLayout();
};

#endif // COCKATRICE_TILING_LIST_VIEW_H