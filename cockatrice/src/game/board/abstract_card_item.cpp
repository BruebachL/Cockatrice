#include "abstract_card_item.h"

#include "../../database/card_database.h"
#include "../../database/card_database_manager.h"
#include "../../picture_loader/picture_loader.h"
#include "../../settings/cache_settings.h"
#include "../game_scene.h"

#include <QCursor>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QPainter>
#include <algorithm>

AbstractCardItem::AbstractCardItem(QGraphicsItem *parent, const CardRef &cardRef, Player *_owner, int _id)
    : ArrowTarget(_owner, parent), id(_id), cardRef(cardRef), tapped(false), facedown(false), tapAngle(0),
      bgColor(Qt::transparent), isHovered(false), realZValue(0)
{
    setCursor(Qt::OpenHandCursor);
    setFlag(ItemIsSelectable);
    setCacheMode(DeviceCoordinateCache);
    setAcceptHoverEvents(true);

    popup = new AbstractCardHoverItem(this);
    popup->setHidden(true);
    popup->setAttribute(Qt::WA_ShowWithoutActivating);

    hideTimer = new QTimer(this);
    hideTimer->setInterval(1500);
    hideTimer->setSingleShot(true);

    connect(hideTimer, &QTimer::timeout, popup, [this]() {
        qInfo() << "Timer timed out";
        popup->hide();
    });

    connect(&SettingsCache::instance(), &SettingsCache::displayCardNamesChanged, this, [this] { update(); });
    refreshCardInfo();

    connect(&SettingsCache::instance(), &SettingsCache::roundCardCornersChanged, this, [this](bool _roundCardCorners) {
        Q_UNUSED(_roundCardCorners);

        prepareGeometryChange();
        update();
    });
}

AbstractCardItem::~AbstractCardItem()
{
    emit deleteCardInfoPopup(cardRef.name);
}

QRectF AbstractCardItem::boundingRect() const
{
    return QRectF(0, 0, CARD_WIDTH, CARD_HEIGHT);
}

QPainterPath AbstractCardItem::shape() const
{
    QPainterPath shape;
    qreal cardCornerRadius = SettingsCache::instance().getRoundCardCorners() ? 0.05 * CARD_WIDTH : 0.0;
    shape.addRoundedRect(boundingRect(), cardCornerRadius, cardCornerRadius);
    return shape;
}

void AbstractCardItem::pixmapUpdated()
{
    update();
    emit sigPixmapUpdated();
}

void AbstractCardItem::refreshCardInfo()
{
    exactCard = CardDatabaseManager::query()->getCard(cardRef);

    if (!exactCard && !cardRef.name.isEmpty()) {
        auto info = CardInfo::newInstance(cardRef.name, "", true, {}, {}, {}, {}, false, false, -1, false);
        exactCard = ExactCard(info);
    }
    if (exactCard) {
        connect(exactCard.getCardPtr().data(), &CardInfo::pixmapUpdated, this, &AbstractCardItem::pixmapUpdated);
    }

    popup->updateCard();

    cacheBgColor();
    update();
}

/**
 * Convenience method to get the CardInfo of the exactCard
 * @return A const reference to the CardInfo, or an empty CardInfo if card was null
 */
const CardInfo &AbstractCardItem::getCardInfo() const
{
    return exactCard.getInfo();
}

void AbstractCardItem::setRealZValue(qreal _zValue)
{
    realZValue = _zValue;
    setZValue(_zValue);
}

QSizeF AbstractCardItem::getTranslatedSize(QPainter *painter) const
{
    return QSizeF(painter->combinedTransform().map(QLineF(0, 0, boundingRect().width(), 0)).length(),
                  painter->combinedTransform().map(QLineF(0, 0, 0, boundingRect().height())).length());
}

