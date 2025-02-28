#include "card_info_picture_enlarged_widget.h"

#include "../../picture_loader/picture_loader.h"

#include <QPainterPath>
#include <QStylePainter>
#include <qpropertyanimation.h>
#include <utility>

/**
 * @brief Constructs a CardPictureEnlargedWidget.
 * @param parent The parent widget.
 *
 * Sets the widget's window flags to keep it displayed as a tooltip overlay.
 */
CardInfoPictureEnlargedWidget::CardInfoPictureEnlargedWidget(QWidget *parent)
    : QWidget(parent), pixmapDirty(true), info(nullptr)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::ToolTip); // Keeps this widget on top of everything
    setAttribute(Qt::WA_TranslucentBackground);

    fadeInAnimation = new QPropertyAnimation(this, "windowOpacity");
    fadeInAnimation->setDuration(1000);
    fadeInAnimation->setStartValue(0.0);
    fadeInAnimation->setEndValue(1.0);
    fadeInAnimation->setEasingCurve(QEasingCurve::OutBack);
}

/**
 * @brief Loads the pixmap based on the given size and card information.
 * @param size The desired size for the loaded pixmap.
 *
 * If card information is available, it loads the card's specific pixmap. Otherwise, it loads a default card back
 * pixmap.
 */
void CardInfoPictureEnlargedWidget::loadPixmap(const QSize &size)
{
    if (info) {
        PictureLoader::getPixmap(enlargedPixmap, info, size);
    } else {
        PictureLoader::getCardBackPixmap(enlargedPixmap, size);
    }
    pixmapDirty = false;
}

/**
 * @brief Sets the pixmap for the widget based on a provided card.
 * @param card The card information to load.
 * @param size The desired size for the pixmap.
 *
 * Sets the widget's pixmap to the card image and resizes the widget to match the specified size. Triggers a repaint.
 */
void CardInfoPictureEnlargedWidget::setCardPixmap(CardInfoPtr card, const QSize size)
{
    disconnect(info.data());
    setFixedSize(size); // Set the widget size to the enlarged size
    info = std::move(card);
    loadPixmap(size);
    connect(info.data(), &CardInfo::pixmapUpdated, this, &CardInfoPictureEnlargedWidget::fadeIn);
}

void CardInfoPictureEnlargedWidget::fadeIn()
{
    qDebug() << "Fade in should happen";
    disconnect(fadeInAnimation);
    show();

    if (fadeInAnimation->state() == QAbstractAnimation::Running) {
        fadeInAnimation->pause(); // Pause current animation
    } else {
        fadeInAnimation->setStartValue(0.0);
        fadeInAnimation->setEndValue(1.0);
    }
    fadeInAnimation->setDirection(QAbstractAnimation::Forward);
    fadeInAnimation->start();
}

void CardInfoPictureEnlargedWidget::fadeOut()
{
    qDebug() << "Fade out should happen";
    // Hide enlarged image with a fade-out animation
    if (fadeInAnimation->state() == QAbstractAnimation::Running) {
        fadeInAnimation->pause(); // Pause current animation
    }
    fadeInAnimation->setDirection(QAbstractAnimation::Backward);
    fadeInAnimation->start();
    connect(fadeInAnimation, &QPropertyAnimation::finished, this, &QWidget::hide);
}

/**
 * @brief Custom paint event that draws the enlarged card image with rounded corners.
 * @param event The paint event (unused).
 *
 * Checks if the pixmap is valid. Then, calculates the size and position for centering the
 * scaled pixmap within the widget, applies rounded corners, and draws the pixmap.
 */
void CardInfoPictureEnlargedWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    if (width() == 0 || height() == 0 || enlargedPixmap.isNull()) {
        return;
    }

    if (pixmapDirty) {
        loadPixmap(size());
    }

    // Scale the size of the pixmap to fit the widget while maintaining the aspect ratio
    QSize scaledSize = enlargedPixmap.size().scaled(size().width(), size().height(), Qt::KeepAspectRatio);

    // Calculate the position to center the scaled pixmap
    QPoint topLeft{(width() - scaledSize.width()) / 2, (height() - scaledSize.height()) / 2};

    // Define the radius for rounded corners
    qreal radius = 0.05 * scaledSize.width(); // Adjust the radius as needed for rounded corners

    QStylePainter painter(this);
    // Fill the background with transparent color to ensure rounded corners are rendered properly
    painter.fillRect(rect(), Qt::transparent); // Use the transparent background

    QPainterPath shape;
    shape.addRoundedRect(QRect(topLeft, scaledSize), radius, radius);
    painter.setClipPath(shape); // Set the clipping path

    // Draw the pixmap scaled to the calculated size
    painter.drawItemPixmap(QRect(topLeft, scaledSize), Qt::AlignCenter,
                           enlargedPixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

QSize CardInfoPictureEnlargedWidget::sizeHint() const
{
    return enlargedPixmap.size().scaled(size().width(), size().height(), Qt::KeepAspectRatio);
    ;
}
