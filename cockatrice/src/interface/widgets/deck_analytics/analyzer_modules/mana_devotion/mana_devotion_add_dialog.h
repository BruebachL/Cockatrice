
#ifndef COCKATRICE_MANA_DEVOTION_ADD_DIALOG_H
#define COCKATRICE_MANA_DEVOTION_ADD_DIALOG_H

#include "../../deck_list_statistics_analyzer.h"
#include "mana_devotion_config.h"

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

class ManaDevotionConfigDialog : public QDialog
{
    Q_OBJECT
public:
    ManaDevotionConfigDialog(DeckListStatisticsAnalyzer *analyzer,
                             ManaDevotionConfig initial = {},
                             QWidget *parent = nullptr)
        : QDialog(parent), cfg(initial)
    {
        auto *lay = new QVBoxLayout(this);

        lay->addWidget(new QLabel("Filter Colors (optional):", this));
        filterList = new QListWidget(this);
        filterList->setSelectionMode(QAbstractItemView::MultiSelection);

        QStringList colors = analyzer->getDevotionPipCount().keys();
        colors.sort();
        filterList->addItems(colors);

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
        connect(bb, &QDialogButtonBox::accepted, this, &ManaDevotionConfigDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, this, &ManaDevotionConfigDialog::reject);
    }

    void accept() override
    {
        cfg.filters.clear();
        for (auto *item : filterList->selectedItems())
            cfg.filters << item->text();
        cfg.showTotals = showTotalsCheck->isChecked();
        QDialog::accept();
    }

    ManaDevotionConfig result() const
    {
        return cfg;
    }

private:
    QListWidget *filterList;
    QCheckBox *showTotalsCheck;
    ManaDevotionConfig cfg;
};

#endif // COCKATRICE_MANA_DEVOTION_ADD_DIALOG_H
