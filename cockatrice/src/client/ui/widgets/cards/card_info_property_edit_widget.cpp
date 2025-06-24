#include "card_info_property_edit_widget.h"

#include "../../../../game/board/card_item.h"
#include "../../../../game/game_specific_terms.h"
#include "card_info_property_display_widget.h"

#include <QGridLayout>
#include <QLabel>
#include <QTextEdit>

CardInfoPropertyEditWidget::CardInfoPropertyEditWidget(QWidget *parent, CardInfoPtr _info) : QFrame(parent), info(_info)
{
    layout = new QGridLayout(this);

    nameLabel = new QLabel;
    nameLabel->setOpenExternalLinks(false);
    nameLabel->setWordWrap(true);
    connect(nameLabel, SIGNAL(linkActivated(const QString &)), this, SIGNAL(linkActivated(const QString &)));



    layout->addWidget(nameLabel, 0, 0);
    //grid->addWidget(textLabel, 1, 0, -1, 2);
    layout->setRowStretch(1, 1);
    layout->setColumnStretch(1, 1);

    retranslateUi();
}

void CardInfoPropertyEditWidget::setCard(CardInfoPtr card)
{
    info = card;
    if (card == nullptr) {
        nameLabel->setText("");
        // TODO: Clear grid layout here
        return;
    }

    QString text = "<table width=\"100%\" border=0 cellspacing=0 cellpadding=0>";
    text += QString("<tr><td>%1</td><td width=\"5\"></td><td>%2</td></tr>")
                .arg(tr("Name:"), card->getName().toHtmlEscaped());

    generateWidgetsForProperties();

    auto relatedCards = card->getAllRelatedCards();
    if (!relatedCards.empty()) {
        text += QString("<tr><td>%1</td><td width=\"5\"></td><td>").arg(tr("Related cards:"));

        for (auto *relatedCard : relatedCards) {
            QString tmp = relatedCard->getName().toHtmlEscaped();
            text += "<a href=\"" + tmp + "\">" + tmp + "</a><br>";
        }

        text += "</td></tr>";
    }

    text += "</table>";
    nameLabel->setText(text);
    //textLabel->setText(card->getText());
}

void CardInfoPropertyEditWidget::generateWidgetsForProperties()
{
    QStringList cardProps = info->getProperties();
    for (const QString &key : cardProps) {
        if (key.contains("-"))
            continue;
        QString keyText = Mtg::getNicePropertyName(key).toHtmlEscaped() + ":";
        auto propertyDisplayWidget = new CardInfoPropertyDisplayWidget(this, keyText, info->getProperty(key).toHtmlEscaped());
        layout->addWidget(propertyDisplayWidget, layout->rowCount(), 0);
    }
}

void CardInfoPropertyEditWidget::setInvalidCardName(const QString &cardName)
{
    nameLabel->setText(tr("Unknown card:") + " " + cardName);
}

void CardInfoPropertyEditWidget::retranslateUi()
{
    /*
     * There's no way we can really translate the text currently being rendered.
     * The best we can do is invalidate the current text.
     */
    setInvalidCardName("");
}
