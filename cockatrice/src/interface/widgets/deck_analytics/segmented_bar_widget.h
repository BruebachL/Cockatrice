#ifndef COCKATRICE_SEGMENTED_BAR_WIDGET_H
#define COCKATRICE_SEGMENTED_BAR_WIDGET_H

#include <QWidget>

class SegmentedBarWidget : public QWidget
{
    Q_OBJECT
public:
    struct Segment
    {
        QString category;
        int value;
        QStringList cards;
        QColor color;
    };

    SegmentedBarWidget(QString label, QVector<Segment> segments, int total, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override
    {
        QWidget::enterEvent(event);
        hovered = true;
    }
    void leaveEvent(QEvent *event) override
    {
        QWidget::leaveEvent(event);
        hovered = false;
    }

private:
    QString label;
    QVector<Segment> segments;
    int total = 0;
    bool hovered = false;

    int segmentAt(int y) const;
};

#endif // COCKATRICE_SEGMENTED_BAR_WIDGET_H
