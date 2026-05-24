#include "card_animation_controller.h"

#include "../../card_picture_loader/card_picture_loader.h"

#include <QPixmapCache>
#include <QRandomGenerator>
#include <libcockatrice/card/card_info.h>
#include <libcockatrice/card/database/card_database_manager.h>

CardAnimationController::CardAnimationController(QObject *parent) : QObject(parent)
{
    qInfo() << "CardAnimationController: constructed";
    auto *db = CardDatabaseManager::getInstance();
    if (db->getLoadStatus() == LoadStatus::Ok) {
        qInfo() << "CardAnimationController: DB ready, filling pool";
        refillPool();
    } else {
        qInfo() << "CardAnimationController: DB not ready, waiting";
        connect(db, &CardDatabase::cardDatabaseLoadingFinished, this, &CardAnimationController::refillPool,
                Qt::SingleShotConnection);
    }
}

void CardAnimationController::applyConfig(const AnimatedCardBackgroundConfig &config)
{
    m_cfg = config;
    qInfo() << "CardAnimationController: config applied —"
            << "cardsPerSecond:" << m_cfg.cardsPerSecond << "maxCards:" << m_cfg.maxCards;
    emit configChanged();
}

// ── Thread-safe image access ──────────────────────────────────────────────────

QImage CardAnimationController::getImageById(const QString &id) const
{
    QReadLocker lock(&m_imgLock);
    return m_cardImages.value(id);
}

// ── QML API ───────────────────────────────────────────────────────────────────

QVariantMap CardAnimationController::nextCard()
{
    refillPool();

    if (m_available.isEmpty()) {
        qInfo() << "CardAnimationController::nextCard — pool empty"
                << "(pending:" << m_pending.size() << ")";
        return {};
    }

    const QString id = m_available.takeFirst();
    m_inUse[id] = m_allCards.value(id);

    qInfo() << "CardAnimationController::nextCard — handing out id" << id
            << "| available remaining:" << m_available.size() << "| inUse:" << m_inUse.size();

    return {{"id", id}, {"aspectRatio", 1.4}};
}

void CardAnimationController::returnCard(const QString &id)
{
    if (!m_inUse.remove(id)) {
        return;
    }

    QReadLocker lock(&m_imgLock);
    if (m_cardImages.contains(id)) {
        m_available.append(id);
    }

    qInfo() << "CardAnimationController::returnCard" << id << "| available:" << m_available.size();
}

// ── Pool management ───────────────────────────────────────────────────────────

void CardAnimationController::refillPool()
{
    if (CardDatabaseManager::getInstance()->getLoadStatus() != LoadStatus::Ok) {
        return;
    }

    const int have = m_available.size() + static_cast<int>(m_pending.size());
    const int want = POOL_TARGET - have;
    if (want <= 0) {
        // 🔥 IMPORTANT: even if no refill needed, we may still be "ready"
        if (!m_readyEmitted && !m_available.isEmpty()) {
            m_readyEmitted = true;
            qInfo() << "CardAnimationController: READY (no refill needed)";
            emit readyChanged();
        }
        return;
    }

    qInfo() << "CardAnimationController::refillPool — want" << want << "more cards (have" << have << ", target"
            << POOL_TARGET << ")";

    int spawned = 0;
    for (int attempt = 0; attempt < want * 4 && spawned < want; ++attempt) {
        ExactCard card = CardDatabaseManager::query()->getRandomCard();
        if (!card) {
            continue;
        }
        if (card.getCardPtr()->getProperty("layout") != "normal") {
            continue;
        }
        if (card.getPrinting().getSet() == nullptr) {
            continue;
        }

        const QString numId = QString::number(m_nextId++);
        const QString cacheKey = card.getPixmapCacheKey();

        m_allCards[numId] = card;

        QPixmap px;
        if (QPixmapCache::find(cacheKey, &px) && !px.isNull()) {
            qInfo() << "CardAnimationController: card" << numId << "(" << card.getName() << ") already in QPixmapCache";
            onCardReady(numId, px.toImage());
        } else {
            qInfo() << "CardAnimationController: card" << numId << "(" << card.getName() << ") queued for load";
            m_pending.insert(numId);
            // Kick the loader; it fills QPixmapCache asynchronously.
            CardPictureLoader::getInstance().getPixmap(px, card, QSize(300, 420));

            connect(
                card.getCardPtr().data(), &CardInfo::pixmapUpdated, this,
                [this, numId, cacheKey]() {
                    QPixmap loaded;
                    if (QPixmapCache::find(cacheKey, &loaded) && !loaded.isNull()) {
                        qInfo() << "CardAnimationController: async load done for" << numId;
                        onCardReady(numId, loaded.toImage());
                    } else {
                        qWarning() << "CardAnimationController: async load for" << numId << "yielded null pixmap";
                        m_pending.remove(numId);
                    }
                },
                Qt::SingleShotConnection);
        }
        ++spawned;
    }

    qInfo() << "CardAnimationController::refillPool done —"
            << "available:" << m_available.size() << "pending:" << m_pending.size();

    // 🔥 READY SIGNAL (only once)
    if (!m_readyEmitted) {
        m_readyEmitted = true;
        qInfo() << "CardAnimationController: READY";
        emit readyChanged();
    }
}

void CardAnimationController::onCardReady(const QString &numericId, const QImage &img)
{
    QImage finalImg = img;

    if (finalImg.isNull()) {
        QPixmap fallback;
        CardPictureLoader::getCardBackLoadingFailedPixmap(fallback, QSize(300, 420));
        finalImg = fallback.toImage();

        qWarning() << "CardAnimationController: using fallback image for" << numericId;
    }

    {
        QWriteLocker lock(&m_imgLock);
        m_cardImages[numericId] = finalImg;
    }

    m_pending.remove(numericId);
    m_available.append(numericId);

    qInfo() << "CardAnimationController::onCardReady" << numericId << "| available:" << m_available.size()
            << "| pending:" << m_pending.size();
}
