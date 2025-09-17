#ifndef COCKATRICE_QUICK_TEST_H
#define COCKATRICE_QUICK_TEST_H

#include "pixmap_provider.h"

#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWidget>
#include <QVBoxLayout>
#include <QWidget>

class QuickTestWidget : public QWidget
{
    Q_OBJECT
public:
    QuickTestWidget(QWidget *parent = nullptr);

    void setPixmap(const QPixmap &pm)
    {
        provider->setPixmap(pm);

        // Force the QML Image to reload:
        quick->engine()->clearComponentCache();
        quick->setSource(QUrl(QStringLiteral("qrc:resources/qml/QuickTest.qml")));
    }

private:
    QQuickWidget *quick;
    PixmapProvider *provider;
};

#endif // COCKATRICE_QUICK_TEST_H
