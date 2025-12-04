
#ifndef COCKATRICE_MANA_BASE_ADD_DIALOG_H
#define COCKATRICE_MANA_BASE_ADD_DIALOG_H

#include "../../deck_list_statistics_analyzer.h"
#include "mana_base_config.h"

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

class ManaBaseConfigDialog : public QDialog
{
    Q_OBJECT
public:
    ManaBaseConfigDialog(DeckListStatisticsAnalyzer *analyzer, ManaBaseConfig initial = {}, QWidget *parent = nullptr)
        : QDialog(parent), cfg(initial)
    {
        auto *lay = new QVBoxLayout(this);

        lay->addWidget(new QLabel("Filter Colors (optional):", this));
        filterList = new QListWidget(this);
        filterList->setSelectionMode(QAbstractItemView::MultiSelection);

        QStringList colors = analyzer->getManaBase().keys();
        colors.sort();
        filterList->addItems(colors);

        // select initial filters
        for (int i = 0; i < filterList->count(); ++i) {
            if (cfg.filters.contains(filterList->item(i)->text()))
                filterList->item(i)->setSelected(true);
        }

        lay->addWidget(filterList);

        showTotalsCheck = new QCheckBox("Show totals", this);
        showTotalsCheck->setChecked(cfg.showTotals);
        lay->addWidget(showTotalsCheck);

        auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        lay->addWidget(bb);
        connect(bb, &QDialogButtonBox::accepted, this, &ManaBaseConfigDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, this, &ManaBaseConfigDialog::reject);
    }

    void accept() override
    {
        cfg.filters.clear();
        for (auto *item : filterList->selectedItems())
            cfg.filters << item->text();
        cfg.showTotals = showTotalsCheck->isChecked();
        QDialog::accept();
    }

    ManaBaseConfig result() const
    {
        return cfg;
    }

private:
    QListWidget *filterList;
    QCheckBox *showTotalsCheck;
    ManaBaseConfig cfg;
};

#endif // COCKATRICE_MANA_BASE_ADD_DIALOG_H
