#pragma once

#include "card_info_picture_widget.h"

#include <QQuickWidget>
#include <QTimer>

class CardInfoPictureFoilWidget : public CardInfoPictureWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal gradientOffset READ gradientOffset WRITE setGradientOffset NOTIFY gradientOffsetChanged)
    Q_PROPERTY(qreal highlightX READ highlightX WRITE setHighlightX NOTIFY highlightXChanged)
    Q_PROPERTY(QString cardImageUrl READ cardImageUrl WRITE setCardImageUrl NOTIFY cardImageUrlChanged)
    Q_PROPERTY(QRectF artRectNormalized READ artRectNormalized NOTIFY artRectNormalizedChanged)
    Q_PROPERTY(bool applyToArtOnly READ applyToArtOnly WRITE setApplyToArtOnly NOTIFY applyToArtOnlyChanged)

public:
    explicit CardInfoPictureFoilWidget(QWidget *parent = nullptr, bool hoverToZoomEnabled = true);
    ~CardInfoPictureFoilWidget() override = default;

    qreal gradientOffset() const
    {
        return m_gradientOffset;
    }
    void setGradientOffset(qreal v);

    qreal highlightX() const
    {
        return m_highlightX;
    }
    void setHighlightX(qreal v);

    QString cardImageUrl() const
    {
        return m_cardImageUrl;
    }
    void setCardImageUrl(const QString &url);

    QRectF artRectNormalized() const
    {
        return m_artRectNormalized;
    }

    bool applyToArtOnly() const
    {
        return m_applyToArtOnly;
    }
    void setApplyToArtOnly(bool v);

public slots:
    void setCard(const ExactCard &card) override;

signals:
    void gradientOffsetChanged();
    void highlightXChanged();
    void cardImageUrlChanged();
    void artRectNormalizedChanged();
    void applyToArtOnlyChanged();

protected:
    void resizeEvent(QResizeEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;

private slots:
    void updateFoilEffect();

private:
    void updateArtRectFromPixmap();

    QQuickWidget *m_quick = nullptr;
    QTimer *m_timer = nullptr;

    qreal m_gradientOffset = 0.0; // 0..1
    qreal m_highlightX = 0.5;     // normalized across art
    QString m_cardImageUrl;
    QRectF m_artRectNormalized = QRectF(0, 0, 1, 1);
    bool m_applyToArtOnly = true;
    bool m_gradientForward = true;
};
