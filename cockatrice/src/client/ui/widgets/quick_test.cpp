#include "quick_test.h"

#include "../../../game/cards/card_database_manager.h"

QuickTestWidget::QuickTestWidget(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    quick = new QQuickWidget(this);
    quick->setResizeMode(QQuickWidget::SizeRootObjectToView);

    provider = new PixmapProvider;
    quick->engine()->addImageProvider("pixmapProvider", provider);

    quick->setSource(QUrl(QStringLiteral("qrc:resources/shaders/QuickTest.qml")));

    layout->addWidget(quick);
    setLayout(layout);
}
