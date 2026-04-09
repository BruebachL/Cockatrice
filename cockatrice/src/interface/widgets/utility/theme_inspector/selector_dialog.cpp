#include "selector_dialog.h"

SelectorDialog::SelectorDialog(const QStringList &types,
                               const QStringList &objects,
                               const QStringList &pseudos,
                               QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Select Selector");
    setMinimumWidth(300);

    auto *layout = new QVBoxLayout(this);

    typeCombo = new QComboBox;
    typeCombo->addItems(types);
    layout->addWidget(typeCombo);

    objectCombo = new QComboBox;
    objectCombo->addItem("(none)");
    objectCombo->addItems(objects);
    layout->addWidget(objectCombo);

    pseudoCombo = new QComboBox;
    pseudoCombo->addItem("(none)");
    pseudoCombo->addItems(pseudos);
    layout->addWidget(pseudoCombo);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}