void AbstractCardItem::transformPainter(QPainter *painter, const QSizeF &translatedSize, int angle)
{
    const int MAX_FONT_SIZE = SettingsCache::instance().getMaxFontSize();
    const int fontSize = std::max(9, MAX_FONT_SIZE);

    QRectF totalBoundingRect = painter->combinedTransform().mapRect(boundingRect());

    int scale = resetPainterTransform(painter);

    painter->translate(totalBoundingRect.width() / 2, totalBoundingRect.height() / 2);
    painter->rotate(angle);
    painter->translate(-translatedSize.width() / 2, -translatedSize.height() / 2);

    QFont f;
    f.setPixelSize(fontSize * scale);

    painter->setFont(f);
}

void AbstractCardItem::paintPicture(QPainter *painter, const QSizeF &translatedSize, int angle)
{
    qreal scaleFactor = translatedSize.width() / boundingRect().width();
    QPixmap translatedPixmap;
    bool paintImage = true;

    if (facedown || cardRef.name.isEmpty()) {
        // never reveal card color, always paint the card back
        PictureLoader::getCardBackPixmap(translatedPixmap, translatedSize.toSize());
    } else {
        // don't even spend time trying to load the picture if our size is too small
        if (translatedSize.width() > 10) {
            PictureLoader::getPixmap(translatedPixmap, exactCard, translatedSize.toSize());
            if (translatedPixmap.isNull())
                paintImage = false;
        } else {
            paintImage = false;
        }
    }

    painter->save();

    if (paintImage) {
        painter->save();
        painter->setClipPath(shape());
        painter->drawPixmap(boundingRect(), translatedPixmap, QRectF({0, 0}, translatedPixmap.size()));
        painter->restore();
    } else {
        painter->setBrush(bgColor);
        painter->drawPath(shape());
    }

    if (translatedPixmap.isNull() || SettingsCache::instance().getDisplayCardNames() || facedown) {
        painter->save();
        transformPainter(painter, translatedSize, angle);
        painter->setPen(Qt::white);
        painter->setBackground(Qt::black);
        painter->setBackgroundMode(Qt::OpaqueMode);
        QString nameStr;
        if (facedown)
            nameStr = "# " + QString::number(id);
        else {
            QString prefix = "";
            if (SettingsCache::instance().debug().getShowCardId()) {
                prefix = "#" + QString::number(id) + " ";
            }
            nameStr = prefix + cardRef.name;
        }
        painter->drawText(QRectF(3 * scaleFactor, 3 * scaleFactor, translatedSize.width() - 6 * scaleFactor,
                                 translatedSize.height() - 6 * scaleFactor),
                          Qt::AlignTop | Qt::AlignLeft | Qt::TextWrapAnywhere, nameStr);
        painter->restore();
    }

    painter->restore();
}

void AbstractCardItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    painter->save();

    QSizeF translatedSize = getTranslatedSize(painter);
    paintPicture(painter, translatedSize, tapAngle);

    painter->setRenderHint(QPainter::Antialiasing, false);

    if (isSelected() || isHovered) {
        QPen pen;
        if (isHovered)
            pen.setColor(Qt::yellow);
        if (isSelected())
            pen.setColor(Qt::red);
        pen.setWidth(0); // Cosmetic pen
        painter->setPen(pen);
        painter->drawPath(shape());
    }

    painter->restore();
}

void AbstractCardItem::setCardRef(const CardRef &_cardRef)
{
    if (cardRef == _cardRef) {
        return;
    }

    emit deleteCardInfoPopup(cardRef.name);
    if (exactCard) {
        disconnect(exactCard.getCardPtr().data(), nullptr, this, nullptr);
    }
    cardRef = _cardRef;

    refreshCardInfo();
}

void AbstractCardItem::setHovered(bool _hovered)
{
    if (isHovered == _hovered)
        return;

    if (_hovered)
        processHoverEvent();
    isHovered = _hovered;
    setZValue(_hovered ? 2000000004 : realZValue);
    setScale(_hovered && SettingsCache::instance().getScaleCards() ? 1.1 : 1);
    setTransformOriginPoint(_hovered ? CARD_WIDTH / 2 : 0, _hovered ? CARD_HEIGHT / 2 : 0);
    update();
}

void AbstractCardItem::setColor(const QString &_color)
{
    color = _color;
    cacheBgColor();
    update();
}

