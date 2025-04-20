#include "card_info_picture_foil_widget.h"

#include <QMouseEvent>
#include <QQmlContext>
#include <QQuickItem>
#include <QResizeEvent>
#include <QVBoxLayout>

CardInfoPictureFoilWidget::CardInfoPictureFoilWidget(QWidget *parent, bool hoverToZoomEnabled)
    : CardInfoPictureWidget(parent, hoverToZoomEnabled)
{
    setMouseTracking(true);

    /*
    m_quick = new QQuickWidget(this);
    m_quick->rootContext()->setContextProperty(QStringLiteral("foilWidget"), this);
    m_quick->setSource(QUrl(QStringLiteral("qrc:/resources/qml/FoilEffect.qml")));
    m_quick->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quick->setClearColor(Qt::transparent);
    m_quick->setAttribute(Qt::WA_TransparentForMouseEvents);
    */

    auto *layout = new QVBoxLayout(this);
    quick = new QQuickWidget(this);
    quick->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quick->setClearColor(Qt::transparent);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);

    provider = new PixmapProvider;
    quick->engine()->addImageProvider("pixmapProvider", provider);

    quick->setSource(QUrl(QStringLiteral("qrc:resources/qml/QuickTest.qml")));

    layout->addWidget(quick);
    setLayout(layout);
}


void CardInfoPictureFoilWidget::setCard(const ExactCard &card) {
    CardInfoPictureWidget::setCard(card);
    provider->setPixmap(getResizedPixmap());
    quick->engine()->clearComponentCache();
    quick->setSource(QUrl(QStringLiteral("qrc:resources/qml/QuickTest.qml")));
}

void CardInfoPictureFoilWidget::resizeEvent(QResizeEvent *event)
{
    CardInfoPictureWidget::resizeEvent(event);

    if (provider) {
        provider->setPixmap(getResizedPixmap());
        // force refresh
        quick->engine()->clearComponentCache();
        quick->setSource(QUrl(QStringLiteral("qrc:resources/qml/QuickTest.qml")));
    }
}
