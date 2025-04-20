#ifndef COCKATRICE_PIXMAP_PROVIDER_H
#define COCKATRICE_PIXMAP_PROVIDER_H

#include <QImage>
#include <QObject>
#include <QPixmap>

#pragma once
#include <QPixmap>
#include <QQuickImageProvider>

class PixmapProvider : public QQuickImageProvider
{
public:
    PixmapProvider();

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override
    {
        Q_UNUSED(id)
        if (size)
            *size = m_pixmap.size();

        if (requestedSize.isValid())
            return m_pixmap.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        return m_pixmap;
    }

    void setPixmap(const QPixmap &pm)
    {
        m_pixmap = pm;
    }

private:
    QPixmap m_pixmap;
};

#endif // COCKATRICE_PIXMAP_PROVIDER_H