void AbstractCardItem::cacheBgColor()
{
    QChar colorChar;
    if (color.isEmpty()) {
        colorChar = exactCard.getInfo().getColorChar();
    } else {
        colorChar = color.at(0);
    }

    switch (colorChar.toLower().toLatin1()) {
        case 'b':
            bgColor = QColor(0, 0, 0);
            break;
        case 'u':
            bgColor = QColor(0, 140, 180);
            break;
        case 'w':
            bgColor = QColor(255, 250, 140);
            break;
        case 'r':
            bgColor = QColor(230, 0, 0);
            break;
        case 'g':
            bgColor = QColor(0, 160, 0);
            break;
        case 'm':
            bgColor = QColor(250, 190, 30);
            break;
        default:
            bgColor = QColor(230, 230, 230);
            break;
    }
}

void AbstractCardItem::setTapped(bool _tapped, bool canAnimate)
{
    if (tapped == _tapped)
        return;

    tapped = _tapped;
    if (SettingsCache::instance().getTapAnimation() && canAnimate)
        static_cast<GameScene *>(scene())->registerAnimationItem(this);
    else {
        tapAngle = tapped ? 90 : 0;
        setTransform(QTransform()
                         .translate((float)CARD_WIDTH / 2, (float)CARD_HEIGHT / 2)
                         .rotate(tapAngle)
                         .translate((float)-CARD_WIDTH / 2, (float)-CARD_HEIGHT / 2));
        update();
    }
}

void AbstractCardItem::setFaceDown(bool _facedown)
{
    facedown = _facedown;
    update();
}

void AbstractCardItem::cancelHideTimer()
{
    qInfo() << "AbstractCardItem is canceling the hide timer.";
    if (hideTimer) {
        hideTimer->stop();
        qInfo() << "Cancelled";
    }
}

void AbstractCardItem::startHideTimerIfNotHovered()
{
    qInfo() << "AbstractCardItem is starting hide timer.";
    checkMouseLeave();
}

void AbstractCardItem::positionPopup()
{
    if (!popup || !popupProxy || !scene())
        return;

    // Use floating-point size to avoid integer rounding artifacts
    popup->adjustSize();
    QSizeF popupSizeF = QSizeF(popup->size());

    QRectF sceneRect = scene()->sceneRect();
    QRectF cardRect = mapToScene(boundingRect()).boundingRect();

    // Candidate top-left positions in scene coords (prefer right-first in the list)
    struct Candidate
    {
        QPointF pos;
        bool isRight;
    };
    QVector<Candidate> candidates;
    candidates.append({QPointF(cardRect.right(), cardRect.top() - popupSizeF.height()), true}); // TR
    candidates.append({QPointF(cardRect.right(), cardRect.bottom()), true});                    // BR
    candidates.append(
        {QPointF(cardRect.left() - popupSizeF.width(), cardRect.top() - popupSizeF.height()), false}); // TL
    candidates.append({QPointF(cardRect.left() - popupSizeF.width(), cardRect.bottom()), false});      // BL

    QRectF bestRect;
    qreal bestVisibleArea = -1.0;
    bool bestIsRight = false;
    qreal bestYDist = std::numeric_limits<qreal>::infinity();

    const qreal EPS = 1e-6;
    for (const auto &c : candidates) {
        QRectF popupRect(c.pos, popupSizeF);
        QRectF visible = sceneRect.intersected(popupRect);
        qreal visibleArea = visible.width() * visible.height();
        qreal yDist = qAbs(popupRect.center().y() - cardRect.center().y());

        if (visibleArea > bestVisibleArea + EPS) {
            // strictly better visible area
            bestVisibleArea = visibleArea;
            bestRect = popupRect;
            bestIsRight = c.isRight;
            bestYDist = yDist;
        } else if (qAbs(visibleArea - bestVisibleArea) <= EPS) {
            // tie on visible area -> prefer right-side
            if (c.isRight != bestIsRight) {
                if (c.isRight) {
                    bestRect = popupRect;
                    bestIsRight = c.isRight;
                    bestYDist = yDist;
                }
            } else {
                // same side preference: pick the one that is more vertically centered
                if (yDist + EPS < bestYDist) {
                    bestRect = popupRect;
                    bestYDist = yDist;
                }
            }
        }
    }

    // Clamp popup to sceneRect if still going out
    if (!sceneRect.contains(bestRect)) {
        if (bestRect.left() < sceneRect.left())
            bestRect.moveLeft(sceneRect.left());
        if (bestRect.right() > sceneRect.right())
            bestRect.moveRight(sceneRect.right());
        if (bestRect.top() < sceneRect.top())
            bestRect.moveTop(sceneRect.top());
        if (bestRect.bottom() > sceneRect.bottom())
            bestRect.moveBottom(sceneRect.bottom());
    }

    popupProxy->setPos(bestRect.topLeft());

    // Optional debug if you want to see what's chosen:
    // qInfo() << "[positionPopup] chosen:" << bestRect << "area:" << bestVisibleArea << "isRight:" << bestIsRight <<
    // "yDist:" << bestYDist;
}

void AbstractCardItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if ((event->modifiers() & Qt::AltModifier) && event->button() == Qt::LeftButton) {
        emit cardShiftClicked(cardRef.name);
    } else if ((event->modifiers() & Qt::ControlModifier)) {
        setSelected(!isSelected());
    } else if (!isSelected()) {
        scene()->clearSelection();
        setSelected(true);
    }
    if (event->button() == Qt::LeftButton)
        setCursor(Qt::ClosedHandCursor);
    else if (event->button() == Qt::MiddleButton)
        emit showCardInfoPopup(event->screenPos(), cardRef);
    event->accept();
}

void AbstractCardItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton)
        emit deleteCardInfoPopup(cardRef.name);

    // This function ensures the parent function doesn't mess around with our selection.
    event->accept();
}

void AbstractCardItem::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
    cancelHideTimer();

    if (!popup)
        return;

    if (!popupProxy) {
        popupProxy = scene()->addWidget(popup);
        popupProxy->setZValue(99999);
    }
    positionPopup();
    popup->show();
    popup->raise();
}

void AbstractCardItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
    QTimer::singleShot(10, this, [this]() { checkMouseLeave(); });
}

void AbstractCardItem::processHoverEvent()
{
    emit hovered(this);
}

void AbstractCardItem::checkMouseLeave()
{
    if (!popup) {
        qInfo() << "No popup found. No need to start a hide timer.";
        return;
    }

    if (!isMouseOverPopupOrItem()) {
        qInfo() << "Mouse not over popup or card. Starting hide timer.";
        hideTimer->start();
    } else {
        qInfo() << "Mouse is still over popup or card.";
    }
}

bool AbstractCardItem::isMouseOverPopupOrItem() const
{
    if (!scene() || scene()->views().isEmpty() || !popupProxy) {
        qInfo() << "[isMouseOverPopupOrItem] Missing scene, view, or popupProxy.";
        return false;
    }

    QGraphicsView *view = scene()->views().first();
    QPoint globalPos = QCursor::pos();
    QPoint viewPos = view->mapFromGlobal(globalPos);
    QPointF scenePos = view->mapToScene(viewPos);

    QPointF localPosCard = this->mapFromScene(scenePos);
    QPointF localPosPopup = popupProxy->mapFromScene(scenePos);

    QRectF cardRect = this->boundingRect();
    QRectF popupRect = popupProxy->boundingRect();

    bool overCard = cardRect.contains(localPosCard);
    bool overPopup = popupRect.contains(localPosPopup);

    qInfo() << "\n[isMouseOverPopupOrItem]"
            << "\n  Global Cursor Pos:" << globalPos << "\n  View Pos:" << viewPos << "\n  Scene Pos:" << scenePos
            << "\n  Card Local Pos:" << localPosCard << " | In Card:" << overCard
            << "\n  Popup Local Pos:" << localPosPopup << " | In Popup:" << overPopup << "\n  Card Rect:" << cardRect
            << "\n  Popup Rect:" << popupRect;

    return overCard || overPopup;
}

QVariant AbstractCardItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedHasChanged) {
        update();
        return value;
    } else
        return ArrowTarget::itemChange(change, value);
}
