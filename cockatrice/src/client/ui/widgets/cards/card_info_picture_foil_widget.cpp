#include "card_info_picture_foil_widget.h"

#include "../../../../main.h"

#include <QDebug>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainterPath>
#include <QRadialGradient>
#include <QStylePainter>
#include <QTime>
#include <QTimer>
#include <QWidget>

/**
 * @class CardInfoPictureFoilWidget
 * @brief Widget that displays an enlarged image of a card, loading the image based on the card's info or showing a
 * default image.
 *
 * This widget can optionally display a larger version of the card's image when hovered over,
 * depending on the `hoverToZoomEnabled` parameter.
 */

/**
 * @brief Constructs a CardInfoPictureFoilWidget.
 * @param parent The parent widget, if any.
 * @param hoverToZoomEnabled If this widget will spawn a larger widget when hovered over.
 *
 * Initializes the widget with a minimum height and sets the pixmap to a dirty state for initial loading.
 */
CardInfoPictureFoilWidget::CardInfoPictureFoilWidget(QWidget *parent, bool hoverToZoomEnabled)
    : CardInfoPictureWidget(parent, hoverToZoomEnabled), gradientOffset(0),
      isGradientForward(true)
{
    setMouseTracking(true);

    foilThemes = {{"Iridescent", {QColor(255, 0, 255, 100), QColor(0, 255, 255, 100), QColor(0, 255, 0, 100)}},
                  {"Warm Spectrum", {QColor(255, 128, 0, 80), QColor(255, 0, 64, 80), QColor(128, 0, 255, 80)}},
                  {"Cool Spectrum", {QColor(0, 128, 255, 90), QColor(0, 255, 128, 90), QColor(0, 64, 255, 90)}}};

    if (getCard().getPrinting().getProperty("isFoil") == "fullart") {
        applyToArtOnly = false;
    } else {
        applyToArtOnly = true;
    }

    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &CardInfoPictureFoilWidget::updateFoilEffect);
    timer->start(50); // 20 FPS
}

void CardInfoPictureFoilWidget::paintEvent(QPaintEvent *event)
{
    CardInfoPictureWidget::paintEvent(event);

    if (getCard().getPrinting().getProperty("isFoil") == "false") {
        return;
    }

    if (width() == 0 || height() == 0 || getResizedPixmap().isNull())
        return;

    QPainter painter(this);

    QSize scaledSize = getResizedPixmap().size().scaled(size(), Qt::KeepAspectRatio);
    QPoint topLeft{(width() - scaledSize.width()) / 2, (height() - scaledSize.height()) / 2};
    qreal radius = 0.05 * scaledSize.width();

    QPainterPath shape;
    shape.addRoundedRect(QRect(topLeft, scaledSize), radius, radius);
    painter.setClipPath(shape);

    QRect foilRect = rect();
    if (applyToArtOnly) {
        int marginX = scaledSize.width() * 0.07;
        int topMargin = scaledSize.height() * 0.11;
        int bottomMargin = scaledSize.height() * 0.45;

        foilRect = QRect(topLeft.x() + marginX, topLeft.y() + topMargin, scaledSize.width() - 2 * marginX,
                         scaledSize.height() - topMargin - bottomMargin);
    }

    // Setup and animate gradient
    const QVector<QColor> &themeColors = foilThemes[0].colors;
    QLinearGradient gradient(foilRect.topLeft(), foilRect.bottomRight());
    gradient.setColorAt(0.0, themeColors[0]);
    gradient.setColorAt(0.5, themeColors[1]);
    gradient.setColorAt(1.0, themeColors[2]);

    qreal animationOffset = (gradientOffset / 100.0) * foilRect.width();
    gradient.setStart(foilRect.topLeft() + QPointF(animationOffset, 0));
    gradient.setFinalStop(foilRect.bottomRight() + QPointF(animationOffset, 0));

    painter.fillRect(foilRect, gradient);

    // Add dynamic radial highlight
    QRadialGradient highlightGradient(highlightX, height() / 2, width() / 3);
    highlightGradient.setColorAt(0, QColor(255, 255, 255, 50));
    highlightGradient.setColorAt(1, QColor(255, 255, 255, 0));
    painter.setCompositionMode(QPainter::CompositionMode_Screen);
    painter.fillRect(foilRect, highlightGradient);
}

void CardInfoPictureFoilWidget::mouseMoveEvent(QMouseEvent *event)
{
    highlightX = event->pos().x();
    update();
}

void CardInfoPictureFoilWidget::setCard(const ExactCard &card)
{
    CardInfoPictureWidget::setCard(card);

    if (getCard().getPrinting().getProperty("isFoil") == "fullart") {
        applyToArtOnly = false;
    } else {
        applyToArtOnly = true;
    }
}

void CardInfoPictureFoilWidget::updateFoilEffect()
{
    // Animate the gradient offset smoothly between 0 and 100
    if (isGradientForward) {
        gradientOffset += 1; // Increment the offset smoothly
        if (gradientOffset >= 100) {
            gradientOffset = 100;      // Cap it at 100
            isGradientForward = false; // Reverse direction
        }
    } else {
        gradientOffset -= 1; // Decrement the offset smoothly
        if (gradientOffset <= 0) {
            gradientOffset = 0;       // Cap it at 0
            isGradientForward = true; // Reverse direction
        }
    }

    qDebug() << gradientOffset;
    update(); // Redraw the widget with the updated gradient
}
