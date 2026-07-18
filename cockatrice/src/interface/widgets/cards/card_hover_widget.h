/**
 * @file card_hover_widget.h
 * @ingroup CardWidgets
 * @brief Hover popup that shows a card image with overlaid controls and tabbed actions.
 */

#ifndef CARDHOVERWIDGET_H
#define CARDHOVERWIDGET_H

#include <QFrame>
#include <QPointer>
#include <QTransform>
#include <functional>
#include <libcockatrice/card/printing/exact_card.h>

class CardInfoPictureWidget;
class CardInfoTextWidget;
class CardItem;
class PlayerGraphicsItem;
class QTabWidget;
class QVBoxLayout;
class QHBoxLayout;
class QPushButton;
class QLabel;
class QGraphicsOpacityEffect;
class QPropertyAnimation;

class CardHoverWidget : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

public:
    explicit CardHoverWidget(QWidget *parent = nullptr);

    void setCard(CardItem *card, PlayerGraphicsItem *player);
    void clearCard();
    bool hasCard() const;
    CardItem *getCardItem() const
    {
        return currentCard;
    }

    qreal opacity() const
    {
        return m_opacity;
    }
    void setOpacity(qreal val);

    void positionNextTo(const QRectF &cardSceneRect, const QTransform &viewportTransform, const QWidget *viewport);

    void showAnimated();
    void hideAnimated();

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

signals:
    void hoverEntered();
    void hoverLeft();

private:
    QPointer<CardItem> currentCard;
    QPointer<PlayerGraphicsItem> currentPlayer;

    // Image area (always visible, left side)
    QWidget *imageContainer;
    CardInfoPictureWidget *pic;
    QWidget *overlayBar;
    QPushButton *btnIncP, *btnDecP;
    QPushButton *btnIncT, *btnDecT;
    QPushButton *btnSetPT;
    QPushButton *btnResetPT;
    QLabel *counterLabel;
    QPushButton *btnAddCounter, *btnRemoveCounter;
    QGraphicsOpacityEffect *overlayEffect;

    // Tabs (right side)
    QTabWidget *tabWidget;
    QWidget *infoTab;
    QWidget *actionsTab;
    QWidget *allActionsTab;
    QVBoxLayout *infoTabLayout;
    QVBoxLayout *actionsTabLayout;
    QVBoxLayout *allActionsTabLayout;
    CardInfoTextWidget *text;

    // Fade animation
    QGraphicsOpacityEffect *fadeEffect;
    QPropertyAnimation *fadeIn;
    QPropertyAnimation *fadeOut;
    qreal m_opacity;

    void createImageArea();
    void createInfoTab();
    void createActionsTab();
    void createAllActionsTab();
    void createOverlayBar();

    void rebuildActionsForZone();
    void clearLayout(QLayout *layout);

    QPushButton *createSmallButton(const QString &text, const std::function<void()> &slot);
    QPushButton *createIconOnlyButton(const QString &icon, const QString &tooltip, const std::function<void()> &slot);
};

#endif // CARDHOVERWIDGET_H
