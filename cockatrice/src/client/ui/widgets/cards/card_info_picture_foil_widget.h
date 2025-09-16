#ifndef CARD_INFO_PICTURE_FOIL_WIDGET_H
#define CARD_INFO_PICTURE_FOIL_WIDGET_H

#include "card_info_picture_widget.h"

#include <QColor>
#include <QPainter>
#include <QTimer>
#include <QVector>

/**
 * @brief Widget that displays a Magic: The Gathering card with animated foil effects.
 */
class CardInfoPictureFoilWidget : public CardInfoPictureWidget
{
    Q_OBJECT

public:
    explicit CardInfoPictureFoilWidget(QWidget *parent = nullptr,
                                       bool hoverToZoomEnabled = true);

    public slots:
    void setCard(const ExactCard &card) override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void updateFoilEffect();

private:
    struct FoilTheme
    {
        QString name;
        QVector<QColor> colors;
    };

    QVector<FoilTheme> foilThemes;
    int gradientOffset = 0;
    int highlightX = 0;
    bool applyToArtOnly = false;
    bool isGradientForward = true;
};

#endif // CARD_INFO_PICTURE_FOIL_WIDGET_H
