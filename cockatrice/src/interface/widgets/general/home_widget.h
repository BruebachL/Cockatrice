/**
 * @file home_widget.h
 * @ingroup Core
 * @ingroup Widgets
 */
//! \todo Document this file.

#ifndef HOME_WIDGET_H
#define HOME_WIDGET_H
#include "../../../interface/widgets/tabs/tab_supervisor.h"
#include "../cards/card_info_picture_art_crop_widget.h"
#include "card_animation_controller.h"
#include "card_animation_image_provider.h"
#include "home_styled_button.h"

#include <QGridLayout>
#include <QQuickWidget>
#include <QWidget>
#include <libcockatrice/network/client/abstract/abstract_client.h>

class HomeWidget : public QWidget
{

    Q_OBJECT

public:
    HomeWidget(QWidget *parent, TabSupervisor *tabSupervisor);
    void updateRandomCard();
    QPair<QColor, QColor> extractDominantColors(const QPixmap &pixmap);

public slots:
    void paintEvent(QPaintEvent *event) override;
    void initializeBackgroundFromSource();
    void onBackgroundShuffleFrequencyChanged();
    void updateBackgroundProperties();
    void updateButtonsToBackgroundColor();
    QGroupBox *createButtons();
    void updateConnectButton(const ClientStatus status);

private:
    CardAnimationController *m_animController{nullptr};
    QQuickWidget *m_animWidget{nullptr};
    QGridLayout *layout;
    QTimer *cardChangeTimer;
    TabSupervisor *tabSupervisor;
    QPixmap background;
    CardInfoPictureArtCropWidget *backgroundSourceCard = nullptr;
    DeckList backgroundSourceDeck;
    QPixmap overlay;
    QPair<QColor, QColor> gradientColors;
    HomeStyledButton *connectButton;

    void setRandomCard(ExactCard &newCard);
    void loadBackgroundSourceDeck();

protected:
    void resizeEvent(QResizeEvent *event) override;
};

#endif // HOME_WIDGET_H
