#include "pixmap_provider.h"

#include "../picture_loader/picture_loader.h"

PixmapProvider::PixmapProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
    PictureLoader::getCardBackPixmap(m_pixmap, QSize(100, 100));
}
