#ifndef COCKATRICE_CARD_ANIMATION_CONTROLLER_H
#define COCKATRICE_CARD_ANIMATION_CONTROLLER_H

#include "home_background_provider.h"

#include <QHash>
#include <QImage>
#include <QObject>
#include <QReadWriteLock>
#include <QSet>
#include <QVariantMap>
#include <libcockatrice/card/printing/exact_card.h>

// ─── Configuration ────────────────────────────────────────────────────────────

struct AnimatedCardBackgroundConfig
{
    // Spawn
    float cardsPerSecond = 0.6f;
    int maxCards = 10;

    // Lifetime (seconds)
    float minLifetimeSecs = 9.0f;
    float maxLifetimeSecs = 15.0f;

    // Motion — angle in degrees: 0 = rightward, -90 = upward
    float angleDeg = -28.0f;
    float minSpeed = 35.0f; // px / sec
    float maxSpeed = 70.0f;

    // Card size & scale
    float baseCardWidth = 110.0f;
    float minScale = 0.55f;
    float maxScale = 1.05f;

    // Rotation
    float maxInitialRotDeg = 18.0f;
    float maxRotSpeedDeg = 7.0f; // deg / sec

    // Opacity envelope
    float peakOpacity = 0.88f;
    float fadeInFrac = 0.15f; // fraction of lifetime
    float fadeOutFrac = 0.22f;

    // Trail
    int trailLength = 4;
    int trailIntervalMs = 220;       // ms between trail ghost samples
    float trailOpacityDecay = 0.55f; // multiplied per step
    float trailScaleDecay = 0.90f;

    // Perpendicular turbulence (sine wave off the travel axis)
    float turbAmplitude = 22.0f; // px
    float turbFrequency = 0.30f; // Hz

    // Visual FX  (requires QtQuick.Effects / Qt 6.5+)
    bool enableGlow = true;
    float glowOpacity = 0.20f;
    bool enableShadow = true;
    float shadowOffsetPx = 4.0f;

    // Background sparkle particles
    bool enableParticles = true;
};

// ─── Presets ─────────────────────────────────────────────────────────────────

namespace AnimatedCardPresets
{

// Sparse, large, dreamy — almost no motion
inline AnimatedCardBackgroundConfig whisper()
{
    AnimatedCardBackgroundConfig c;
    c.cardsPerSecond = 0.18f;
    c.maxCards = 4;
    c.minLifetimeSecs = 16.0f;
    c.maxLifetimeSecs = 24.0f;
    c.angleDeg = -20.0f;
    c.minSpeed = 18.0f;
    c.maxSpeed = 30.0f;
    c.minScale = 0.85f;
    c.maxScale = 1.40f;
    c.maxInitialRotDeg = 8.0f;
    c.maxRotSpeedDeg = 1.5f;
    c.peakOpacity = 0.70f;
    c.fadeInFrac = 0.25f;
    c.fadeOutFrac = 0.35f;
    c.trailLength = 0;
    c.turbAmplitude = 12.0f;
    c.turbFrequency = 0.12f;
    c.glowOpacity = 0.10f;
    c.enableParticles = false;
    return c;
}

// Default diagonal river — balanced feel
inline AnimatedCardBackgroundConfig river()
{
    return {};
}

// Dense, fast, dramatic — Storm Mode™
inline AnimatedCardBackgroundConfig storm()
{
    AnimatedCardBackgroundConfig c;
    c.cardsPerSecond = 3.5f;
    c.maxCards = 50;
    c.minLifetimeSecs = 4.0f;
    c.maxLifetimeSecs = 8.0f;
    c.angleDeg = -42.0f;
    c.minSpeed = 100.0f;
    c.maxSpeed = 180.0f;
    c.minScale = 2.30f;
    c.maxScale = 2.90f;
    c.maxInitialRotDeg = 35.0f;
    c.maxRotSpeedDeg = 28.0f;
    c.peakOpacity = 1.00f;
    c.fadeInFrac = 0.07f;
    c.fadeOutFrac = 0.10f;
    c.trailLength = 0;
    c.trailIntervalMs = 0;
    c.trailOpacityDecay = 0.42f;
    c.turbAmplitude = 45.0f;
    c.turbFrequency = 0.90f;
    c.glowOpacity = 0.00f;
    c.enableParticles = false;
    return c;
}

// Cards drift in place, swell, then dissolve — no real travel
inline AnimatedCardBackgroundConfig constellation()
{
    AnimatedCardBackgroundConfig c;
    c.cardsPerSecond = 0.28f;
    c.maxCards = 6;
    c.minLifetimeSecs = 12.0f;
    c.maxLifetimeSecs = 20.0f;
    c.angleDeg = 0.0f;
    c.minSpeed = 0.0f;
    c.maxSpeed = 3.0f;
    c.minScale = 0.70f;
    c.maxScale = 1.10f;
    c.maxInitialRotDeg = 5.0f;
    c.maxRotSpeedDeg = 0.5f;
    c.peakOpacity = 0.82f;
    c.fadeInFrac = 0.30f;
    c.fadeOutFrac = 0.40f;
    c.trailLength = 0;
    c.turbAmplitude = 5.0f;
    c.turbFrequency = 0.08f;
    c.glowOpacity = 0.28f;
    c.enableParticles = true;
    return c;
}

} // namespace AnimatedCardPresets

