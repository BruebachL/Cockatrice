#include "home_widget.h"

#include "../../../client/settings/cache_settings.h"
#include "../../../interface/widgets/tabs/tab_supervisor.h"
#include "../../theme_manager.h"
#include "../../window_main.h"
#include "background_sources.h"
#include "home_styled_button.h"

#include <QGroupBox>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QQmlContext>
#include <QVBoxLayout>
#include <libcockatrice/card/database/card_database_manager.h>
#include <libcockatrice/network/client/remote/remote_client.h>

HomeWidget::HomeWidget(QWidget *parent, TabSupervisor *_tabSupervisor)
    : QWidget(parent), tabSupervisor(_tabSupervisor), background("theme:backgrounds/home"), overlay("theme:cockatrice")
{
    layout = new QGridLayout(this);

    backgroundSourceCard = new CardInfoPictureArtCropWidget(this);

    gradientColors = extractDominantColors(background);

    auto buttonsGroupBox = createButtons();
    layout->addWidget(buttonsGroupBox, 1, 1, Qt::AlignVCenter | Qt::AlignHCenter);

    layout->setRowStretch(0, 1);
    layout->setRowStretch(2, 1);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(2, 1);

    setLayout(layout);

    m_animController = new CardAnimationController(this);

    // ── Surface format ────────────────────────────────────────────────────────
    // No alpha needed: the QML scene renders the background itself now.
    QSurfaceFormat fmt;
    fmt.setSamples(4);
    m_animWidget = new QQuickWidget(this);
    m_animWidget->setFormat(fmt);

    connect(m_animWidget->engine(), &QQmlEngine::warnings, this, [](const QList<QQmlError> &w) {
        for (const auto &e : w) {
            qWarning() << "QML:" << e.toString();
        }
    });

    // ── Image providers ───────────────────────────────────────────────────────
    auto *bgProvider = new HomeBackgroundProvider(); // engine takes ownership
    m_animWidget->engine()->addImageProvider(QStringLiteral("homebg"), bgProvider);
    m_animWidget->engine()->addImageProvider(QStringLiteral("cardanim"), new CardAnimImageProvider(m_animController));

    // Give the controller a handle so it can push new backgrounds to the provider
    m_animController->setBackgroundProvider(bgProvider);
    // Push the initial background immediately
    m_animController->setBackground(background);

    // ── Context properties ────────────────────────────────────────────────────
    m_animWidget->rootContext()->setContextProperty(QStringLiteral("cardAnimController"), m_animController);

    // ── QML source ────────────────────────────────────────────────────────────
    m_animWidget->setClearColor(Qt::black); // opaque — QML draws the bg itself
    m_animWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_animWidget->setSource(QUrl(QStringLiteral("qrc:/resources/qml/CardAnimBackground.qml")));

    if (m_animWidget->status() == QQuickWidget::Error) {
        for (const auto &e : m_animWidget->errors()) {
            qCritical() << "QML error:" << e.toString();
        }
    }

    // ── Z-order & geometry ────────────────────────────────────────────────────
    m_animWidget->stackUnder(buttonsGroupBox);
    m_animWidget->move(0, 0);
    if (!size().isEmpty()) {
        m_animWidget->resize(size());
    }

    static const AnimatedCardBackgroundConfig presets[] = {AnimatedCardPresets::river(), AnimatedCardPresets::whisper(),
                                                           AnimatedCardPresets::storm(),
                                                           AnimatedCardPresets::constellation()};

    m_animController->applyConfig(presets[2]);

    // ── Preset wiring (uncomment + add signal to SettingsCache) ──────────────
    /*
    connect(&SettingsCache::instance(), &SettingsCache::homeTabAnimPresetChanged, this,
            [this](int id) {
                using P = AnimatedCardPresets;
                static const AnimatedCardBackgroundConfig presets[] = {
                    P::river(), P::whisper(), P::storm(), P::constellation()
                };
                if (id >= 0 && id < 4) {
                    m_animController->applyConfig(presets[id]);
                    qInfo() << "HomeWidget: animation preset" << id << "applied";
                }
            });
    */

    cardChangeTimer = new QTimer(this);
    connect(cardChangeTimer, &QTimer::timeout, this, &HomeWidget::updateRandomCard);

    initializeBackgroundFromSource();

    updateConnectButton(tabSupervisor->getClient()->getStatus());

    connect(tabSupervisor->getClient(), &RemoteClient::statusChanged, this, &HomeWidget::updateConnectButton);
    connect(&SettingsCache::instance(), &SettingsCache::homeTabBackgroundSourceChanged, this,
            &HomeWidget::initializeBackgroundFromSource);
    connect(&SettingsCache::instance(), &SettingsCache::homeTabBackgroundShuffleFrequencyChanged, this,
            &HomeWidget::onBackgroundShuffleFrequencyChanged);
    // Lambda is cleaner to read than overloading this
    connect(&SettingsCache::instance(), &SettingsCache::homeTabDisplayCardNameChanged, this, [this] { repaint(); });
    connect(&SettingsCache::instance(), &SettingsCache::themeChanged, this,
            &HomeWidget::initializeBackgroundFromSource);
    connect(&SettingsCache::instance(), &SettingsCache::themeChanged, this,
            &HomeWidget::updateButtonsToBackgroundColor);
}

void HomeWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_animWidget) {
        m_animWidget->resize(size());
    }
}

void HomeWidget::initializeBackgroundFromSource()
{
    if (CardDatabaseManager::getInstance()->getLoadStatus() != LoadStatus::Ok) {
        connect(CardDatabaseManager::getInstance(), &CardDatabase::cardDatabaseLoadingFinished, this,
                &HomeWidget::initializeBackgroundFromSource);
        return;
    }

    auto backgroundSourceType = BackgroundSources::fromId(SettingsCache::instance().getHomeTabBackgroundSource());

    switch (backgroundSourceType) {
        case BackgroundSources::Theme:
            cardChangeTimer->stop();
            background = QPixmap("theme:backgrounds/home");
            backgroundSourceDeck = DeckList();
            backgroundSourceCard->setCard(ExactCard());
            updateButtonsToBackgroundColor();
            update();
            break;
        case BackgroundSources::RandomCardArt:
            backgroundSourceDeck = DeckList();
            updateRandomCard();
            onBackgroundShuffleFrequencyChanged();
            break;
        case BackgroundSources::DeckFileArt:
            loadBackgroundSourceDeck();
            updateRandomCard();
            onBackgroundShuffleFrequencyChanged();
            break;
    }
}

void HomeWidget::loadBackgroundSourceDeck()
{
    std::optional<LoadedDeck> deckOpt = DeckLoader::loadFromFile(
        SettingsCache::instance().getDeckPath() + "background.cod", DeckFileFormat::Cockatrice, false);
    backgroundSourceDeck = deckOpt.has_value() ? deckOpt.value().deckList : DeckList();
}

void HomeWidget::setRandomCard(ExactCard &newCard)
{
    static constexpr int ATTEMPTS = 10;
    for (int i = 0; i < ATTEMPTS; ++i) {
        ExactCard tmpCard = CardDatabaseManager::query()->getRandomCard();
        if (tmpCard != backgroundSourceCard->getCard() && tmpCard.getCardPtr()->getProperty("layout") == "normal" &&
            tmpCard.getPrinting().getSet() != nullptr) {
            newCard = tmpCard;
            return;
        }
    }
    qWarning() << "failed to set random card image after" << ATTEMPTS << "attempts";
}

