#include "printing_selector_card_display_widget.h"

#include "../../../../deck/deck_loader.h"
#include "../../../../game/cards/card_database_manager.h"
#include "card_amount_widget.h"
#include "set_name_and_collectors_number_display_widget.h"

#include <QGraphicsEffect>
#include <QVBoxLayout>

PrintingSelectorCardDisplayWidget::PrintingSelectorCardDisplayWidget(QWidget *parent,
                                                                     TabDeckEditor *deckEditor,
                                                                     DeckListModel *deckModel,
                                                                     QTreeView *deckView,
                                                                     QSlider *cardSizeSlider,
                                                                     CardInfoPtr rootCard,
                                                                     CardInfoPerSet setInfoForCard,
                                                                     QString &currentZone)
    : QWidget(parent), deckEditor(deckEditor), deckModel(deckModel), deckView(deckView), cardSizeSlider(cardSizeSlider),
      rootCard(rootCard), setInfoForCard(setInfoForCard), currentZone(currentZone)
{
    layout = new QVBoxLayout(this);
    setLayout(layout);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(15);
    shadow->setOffset(3, 3);
    shadow->setColor(Qt::black);
    setGraphicsEffect(shadow);

    cardInfoPicture = new CardInfoPictureWidget(this);
    cardInfoPicture->setMinimumSize(0, 0);
    cardInfoPicture->setScaleFactor(cardSizeSlider->value());
    setCard = CardDatabaseManager::getInstance()->getCardByNameAndProviderId(rootCard->getName(),
                                                                             setInfoForCard.getProperty("uuid"));
    cardInfoPicture->setCard(setCard);

    allZonesCardAmountWidget =
        new AllZonesCardAmountWidget(this, deckEditor, deckModel, deckView, setCard, setInfoForCard);

    const QString combinedSetName =
        QString(setInfoForCard.getPtr()->getLongName() + " (" + setInfoForCard.getPtr()->getShortName() + ")");
    setNameAndCollectorsNumberDisplayWidget =
        new SetNameAndCollectorsNumberDisplayWidget(this, combinedSetName, setInfoForCard.getProperty("num"));

    layout->addWidget(cardInfoPicture, 1, Qt::AlignHCenter | Qt::AlignTop);
    layout->addWidget(allZonesCardAmountWidget, 0, Qt::AlignCenter);
    layout->addWidget(setNameAndCollectorsNumberDisplayWidget, 1, Qt::AlignHCenter | Qt::AlignBottom);

    connect(cardSizeSlider, &QSlider::valueChanged, cardInfoPicture, &CardInfoPictureWidget::setScaleFactor);
}

void PrintingSelectorCardDisplayWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event); // Ensure the parent class handles the event first

    // Determine the smaller width between cardInfoPicture and buttonBoxMainboardContainer
    int maxWidth = qMax(cardInfoPicture->width(), allZonesCardAmountWidget->width());

    // Set the maximum width for the setName QLabel
    setNameAndCollectorsNumberDisplayWidget->setMaximumWidth(maxWidth);
}
