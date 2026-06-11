#include "tiling_list_view.h"

TilingListView::TilingListView(QWidget *parent) : QListView(parent)
{
    setViewMode(QListView::IconMode);
    setResizeMode(QListView::Adjust);
    setWrapping(true);
    setFlow(QListView::LeftToRight);
    setSpacing(cardSpacing);
    setUniformItemSizes(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

int TilingListView::maxColumns() const
{
    const int vw = viewport()->width();
    return qMax(1, (vw + cardSpacing) / (minCardW + cardSpacing));
}

void TilingListView::setRequestedColumns(int n)
{
    requestedCols = qMax(0, n);
    recalcLayout();
}

void TilingListView::resizeEvent(QResizeEvent *e)
{
    QListView::resizeEvent(e);
    recalcLayout();
}

void TilingListView::recalcLayout()
{
    const int vw = viewport()->width();
    const int maxCols = qMax(1, (vw + cardSpacing) / (minCardW + cardSpacing));

    int numCols;
    if (requestedCols <= 0) {
        // Auto: target preferredCardW, but never exceed maxCols
        numCols = qMax(1, (vw + cardSpacing) / (preferredCardW + cardSpacing));
        numCols = qMin(numCols, maxCols);
    } else {
        // Fixed: honour the request, cap at what actually fits above minCardW
        numCols = qMin(requestedCols, maxCols);
    }
    numCols = qMax(1, numCols); // belt-and-suspenders for a zero-width viewport

    effectiveCols = numCols;
    currentCellW = (vw - cardSpacing * (numCols - 1)) / numCols;
    setGridSize(QSize(currentCellW, cardH + cardSpacing));

    emit layoutRecalculated(effectiveCols, maxCols);
}