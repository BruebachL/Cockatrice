#ifndef COCKATRICE_MANA_CURVE_ADD_DIALOG_H
#define COCKATRICE_MANA_CURVE_ADD_DIALOG_H

#include "../../deck_list_statistics_analyzer.h"
#include "mana_curve_config.h"

#include <QDialog>

class QListWidget;
class QCheckBox;
class QComboBox;

class ManaCurveAddDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ManaCurveAddDialog(DeckListStatisticsAnalyzer *analyzer, QWidget *parent = nullptr);
    void setFromConfig(const ManaCurveConfig &cfg);

    ManaCurveConfig result() const
    {
        return cfg;
    }

private:
    ManaCurveConfig cfg;
    DeckListStatisticsAnalyzer *analyzer;

    QComboBox *groupBy;
    QListWidget *filterList;
    QCheckBox *showMain;
    QCheckBox *showCatRows;

private slots:
    void accept() override;
};

#endif // COCKATRICE_MANA_CURVE_ADD_DIALOG_H
