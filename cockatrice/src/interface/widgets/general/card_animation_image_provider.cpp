#include "card_animation_image_provider.h"

CardAnimImageProvider::CardAnimImageProvider(CardAnimationController *controller)
    : QQuickImageProvider(QQuickImageProvider::Image), m_controller(controller)
{
}

QImage CardAnimImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QImage img = m_controller->getImageById(id);

    if (img.isNull()) {
        // Pool not warmed up yet — return a tiny transparent placeholder;
        // Qt Quick will re-request once the Image source is reloaded.
        QImage placeholder(2, 3, QImage::Format_ARGB32);
        placeholder.fill(Qt::transparent);
        if (size) {
            *size = placeholder.size();
        }
        return placeholder;
    }

    if (requestedSize.isValid()) {
        img = img.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    if (size) {
        *size = img.size();
    }
    return img;
}