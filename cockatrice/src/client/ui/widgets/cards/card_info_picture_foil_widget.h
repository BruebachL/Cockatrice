#pragma once

#include "../pixmap_provider.h"
#include "card_info_picture_widget.h"

#include <QQuickWidget>
#include <QTimer>

class CardInfoPictureFoilWidget : public CardInfoPictureWidget
{
    Q_OBJECT

public:
    explicit CardInfoPictureFoilWidget(QWidget *parent = nullptr, bool hoverToZoomEnabled = true);
    ~CardInfoPictureFoilWidget() override = default;

public slots:
    void setCard(const ExactCard &card) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QQuickWidget *quick;
    PixmapProvider *provider;
};
