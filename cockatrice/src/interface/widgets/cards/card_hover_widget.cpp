#include "card_hover_widget.h"

#include "../../../game/player/player_actions.h"
#include "../../../game/player/player_logic.h"
#include "../../../game/zones/card_zone_logic.h"
#include "../../../game_graphics/board/card_item.h"
#include "../../../game_graphics/game_scene.h"
#include "../../../game_graphics/player/card_menu_action_type.h"
#include "../../../game_graphics/player/player_graphics_item.h"
#include "../../../game_graphics/player/menu/card_menu.h"
#include "../../../game_graphics/player/menu/player_menu.h"
#include "card_info_picture_widget.h"
#include "card_info_text_widget.h"

#include <QApplication>
#include <QEnterEvent>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScreen>
#include <QTabWidget>
#include <QVBoxLayout>
#include <libcockatrice/card/database/card_database_manager.h>
#include <libcockatrice/utility/zone_names.h>

static constexpr int OVERLAY_HEIGHT = 36;
static constexpr int IMAGE_MIN_WIDTH = 220;
static constexpr int IMAGE_MIN_HEIGHT = 310;
static constexpr int TAB_WIDTH = 260;
static constexpr int TAB_MIN_HEIGHT = 310;
static constexpr int WIDGET_SPACING = 0;
static constexpr int POPUP_GAP = 12;
static constexpr int FADE_DURATION_MS = 150;

CardHoverWidget::CardHoverWidget(QWidget *parent)
    : QFrame(parent), currentCard(nullptr), currentPlayer(nullptr), m_opacity(0.0)
{
    setObjectName("cardHoverWidget");
    setFrameShape(QFrame::StyledPanel);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setMouseTracking(true);

    fadeEffect = new QGraphicsOpacityEffect(this);
    fadeEffect->setOpacity(0.0);
    setGraphicsEffect(fadeEffect);

    fadeIn = new QPropertyAnimation(fadeEffect, "opacity", this);
    fadeIn->setDuration(FADE_DURATION_MS);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);

    fadeOut = new QPropertyAnimation(fadeEffect, "opacity", this);
    fadeOut->setDuration(FADE_DURATION_MS);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);
    connect(fadeOut, &QPropertyAnimation::finished, this, &QWidget::hide);

    createImageArea();
    createInfoTab();
    createActionsTab();
    createAllActionsTab();

    tabWidget = new QTabWidget(this);
    tabWidget->setObjectName("cardHoverTabs");
    tabWidget->setMinimumWidth(TAB_WIDTH);
    tabWidget->setMinimumHeight(TAB_MIN_HEIGHT);
    tabWidget->addTab(infoTab, tr("Info"));
    tabWidget->addTab(actionsTab, tr("Actions"));
    tabWidget->addTab(allActionsTab, tr("All Actions"));

    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(WIDGET_SPACING);
    mainLayout->addWidget(imageContainer, 0);
    mainLayout->addWidget(tabWidget, 1);
    setLayout(mainLayout);
}

// ---------- Image area with overlaid controls ----------

