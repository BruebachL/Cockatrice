#ifndef COCKATRICE_TILING_LIST_VIEW_H
#define COCKATRICE_TILING_LIST_VIEW_H
#include <QListView>

/**
 * @class TilingListView
 * @brief A QListView that lays out items in a responsive multi-column grid.
 *
 * Items are sized to fill the available width evenly with a fixed height.
 * The number of columns is recalculated on every resize so that each card is
 * as close to @c preferredCardW pixels wide as possible.  The computed width
 * is exposed via cellWidth() so that GameListItemDelegate::sizeHint() can
 * match it exactly.
 */
class TilingListView : public QListView
{
    Q_OBJECT

    static constexpr int preferredCardW = 320;
    static constexpr int cardH = 90;
    static constexpr int cardSpacing = 6;

    int currentCellW = preferredCardW;

public:
    explicit TilingListView(QWidget *parent = nullptr);

    /** Current computed card width in pixels. Updated on every resize. */
    int cellWidth() const
    {
        return currentCellW;
    }

protected:
    void resizeEvent(QResizeEvent *e) override;
};

#endif // COCKATRICE_TILING_LIST_VIEW_H
