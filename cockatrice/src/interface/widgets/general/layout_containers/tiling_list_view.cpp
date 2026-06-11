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

void TilingListView::resizeEvent(QResizeEvent *e)
{
    QListView::resizeEvent(e);
    const int vw = viewport()->width();
    const int numCols = qMax(1, (vw + cardSpacing) / (preferredCardW + cardSpacing));
    currentCellW = (vw - cardSpacing * (numCols - 1)) / numCols;
    setGridSize(QSize(currentCellW, cardH + cardSpacing));
}
