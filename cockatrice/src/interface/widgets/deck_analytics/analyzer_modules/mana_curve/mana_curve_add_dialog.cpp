#include "mana_curve_add_dialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

ManaCurveAddDialog::ManaCurveAddDialog(DeckListStatisticsAnalyzer *analyzer, QWidget *parent)
    : QDialog(parent), analyzer(analyzer)
{
    auto *lay = new QVBoxLayout(this);

    // groupBy
    lay->addWidget(new QLabel("Group By:", this));
    groupBy = new QComboBox(this);
    groupBy->addItems({"type", "color", "subtype", "power", "toughness"});
    lay->addWidget(groupBy);

    // filters
    lay->addWidget(new QLabel("Filters (optional):", this));
    filterList = new QListWidget(this);
    filterList->setSelectionMode(QAbstractItemView::MultiSelection);

    // Populate with all available categories
    QStringList cats = analyzer->getManaCurveByType().keys();
    cats.append(analyzer->getManaCurveByColor().keys());
    cats.removeDuplicates();
    cats.sort();
    filterList->addItems(cats);
    lay->addWidget(filterList);

    // options
    showMain = new QCheckBox("Show main bar row", this);
    showMain->setChecked(true);
    lay->addWidget(showMain);

    showCatRows = new QCheckBox("Show per-category rows", this);
    showCatRows->setChecked(true);
    lay->addWidget(showCatRows);

    // buttons
    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    lay->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, this, &ManaCurveAddDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &ManaCurveAddDialog::reject);
}

void ManaCurveAddDialog::setFromConfig(const ManaCurveConfig &cfg)
{
    groupBy->setCurrentText(cfg.groupBy);
    // restore filters
    for (int i = 0; i < filterList->count(); ++i)
        filterList->item(i)->setSelected(cfg.filters.contains(filterList->item(i)->text()));

    showMain->setChecked(cfg.showMain);
    showCatRows->setChecked(cfg.showCategoryRows);
}

void ManaCurveAddDialog::accept()
{
    cfg.groupBy = groupBy->currentText();

    cfg.filters.clear();
    for (auto *item : filterList->selectedItems())
        cfg.filters << item->text();

    cfg.showMain = showMain->isChecked();
    cfg.showCategoryRows = showCatRows->isChecked();

    QDialog::accept();
}
