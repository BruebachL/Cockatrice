#include "bar_chart_background_widget.h"

BarChartBackgroundWidget::BarChartBackgroundWidget(QWidget *parent) : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
}

void BarChartBackgroundWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int padding = 6;
    int top = padding;
    int bottom = height() - padding - labelHeight;
    int left = padding + 40; // space for y-axis labels
    int right = width() - padding;

    int barAreaWidth = right - left;
    int barAreaHeight = bottom - top;

    // background
    p.fillRect(QRect(left, top, barAreaWidth, barAreaHeight), QColor(250, 250, 250));

    // Grid lines + Y-axis numbers
    int ticks = 5;
    for (int i = 0; i <= ticks; i++) {
        float r = float(i) / ticks;
        int y = bottom - r * barAreaHeight;

        // line
        p.setPen(QPen(QColor(180, 180, 180, 120), 1));
        p.drawLine(left, y, right, y);

        // number
        p.setPen(Qt::black);
        p.drawText(left - 35, y - 6, 32, 12, Qt::AlignRight | Qt::AlignVCenter, QString::number(int(r * highest)));
    }
}
