#include "card_info_picture_foil_widget.h"

#include <QMouseEvent>
#include <QQmlContext>
#include <QResizeEvent>

CardInfoPictureFoilWidget::CardInfoPictureFoilWidget(QWidget *parent, bool hoverToZoomEnabled)
    : CardInfoPictureWidget(parent, hoverToZoomEnabled)
{
    setMouseTracking(true);

    m_quick = new QQuickWidget(this);
    m_quick->rootContext()->setContextProperty(QStringLiteral("foilWidget"), this);
    m_quick->setSource(QUrl(QStringLiteral("qrc:/resources/qml/FoilEffect.qml")));
    m_quick->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quick->setClearColor(Qt::transparent);
    m_quick->setAttribute(Qt::WA_TransparentForMouseEvents);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &CardInfoPictureFoilWidget::updateFoilEffect);
    m_timer->start(16); // ~60 FPS for smooth shader animation

    updateArtRectFromPixmap();
}

void CardInfoPictureFoilWidget::setGradientOffset(qreal v)
{
    v = qBound(0.0, v, 1.0);
    if (qFuzzyCompare(v, m_gradientOffset))
        return;
    m_gradientOffset = v;
    emit gradientOffsetChanged();
}

void CardInfoPictureFoilWidget::setHighlightX(qreal v)
{
    v = qBound(0.0, v, 1.0);
    if (qFuzzyCompare(v, m_highlightX))
        return;
    m_highlightX = v;
    emit highlightXChanged();
}

void CardInfoPictureFoilWidget::setCardImageUrl(const QString &url)
{
    if (m_cardImageUrl == url)
        return;
    m_cardImageUrl = url;
    emit cardImageUrlChanged();
}

void CardInfoPictureFoilWidget::setApplyToArtOnly(bool v)
{
    if (m_applyToArtOnly == v)
        return;
    m_applyToArtOnly = v;
    emit applyToArtOnlyChanged();
}

void CardInfoPictureFoilWidget::resizeEvent(QResizeEvent *ev)
{
    CardInfoPictureWidget::resizeEvent(ev);
    m_quick->setGeometry(rect());
    updateArtRectFromPixmap();
}

void CardInfoPictureFoilWidget::mouseMoveEvent(QMouseEvent *ev)
{
    QRect scaledArtRect(int(m_artRectNormalized.x() * width()), int(m_artRectNormalized.y() * height()),
                        int(m_artRectNormalized.width() * width()), int(m_artRectNormalized.height() * height()));

    qreal normX;
    if (scaledArtRect.contains(ev->pos())) {
        normX = qreal(ev->pos().x() - scaledArtRect.left()) / qreal(scaledArtRect.width());
    } else {
        normX = ev->pos().x() < scaledArtRect.left() ? 0.0 : 1.0;
    }
    setHighlightX(normX);
    CardInfoPictureWidget::mouseMoveEvent(ev);
}

void CardInfoPictureFoilWidget::setCard(const ExactCard &card) {
    CardInfoPictureWidget::setCard(card);
    setApplyToArtOnly(getCard().getPrinting().getProperty("isFoil") != "fullart");
    QString imagePath = "/home/ascor/yuriko.png"; // <-- implement/replace this
    setCardImageUrl(QUrl::fromLocalFile(imagePath).toString());
}

void CardInfoPictureFoilWidget::updateFoilEffect()
{
    if (m_gradientForward) {
        m_gradientOffset += 0.01;
        if (m_gradientOffset >= 1.0)
            m_gradientForward = false;
    } else {
        m_gradientOffset -= 0.01;
        if (m_gradientOffset <= 0.0) m_gradientForward = true;
    }
    emit gradientOffsetChanged();
}

void CardInfoPictureFoilWidget::updateArtRectFromPixmap()
{
    QPixmap pix = getResizedPixmap();
    if (pix.isNull()) {
        m_artRectNormalized = QRectF(0, 0, 1, 1);
    } else {
        QSize scaledSize = pix.size().scaled(size(), Qt::KeepAspectRatio);
        QPoint topLeft((width() - scaledSize.width()) / 2, (height() - scaledSize.height()) / 2);
        QRect artRect(topLeft, scaledSize);
        m_artRectNormalized = QRectF(qreal(artRect.left()) / width(), qreal(artRect.top()) / height(),
                                     qreal(artRect.width()) / width(), qreal(artRect.height()) / height());
    }
    emit artRectNormalizedChanged();
}