// ─── Controller ───────────────────────────────────────────────────────────────

class CardAnimationController : public QObject
{
    Q_OBJECT

    // Every config field exposed as a read-only QML property.
    // QML reacts to configChanged() when applyConfig() is called.
#define CAP(type, name) Q_PROPERTY(type name READ name NOTIFY configChanged)
    Q_PROPERTY(int backgroundVersion READ backgroundVersion NOTIFY backgroundVersionChanged)
    Q_PROPERTY(QString cardNameText READ cardNameText NOTIFY cardNameTextChanged)
    CAP(float, cardsPerSecond)
    CAP(int, maxCards)
    CAP(float, angleDeg)
    CAP(float, minSpeed)
    CAP(float, maxSpeed)
    CAP(float, baseCardWidth)
    CAP(float, minLifetimeSecs)
    CAP(float, maxLifetimeSecs)
    CAP(float, minScale)
    CAP(float, maxScale)
    CAP(float, maxInitialRotDeg)
    CAP(float, maxRotSpeedDeg)
    CAP(float, peakOpacity)
    CAP(float, fadeInFrac)
    CAP(float, fadeOutFrac)
    CAP(int, trailLength)
    CAP(int, trailIntervalMs)
    CAP(float, trailOpacityDecay)
    CAP(float, trailScaleDecay)
    CAP(float, turbAmplitude)
    CAP(float, turbFrequency)
    CAP(bool, enableGlow)
    CAP(float, glowOpacity)
    CAP(bool, enableShadow)
    CAP(float, shadowOffsetPx)
    CAP(bool, enableParticles)

#undef CAP

public:
    explicit CardAnimationController(QObject *parent = nullptr);

    // Apply a preset or custom config; emits configChanged()
    void applyConfig(const AnimatedCardBackgroundConfig &config);

    // Called by image provider (may be from any thread)
    QImage getImageById(const QString &id) const;

    // Called from QML
    Q_INVOKABLE QVariantMap nextCard();
    Q_INVOKABLE void returnCard(const QString &id);

    // ── Property accessors ──────────────────────────────────────────────────
    int backgroundVersion() const
    {
        return m_backgroundVersion;
    }
    QString cardNameText() const
    {
        return m_cardNameText;
    }

    // Called by HomeWidget when the background pixmap changes.
    // Forwards the image to the provider and bumps backgroundVersion so
    // QML bindings re-trigger automatically.
    void setBackground(const QPixmap &pixmap)
    {
        if (m_bgProvider) {
            m_bgProvider->setBackground(pixmap);
        }
        ++m_backgroundVersion;
        emit backgroundVersionChanged();
    }