void CardHoverWidget::createImageArea()
{
    imageContainer = new QWidget(this);
    imageContainer->setObjectName("cardHoverImageContainer");
    imageContainer->setMinimumWidth(IMAGE_MIN_WIDTH);
    imageContainer->setMinimumHeight(IMAGE_MIN_HEIGHT);
    imageContainer->setMouseTracking(true);

    pic = new CardInfoPictureWidget(imageContainer);
    pic->setObjectName("cardHoverPic");
    pic->setMouseTracking(true);
    pic->setHoverToZoomEnabled(false);

    createOverlayBar();

    auto *layout = new QVBoxLayout(imageContainer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(pic, 1);
    layout->addWidget(overlayBar, 0);
    imageContainer->setLayout(layout);
}

void CardHoverWidget::createOverlayBar()
{
    overlayBar = new QWidget(imageContainer);
    overlayBar->setObjectName("cardHoverOverlayBar");
    overlayBar->setFixedHeight(OVERLAY_HEIGHT);
    overlayBar->setMouseTracking(true);

    auto getActions = [this]() -> PlayerActions * {
        if (currentCard && currentCard->getOwner()) {
            return currentCard->getOwner()->getPlayerActions();
        }
        return nullptr;
    };
    auto getScene = [this]() -> GameScene * {
        if (currentPlayer) {
            return currentPlayer->getGameScene();
        }
        return nullptr;
    };
    auto sel = [getScene]() -> QList<CardItem *> {
        auto *s = getScene();
        return s ? s->selectedCards() : QList<CardItem *>();
    };

    btnIncP = createIconOnlyButton("+P", tr("Increase power"), [getActions, sel]() {
        if (auto *a = getActions()) {
            a->actIncP(sel());
        }
    });
    btnDecP = createIconOnlyButton("-P", tr("Decrease power"), [getActions, sel]() {
        if (auto *a = getActions()) {
            a->actDecP(sel());
        }
    });
    btnIncT = createIconOnlyButton("+T", tr("Increase toughness"), [getActions, sel]() {
        if (auto *a = getActions()) {
            a->actIncT(sel());
        }
    });
    btnDecT = createIconOnlyButton("-T", tr("Decrease toughness"), [getActions, sel]() {
        if (auto *a = getActions()) {
            a->actDecT(sel());
        }
    });
    btnSetPT = createIconOnlyButton("PT", tr("Set power/toughness..."), [getActions, sel]() {
        if (auto *a = getActions()) {
            a->actRequestSetPTDialog(sel());
        }
    });
    btnResetPT = createIconOnlyButton("R", tr("Reset power/toughness"), [getActions, sel]() {
        if (auto *a = getActions()) {
            a->actResetPT(sel());
        }
    });

    counterLabel = new QLabel(overlayBar);
    counterLabel->setObjectName("cardHoverCounterLabel");
    counterLabel->setText("0");
    counterLabel->setAlignment(Qt::AlignCenter);
    counterLabel->setFixedWidth(30);

    btnAddCounter = createIconOnlyButton("+", tr("Add counter"), [getActions, sel]() {
        if (auto *a = getActions()) {
            a->actAddCardCounter(sel(), 0);
        }
    });
    btnRemoveCounter = createIconOnlyButton("-", tr("Remove counter"), [getActions, sel]() {
        if (auto *a = getActions()) {
            a->actRemoveCardCounter(sel(), 0);
        }
    });

    auto *overlayLayout = new QHBoxLayout(overlayBar);
    overlayLayout->setContentsMargins(4, 2, 4, 2);
    overlayLayout->setSpacing(2);
    overlayLayout->addWidget(btnIncP);
    overlayLayout->addWidget(btnDecP);
    overlayLayout->addWidget(btnSetPT);
    overlayLayout->addWidget(btnResetPT);
    overlayLayout->addWidget(btnIncT);
    overlayLayout->addWidget(btnDecT);
    overlayLayout->addSpacing(8);
    overlayLayout->addWidget(btnAddCounter);
    overlayLayout->addWidget(counterLabel);
    overlayLayout->addWidget(btnRemoveCounter);
    overlayBar->setLayout(overlayLayout);

    overlayEffect = new QGraphicsOpacityEffect(overlayBar);
    overlayEffect->setOpacity(0.85);
    overlayBar->setGraphicsEffect(overlayEffect);
}

// ---------- Info tab ----------

void CardHoverWidget::createInfoTab()
{
    infoTab = new QWidget(this);
    text = new CardInfoTextWidget(infoTab);
    text->setObjectName("cardHoverText");
    connect(text, &CardInfoTextWidget::linkActivated, this, [this](const QString &cardName) {
        ExactCard card = CardDatabaseManager::query()->guessCard({cardName});
        if (!card.isEmpty()) {
            text->setCard(card);
        }
    });

    infoTabLayout = new QVBoxLayout(infoTab);
    infoTabLayout->setContentsMargins(4, 4, 4, 4);
    infoTabLayout->addWidget(text, 1);
    infoTab->setLayout(infoTabLayout);
}

// ---------- Actions tab ----------

void CardHoverWidget::createActionsTab()
{
    actionsTab = new QWidget(this);
    actionsTabLayout = new QVBoxLayout(actionsTab);
    actionsTabLayout->setContentsMargins(4, 4, 4, 4);
    actionsTabLayout->setAlignment(Qt::AlignTop);
    actionsTab->setLayout(actionsTabLayout);
}

QPushButton *CardHoverWidget::createSmallButton(const QString &text, const std::function<void()> &slot)
{
    auto *btn = new QPushButton(text, actionsTab);
    btn->setObjectName("cardHoverActionBtn");
    btn->setMinimumHeight(30);
    connect(btn, &QPushButton::clicked, this, [slot]() { slot(); });
    return btn;
}

QPushButton *CardHoverWidget::createIconOnlyButton(const QString &icon, const QString &tooltip,
                                                   const std::function<void()> &slot)
{
    auto *btn = new QPushButton(icon, overlayBar);
    btn->setObjectName("cardHoverOverlayBtn");
    btn->setToolTip(tooltip);
    btn->setFixedSize(28, 28);
    btn->setFlat(true);
    connect(btn, &QPushButton::clicked, this, [slot]() { slot(); });
    return btn;
}

void CardHoverWidget::rebuildActionsForZone()
{
    clearLayout(actionsTabLayout);

    if (!currentCard || !currentPlayer) {
        return;
    }

    auto *card = currentCard.data();
    auto *player = currentPlayer.data();
    auto *actions = player->getLogic()->getPlayerActions();
    auto *scene = player->getGameScene();

    auto sel = [scene]() { return scene->selectedCards(); };
    auto invoke = [actions, sel](CardMenuActionType type) {
        return [actions, sel, type]() { actions->cardMenuAction(sel(), type); };
    };

    bool canModify = player->getLogic()->getPlayerInfo()->getLocalOrJudge();
    if (!canModify) {
        auto *roLabel = new QLabel(tr("Read-only"), actionsTab);
        roLabel->setAlignment(Qt::AlignCenter);
        actionsTabLayout->addWidget(roLabel);
        return;
    }

    QString zoneName = card->getZone() ? card->getZone()->getName() : QString();

    // Play actions
    if (zoneName == ZoneNames::STACK || zoneName == ZoneNames::GRAVE || zoneName == ZoneNames::EXILE ||
        zoneName == ZoneNames::HAND || zoneName.startsWith("custom")) {
        actionsTabLayout->addWidget(createSmallButton(tr("&Play"), [actions, sel]() { actions->actPlay(sel()); }));
        actionsTabLayout->addWidget(
            createSmallButton(tr("Play &Face Down"), [actions, sel]() { actions->actPlayFacedown(sel()); }));
    }

    // Table-specific actions
    if (zoneName == ZoneNames::TABLE) {
        actionsTabLayout->addWidget(createSmallButton(tr("&Tap / Untap"), invoke(cmTap)));
        actionsTabLayout->addWidget(
            createSmallButton(tr("T&urn Over"), [actions, sel]() { actions->cardMenuAction(sel(), cmFlip); }));

        if (card->getFaceDown()) {
            actionsTabLayout->addWidget(
                createSmallButton(tr("&Peek at card face"), [actions, sel]() { actions->cardMenuAction(sel(), cmPeek); }));
        }
    }

    // Clone (all zones)
    if (!zoneName.isEmpty() && zoneName != "zoneless") {
        actionsTabLayout->addWidget(
            createSmallButton(tr("&Clone"), [actions, sel]() { actions->cardMenuAction(sel(), cmClone); }));
    }

    // Attach (table, stack, graveyard, exile, hand)
    if (zoneName != "zoneless") {
        actionsTabLayout->addWidget(
            createSmallButton(tr("Attac&h to card..."), [actions]() { actions->actAttach(); }));
        if (card->getAttachedTo()) {
            actionsTabLayout->addWidget(
                createSmallButton(tr("Unattac&h"), [actions, sel]() { actions->actUnattach(sel()); }));
        }
    }

    // Annotation (table only)
    if (zoneName == ZoneNames::TABLE) {
        actionsTabLayout->addWidget(
            createSmallButton(tr("&Set annotation..."), [actions, sel]() { actions->actRequestSetAnnotationDialog(sel()); }));
    }

    // Select All
    actionsTabLayout->addWidget(
        createSmallButton(tr("&Select All"), [actions]() { actions->actSelectAll(); }));

    actionsTabLayout->addStretch(1);
}

// ---------- All Actions tab ----------

void CardHoverWidget::createAllActionsTab()
{
    allActionsTab = new QWidget(this);
    allActionsTabLayout = new QVBoxLayout(allActionsTab);
    allActionsTabLayout->setContentsMargins(0, 0, 0, 0);
    allActionsTab->setLayout(allActionsTabLayout);
}

void CardHoverWidget::clearLayout(QLayout *layout)
{
    if (!layout) {
        return;
    }
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        if (item->layout()) {
            clearLayout(item->layout());
        }
        delete item;
    }
}

