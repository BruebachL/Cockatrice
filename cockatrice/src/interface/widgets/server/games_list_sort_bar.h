#ifndef COCKATRICE_GAMES_LIST_SORT_BAR_H
#define COCKATRICE_GAMES_LIST_SORT_BAR_H

#include <QPair>
#include <QVector>
#include <QWidget>
#include <Qt>

class QComboBox;
class QLabel;
class QSpinBox;
class QToolButton;

/**
 * @class GamesListSortBar
 * @brief Compact toolbar above the TilingListView providing sort-field / order
 *        selection and a column-count control.
 *
 * Construct with the list of (display label, model column index) pairs to
 * offer as sort keys, then wire it up in GameSelector:
 *
 * @code
 *   using SF = GamesListSortBar::SortField;
 *   auto *bar = new GamesListSortBar({
 *       {tr("Start time"),  gameListModel->startTimeColIndex()},
 *       {tr("Description"), GamesModel::COL_DESCRIPTION},
 *       {tr("Creator"),     GamesModel::COL_CREATOR},
 *       {tr("Players"),     GamesModel::COL_PLAYERS},
 *   }, this);
 *
 *   connect(bar, &GamesListSortBar::sortChanged, this,
 *           [this](int col, Qt::SortOrder ord){ gameListProxyModel->sort(col, ord); });
 *   connect(bar,      &GamesListSortBar::columnsRequested,
 *           cardView, &TilingListView::setRequestedColumns);
 *   connect(cardView, &TilingListView::layoutRecalculated,
 *           bar,      &GamesListSortBar::onLayoutRecalculated);
 *   bar->emitInitialSort();
 * @endcode
 *
 * The columns spinbox uses 0 as a special "Auto" value (Qt::specialValueText).
 * Its maximum is kept in sync with TilingListView::maxColumns() via
 * onLayoutRecalculated().  When the viewport is too narrow to honour the
 * current spinbox value the display reflects the capped effective count, but
 * TilingListView's internal m_requestedCols is unchanged so the preference is
 * restored when the window is widened again.
 */
class GamesListSortBar : public QWidget
{
    Q_OBJECT

public:
    using SortField = QPair<QString, int>; ///< { display label, GamesModel column index }

    explicit GamesListSortBar(const QVector<SortField> &fields, QWidget *parent = nullptr);

    /**
     * Emits sortChanged() with the current field and order.  Call once after
     * all connections have been established to push the initial state into the
     * proxy model.
     */
    void emitInitialSort();

public slots:
    /**
     * Keeps the columns spinbox maximum in sync with the viewport.
     * Connect to TilingListView::layoutRecalculated().
     */
    void onLayoutRecalculated(int effectiveCols, int maxCols);

signals:
    /** Sort field or order changed. Wire to gameListProxyModel->sort(). */
    void sortChanged(int column, Qt::SortOrder order);

    /**
     * User adjusted the column count.  0 means "auto".
     * Wire to TilingListView::setRequestedColumns().
     */
    void columnsRequested(int n);

private slots:
    void onSortFieldChanged(int comboIndex);
    void onSortOrderToggled();
    void onColumnsChanged(int value);

private:
    QComboBox *fieldCombo;
    QToolButton *orderBtn;
    QSpinBox *colsSpinBox;

    Qt::SortOrder order = Qt::AscendingOrder;

    void updateOrderButton();
};

#endif // COCKATRICE_GAMES_LIST_SORT_BAR_H