    // Called by HomeWidget when the displayed card name changes.
    void setCardName(const QString &name)
    {
        if (m_cardNameText == name) {
            return;
        }
        m_cardNameText = name;
        emit cardNameTextChanged();
    }

    // Store a non-owning pointer to the provider so setBackground() can
    // forward to it.  The engine owns the provider; this is just a handle.
    void setBackgroundProvider(HomeBackgroundProvider *p)
    {
        m_bgProvider = p;
    }

    float cardsPerSecond() const
    {
        return m_cfg.cardsPerSecond;
    }
    int maxCards() const
    {
        return m_cfg.maxCards;
    }
    float angleDeg() const
    {
        return m_cfg.angleDeg;
    }
    float minSpeed() const
    {
        return m_cfg.minSpeed;
    }
    float maxSpeed() const
    {
        return m_cfg.maxSpeed;
    }
    float baseCardWidth() const
    {
        return m_cfg.baseCardWidth;
    }
    float minScale() const
    {
        return m_cfg.minScale;
    }
    float maxScale() const
    {
        return m_cfg.maxScale;
    }
    float minLifetimeSecs() const
    {
        return m_cfg.minLifetimeSecs;
    }
    float maxLifetimeSecs() const
    {
        return m_cfg.maxLifetimeSecs;
    }
    float maxInitialRotDeg() const
    {
        return m_cfg.maxInitialRotDeg;
    }
    float maxRotSpeedDeg() const
    {
        return m_cfg.maxRotSpeedDeg;
    }
    float peakOpacity() const
    {
        return m_cfg.peakOpacity;
    }
    float fadeInFrac() const
    {
        return m_cfg.fadeInFrac;
    }
    float fadeOutFrac() const
    {
        return m_cfg.fadeOutFrac;
    }
    int trailLength() const
    {
        return m_cfg.trailLength;
    }
    int trailIntervalMs() const
    {
        return m_cfg.trailIntervalMs;
    }
    float trailOpacityDecay() const
    {
        return m_cfg.trailOpacityDecay;
    }
    float trailScaleDecay() const
    {
        return m_cfg.trailScaleDecay;
    }
    float turbAmplitude() const
    {
        return m_cfg.turbAmplitude;
    }
    float turbFrequency() const
    {
        return m_cfg.turbFrequency;
    }
    bool enableGlow() const
    {
        return m_cfg.enableGlow;
    }
    float glowOpacity() const
    {
        return m_cfg.glowOpacity;
    }
    bool enableShadow() const
    {
        return m_cfg.enableShadow;
    }
    float shadowOffsetPx() const
    {
        return m_cfg.shadowOffsetPx;
    }
    bool enableParticles() const
    {
        return m_cfg.enableParticles;
    }

signals:
    void configChanged();
    void readyChanged();
    void backgroundVersionChanged();
    void cardNameTextChanged();
    void cardAvailableChanged();

private:
    bool m_readyEmitted = false;
    void refillPool();
    void onCardReady(const QString &numericId, const QImage &img);

    AnimatedCardBackgroundConfig m_cfg;

    int m_backgroundVersion{0};
    QString m_cardNameText;
    HomeBackgroundProvider *m_bgProvider{nullptr};

    // ── Card pool ────────────────────────────────────────────────────────────
    // We give QML integer string IDs ("0", "1", …) that are URL-safe.
    int m_nextId{0};
    QHash<QString, ExactCard> m_allCards; // id → card (all ever allocated)
    QHash<QString, QImage> m_cardImages;  // id → loaded QImage  (guarded by m_imgLock)
    mutable QReadWriteLock m_imgLock;

    QList<QString> m_available;        // ids ready to hand to QML
    QHash<QString, ExactCard> m_inUse; // ids currently shown in QML
    QSet<QString> m_pending;           // ids whose images are still loading

    static constexpr int POOL_TARGET = 16;
};

#endif // COCKATRICE_CARD_ANIMATION_CONTROLLER_H