void HomeWidget::updateRandomCard()
{
    auto backgroundSourceType = BackgroundSources::fromId(SettingsCache::instance().getHomeTabBackgroundSource());

    ExactCard newCard;

    switch (backgroundSourceType) {
        case BackgroundSources::Theme:
            break;
        case BackgroundSources::RandomCardArt:
            setRandomCard(newCard);
            break;
        case BackgroundSources::DeckFileArt:
            QList<CardRef> cardRefs = backgroundSourceDeck.getCardRefList();
            ExactCard oldCard = backgroundSourceCard->getCard();

            if (!cardRefs.empty()) {
                if (cardRefs.size() == 1) {
                    newCard = CardDatabaseManager::query()->getCard(cardRefs.first());
                } else {
                    // Keep picking until different
                    do {
                        int idx = QRandomGenerator::global()->bounded(cardRefs.size());
                        newCard = CardDatabaseManager::query()->getCard(cardRefs.at(idx));
                    } while (newCard == oldCard);
                }
            } else {
                do {
                    newCard = CardDatabaseManager::query()->getRandomCard();
                } while (newCard == oldCard);
            }
            break;
    }
    if (!newCard) {
        return;
    }

    connect(newCard.getCardPtr().data(), &CardInfo::pixmapUpdated, this, &HomeWidget::updateBackgroundProperties);
    backgroundSourceCard->setCard(newCard);
    background = backgroundSourceCard->getBackground();
}

void HomeWidget::onBackgroundShuffleFrequencyChanged()
{
    cardChangeTimer->stop();
    if (SettingsCache::instance().getHomeTabBackgroundShuffleFrequency() > 0) {
        cardChangeTimer->start(SettingsCache::instance().getHomeTabBackgroundShuffleFrequency() * 1000);
    }
}

void HomeWidget::updateBackgroundProperties()
{
    background = backgroundSourceCard->getBackground();
    updateButtonsToBackgroundColor();

    // Push new background into QML
    m_animController->setBackground(background);

    // Update card name label in QML
    ExactCard card = backgroundSourceCard->getCard();
    if (card) {
        QString name = card.getCardPtr()->getName();
        if (card.getPrinting().getSet()) {
            name += " (" + card.getPrinting().getSet()->getCorrectedShortName() + ") " +
                    card.getPrinting().getProperty("num");
        }
        m_animController->setCardName(name);
    } else {
        m_animController->setCardName(QString{});
    }

    update();
}

void HomeWidget::updateButtonsToBackgroundColor()
{
    gradientColors = extractDominantColors(background);
    for (HomeStyledButton *button : findChildren<HomeStyledButton *>()) {
        button->updateStylesheet(gradientColors);
        button->update();
    }
}

QGroupBox *HomeWidget::createButtons()
{
    QGroupBox *box = new QGroupBox(this);
    box->setStyleSheet(R"(
    QGroupBox {
        font-size: 20px;
        color: white;         /* Title text color */
        background: transparent;
    }

    QGroupBox::title {
        color: white;
        subcontrol-origin: margin;
        subcontrol-position: top center;  /* or top left / right */
    }
)");
    box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *boxLayout = new QVBoxLayout;
    boxLayout->setAlignment(Qt::AlignHCenter);

    QLabel *logoLabel = new QLabel;
    logoLabel->setPixmap(overlay.scaledToWidth(200, Qt::SmoothTransformation));
    logoLabel->setAlignment(Qt::AlignCenter);
    boxLayout->addWidget(logoLabel);
    boxLayout->addSpacing(25);

    connectButton = new HomeStyledButton("Connect/Play", gradientColors);
    boxLayout->addWidget(connectButton);

    auto visualDeckEditorButton = new HomeStyledButton(tr("Create New Deck"), gradientColors);
    connect(visualDeckEditorButton, &QPushButton::clicked, tabSupervisor,
            [this] { tabSupervisor->openDeckInNewTab(LoadedDeck()); });
    boxLayout->addWidget(visualDeckEditorButton);
    auto visualDeckStorageButton = new HomeStyledButton(tr("Browse Decks"), gradientColors);
    connect(visualDeckStorageButton, &QPushButton::clicked, tabSupervisor,
            [this] { tabSupervisor->actTabVisualDeckStorage(true); });
    boxLayout->addWidget(visualDeckStorageButton);
    auto visualDatabaseDisplayButton = new HomeStyledButton(tr("Browse Card Database"), gradientColors);
    connect(visualDatabaseDisplayButton, &QPushButton::clicked, tabSupervisor,
            &TabSupervisor::addVisualDatabaseDisplayTab);
    boxLayout->addWidget(visualDatabaseDisplayButton);
    auto edhrecButton = new HomeStyledButton(tr("Browse EDHRec"), gradientColors);
    connect(edhrecButton, &QPushButton::clicked, tabSupervisor, &TabSupervisor::addEdhrecMainTab);
    boxLayout->addWidget(edhrecButton);
    auto archidektButton = new HomeStyledButton(tr("Browse Archidekt"), gradientColors);
    connect(archidektButton, &QPushButton::clicked, tabSupervisor, &TabSupervisor::addArchidektTab);
    boxLayout->addWidget(archidektButton);
    auto replaybutton = new HomeStyledButton(tr("View Replays"), gradientColors);
    connect(replaybutton, &QPushButton::clicked, tabSupervisor, [this] { tabSupervisor->actTabReplays(true); });
    boxLayout->addWidget(replaybutton);
    if (qobject_cast<MainWindow *>(tabSupervisor->parentWidget())) {
        auto exitButton = new HomeStyledButton(tr("Quit"), gradientColors);
        connect(exitButton, &QPushButton::clicked, qobject_cast<MainWindow *>(tabSupervisor->parentWidget()),
                &MainWindow::actExit);
        boxLayout->addWidget(exitButton);
    }

    box->setLayout(boxLayout);
    return box;
}

