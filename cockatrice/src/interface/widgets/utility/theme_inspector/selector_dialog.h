#ifndef COCKATRICE_SELECTOR_DIALOG_H
#define COCKATRICE_SELECTOR_DIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>

class SelectorDialog : public QDialog
{
    Q_OBJECT
public:
    SelectorDialog(const QStringList &types,
                   const QStringList &objects,
                   const QStringList &pseudos,
                   QWidget *parent = nullptr);

    QString selectedSelector() const
    {
        QString sel = typeCombo->currentText();
        if (objectCombo->currentIndex() > 0) // not "(none)"
            sel += objectCombo->currentText();
        if (pseudoCombo->currentIndex() > 0)
            sel += pseudoCombo->currentText();
        return sel;
    }

private:
    QComboBox *typeCombo;
    QComboBox *objectCombo;
    QComboBox *pseudoCombo;
};

#endif // COCKATRICE_SELECTOR_DIALOG_H
