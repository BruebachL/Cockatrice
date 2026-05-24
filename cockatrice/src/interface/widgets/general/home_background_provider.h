#ifndef COCKATRICE_HOME_BACKGROUND_PROVIDER_H
#define COCKATRICE_HOME_BACKGROUND_PROVIDER_H

#pragma once
#include <QImage>
#include <QQuickImageProvider>
#include <QReadWriteLock>

/**
 * Serves the HomeWidget background image into the QML engine.
 *
 * URL in QML:  "image://homebg/bg?<version>"
 * The version suffix is a cache-buster: when C++ increments backgroundVersion
 * the binding changes the URL and Qt Quick requests a fresh image automatically.
 *
 * Thread-safe: requestImage() runs on a Qt Quick worker thread.
 */
class HomeBackgroundProvider : public QQuickImageProvider
{
public:
    HomeBackgroundProvider() : QQuickImageProvider(QQuickImageProvider::Image)
    {
    }

    // Called from the main thread (HomeWidget / CardAnimationController)
    void setBackground(const QPixmap &pixmap)
    {
        QWriteLocker lock(&m_lock);
        m_image = pixmap.isNull() ? QImage{} : pixmap.toImage();
    }

    // Called from a Qt Quick worker thread — must be re-entrant
    QImage requestImage(const QString & /*id*/, QSize *size, const QSize &requestedSize) override
    {
        QReadLocker lock(&m_lock);
        QImage img = m_image; // COW copy — safe to release lock
        lock.unlock();

        if (img.isNull()) {
            QImage placeholder(2, 3, QImage::Format_ARGB32);
            placeholder.fill(Qt::black);
            if (size) {
                *size = placeholder.size();
            }
            return placeholder;
        }

        if (requestedSize.isValid() && requestedSize != img.size()) {
            img = img.scaled(requestedSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        }

        if (size) {
            *size = img.size();
        }
        return img;
    }

private:
    QImage m_image;
    mutable QReadWriteLock m_lock;
};

#endif // COCKATRICE_HOME_BACKGROUND_PROVIDER_H
