#ifndef COCKATRICE_CARD_INFO_PROPERTY_POW_TOUGH_EDIT_WIDGET_H
#define COCKATRICE_CARD_INFO_PROPERTY_POW_TOUGH_EDIT_WIDGET_H
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QToolButton>
#include <QWidget>

class CardInfoPropertyPowToughEditWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CardInfoPropertyPowToughEditWidget(QWidget *parent = nullptr, const QString &powTough = "*/*");

    QString power() const;
    QString toughness() const;
    QString powTough() const;

signals:
    void powerChanged(QString newPower);
    void toughnessChanged(QString newToughness);
    void powToughChanged(QString newPowTough);

private slots:
    void onPowerEdited();
    void onToughnessEdited();
    void incrementPower();
    void decrementPower();
    void incrementToughness();
    void decrementToughness();
    void incrementBoth();
    void decrementBoth();

private:
    bool isInt(const QString &text) const;

    QHBoxLayout *layout;

    QToolButton *btnDecPower;
    QLineEdit *editPower;
    QToolButton *btnIncPower;

    QLabel *slashLabel;

    QToolButton *btnDecToughness;
    QLineEdit *editToughness;
    QToolButton *btnIncToughness;

    // both increment/decrement
    QToolButton *btnDecBoth;
    QToolButton *btnIncBoth;
};

#endif // COCKATRICE_CARD_INFO_PROPERTY_POW_TOUGH_EDIT_WIDGET_H
