#include "segmented_bar_widget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

SegmentedBarWidget::SegmentedBarWidget(QString label, QVector<Segment> segments, int total, QWidget *parent)
    : QWidget(parent), label(std::move(label)), segments(std::move(segments)), total(total)
{
    setMouseTracking(true);
    setMinimumWidth(30);
    setMinimumHeight(120);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

void SegmentedBarWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    int padding = 4;

    int barX = padding;
    int barWidth = w - padding * 2;
    int barTop = padding;
    int barHeight = h - padding * 2 - 20; // leave 20px for label

    // background of bar: use widget palette (window background)
    p.setPen(Qt::NoPen);
    p.setBrush(palette().color(QPalette::Window));
    p.drawRect(barX, barTop, barWidth, barHeight);

    // stacked segments (bottom to top)
    int yCurrent = barTop + barHeight;

    for (const auto &seg : segments) {
        int segHeight = total > 0 ? (seg.value * barHeight / total) : 0;
        if (segHeight < 2)
            segHeight = 2;

        QRect r(barX, yCurrent - segHeight, barWidth, segHeight);

        p.setBrush(seg.color);
        p.drawRect(r);

        yCurrent -= segHeight;
    }

    // label under bar
    QRect labelRect(0, h - 20, w, 20);
    p.setPen(palette().color(QPalette::WindowText));
    p.drawText(labelRect, Qt::AlignCenter, label);
}

int SegmentedBarWidget::segmentAt(int y) const
{
    int h = height() - 30;
    int yTop = 10;
    int yBottom = yTop + h;

    int currentTop = yBottom;

    for (int i = 0; i < segments.size(); i++) {
        int segHeight = (total > 0) ? (segments[i].value * h / total) : 0;
        if (segHeight < 1)
            segHeight = 1;

        int top = currentTop - segHeight;
        int bottom = currentTop;

        if (y >= top && y <= bottom)
            return i;

        currentTop -= segHeight;
    }

    return -1;
}

void SegmentedBarWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (!hovered)
        return;

    int idx = segmentAt(e->pos().y());
    if (idx < 0)
        return;

    const Segment &s = segments[idx];
    QString text = QString("%1: %2 cards\n%3").arg(s.category).arg(s.value).arg(s.cards.join(", "));

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QToolTip::showText(e->globalPosition().toPoint(), text, this);
#else
    QToolTip::showText(e->globalPos(), text, this);
#endif
}