// ---------- Card data ----------

void CardHoverWidget::setCard(CardItem *card, PlayerGraphicsItem *player)
{
    if (!card || !player) {
        clearCard();
        return;
    }

    currentCard = card;
    currentPlayer = player;

    ExactCard exactCard = card->getCard();
    pic->setCard(exactCard);
    text->setCard(exactCard);

    rebuildActionsForZone();

    // Rebuild All Actions tab by embedding a fresh CardMenu
    clearLayout(allActionsTabLayout);
    auto *cardMenu = new CardMenu(player, card, player->getPlayerMenu()->getShortcutsActive());
    auto *menuBar = new QMenuBar(allActionsTab);
    menuBar->addMenu(cardMenu);
    allActionsTabLayout->addWidget(menuBar);

    // Update counter display
    auto counters = card->getCounters();
    if (counters.isEmpty()) {
        counterLabel->setText("0");
    } else {
        int total = 0;
        for (auto it = counters.begin(); it != counters.end(); ++it) {
            total += it.value();
        }
        counterLabel->setText(QString::number(total));
    }
}

void CardHoverWidget::clearCard()
{
    currentCard = nullptr;
    currentPlayer = nullptr;
    pic->setCard(ExactCard());
    text->setCard(ExactCard());
    clearLayout(actionsTabLayout);
    clearLayout(allActionsTabLayout);
    counterLabel->setText("0");
}

