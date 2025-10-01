#include "card_info_property_pow_tough_edit_widget.h"

#include <QIntValidator>
#include <QLabel>

CardInfoPropertyPowToughEditWidget::CardInfoPropertyPowToughEditWidget(QWidget *parent, const QString &powTough)
    : QWidget(parent)
{
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    btnDecPower = new QToolButton(this);
    btnDecPower->setText("-");
    editPower = new QLineEdit(this);
    editPower->setAlignment(Qt::AlignCenter);
    editPower->setFixedWidth(40);
    btnIncPower = new QToolButton(this);
    btnIncPower->setText("+");

    slashLabel = new QLabel("/", this);

    btnDecToughness = new QToolButton(this);
    btnDecToughness->setText("-");
    editToughness = new QLineEdit(this);
    editToughness->setAlignment(Qt::AlignCenter);
    editToughness->setFixedWidth(40);
    btnIncToughness = new QToolButton(this);
    btnIncToughness->setText("+");

    btnDecBoth = new QToolButton(this);
    btnDecBoth->setText("(-)");
    btnIncBoth = new QToolButton(this);
    btnIncBoth->setText("(+)");

    layout->addWidget(btnDecBoth);
    layout->addWidget(btnDecPower);
    layout->addWidget(editPower);
    layout->addWidget(btnIncPower);
    layout->addWidget(slashLabel);
    layout->addWidget(btnDecToughness);
    layout->addWidget(editToughness);
    layout->addWidget(btnIncToughness);
    layout->addWidget(btnIncBoth);

    // init values
    QStringList parts = powTough.split('/');
    if (parts.size() == 2) {
        editPower->setText(parts[0]);
        editToughness->setText(parts[1]);
    } else {
        editPower->setText("*");
        editToughness->setText("*");
    }

    connect(editPower, &QLineEdit::editingFinished, this, &CardInfoPropertyPowToughEditWidget::onPowerEdited);
    connect(editToughness, &QLineEdit::editingFinished, this, &CardInfoPropertyPowToughEditWidget::onToughnessEdited);

    connect(btnIncPower, &QToolButton::clicked, this, &CardInfoPropertyPowToughEditWidget::incrementPower);
    connect(btnDecPower, &QToolButton::clicked, this, &CardInfoPropertyPowToughEditWidget::decrementPower);
    connect(btnIncToughness, &QToolButton::clicked, this, &CardInfoPropertyPowToughEditWidget::incrementToughness);
    connect(btnDecToughness, &QToolButton::clicked, this, &CardInfoPropertyPowToughEditWidget::decrementToughness);
    connect(btnIncBoth, &QToolButton::clicked, this, &CardInfoPropertyPowToughEditWidget::incrementBoth);
    connect(btnDecBoth, &QToolButton::clicked, this, &CardInfoPropertyPowToughEditWidget::decrementBoth);
}

QString CardInfoPropertyPowToughEditWidget::power() const
{
    return editPower->text();
}

QString CardInfoPropertyPowToughEditWidget::toughness() const
{
    return editToughness->text();
}

QString CardInfoPropertyPowToughEditWidget::powTough() const
{
    return power() + "/" + toughness();
}

bool CardInfoPropertyPowToughEditWidget::isInt(const QString &text) const
{
    bool ok = false;
    text.toInt(&ok);
    return ok;
}

void CardInfoPropertyPowToughEditWidget::onPowerEdited()
{
    emit powerChanged(editPower->text());
}

void CardInfoPropertyPowToughEditWidget::onToughnessEdited()
{
    emit toughnessChanged(editToughness->text());
}

void CardInfoPropertyPowToughEditWidget::incrementPower()
{
    if (isInt(editPower->text())) {
        int val = editPower->text().toInt();
        editPower->setText(QString::number(val + 1));
        onPowerEdited();
    }
}

void CardInfoPropertyPowToughEditWidget::decrementPower()
{
    if (isInt(editPower->text())) {
        int val = editPower->text().toInt();
        editPower->setText(QString::number(val - 1));
        onPowerEdited();
    }
}

void CardInfoPropertyPowToughEditWidget::incrementToughness()
{
    if (isInt(editToughness->text())) {
        int val = editToughness->text().toInt();
        editToughness->setText(QString::number(val + 1));
        onToughnessEdited();
    }
}

void CardInfoPropertyPowToughEditWidget::decrementToughness()
{
    if (isInt(editToughness->text())) {
        int val = editToughness->text().toInt();
        editToughness->setText(QString::number(val - 1));
        onToughnessEdited();
    }
}

void CardInfoPropertyPowToughEditWidget::incrementBoth()
{
    if (isInt(editPower->text()) && isInt(editToughness->text())) {
        int p = editPower->text().toInt();
        int t = editToughness->text().toInt();
        editPower->setText(QString::number(p + 1));
        editToughness->setText(QString::number(t + 1));
        emit powToughChanged(powTough());
    }
}

void CardInfoPropertyPowToughEditWidget::decrementBoth()
{
    if (isInt(editPower->text()) && isInt(editToughness->text())) {
        int p = editPower->text().toInt();
        int t = editToughness->text().toInt();
        editPower->setText(QString::number(p - 1));
        editToughness->setText(QString::number(t - 1));
        emit powToughChanged(powTough());
    }
}
