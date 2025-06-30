#include "card_info_property_edit_widget.h"

#include "../../../card/card_relation.h"
#include "../../../card/game_specific_terms.h"
#include "../../../game/board/card_item.h"
#include "additional_info/card_info_name_and_cost_widget.h"
#include "card_info_property_display_widget.h"

#include <QGridLayout>
#include <QLabel>
#include <QTextEdit>

CardInfoPropertyEditWidget::CardInfoPropertyEditWidget(QWidget *parent, CardInfoPtr _info) : QFrame(parent), info(_info)
{
    layout = new QGridLayout(this);

    setCard(info);

    retranslateUi();
}

void CardInfoPropertyEditWidget::initializeLayout()
{
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        item->widget()->deleteLater();
        delete item;
    }

    nameLabel = new QLabel(this);
    nameLabel->setOpenExternalLinks(false);
    nameLabel->setWordWrap(true);
    connect(nameLabel, SIGNAL(linkActivated(const QString &)), this, SIGNAL(linkActivated(const QString &)));

    layout->addWidget(nameLabel, 0, 0);
    // grid->addWidget(textLabel, 1, 0, -1, 2);
    layout->setVerticalSpacing(0);
}

void CardInfoPropertyEditWidget::setCard(CardInfoPtr card)
{
    info = card;

    initializeLayout();

    if (card == nullptr) {
        nameLabel->setText("");
        return;
    }

    QStringList cardProps = info->getProperties();

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
    // textLabel->setText(card->getText());
}

void CardInfoPropertyEditWidget::generateWidgetsForProperties()
{
    QStringList cardProps = info->getProperties();

    /*if (info->getName() != "" && cardProps.contains(Mtg::ManaCost)) {
        auto nameAndCostWidget = new CardInfoNameAndCostWidget(this, info);
        layout->addWidget(nameAndCostWidget, layout->rowCount(), 0);
        cardProps.remove(cardProps.indexOf(Mtg::ManaCost));
    }*/
    for (const QString &key : cardProps) {
        if (key.contains("-"))
            continue;
        QString keyText = Mtg::getNicePropertyName(key).toHtmlEscaped() + ":";

        if (key == "layout" && info->getProperty(key).toHtmlEscaped() == "normal") {

        } else {
            auto propertyDisplayWidget =
                new CardInfoPropertyDisplayWidget(this, keyText, info->getProperty(key).toHtmlEscaped());
            layout->addWidget(propertyDisplayWidget, layout->rowCount(), 0);
        }
    }
    qInfo() << cardProps;
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
