#ifndef COCKATRICE_CARD_ANIMATION_IMAGE_PROVIDER_H
#define COCKATRICE_CARD_ANIMATION_IMAGE_PROVIDER_H

#include "card_animation_controller.h"

#include <QQuickImageProvider>

/**
 * Serves card art into Qt Quick's image cache under the "cardanim" provider.
 *
 * URL format used in QML:  "image://cardanim/<numericId>"
 *
 * ImageType::Image lets Qt Quick load on a background thread.
 * getImageById() uses a QReadWriteLock so concurrent reads are safe.
 */
class CardAnimImageProvider : public QQuickImageProvider
{
public:
    explicit CardAnimImageProvider(CardAnimationController *controller);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    CardAnimationController *m_controller; // not owned
};

#endif // COCKATRICE_CARD_ANIMATION_IMAGE_PROVIDER_H
