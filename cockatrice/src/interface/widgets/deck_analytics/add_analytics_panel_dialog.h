
#ifndef COCKATRICE_ADD_ANALYTICS_PANEL_DIALOG_H
#define COCKATRICE_ADD_ANALYTICS_PANEL_DIALOG_H

#include "deck_analytics_widget_factory.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>

class AddAnalyticsPanelDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddAnalyticsPanelDialog(QWidget *parent = nullptr) : QDialog(parent)
    {
        setWindowTitle(tr("Add Analytics Panel"));

        QVBoxLayout *layout = new QVBoxLayout(this);

        typeCombo = new QComboBox(this);
        // populate with all available panel types from the factory
        typeCombo->addItems(AnalyticsWidgetFactory::instance().available());

        layout->addWidget(typeCombo);

        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        layout->addWidget(buttons);

        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    QString selectedType() const
    {
        return typeCombo->currentText();
    }

private:
    QComboBox *typeCombo;
};

#endif // COCKATRICE_ADD_ANALYTICS_PANEL_DIALOG_H
