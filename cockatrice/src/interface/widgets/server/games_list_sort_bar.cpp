#include "games_list_sort_bar.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QToolButton>

GamesListSortBar::GamesListSortBar(const QVector<SortField> &fields, QWidget *parent)
    : QWidget(parent), fieldCombo(new QComboBox(this)), orderBtn(new QToolButton(this)), colsSpinBox(new QSpinBox(this))
{
    for (const auto &f : fields) {
        fieldCombo->addItem(f.first, f.second);
    }

    orderBtn->setAutoRaise(true);
    updateOrderButton();

    // 0 is the "Auto" sentinel; 1–N are explicit column counts.
    // The maximum is updated live by onLayoutRecalculated().
    colsSpinBox->setRange(0, 8);
    colsSpinBox->setSpecialValueText(tr("Auto"));
    colsSpinBox->setValue(0);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 2, 6, 2);
    layout->setSpacing(4);
    layout->addWidget(new QLabel(tr("Sort:"), this));
    layout->addWidget(fieldCombo);
    layout->addWidget(orderBtn);
    layout->addStretch();
    layout->addWidget(new QLabel(tr("Columns:"), this));
    layout->addWidget(colsSpinBox);

    connect(fieldCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &GamesListSortBar::onSortFieldChanged);
    connect(orderBtn, &QToolButton::clicked, this, &GamesListSortBar::onSortOrderToggled);
    connect(colsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &GamesListSortBar::onColumnsChanged);
}

void GamesListSortBar::emitInitialSort()
{
    if (fieldCombo->count() > 0) {
        emit sortChanged(fieldCombo->currentData().toInt(), order);
    }
}

void GamesListSortBar::onLayoutRecalculated(int effectiveCols, int maxCols)
{
    // Block valueChanged for the duration so updating the spinbox doesn't
    // fire columnsRequested — TilingListView already applied the correct cap.
    const QSignalBlocker blocker(colsSpinBox);
    colsSpinBox->setMaximum(qMax(1, maxCols));

    // Mirror the effective column count when the user has a fixed preference.
    // This keeps the spinbox consistent with the live layout and — crucially —
    // restores the original value once the viewport is wide enough again,
    // because TilingListView preserves m_requestedCols across cap events and
    // the next layoutRecalculated will carry the un-capped effectiveCols back.
    if (colsSpinBox->value() > 0) {
        colsSpinBox->setValue(effectiveCols);
    }
}

void GamesListSortBar::onSortFieldChanged(int /*comboIndex*/)
{
    emit sortChanged(fieldCombo->currentData().toInt(), order);
}

void GamesListSortBar::onSortOrderToggled()
{
    order = (order == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
    updateOrderButton();
    emit sortChanged(fieldCombo->currentData().toInt(), order);
}

void GamesListSortBar::onColumnsChanged(int value)
{
    emit columnsRequested(value);
}

void GamesListSortBar::updateOrderButton()
{
    if (order == Qt::AscendingOrder) {
        orderBtn->setText(QStringLiteral("↑"));
        orderBtn->setToolTip(tr("Ascending"));
    } else {
        orderBtn->setText(QStringLiteral("↓"));
        orderBtn->setToolTip(tr("Descending"));
    }
}