bool CardHoverWidget::hasCard() const
{
    return currentCard != nullptr;
}

void CardHoverWidget::setOpacity(qreal val)
{
    m_opacity = val;
    if (fadeEffect) {
        fadeEffect->setOpacity(val);
    }
}

void CardHoverWidget::positionNextTo(const QRectF &cardSceneRect, const QTransform &viewportTransform,
                                     const QWidget *viewport)
{
    QRect cardViewRect = viewportTransform.mapRect(cardSceneRect).toRect();
    QPoint cardTopRight = viewport->mapToGlobal(cardViewRect.topRight());
    QPoint cardTopLeft = viewport->mapToGlobal(cardViewRect.topLeft());
    QPoint cardCenter = viewport->mapToGlobal(cardViewRect.center());

    QSize popupSize = sizeHint();
    popupSize = QSize(qMax(popupSize.width(), TAB_WIDTH + IMAGE_MIN_WIDTH + 12),
                      qMax(popupSize.height(), TAB_MIN_HEIGHT + 8));

    auto *screen = QGuiApplication::screenAt(cardCenter);
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    QRect screenRect = screen->availableGeometry();

    int x, y;

    // Prefer placing to the right of the card
    x = cardTopRight.x() + POPUP_GAP;
    if (x + popupSize.width() > screenRect.right()) {
        // Not enough space on right, place to the left
        x = cardTopLeft.x() - popupSize.width() - POPUP_GAP;
    }
    if (x < screenRect.left()) {
        x = screenRect.left() + POPUP_GAP;
    }

    // Vertically center relative to card, clamped to screen
    y = cardCenter.y() - popupSize.height() / 2;
    if (y < screenRect.top()) {
        y = screenRect.top() + POPUP_GAP;
    }
    if (y + popupSize.height() > screenRect.bottom()) {
        y = screenRect.bottom() - popupSize.height() - POPUP_GAP;
    }

    move(x, y);
    resize(popupSize);
}

void CardHoverWidget::showAnimated()
{
    show();
    fadeIn->start();
}

void CardHoverWidget::hideAnimated()
{
    fadeOut->start();
}

// ---------- Mouse tracking ----------

void CardHoverWidget::enterEvent(QEnterEvent *event)
{
    QFrame::enterEvent(event);
    emit hoverEntered();
}

void CardHoverWidget::leaveEvent(QEvent *event)
{
    QFrame::leaveEvent(event);
    emit hoverLeft();
}