void HomeWidget::updateConnectButton(const ClientStatus status)
{
    disconnect(connectButton, &QPushButton::clicked, nullptr, nullptr);
    switch (status) {
        case StatusConnecting:
            connectButton->setText(tr("Connecting..."));
            connectButton->setEnabled(false);
            break;
        case StatusDisconnected:
            connectButton->setText(tr("Connect"));
            connectButton->setEnabled(true);
            connect(connectButton, &QPushButton::clicked, qobject_cast<MainWindow *>(tabSupervisor->parentWidget()),
                    &MainWindow::actConnect);
            break;
        case StatusLoggedIn:
            connectButton->setText(tr("Play"));
            connectButton->setEnabled(true);
            connect(connectButton, &QPushButton::clicked, tabSupervisor,
                    &TabSupervisor::switchToFirstAvailableNetworkTab);
            break;
        default:
            break;
    }
}

QPair<QColor, QColor> HomeWidget::extractDominantColors(const QPixmap &pixmap)
{
    if (themeManager->isBuiltInTheme() &&
        SettingsCache::instance().getHomeTabBackgroundSource() == BackgroundSources::toId(BackgroundSources::Theme)) {
        return QPair<QColor, QColor>(QColor::fromRgb(20, 140, 60), QColor::fromRgb(120, 200, 80));
    }

    // Step 1: Downscale image for performance
    QImage image = pixmap.toImage()
                       .scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                       .convertToFormat(QImage::Format_RGB32);

    QMap<QRgb, int> colorCount;

    // Step 2: Count quantized colors
    for (int y = 0; y < image.height(); ++y) {
        const QRgb *scanLine = reinterpret_cast<const QRgb *>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            QColor color = QColor::fromRgb(scanLine[x]);

            int r = color.red() & 0xF0;
            int g = color.green() & 0xF0;
            int b = color.blue() & 0xF0;

            QRgb quantized = qRgb(r, g, b);
            colorCount[quantized]++;
        }
    }

    // Step 3: Sort by frequency
    QVector<QPair<QRgb, int>> sortedColors;
    for (auto it = colorCount.constBegin(); it != colorCount.constEnd(); ++it) {
        sortedColors.append(qMakePair(it.key(), it.value()));
    }

    std::sort(sortedColors.begin(), sortedColors.end(),
              [](const QPair<QRgb, int> &a, const QPair<QRgb, int> &b) { return a.second > b.second; });

    // Step 4: Pick top two distinct colors
    QColor first = QColor(sortedColors.value(0).first);
    QColor second = first;

    for (int i = 1; i < sortedColors.size(); ++i) {
        QColor candidate = QColor(sortedColors[i].first);
        if (candidate != first) {
            second = candidate;
            break;
        }
    }

    return QPair<QColor, QColor>(first, second);
}

void HomeWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
}
