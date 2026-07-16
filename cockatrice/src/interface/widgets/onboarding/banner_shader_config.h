#ifndef BANNER_SHADER_CONFIG_H
#define BANNER_SHADER_CONFIG_H

#include <QColor>
#include <QObject>

/**
 * Uniform values fed to brand_banner.frag, exposed to QML as the
 * "bannerConfig" context property.
 *
 * Two independent "banks" (A/B) each carry their own mode/speed/seed so
 * BrandBanner.qml can render both simultaneously and crossfade between
 * them via opacity -- see frontIsA. The shared palette (colorA/colorB/
 * accent) and clock (time/aspect) apply to both banks identically, since
 * only the foreground motif changes between onboarding pages, never the
 * brand palette. showTrace gates the traced brand-mark reveal animation
 * (visible only on the welcome page).
 *
 * Deliberately plain `property` (not `required property`) on the QML side
 * -- a required-property shadowing bug bit the home-screen particle
 * background before, and there's no reason to reintroduce that risk here.
 */
class BannerShaderConfig : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal time READ time WRITE setTime NOTIFY timeChanged)
    Q_PROPERTY(qreal aspect READ aspect WRITE setAspect NOTIFY aspectChanged)

    Q_PROPERTY(qreal modeA READ modeA WRITE setModeA NOTIFY modeAChanged)
    Q_PROPERTY(qreal speedA READ speedA WRITE setSpeedA NOTIFY speedAChanged)
    Q_PROPERTY(qreal seedA READ seedA WRITE setSeedA NOTIFY seedAChanged)

    Q_PROPERTY(qreal modeB READ modeB WRITE setModeB NOTIFY modeBChanged)
    Q_PROPERTY(qreal speedB READ speedB WRITE setSpeedB NOTIFY speedBChanged)
    Q_PROPERTY(qreal seedB READ seedB WRITE setSeedB NOTIFY seedBChanged)

    Q_PROPERTY(bool frontIsA READ frontIsA WRITE setFrontIsA NOTIFY frontIsAChanged)
    Q_PROPERTY(bool showTrace READ showTrace WRITE setShowTrace NOTIFY showTraceChanged)

    Q_PROPERTY(QColor colorA READ colorA WRITE setColorA NOTIFY colorAChanged)
    Q_PROPERTY(QColor colorB READ colorB WRITE setColorB NOTIFY colorBChanged)
    Q_PROPERTY(QColor accent READ accent WRITE setAccent NOTIFY accentChanged)

public:
    explicit BannerShaderConfig(QObject *parent = nullptr) : QObject(parent)
    {
    }

    qreal time() const { return m_time; }
    void setTime(qreal v) { if (v != m_time) { m_time = v; emit timeChanged(); } }

    qreal aspect() const { return m_aspect; }
    void setAspect(qreal v) { if (v != m_aspect) { m_aspect = v; emit aspectChanged(); } }

    qreal modeA() const { return m_modeA; }
    void setModeA(qreal v) { if (v != m_modeA) { m_modeA = v; emit modeAChanged(); } }
    qreal speedA() const { return m_speedA; }
    void setSpeedA(qreal v) { if (v != m_speedA) { m_speedA = v; emit speedAChanged(); } }
    qreal seedA() const { return m_seedA; }
    void setSeedA(qreal v) { if (v != m_seedA) { m_seedA = v; emit seedAChanged(); } }

    qreal modeB() const { return m_modeB; }
    void setModeB(qreal v) { if (v != m_modeB) { m_modeB = v; emit modeBChanged(); } }
    qreal speedB() const { return m_speedB; }
    void setSpeedB(qreal v) { if (v != m_speedB) { m_speedB = v; emit speedBChanged(); } }
    qreal seedB() const { return m_seedB; }
    void setSeedB(qreal v) { if (v != m_seedB) { m_seedB = v; emit seedBChanged(); } }

    bool frontIsA() const { return m_frontIsA; }
    void setFrontIsA(bool v) { if (v != m_frontIsA) { m_frontIsA = v; emit frontIsAChanged(); } }

    bool showTrace() const { return m_showTrace; }
    void setShowTrace(bool v) { if (v != m_showTrace) { m_showTrace = v; emit showTraceChanged(); } }

    QColor colorA() const { return m_colorA; }
    void setColorA(const QColor &c) { if (c != m_colorA) { m_colorA = c; emit colorAChanged(); } }
    QColor colorB() const { return m_colorB; }
    void setColorB(const QColor &c) { if (c != m_colorB) { m_colorB = c; emit colorBChanged(); } }
    QColor accent() const { return m_accent; }
    void setAccent(const QColor &c) { if (c != m_accent) { m_accent = c; emit accentChanged(); } }

signals:
    void timeChanged();
    void aspectChanged();
    void modeAChanged();
    void speedAChanged();
    void seedAChanged();
    void modeBChanged();
    void speedBChanged();
    void seedBChanged();
    void frontIsAChanged();
    void showTraceChanged();
    void colorAChanged();
    void colorBChanged();
    void accentChanged();

private:
    qreal m_time = 0.0;
    qreal m_aspect = 16.0 / 9.0;

    qreal m_modeA = 0.0;
    qreal m_speedA = 1.0;
    qreal m_seedA = 0.0;

    qreal m_modeB = 0.0;
    qreal m_speedB = 1.0;
    qreal m_seedB = 0.0;

    bool m_frontIsA = true;
    bool m_showTrace = false;

    QColor m_colorA{0x1A, 0x1A, 0x20};
    QColor m_colorB{0x0E, 0x0E, 0x12};
    QColor m_accent{0x8B, 0xDD, 0x6B};
};

#endif // BANNER_SHADER_CONFIG_H